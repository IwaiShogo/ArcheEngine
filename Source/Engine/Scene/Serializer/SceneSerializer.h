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
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Scene/Systems/Physics/CollisionSystem.h"
#include "Engine/Scene/Serializer/ComponentSerializer.h"
#include "Engine/Scene/Serializer/SystemRegistry.h"

#include <fstream>
#include <functional>

namespace Arche
{

	class SceneSerializer
	{
	public:
		/**
		 * @brief シーン全体をJSONファイルに保存
		 */
		static void SaveScene(World& world, const std::string& filepath)
		{
			json sceneJson;
			sceneJson["SceneName"] = "Untitled Scene";

			// 1. レイヤー衝突設定の保存
			// ------------------------------------------------------------
			sceneJson["Physics"]["LayerCollision"] = json::array();
			for (int i = 0; i < 32; ++i)
			{
				Layer layer = (Layer)(1 << i);
				Layer mask = PhysicsConfig::GetMask(layer);
				if ((int)mask != 0 && (int)mask != -1)
				{
					json layerJson;
					layerJson["Layer"] = (int)layer;
					layerJson["Mask"] = (int)mask;
					sceneJson["Physics"]["LayerCollision"].push_back(layerJson);
				}
			}

			// 2. システム構成の保存
			// ------------------------------------------------------------
			sceneJson["Systems"] = json::array();
			for (const auto& sys : world.getSystems())
			{
				json sysJson;
				sysJson["Name"] = sys->m_systemName;	// システム名
				sysJson["Group"] = (int)sys->m_group;	// 実行グループ（Always / PlayOnly 等）
				sceneJson["Systems"].push_back(sysJson);
			}

			// 3. エンティティの保存
			// ------------------------------------------------------------
			sceneJson["Entities"] = json::array();
			auto& registry = world.getRegistry();

			// 生きている全エンティティをループ
			registry.each([&](auto entityID)
			{
				Entity entity = entityID;

				// エンティティごとのJSONオブジェクト
				json entityJson;
				entityJson["ID"] = (uint32_t)entity;

				// 全コンポーネントを自動保存
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

			// リセット処理
			world.getRegistry().clear();	// エンティティのクリア
			world.clearSystems();
			CollisionSystem::Reset();		// 物理システムの内部情報のリセット
			PhysicsConfig::Reset();

			json sceneJson;
			try {
				fin >> sceneJson;
			}
			catch (json::parse_error& e) {
				Logger::LogError(std::string("JSON Parse Error: ") + e.what());
				return;
			}
			
			// 1. レイヤー衝突設定の復元
			// ------------------------------------------------------------
			if (sceneJson.contains("Physics") && sceneJson["Physics"].contains("LayerCollision"))
			{
				for (auto& layerJson : sceneJson["Physics"]["LayerCollision"])
				{
					Layer layer = (Layer)layerJson["Layer"].get<int>();
					Layer mask = (Layer)layerJson["Mask"].get<int>();

					PhysicsConfig::Configure(layer).setMask(mask);
				}
			}

			// 1. システム構成の復元
			// ------------------------------------------------------------
			if (sceneJson.contains("Systems"))
			{
				// world.CrearSystems();
				for (auto& sysJson : sceneJson["Systems"])
				{
					std::string name = sysJson["Name"].get<std::string>();
					SystemGroup group = SystemGroup::PlayOnly;

					if (sysJson.contains("Group")) group = (SystemGroup)sysJson["Group"].get<int>();

					// Registryをつかって動的に生成・登録
					SystemRegistry::Instance().CreateSystem(world, name, group);
				}
			}

			// 2. エンティティの復元
			// ------------------------------------------------------------
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
		
		/**
		 * @brief	プレファブ保存（再帰的に子孫を含めて保存）
		 */
		static void SavePrefab(Registry& reg, Entity root, const std::string& filepath)
		{
			// 1. 保存対象の全エンティティを収集
			std::vector<Entity> entities;
			std::function<void(Entity)> collect = [&](Entity e)
			{
				entities.push_back(e);
				if (reg.has<Relationship>(e))
				{
					for (auto c : reg.get<Relationship>(e).children) collect(c);
				}
			};
			collect(root);

			// 2. JSON配列にシリアライズ
			json prefabJson = json::array();
			for (auto e : entities)
			{
				json eJson;
				eJson["ID"] = (uint32_t)e;
				ComponentSerializer::SerializeEntity(reg, e, eJson);
				prefabJson.push_back(eJson);
			}

			// 3. ファイル書き込み
			std::ofstream fout(filepath);
			if (fout.is_open())
			{
				fout << prefabJson.dump(4);
				fout.close();
				Logger::Log("Save Prefab: " + filepath);
			}
		}

		/**
		 * @brief	プレファブ読み込み（ID理マッピングを行いながら）
		 * @return	生成されたルートエンティティ
		 */
		static Entity LoadPrefab(World& world, const std::string& filepath)
		{
			std::ifstream fin(filepath);
			if (!fin.is_open())
			{
				Logger::LogError("Failed to load prefab: " + filepath);
				return NullEntity;
			}

			json prefabJson;
			fin >> prefabJson;
			if (!prefabJson.is_array()) return NullEntity;

			Registry& reg = world.getRegistry();

			// 旧ID -> 新Entity のマッピングテーブル
			std::map<uint32_t, Entity> idMap;
			std::vector<Entity> createdEntities;

			// 1. 全エンティティを生成 & コンポーネント復元
			for (const auto& eJson : prefabJson)
			{
				uint32_t oldID = eJson["ID"].get<uint32_t>();

				// 新しいエンティティ生成
				Entity newEntity = world.create_entity().id();

				// コンポーネント復元
				ComponentSerializer::DeserializeEntity(reg, newEntity, eJson);

				idMap[oldID] = newEntity;
				createdEntities.push_back(newEntity);
			}

			// 2. RelationshipのIDリンクを修正
			for (auto e : createdEntities)
			{
				if (reg.has<Relationship>(e))
				{
					auto& rel = reg.get<Relationship>(e);

					// Parentの書き換え
					if (rel.parent != NullEntity && idMap.count((uint32_t)rel.parent))
					{
						rel.parent = idMap[(uint32_t)rel.parent];
					}
					else
					{
						rel.parent = NullEntity;
					}

					// Childrenの書き換え
					std::vector<Entity> newChildren;
					for (auto c : rel.children)
					{
						if (idMap.count((uint32_t)c))
						{
							newChildren.push_back(idMap[(uint32_t)c]);
						}
					}
					rel.children = newChildren;
				}
			}

			// 先頭のエンティティをルートとして返す
			if (!createdEntities.empty()) return createdEntities[0];
			return NullEntity;
		}

		static void SerializeEntityToJson(Registry& registry, Entity entity, json& outJson)
		{
			outJson["ID"] = (uint32_t)entity;
			ComponentSerializer::SerializeEntity(registry, entity, outJson);

			// 完璧な階層復活も後々
		}

		static Entity DeserializeEntityFromJson(Registry& registry, const json& inJson)
		{
			Entity entity = registry.create();
			ComponentSerializer::DeserializeEntity(registry, entity, inJson);
			return entity;
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

}	// namespace Arche

#endif // !___SCENE_SERIALIZER_H___