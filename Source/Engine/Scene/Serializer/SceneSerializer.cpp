#include "Engine/pch.h"
#include "SceneSerializer.h"
#include "Engine/Scene/Systems/Physics/CollisionSystem.h"
#include "Engine/Scene/Serializer/ComponentSerializer.h"
#include "Engine/Scene/Serializer/SystemRegistry.h"
#include "Engine/Scene/Components/Components.h" // Tagなどのために必要

namespace Arche
{
	void SceneSerializer::SaveScene(World& world, const std::string& filepath)
	{
		json sceneJson;
		sceneJson["SceneName"] = "Untitled Scene";

		// 1. レイヤー衝突設定
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

		// 2. システム構成
		sceneJson["Systems"] = json::array();
		for (const auto& sys : world.getSystems())
		{
			json sysJson;
			sysJson["Name"] = sys->m_systemName;
			sysJson["Group"] = (int)sys->m_group;
			sceneJson["Systems"].push_back(sysJson);
		}

		// 3. エンティティ
		sceneJson["Entities"] = json::array();
		auto& registry = world.getRegistry();

		registry.each([&](auto entityID)
			{
				Entity entity = entityID;
				// Tagを持たない（内部的な）エンティティは保存しない
				if (!registry.has<Tag>(entity)) return;

				json entityJson;
				entityJson["ID"] = (uint32_t)entity;

				entityJson["IsActive"] = registry.isActiveSelf(entity);

				ComponentSerializer::SerializeEntity(registry, entity, entityJson);
				sceneJson["Entities"].push_back(entityJson);
			});

		std::ofstream fout(filepath);
		if (fout.is_open()) {
			fout << sceneJson.dump(4);
			fout.close();
			Logger::Log("Scene Saved: " + filepath);
		}
		else {
			Logger::LogError("Failed to save scene: " + filepath);
		}
	}

	void SceneSerializer::LoadScene(World& world, const std::string& filepath)
	{
		std::ifstream fin(filepath);
		if (!fin.is_open()) {
			Logger::LogError("Failed to load scene: " + filepath);
			return;
		}

		world.clearSystems();
		world.clearEntities();
		CollisionSystem::Reset();
		PhysicsConfig::Reset();

		json sceneJson;
		try { fin >> sceneJson; }
		catch (json::parse_error& e) {
			Logger::LogError(std::string("JSON Parse Error: ") + e.what());
			return;
		}

		// Physics
		if (sceneJson.contains("Physics") && sceneJson["Physics"].contains("LayerCollision"))
		{
			for (auto& layerJson : sceneJson["Physics"]["LayerCollision"])
			{
				Layer layer = (Layer)layerJson["Layer"].get<int>();
				Layer mask = (Layer)layerJson["Mask"].get<int>();
				PhysicsConfig::Configure(layer).setMask(mask);
			}
		}

		// Systems
		if (sceneJson.contains("Systems"))
		{
			for (auto& sysJson : sceneJson["Systems"])
			{
				std::string name = sysJson["Name"].get<std::string>();
				SystemGroup group = SystemGroup::PlayOnly;
				if (sysJson.contains("Group")) group = (SystemGroup)sysJson["Group"].get<int>();
				SystemRegistry::Instance().CreateSystem(world, name, group);
			}
		}

		// Entities
		auto& registry = world.getRegistry();
		registry.clear();
		std::unordered_map<uint32_t, Entity> idMap;

		if (sceneJson.contains("Entities")) {
			for (auto& entityJson : sceneJson["Entities"]) {
				Entity newEntity = registry.create();
				uint32_t oldID = entityJson["ID"].get<uint32_t>();
				idMap[oldID] = newEntity;

				if (entityJson.contains("IsActive"))
				{
					registry.setActive(newEntity, entityJson["IsActive"].get<bool>());
				}

				ComponentSerializer::DeserializeEntity(registry, newEntity, entityJson);
			}
		}

		// Relationship Fix
		auto view = registry.view<Relationship>();
		for (auto entity : view)
		{
			auto& rel = view.get<Relationship>(entity);
			if (rel.parent != NullEntity) {
				uint32_t oldParentID = (uint32_t)rel.parent;
				if (idMap.count(oldParentID)) rel.parent = idMap[oldParentID];
				else rel.parent = NullEntity;
			}
			std::vector<Entity> newChildren;
			for (auto oldChildID : rel.children) {
				uint32_t oldID = (uint32_t)oldChildID;
				if (idMap.count(oldID)) newChildren.push_back(idMap[oldID]);
			}
			rel.children = newChildren;
		}
		Logger::Log("Scene Loaded: " + filepath);
	}

	void SceneSerializer::CreateEmptyScene(const std::string& filepath)
	{
		json sceneJson;
		sceneJson["SceneName"] = "New Scene";
		sceneJson["Physics"]["LayerCollision"] = json::array();

		// 標準システム
		struct SysDef { std::string name; SystemGroup group; };
		std::vector<SysDef> standardSystems = {
			{ "Input System", SystemGroup::Always }, { "Physics System", SystemGroup::PlayOnly },
			{ "Collision System", SystemGroup::PlayOnly }, { "UI System", SystemGroup::Always },
			{ "Lifetime System", SystemGroup::PlayOnly }, { "Hierarchy System", SystemGroup::Always },
			{ "Render System", SystemGroup::Always }, { "Model Render System", SystemGroup::Always },
			{ "Billboard System", SystemGroup::Always }, { "Sprite Render System", SystemGroup::Always },
			{ "Text Render System", SystemGroup::Always }, { "Audio System", SystemGroup::Always },
			{ "Button System", SystemGroup::Always }
		};
		sceneJson["Systems"] = json::array();
		for (const auto& sys : standardSystems) {
			json s; s["Name"] = sys.name; s["Group"] = (int)sys.group;
			sceneJson["Systems"].push_back(s);
		}
		sceneJson["Entities"] = json::array();

		std::ofstream fout(filepath);
		if (fout.is_open()) { fout << sceneJson.dump(4); fout.close(); Logger::Log("Created: " + filepath); }
	}

