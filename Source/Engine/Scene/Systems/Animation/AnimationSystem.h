/*****************************************************************//**
 * @file	AnimationSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___ANIMATION_SYSTEM_H___
#define ___ANIMATION_SYSTEM_H___

// ===== インクルード =====
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Scene/Animation/Animation.h"
#include "Engine/Resource/ResourceManager.h"

namespace Arche
{
	class AnimationSystem : public ISystem
	{
	public:
		AnimationSystem()
		{
			m_systemName = "Animation System";
			m_group = SystemGroup::PlayOnly;
		}

		void Update(Registry& registry) override
		{
			float dt = Time::DeltaTime();

			// Lキーでデバッグログ出力
			bool debugLog = Input::GetKeyDown('L');

			auto view = registry.view<Animator, MeshComponent>();
			for (auto entity : view)
			{
				auto& animator = view.get<Animator>(entity);
				auto& mesh = view.get<MeshComponent>(entity);

				// 0. アニメーションデータの解決
				std::shared_ptr<Model> model = nullptr;

				if (!animator.currentAnimationName.empty())
				{
					model = ResourceManager::Instance().GetModel(mesh.modelKey);
					if (!animator.currentAnimation && model)
					{
						animator.currentAnimation = ResourceManager::Instance().GetAnimation(animator.currentAnimationName, model);
					}
				}

				if (!animator.currentAnimation || !model) continue;
				if (!animator.isPlaying) continue;

				auto& clip = animator.currentAnimation;

				if (animator.finalBoneMatrices.size() < 100)
				{
					animator.finalBoneMatrices.resize(100);
					for (auto& m : animator.finalBoneMatrices)
					{
						XMStoreFloat4x4(&m, DirectX::XMMatrixIdentity());
					}
				}

				// 1. 時間の進行
				animator.currentTime += clip->GetTicksPerSecond() * dt * animator.speed;

				float duration = clip->GetDuration();
				if (animator.loop)
				{
					if (duration > 0.0f)
						animator.currentTime = std::fmod(animator.currentTime, duration);
				}
				else if (animator.currentTime > duration)
				{
					animator.currentTime = duration;
				}

				// 2. 行列計算 (デバッグフラグを渡す)
				if (debugLog)
				{
					Logger::Log("===== Animation Debug Frame (Time: " + std::to_string(animator.currentTime) + ") =====");
				}

				CalculateBoneTransform(clip.get(), animator.currentTime, animator.finalBoneMatrices, model->globalInverseTransform, debugLog);
			}
		}

	private:
		// デバッグ用ヘルパー：ベクトルを文字列化
		std::string VectorToString(DirectX::XMVECTOR v)
		{
			DirectX::XMFLOAT4 f;
			DirectX::XMStoreFloat4(&f, v);
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << "(" << f.x << ", " << f.y << ", " << f.z << ", " << f.w << ")";
			return ss.str();
		}

		// デバッグ用ヘルパー：行列を文字列化
		std::string MatrixToString(DirectX::XMMATRIX m)
		{
			DirectX::XMFLOAT4X4 f;
			DirectX::XMStoreFloat4x4(&f, m);
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2);
			ss << "\n	[" << f._11 << ", " << f._12 << ", " << f._13 << ", " << f._14 << "]";
			ss << "\n	[" << f._21 << ", " << f._22 << ", " << f._23 << ", " << f._24 << "]";
			ss << "\n	[" << f._31 << ", " << f._32 << ", " << f._33 << ", " << f._34 << "]";
			ss << "\n	[" << f._41 << ", " << f._42 << ", " << f._43 << ", " << f._44 << "]";
			return ss.str();
		}

		// 1. エントリーポイント
		void CalculateBoneTransform(const AnimationClip* animation, float currentTime, std::vector<XMFLOAT4X4>& outputMatrices, const XMFLOAT4X4& globalInverseTransform, bool debug)
		{
			XMMATRIX identity = DirectX::XMMatrixIdentity();
			XMMATRIX globalInv = DirectX::XMLoadFloat4x4(&globalInverseTransform);

			if (debug)
			{
				Logger::Log("Global Inverse Transform:" + MatrixToString(globalInv));
			}

			CalculateBoneTransformRecursive(&animation->GetRootNode(), identity, animation, currentTime, outputMatrices, globalInv, debug);
		}

		// 2. 再帰関数
		void CalculateBoneTransformRecursive(const AssimpNodeData* node, XMMATRIX parentTransform, const AnimationClip* animation, float currentTime, std::vector<XMFLOAT4X4>& outputMatrices, XMMATRIX globalInverseTransform, bool debug)
		{
			std::string nodeName = node->name;

			bool isDebugTarget = debug && (
				nodeName.find("Hips") != std::string::npos ||
				nodeName.find("Head") != std::string::npos
				);

			// 1. 静的姿勢
			XMMATRIX nodeTransform = DirectX::XMLoadFloat4x4(&node->transformation);

			const BoneChannel* channel = FindBoneChannel(animation, nodeName);
			if (channel)
			{
				// 2. アニメーション姿勢
				XMMATRIX animTransform = channel->GetLocalTransform(currentTime);

				DirectX::XMVECTOR s_anim, r_anim, t_anim;
				DirectX::XMMatrixDecompose(&s_anim, &r_anim, &t_anim, animTransform);

				// --- 【修正】 全ボーン回転補正テスト ---
				// Hipsだけでなく、全てのボーンのデータが「Z軸180度」ズレていると仮定して一律補正する。
				// Mixamo -> Assimp -> DirectX の変換相性問題への強力な対策。

				// Z軸(0,0,1) 周りに 180度(PI) 回転するクォータニオン
				DirectX::XMVECTOR fixRot = DirectX::XMQuaternionRotationNormal(DirectX::XMVectorSet(0, 0, 1, 0), DirectX::XM_PI);

				// 回転を合成 (r_anim * fixRot)
				// ※ もしこれでダメなら、順序を逆 (fixRot * r_anim) にする手もあります
				r_anim = DirectX::XMQuaternionMultiply(r_anim, fixRot);


				// 位置とスケールはモデルの初期値（静的）を使う安全策を維持
				XMMATRIX staticM = DirectX::XMLoadFloat4x4(&node->transformation);
				DirectX::XMVECTOR s_stat, r_stat, t_stat;
				DirectX::XMMatrixDecompose(&s_stat, &r_stat, &t_stat, staticM);

				// 合成： スケール(静的) * 回転(アニメ+補正) * 位置(静的)
				nodeTransform = DirectX::XMMatrixScalingFromVector(s_stat) * DirectX::XMMatrixRotationQuaternion(r_anim) * DirectX::XMMatrixTranslationFromVector(t_stat);
			}

			// 親行列 * 自分
			XMMATRIX globalTransformation = nodeTransform * parentTransform;

			auto& boneInfoMap = animation->GetBoneIDMap();
			if (boneInfoMap.find(nodeName) != boneInfoMap.end())
			{
				int index = boneInfoMap.at(nodeName).id;
				XMMATRIX offset = DirectX::XMLoadFloat4x4(&boneInfoMap.at(nodeName).offset);

				offset.r[3] = DirectX::XMVectorMultiply(offset.r[3], DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));

				XMMATRIX finalM = offset * globalTransformation;

				if (index < outputMatrices.size())
				{
					DirectX::XMStoreFloat4x4(&outputMatrices[index], DirectX::XMMatrixTranspose(finalM));
				}
			}

			for (const auto& child : node->children)
			{
				CalculateBoneTransformRecursive(&child, globalTransformation, animation, currentTime, outputMatrices, globalInverseTransform, debug);
			}
		}

		const BoneChannel* FindBoneChannel(const AnimationClip* anim, const std::string& name)
		{
			for (const auto& ch : anim->GetBoneChannels())
			{
				if (ch.name == name) return &ch;
			}
			return nullptr;
		}
	};
}

#include "Engine/Scene/Serializer/SystemRegistry.h"
ARCHE_REGISTER_SYSTEM(Arche::AnimationSystem, "Animation System")

#endif // !___ANIMATION_SYSTEM_H___