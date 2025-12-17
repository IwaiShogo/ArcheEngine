/*****************************************************************//**
 * @file	Serializer.h
 * @brief	各コンポーネントのデータを読み書きするクラス
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
 * @note	
 * コンポーネントを追加した際：
 * 1. SaveEntity()内に登録
 * 2. LoadEntity()内に登録
 * 3. ToJson(), FromJson()を作成
 *********************************************************************/

#ifndef ___SERIALIZER_H___
#define ___SERIALIZER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/ECS/ECS.h"
#include "Engine/Components/Components.h"

using json = nlohmann::json;

class Serializer {
public:
	// エンティティをJSONファイルとして保存（プレハブ化）
	static void SaveEntity(Registry& registry, Entity entity, const std::string& filepath) {
		if (!registry.has<Tag>(entity)) return;

		json j;
		// 各コンポーネントをチェックして保存
		SerializeComponent<Tag>(registry, entity, j, "Tag");
		SerializeComponent<Transform>(registry, entity, j, "Transform");
		SerializeComponent<Camera>(registry, entity, j, "Camera");

		// レンダリング系
		SerializeComponent<MeshComponent>(registry, entity, j, "MeshComponent");
		SerializeComponent<SpriteComponent>(registry, entity, j, "SpriteComponent");
		SerializeComponent<BillboardComponent>(registry, entity, j, "BillboardComponent");

		// 物理・入力系
		SerializeComponent<Rigidbody>(registry, entity, j, "Rigidbody");
		SerializeComponent<Collider>(registry, entity, j, "Collider");
		SerializeComponent<PlayerInput>(registry, entity, j, "PlayerInput");

		// オーディオ・その他
		SerializeComponent<AudioSource>(registry, entity, j, "AudioSource");
		SerializeComponent<AudioListener>(registry, entity, j, "AudioListener");
		SerializeComponent<Lifetime>(registry, entity, j, "Lifetime");

		// ファイル書き出し
		std::ofstream o(filepath);
		o << j.dump(4); // 4スペースインデントで見やすく
	}

	// JSONファイルからエンティティを生成
	static Entity LoadEntity(World& world, const std::string& filepath) {
		std::ifstream i(filepath);
		if (!i.is_open()) return NullEntity;

		json j;
		i >> j;

		Entity e = world.create_entity().id();
		Registry& reg = world.getRegistry();

		// 各コンポーネントを読み込み
		DeserializeComponent<Tag>(reg, e, j, "Tag");
		DeserializeComponent<Transform>(reg, e, j, "Transform");
		DeserializeComponent<Camera>(reg, e, j, "Camera");

		DeserializeComponent<MeshComponent>(reg, e, j, "MeshComponent");
		DeserializeComponent<SpriteComponent>(reg, e, j, "SpriteComponent");
		DeserializeComponent<BillboardComponent>(reg, e, j, "BillboardComponent");

		DeserializeComponent<Rigidbody>(reg, e, j, "Rigidbody");
		DeserializeComponent<Collider>(reg, e, j, "Collider");
		DeserializeComponent<PlayerInput>(reg, e, j, "PlayerInput");

		DeserializeComponent<AudioSource>(reg, e, j, "AudioSource");
		DeserializeComponent<AudioListener>(reg, e, j, "AudioListener");
		DeserializeComponent<Lifetime>(reg, e, j, "Lifetime");

		return e;
	}

private:
	// --- ヘルパー関数 ---

	// 保存用
	template<typename T>
	static void SerializeComponent(Registry& reg, Entity e, json& j, const std::string& key) {
		if (reg.has<T>(e)) {
			T& comp = reg.get<T>(e);
			j[key] = ToJson(comp);
		}
	}

	// 読み込み用
	template<typename T>
	static void DeserializeComponent(Registry& reg, Entity e, const json& j, const std::string& key) {
		if (j.contains(key)) {
			T comp;
			FromJson(j[key], comp);
			reg.emplace<T>(e, comp);
		}
	}

	// --- 各コンポーネントの変換定義 (ToJson / FromJson) ---

	// Tag
	static json ToJson(const Tag& c) { return { {"name", c.name.c_str()}}; }
	static void FromJson(const json& j, Tag& c) { c.name = j["name"].get<std::string>(); }

	// Transform
	static json ToJson(const Transform& c) {
		return {
			{"pos", {c.position.x, c.position.y, c.position.z}},
			{"rot", {c.rotation.x, c.rotation.y, c.rotation.z}},
			{"scl", {c.scale.x, c.scale.y, c.scale.z}}
		};
	}
	static void FromJson(const json& j, Transform& c) {
		auto p = j["pos"]; c.position = { p[0], p[1], p[2] };
		auto r = j["rot"]; c.rotation = { r[0], r[1], r[2] };
		auto s = j["scl"]; c.scale = { s[0], s[1], s[2] };
	}

	// Camera
	static json ToJson(const Camera& c) {
		return { {"fov", c.fov}, {"near", c.nearZ}, {"far", c.farZ}, {"aspect", c.aspect} };
	}
	static void FromJson(const json& j, Camera& c) {
		c.fov = j["fov"]; c.nearZ = j["near"]; c.farZ = j["far"]; c.aspect = j["aspect"];
	}

