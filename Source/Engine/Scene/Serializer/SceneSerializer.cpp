#include "Engine/pch.h"
#include "SceneSerializer.h"
#include "Engine/Scene/Systems/Physics/CollisionSystem.h"
#include "Engine/Scene/Serializer/ComponentSerializer.h"
#include "Engine/Scene/Serializer/SystemRegistry.h"
#include "Engine/Scene/Serializer/ComponentRegistry.h"
#include "Engine/Scene/Components/Components.h"

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
		for (auto const& [oldID, entity] : idMap)
		{
			if (registry.has<Relationship>(entity))
			{
				auto& rel = registry.get<Relationship>(entity);

				// 親IDの解決
				if (rel.parent != NullEntity) {
					uint32_t oldParentID = (uint32_t)rel.parent;
					if (idMap.count(oldParentID)) rel.parent = idMap[oldParentID];
					else rel.parent = NullEntity;
				}

				// 子IDリストの解決
				std::vector<Entity> newChildren;
				for (auto oldChildID : rel.children) {
					uint32_t oldID = (uint32_t)oldChildID;
					if (idMap.count(oldID)) newChildren.push_back(idMap[oldID]);
				}
				rel.children = newChildren;
			}
		}

		Logger::Log("Scene Loaded: " + filepath);
	}

	void SceneSerializer::RevertPrefab(World& world, Entity entity)
	{
		Registry& reg = world.getRegistry();

		// プレファブ情報を持っていなければ何もしない
		if (!reg.has<PrefabInstance>(entity)) return;

		std::string path = reg.get<PrefabInstance>(entity).prefabPath;
		if (!std::filesystem::exists(path))
		{
			Logger::LogError("Prefab file not found: " + path);
			return;
		}

		// ファイル読み込み
		std::ifstream fin(path);
		if (!fin.is_open()) return;
		json prefabJson;
		fin >> prefabJson;
		if (!prefabJson.is_array() || prefabJson.empty()) return;

		// --- 復元処理 ---

		// 1. 親子関係とPrefab情報のバックアップ
		Entity parent = NullEntity;
		if (reg.has<Relationship>(entity)) parent = reg.get<Relationship>(entity).parent;

		// 2. コンポーネントを一度すべて削除（クリーンな状態にする）
		// ComponentRegistryを使って全削除
		for (const auto& [name, iface] : ComponentRegistry::Instance().GetInterfaces())
		{
			iface.remove(reg, entity);
		}

		// 3. プレファブからデータをロード
		ComponentSerializer::DeserializeEntity(reg, entity, prefabJson[0]);

		// 4. バックアップ情報の復元
		// 親子関係
		if (reg.has<Relationship>(entity))
		{
			reg.get<Relationship>(entity).parent = parent;
		}
		else if (parent != NullEntity)
		{
			auto& rel = reg.emplace<Relationship>(entity);
			rel.parent = parent;
		}

		// PrefabInstance (Deserializeで入っているはずだが念のためパスを保証)
		if (!reg.has<PrefabInstance>(entity))
		{
			reg.emplace<PrefabInstance>(entity, path);
		}
		else
		{
			reg.get<PrefabInstance>(entity).prefabPath = path;
		}

		// 5. 子階層の再構築 (プレファブ構造に強制一致させる)
		// まず今の子を全削除
		std::function<void(Entity)> deleteChildren = [&](Entity e) {
			if (reg.has<Relationship>(e)) {
				auto children = reg.get<Relationship>(e).children;
				for (auto c : children) {
					deleteChildren(c);
					reg.destroy(c);
				}
				reg.get<Relationship>(e).children.clear();
			}
			};
		deleteChildren(entity);

		// JSONから子を再生成
		ReconstructPrefabChildren(world, entity, prefabJson);

		Logger::Log("Reverted to Prefab: " + path);
	}

	void SceneSerializer::CreateEmptyScene(const std::string& filepath)
	{
		json sceneJson;
		sceneJson["SceneName"] = "New Scene";
		sceneJson["Physics"]["LayerCollision"] = json::array();

		// 標準システム
		struct SysDef { std::string name; SystemGroup group; };
		std::vector<SysDef> standardSystems = {
			{ "Physics System", SystemGroup::PlayOnly },
			{ "Collision System", SystemGroup::PlayOnly }, { "UI System", SystemGroup::Always },
			{ "Lifetime System", SystemGroup::PlayOnly }, { "Hierarchy System", SystemGroup::Always },
			{"Animation System", SystemGroup::PlayOnly },
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

	// パス比較用ヘルパー
	bool IsSamePath(const std::string& pathA, const std::string& pathB)
	{
		try
		{
			std::filesystem::path a = std::filesystem::u8path(pathA);
			std::filesystem::path b = std::filesystem::u8path(pathB);
			// パスを正規化して比較
			return a.make_preferred().string() == b.make_preferred().string();
		}
		catch (...)
		{
			return pathA == pathB;
		}
	}

	// プレファブの変更をシーン上のインスタンスに反映する
	void SceneSerializer::ReloadPrefabInstances(World& world, const std::string& filepath)
	{
		Registry& reg = world.getRegistry();
		std::vector<Entity> targets;

		// 1. 更新対象のエンティティ（プレファブのルート）を収集
		reg.view<PrefabInstance>().each([&](Entity e, PrefabInstance& pref) {
			if (IsSamePath(pref.prefabPath, filepath)) {
				targets.push_back(e);
			}
			});

		if (targets.empty()) return;

		// プレファブデータの読み込み
		std::ifstream fin(filepath);
		if (!fin.is_open()) return;
		json prefabJson;
		fin >> prefabJson;
		if (!prefabJson.is_array() || prefabJson.empty()) return;

		// 各インスタンスに対して更新処理を実行
		for (Entity root : targets)
		{
			// A. 固有データのバックアップ
			Transform backupTransform;
			bool hasTransform = reg.has<Transform>(root);
			if (hasTransform) backupTransform = reg.get<Transform>(root);

			Transform2D backupTransform2D;
			bool hasTransform2D = reg.has<Transform2D>(root);
			if (hasTransform2D) backupTransform2D = reg.get<Transform2D>(root);

			Tag backupTag;
			bool hasTag = reg.has<Tag>(root);
			if (hasTag) backupTag = reg.get<Tag>(root);

			// 親子関係のバックアップ（Editor.cppの実装から補完）
			Entity parent = NullEntity;
			if (reg.has<Relationship>(root)) parent = reg.get<Relationship>(root).parent;

			// B. 既存の子階層を全削除
			if (reg.has<Relationship>(root)) {
				std::function<void(Entity)> deleteRecursive = [&](Entity e) {
					if (reg.has<Relationship>(e)) {
						for (auto child : reg.get<Relationship>(e).children) deleteRecursive(child);
					}
					world.getRegistry().destroy(e);
					};
				for (auto child : reg.get<Relationship>(root).children) {
					deleteRecursive(child);
				}
			}

			// C. コンポーネント全削除
			for (const auto& [name, iface] : ComponentRegistry::Instance().GetInterfaces()) {
				iface.remove(reg, root);
			}

			// D. プレファブから復元（ここで回転・スケールは新しい値になる）
			ComponentSerializer::DeserializeEntity(reg, root, prefabJson[0]);

			// E. 子エンティティ再構築
			ReconstructPrefabChildren(world, root, prefabJson);

			// F. データのマージ復元
			if (hasTransform) {
				if (!reg.has<Transform>(root)) reg.emplace<Transform>(root);

				auto& currentTransform = reg.get<Transform>(root);

				// 位置のみ「シーンにあった状態」に戻す
				currentTransform.position = backupTransform.position;

				// 回転・スケール
				// currentTransform.rotation = backupTransform.rotation; 
				// currentTransform.scale = backupTransform.scale;

				// 行列更新
				XMStoreFloat4x4(&currentTransform.worldMatrix, currentTransform.GetLocalMatrix());

				// 変更通知
				reg.patch<Transform>(root);
			}

			if (hasTransform2D)
			{
				if (!reg.has<Transform2D>(root)) reg.emplace<Transform2D>(root);
				auto& current = reg.get<Transform2D>(root);

				current.position = backupTransform2D.position;
				
				reg.patch<Transform2D>(root);
			}

			if (hasTag) {
				reg.emplace<Tag>(root, backupTag);
			}

			// 親子関係の復元 (Editor.cppの実装を取り込み)
			if (reg.has<Relationship>(root))
			{
				reg.get<Relationship>(root).parent = parent;
			}
			else if (parent != NullEntity)
			{
				auto& rel = reg.emplace<Relationship>(root);
				rel.parent = parent;
			}

			// PrefabInstance情報の復元
			reg.emplace<PrefabInstance>(root, filepath);
		}

		Logger::Log("Reloaded instances for: " + filepath);
	}
}