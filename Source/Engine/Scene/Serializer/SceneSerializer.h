/*****************************************************************//**
 * @file	SceneSerializer.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/17	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SCENE_SERIALIZER_H___
#define ___SCENE_SERIALIZER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/ECS/ECS.h"
#include "Engine/Systems/Physics/CollisionSystem.h"
#include "Engine/Scene/Serializer/ComponentSerializer.h"

class SceneSerializer
{
public:
	/**
	 * @brief シーン全体をJSONファイルに保存
	 */
	static void SaveScene(World& world, const std::string& filepath)
	{
		json sceneJson;
		sceneJson["SceneName"] = "Untitled Scene"; // 必要ならシーン名を持たせる

		// エンティティ配列
		sceneJson["Entities"] = json::array();

		auto& registry = world.getRegistry();

		// 生きている全エンティティをループ
		registry.each([&](auto entityID) {
			Entity entity = entityID;

			// エンティティごとのJSONオブジェクト
			json entityJson;
			entityJson["ID"] = (uint32_t)entity;

			// ★魔法の1行: 全コンポーネントを自動保存
			ComponentSerializer::SerializeEntity(registry, entity, entityJson);

			// 配列に追加
			sceneJson["Entities"].push_back(entityJson);
			});

		// ファイル書き込み
		std::ofstream fout(filepath);
		if (fout.is_open()) {
			fout << sceneJson.dump(4); // 4スペースインデントで見やすく
			fout.close();
			Logger::Log("Scene Saved: " + filepath);
		}
		else {
			Logger::LogError("Failed to save scene: " + filepath);
		}
	}

	/**
	 * @brief JSONファイルからシーンを読み込み（現在のシーンに追加）
	 */
	static void LoadScene(World& world, const std::string& filepath)
	{
		std::ifstream fin(filepath);
		if (!fin.is_open()) {
			Logger::LogError("Failed to load scene: " + filepath);
			return;
		}

		// 現在のエンティティをすべて削除
		world.getRegistry().clear();

		// 物理システムの内部状態もリセットする
		CollisionSystem::Reset();

		json sceneJson;
		try {
			fin >> sceneJson;
		}
		catch (json::parse_error& e) {
			Logger::LogError(std::string("JSON Parse Error: ") + e.what());
			return;
		}

		auto& registry = world.getRegistry();
		registry.clear(); // 既存のエンティティを全消去

		// ID変換マップ: [古いJSON上のID] -> [新しく生成されたEntity]
		std::unordered_map<uint32_t, Entity> idMap;

		// 1. エンティティ生成とコンポーネント復元
		if (sceneJson.contains("Entities")) {
			for (auto& entityJson : sceneJson["Entities"]) {

				// 新しいエンティティを作成
				Entity newEntity = registry.create();

				// 古いIDを取得してマップに登録
				uint32_t oldID = entityJson["ID"].get<uint32_t>();
				idMap[oldID] = newEntity;

				// コンポーネント復元 (この時点では parent/children は古いIDのまま！)
				ComponentSerializer::DeserializeEntity(registry, newEntity, entityJson);
			}
		}

		// 2. 親子関係のリンクIDを新しいIDに書き換える (Fix References)
		// Relationshipを持っている全エンティティを走査
		auto view = registry.view<Relationship>();
		for (auto entity : view)
		{
			auto& rel = view.get<Relationship>(entity);

			// 親IDの修正
			if (rel.parent != NullEntity) {
				uint32_t oldParentID = (uint32_t)rel.parent;
				if (idMap.count(oldParentID)) {
					rel.parent = idMap[oldParentID];
				}
				else {
					rel.parent = NullEntity; // 親が見つからなければ解除
				}
			}

			// 子IDリストの修正
			std::vector<Entity> newChildren;
			for (auto oldChildID : rel.children) {
				uint32_t oldID = (uint32_t)oldChildID;
				if (idMap.count(oldID)) {
					newChildren.push_back(idMap[oldID]);
				}
			}
			rel.children = newChildren;
		}

		Logger::Log("Scene Loaded: " + filepath);
	}

	static void SaveEntity(Registry& registry, Entity entity, const std::string& filepath)
	{
		json entityJson;
		entityJson["ID"] = (uint32_t)entity;

		ComponentSerializer::SerializeEntity(registry, entity, entityJson);

		std::ofstream fout(filepath);
		if (fout.is_open()) {
			fout << entityJson.dump(4);
			fout.close();
		}
	}

	static Entity LoadEntity(World& world, const std::string& filepath)
	{
		std::ifstream fin(filepath);
		if (!fin.is_open()) {
			Logger::LogError("Failed to load entity: " + filepath);
			return NullEntity;
		}

		json entityJson;
		try {
			fin >> entityJson;
		}
		catch (json::parse_error& e) {
			Logger::LogError(std::string("JSON Parse Error: ") + e.what());
			return NullEntity;
		}

		auto& registry = world.getRegistry();
		Entity entity = registry.create();

		// コンポーネント復元
		ComponentSerializer::DeserializeEntity(registry, entity, entityJson);

		return entity;
	}
};

#endif // !___SCENE_SERIALIZER_H___