	// MeshComponent
	static json ToJson(const MeshComponent& c) {
		return {
			{"key", c.modelKey},
			{"scale", {c.scaleOffset.x, c.scaleOffset.y, c.scaleOffset.z}},
			{"color", {c.color.x, c.color.y, c.color.z, c.color.w}}
		};
	}
	static void FromJson(const json& j, MeshComponent& c) {
		c.modelKey = j["key"];
		auto s = j["scale"]; c.scaleOffset = { s[0], s[1], s[2] };
		auto col = j["color"]; c.color = { col[0], col[1], col[2], col[3] };
	}

	// SpriteComponent
	static json ToJson(const SpriteComponent& c) {
		return {
			{"key", c.textureKey}, {"w", c.width}, {"h", c.height},
			{"col", {c.color.x, c.color.y, c.color.z, c.color.w}},
			{"piv", {c.pivot.x, c.pivot.y}}
		};
	}
	static void FromJson(const json& j, SpriteComponent& c) {
		c.textureKey = j["key"];
		c.width = j["w"]; c.height = j["h"];
		auto col = j["col"]; c.color = { col[0], col[1], col[2], col[3] };
		auto piv = j["piv"]; c.pivot = { piv[0], piv[1] };
	}

	// BillboardComponent
	static json ToJson(const BillboardComponent& c) {
		return {
			{"key", c.textureKey}, {"w", c.size.x}, {"h", c.size.y},
			{"col", {c.color.x, c.color.y, c.color.z, c.color.w}}
		};
	}
	static void FromJson(const json& j, BillboardComponent& c) {
		c.textureKey = j["key"];
		c.size = { j["w"], j["h"] };
		auto col = j["col"]; c.color = { col[0], col[1], col[2], col[3] };
	}

	// Rigidbody
	static json ToJson(const Rigidbody& c) {
		return {
			{"type", (int)c.type},	// enumをintで保存
			{"vel", {c.velocity.x, c.velocity.y, c.velocity.z}},
			{"mass", c.mass}, {"drag", c.drag}, {"grav", c.useGravity}
		};
	}
	static void FromJson(const json& j, Rigidbody& c) {
		if (j.contains("type")) c.type = (BodyType)j["type"];
		if (j.contains("vel")) { auto v = j["vel"]; c.velocity = { v[0], v[1], v[2] }; }
		c.mass = j["mass"]; c.drag = j["drag"];
		c.useGravity = j["grav"];
	}

	// Collider
	static json ToJson(const Collider& c) {
		json j;
		j["type"] = (int)c.type;
		j["offset"] = { c.offset.x, c.offset.y, c.offset.z };
		// タイプに応じて保存内容を変える
		if (c.type == ColliderType::Box) {
			j["boxSize"] = { c.boxSize.x, c.boxSize.y, c.boxSize.z };
		}
		else if (c.type == ColliderType::Sphere) {
			j["radius"] = c.sphere.radius;
		}
		else if (c.type == ColliderType::Capsule) {
			j["radius"] = c.capsule.radius;
			j["height"] = c.capsule.height;
		}
		j["isTrigger"] = c.isTrigger;
		return j;
	}
	static void FromJson(const json& j, Collider& c) {
		c.type = (ColliderType)j["type"];
		auto o = j["offset"]; c.offset = { o[0], o[1], o[2] };

		if (j.contains("boxSize")) {
			auto s = j["boxSize"]; c.boxSize = { s[0], s[1], s[2] };
		}
		if (j.contains("radius")) c.sphere.radius = j["radius"];
		if (j.contains("radius") && j.contains("height")) {
			c.capsule.radius = j["radius"];
			c.capsule.height = j["height"];
		}
		if (j.contains("isTrigger")) c.isTrigger = j["isTrigger"];
	}

	// PlayerInput
	static json ToJson(const PlayerInput& c) {
		return { {"speed", c.speed}, {"jump", c.jumpPower} };
	}
	static void FromJson(const json& j, PlayerInput& c) {
		c.speed = j["speed"]; c.jumpPower = j["jump"];
	}

	// AudioSource
	static json ToJson(const AudioSource& c) {
		return {
			{"key", c.soundKey}, {"vol", c.volume},
			{"range", c.range}, {"loop", c.isLoop}, {"awake", c.playOnAwake}
		};
	}
	static void FromJson(const json& j, AudioSource& c) {
		c.soundKey = j["key"];
		c.volume = j["vol"];
		c.range = j["range"];
		c.isLoop = j["loop"];
		if (j.contains("awake")) c.playOnAwake = j["awake"];
	}

	// AudioListener (データなし)
	static json ToJson(const AudioListener& c) { return json::object(); }
	static void FromJson(const json& j, AudioListener& c) {}

	// Lifetime
	static json ToJson(const Lifetime& c) { return { {"time", c.time} }; }
	static void FromJson(const json& j, Lifetime& c) { c.time = j["time"]; }
};

#endif // !___SERIALIZER_H___