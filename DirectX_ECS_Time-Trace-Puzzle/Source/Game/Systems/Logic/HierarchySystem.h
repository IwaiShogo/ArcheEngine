/*****************************************************************//**
 * @file	HierarchySystem.h
 * @brief	親から順に座標を計算していくシステム（ヒエラルキー）
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/27	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___HIERARCHY_SYSTEM_H___
#define ___HIERARCHY_SYSTEM_H___

// ===== インクルード =====
#include "Engine/ECS/ECS.h"
#include "Game/Components/Components.h"
#include <functional>

class HierarchySystem
	: public ISystem
{
public:
	HierarchySystem() { m_systemName = "Hierarchy System"; }

	void Update(Registry& registry) override
	{
		// 再帰的に行列を更新する関数
		std::function<void(Entity, const DirectX::XMMATRIX&)> updateMatrix =
			[&](Entity entity, const DirectX::XMMATRIX& parentMatrix)
			{
				if (registry.has<Transform>(entity)) {
					auto& t = registry.get<Transform>(entity);

					// 1. ローカル行列を作る (S * R * T)
					DirectX::XMMATRIX localMat =
						DirectX::XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z) *
						DirectX::XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, t.rotation.z) *
						DirectX::XMMatrixTranslation(t.position.x, t.position.y, t.position.z);

					// 2. 親の行列を掛けてワールド行列にする
					// 結果を Transform 自身の worldMatrix に保存！
					t.worldMatrix = localMat * parentMatrix;

					// 3. 子供たちにも自分のワールド行列を渡して更新させる
					if (registry.has<Relationship>(entity)) {
						for (Entity child : registry.get<Relationship>(entity).children) {
							updateMatrix(child, t.worldMatrix);
						}
					}
				}
			};

		// --- ルート（親なし）エンティティを探して更新開始 ---
		registry.view<Transform>([&](Entity e, Transform& t) {
			bool isRoot = true;
			if (registry.has<Relationship>(e)) {
				if (registry.get<Relationship>(e).parent != NullEntity) isRoot = false;
			}

			if (isRoot) {
				// ルートの親行列は単位行列
				updateMatrix(e, DirectX::XMMatrixIdentity());
			}
			});
	}
};

#endif // !___HIERARCHY_SYSTEM_H___