	void SceneSerializer::SavePrefab(Registry& reg, Entity root, const std::string& filepath)
	{
		std::vector<Entity> entities;
		std::function<void(Entity)> collect = [&](Entity e) {
			entities.push_back(e);
			if (reg.has<Relationship>(e)) for (auto c : reg.get<Relationship>(e).children) collect(c);
			};
		collect(root);

		json prefabJson = json::array();
		for (auto e : entities) {
			json eJson;
			eJson["ID"] = (uint32_t)e;
			ComponentSerializer::SerializeEntity(reg, e, eJson);
			prefabJson.push_back(eJson);
		}
		std::ofstream fout(filepath);
		if (fout.is_open()) { fout << prefabJson.dump(4); fout.close(); Logger::Log("Saved Prefab: " + filepath); }
	}

	Entity SceneSerializer::LoadPrefab(World& world, const std::string& filepath)
	{
		std::ifstream fin(filepath);
		if (!fin.is_open()) return NullEntity;
		json prefabJson;
		fin >> prefabJson;
		if (!prefabJson.is_array()) return NullEntity;

		Registry& reg = world.getRegistry();
		std::map<uint32_t, Entity> idMap;
		std::vector<Entity> createdEntities;

		for (const auto& eJson : prefabJson) {
			uint32_t oldID = eJson["ID"].get<uint32_t>();
			Entity newEntity = world.create_entity().id();
			ComponentSerializer::DeserializeEntity(reg, newEntity, eJson);
			idMap[oldID] = newEntity;
			createdEntities.push_back(newEntity);
		}

		for (auto e : createdEntities) {
			if (reg.has<Relationship>(e)) {
				auto& rel = reg.get<Relationship>(e);
				if (rel.parent != NullEntity && idMap.count((uint32_t)rel.parent)) rel.parent = idMap[(uint32_t)rel.parent];
				else rel.parent = NullEntity;
				std::vector<Entity> newChildren;
				for (auto c : rel.children) if (idMap.count((uint32_t)c)) newChildren.push_back(idMap[(uint32_t)c]);
				rel.children = newChildren;
			}
		}

		Entity root = createdEntities[0];

		// ルートにPrefabInstanceコンポーネントを付与
		if (!world.getRegistry().has<PrefabInstance>(root))
		{
			world.getRegistry().emplace<PrefabInstance>(root, filepath);
		}

		world.getRegistry().get<PrefabInstance>(root).prefabPath = filepath;

		return root;
	}

	void SceneSerializer::SerializeEntityToJson(Registry& registry, Entity entity, json& outJson) {
		outJson["ID"] = (uint32_t)entity;
		ComponentSerializer::SerializeEntity(registry, entity, outJson);
	}

	Entity SceneSerializer::DeserializeEntityFromJson(Registry& registry, const json& inJson) {
		Entity entity = registry.create();
		ComponentSerializer::DeserializeEntity(registry, entity, inJson);
		return entity;
	}

	void SceneSerializer::ReconstructPrefabChildren(World& world, Entity root, const json& prefabJson)
	{
		Registry& reg = world.getRegistry();
		std::map<uint32_t, Entity> idMap; // 旧ID -> 新ID

		// ルートのIDマップ登録 (JSONの最初の要素がルートと仮定)
		uint32_t rootOldID = prefabJson[0]["ID"].get<uint32_t>();
		idMap[rootOldID] = root;

		// JSONの2つ目以降（子要素）を生成
		for (size_t i = 1; i < prefabJson.size(); ++i)
		{
			const auto& eJson = prefabJson[i];
			uint32_t oldID = eJson["ID"].get<uint32_t>();

			// 新規作成
			Entity newEntity = world.create_entity().id();
			ComponentSerializer::DeserializeEntity(reg, newEntity, eJson);

			idMap[oldID] = newEntity;
		}

		// 親子関係の再構築
		// rootとその子要素の関係のみを復元する
		for (auto const& [oldID, newEntity] : idMap)
		{
			if (reg.has<Relationship>(newEntity))
			{
				auto& rel = reg.get<Relationship>(newEntity);

				// 親IDの解決
				if (rel.parent != NullEntity)
				{
					uint32_t oldParentID = (uint32_t)rel.parent;
					if (idMap.count(oldParentID))
					{
						rel.parent = idMap[oldParentID];
					}
					else
					{
						// マップにない親（プレファブ外）の場合はリンクを切るか、rootにする
						// 基本的にプレファブ内の子はプレファブ内の親を持つはず
						rel.parent = NullEntity;
					}
				}

				// 子IDリストの解決
				std::vector<Entity> newChildren;
				for (auto oldChildID : rel.children)
				{
					uint32_t oldCID = (uint32_t)oldChildID;
					if (idMap.count(oldCID))
					{
						newChildren.push_back(idMap[oldCID]);
					}
				}
				rel.children = newChildren;
			}
		}
	}
}