/*****************************************************************//**
 * @file	CollisionSystem.cpp
 * @brief	衝突検出
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/24	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Systems/CollisionSystem.h"
#include <cmath>
#include <iostream>

void CollisionSystem::Update(Registry& registry)
{
	// 対象となるEntityのグループを取得
	std::vector<Entity> entities;
	std::vector<Transform*> transforms;
	std::vector<BoxCollider*> colliders;

	registry.view<Transform, BoxCollider>([&](Entity e, Transform& t, BoxCollider& b) {
		entities.push_back(e);
		transforms.push_back(&t);
		colliders.push_back(&b);
	});

	size_t count = entities.size();
	if (count < 2) return;	// 相手がいなければ判定不要

	// 総当たり判定（0(N^2)）
	for (size_t i = 0; i < count; ++i)
	{
		for (size_t j = i + 1; j < count; ++j)
		{
			// 判定実行
			if (CheckAABB(transforms[i]->position, *colliders[i],
						  transforms[j]->position, *colliders[j]))
			{
				// --- 衝突時の処理 ---
				Entity entA = entities[i];
				Entity entB = entities[j];

				bool isPlayerA = registry.has<Tag>(entA) && strcmp(registry.get<Tag>(entA).name, "Player") == 0;
				bool isEnemyA = registry.has<Tag>(entA) && strcmp(registry.get<Tag>(entA).name, "Enemy") == 0;

				bool isPlayerB = registry.has<Tag>(entB) && strcmp(registry.get<Tag>(entB).name, "Player") == 0;
				bool isEnemyB = registry.has<Tag>(entB) && strcmp(registry.get<Tag>(entB).name, "Enemy") == 0;

				// プレイヤーと敵が当たったら
				if ((isPlayerA && isEnemyB) || (isPlayerB && isEnemyA))
				{
					// ここにゲームオーバーなどを書く
					if (isPlayerA) colliders[i]->size = XMFLOAT3(0.5f, 0.5f, 0.5f);
					if (isPlayerB) colliders[j]->size = XMFLOAT3(0.5f, 0.5f, 0.5f);

					Logger::LogError("Player Hit Enemy!");
				}
			}
		}
	}
}

// AABB判定ロジック
bool CollisionSystem::CheckAABB(const XMFLOAT3& posA, const BoxCollider& boxA, const XMFLOAT3& posB, const BoxCollider& boxB)
{
	// Aの最小・最大
	float minXA = posA.x - boxA.size.x / 2.0f;
	float maxXA = posA.x + boxA.size.x / 2.0f;
	float minYA = posA.y - boxA.size.y / 2.0f;
	float maxYA = posA.y + boxA.size.y / 2.0f;
	float minZA = posA.z - boxA.size.z / 2.0f;
	float maxZA = posA.z + boxA.size.z / 2.0f;
	
	// Bの最小・最大
	float minXB = posB.x - boxB.size.x / 2.0f;
	float maxXB = posB.x + boxB.size.x / 2.0f;
	float minYB = posB.y - boxB.size.y / 2.0f;
	float maxYB = posB.y + boxB.size.y / 2.0f;
	float minZB = posB.z - boxB.size.z / 2.0f;
	float maxZB = posB.z + boxB.size.z / 2.0f;

	// 全軸で重なっていれば衝突
	bool collisionX = maxXA >= minXB && maxXB >= minXA;
	bool collisionY = maxYA >= minYB && maxYB >= minYA;
	bool collisionZ = maxZA >= minZB && maxZB >= minZA;

	return collisionX && collisionY && collisionZ;
}