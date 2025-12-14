/*****************************************************************//**
 * @file	PhysicsSystem.h
 * @brief	物理挙動
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/01	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___PHYSICS_SYSTEM_H___
#define ___PHYSICS_SYSTEM_H___

// ===== インクルード =====
#include "Engine/ECS/ECS.h"
#include "Game/Components/Components.h"
#include "Engine/Core/Time.h"
#include <vector>

// 接触情報
namespace Physics
{
	struct Contact
	{
		Entity a;
		Entity b;
		DirectX::XMFLOAT3 normal;	// A -> Bの法線
		float depth;				// めり込み量
	};
}

class PhysicsSystem
	: public ISystem
{
public:
	PhysicsSystem() { m_systemName = "Physics System"; }

	void Update(Registry& registry) override
	{
		float dt = Time::DeltaTime();
		const float GRAVITY = 9.81f;

		registry.view<Rigidbody, Transform>().each([&](Entity e, Rigidbody& rb, Transform& t)
			{
				// Staticは何もしない
				if (rb.type == BodyType::Static) return;

				// Kinematicは物理演算（重力・抵抗）を受けないが、速度による移動は適用する
				// Dynamicはフルに演算する
				if (rb.type == BodyType::Dynamic)
				{
					// 1. 重力
					if (rb.useGravity)
					{
						rb.velocity.y -= GRAVITY * dt;
					}

					// 2. 空気抵抗
					float dump = 1.0f - (rb.drag * dt);
					if (dump < 0.0f) dump = 0.0f;
					rb.velocity.x *= dump;
					rb.velocity.z *= dump;
					// Y軸（落下）は空気抵抗を受けにくい設定にするか、全体にかけるか
					// rb.velocity.y *= dump;
				}

				// 3. 位置更新（Kinematic & Dynamic）
				t.position.x += rb.velocity.x * dt;
				t.position.y += rb.velocity.y * dt;
				t.position.z += rb.velocity.z * dt;

				// （デバッグ用）床抜け防止リセット
				if (t.position.y < -50.0f)
				{
					t.position = { 0, 10, 0 };
					rb.velocity = { 0, 0, 0 };
				}
			});
	}

	static void Solve(Registry& registry, const std::vector<Physics::Contact>& contacts)
	{
		for (const auto& contact : contacts)
		{
			if (!registry.has<Rigidbody>(contact.a) || !registry.has<Rigidbody>(contact.b)) continue;
			if (!registry.has<Transform>(contact.a) || !registry.has<Transform>(contact.b)) continue;

			auto& rbA = registry.get<Rigidbody>(contact.a);
			auto& rbB = registry.get<Rigidbody>(contact.b);
			auto& tA = registry.get<Transform>(contact.a);
			auto& tB = registry.get<Transform>(contact.b);

			bool fixedA = (rbA.type == BodyType::Static || rbA.type == BodyType::Kinematic);
			bool fixedB = (rbB.type == BodyType::Static || rbB.type == BodyType::Kinematic);

			// 両方固定なら何もしない
			if (fixedA && fixedB) continue;

			using namespace DirectX;
			XMVECTOR n = XMLoadFloat3(&contact.normal); // A -> B の法線
			float depth = contact.depth;

			// ---------------------------------------------------------
			// 1. 両方動く場合 (Dynamic vs Dynamic)
			// ---------------------------------------------------------
			if (!fixedA && !fixedB) {
				// 位置補正 (既存)
				float totalMass = rbA.mass + rbB.mass;
				float ratioA = rbB.mass / totalMass;
				float ratioB = rbA.mass / totalMass;

				XMVECTOR posA = XMLoadFloat3(&tA.position);
				XMVECTOR posB = XMLoadFloat3(&tB.position);
				posA -= n * (depth * ratioA);
				posB += n * (depth * ratioB);
				XMStoreFloat3(&tA.position, posA);
				XMStoreFloat3(&tB.position, posB);

				// 【追加】速度補正
				// お互いの相対速度を計算し、法線方向の成分を打ち消す
				XMVECTOR velA = XMLoadFloat3(&rbA.velocity);
				XMVECTOR velB = XMLoadFloat3(&rbB.velocity);
				XMVECTOR relativeVel = velB - velA;
				float velDot = XMVectorGetX(XMVector3Dot(relativeVel, n));

				// 接近している場合のみ (離れようとしている時は何もしない)
				if (velDot < 0.0f) {
					// 衝撃係数(e) = 0 (跳ね返らない) と仮定
					// 法線方向の相対速度成分を分配して加算
					XMVECTOR impulse = n * velDot;
					velA += impulse * ratioA;
					velB -= impulse * ratioB;

					XMStoreFloat3(&rbA.velocity, velA);
					XMStoreFloat3(&rbB.velocity, velB);
				}
			}
			// ---------------------------------------------------------
			// 2. Aだけ動く場合 (Player vs Wall)
			// ---------------------------------------------------------
			else if (!fixedA && fixedB) {
				// 位置補正 (既存)
				XMVECTOR posA = XMLoadFloat3(&tA.position);
				posA -= n * depth;
				XMStoreFloat3(&tA.position, posA);

				// 【追加】速度補正 (これが重要！)
				XMVECTOR velA = XMLoadFloat3(&rbA.velocity);
				// 法線方向の速度成分を計算
				// AはBに対して -n 方向に押し出されるので、velocity と -n の内積を見るべきだが、
				// ここでは「壁に向かう成分」を消すと考えればよい。
				// 法線 n は A->B なので、壁の向き。velocity と n の内積がプラスなら壁に向かっている。
				float velDot = XMVectorGetX(XMVector3Dot(velA, n));

				// 壁に向かって進んでいる場合のみ補正
				if (velDot > 0.0f) {
					// 法線方向の成分を引くことで、壁に沿って滑る動きになる
					velA -= n * velDot;
					XMStoreFloat3(&rbA.velocity, velA);
				}
			}
			// ---------------------------------------------------------
			// 3. Bだけ動く場合 (Wall vs Enemy)
			// ---------------------------------------------------------
			else if (fixedA && !fixedB) {
				// 位置補正 (既存)
				XMVECTOR posB = XMLoadFloat3(&tB.position);
				posB += n * depth;
				XMStoreFloat3(&tB.position, posB);

				// 【追加】速度補正
				XMVECTOR velB = XMLoadFloat3(&rbB.velocity);
				// Bは n 方向に押し出される。
				// n は A->B なので、velocity と n の内積がマイナスならA(壁)に向かっている。
				float velDot = XMVectorGetX(XMVector3Dot(velB, n));

				if (velDot < 0.0f) {
					velB -= n * velDot;
					XMStoreFloat3(&rbB.velocity, velB);
				}
			}
		}
	}
};

#endif // !___PHYSICS_SYSTEM_H___