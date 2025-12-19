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
#include "Engine/ECS/ECS.h"
#include "Engine/Resource/Serializer.h"
#include "Engine/Physics/CollisionSystem.h"

class SceneSerializer
{
public:
	/**
	 * @brief シーン全体をJSONファイルに保存
	 */
	static void SaveScene(World& world, const std::string& filepath)
	{
		nlohmann::json sceneJson;
		sceneJson["name"] = "Untitled Scene"; // シーン名（必要なら引数で渡す）

		// エンティティ配列
		sceneJson["entities"] = nlohmann::json::array();

		Registry& reg = world.getRegistry();

		// 生きている全エンティティをループ
		reg.each([&](Entity entity) {
			// Tagを持たないエンティティは保存しない（内部処理用などの除外）
			if (!reg.has<Tag>(entity)) return;

			// Serializerの既存ロジックを再利用したいが、
			// Serializer::SaveEntityはファイル書き出しまでしてしまうので、
			// JSONオブジェクトを生成する部分だけ切り出すのがベスト。
			// ★今回は簡易的に、Serializerのロジックをここで再現して配列に追加します。

			nlohmann::json eJson;

			// 基本
			SerializeIf<Tag>(reg, entity, eJson, "Tag");
			SerializeIf<Transform>(reg, entity, eJson, "Transform");
			SerializeIf<Camera>(reg, entity, eJson, "Camera");
			SerializeIf<Relationship>(reg, entity, eJson, "Relationship");

			// 描画
			SerializeIf<MeshComponent>(reg, entity, eJson, "MeshComponent");
			SerializeIf<SpriteComponent>(reg, entity, eJson, "SpriteComponent");
			SerializeIf<BillboardComponent>(reg, entity, eJson, "BillboardComponent");

			// 物理・ロジック
			SerializeIf<Rigidbody>(reg, entity, eJson, "Rigidbody");
			SerializeIf<Collider>(reg, entity, eJson, "Collider");
			SerializeIf<PlayerInput>(reg, entity, eJson, "PlayerInput");
			SerializeIf<Lifetime>(reg, entity, eJson, "Lifetime");

			// オーディオ
			SerializeIf<AudioSource>(reg, entity, eJson, "AudioSource");
			SerializeIf<AudioListener>(reg, entity, eJson, "AudioListener");

			// 配列に追加
			sceneJson["entities"].push_back(eJson);
			});

		// ファイル書き出し
		std::ofstream file(filepath);
		if (file.is_open()) {
			file << sceneJson.dump(4);
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
		std::ifstream file(filepath);
		if (!file.is_open()) {
			Logger::LogError("Failed to load scene: " + filepath);
			return;
		}

		nlohmann::json sceneJson;
		file >> sceneJson;

		// 現在のエンティティをすべて削除
		world.getRegistry().clear();

		// 物理システムの内部状態もリセットする
		CollisionSystem::Reset();

		// 現在のエンティティを全て削除（シーン遷移時）
		// ※マージロードしたい場合はここをスキップするフラグが必要
		world.getRegistry().clear();

		if (sceneJson.contains("entities"))
		{
			for (auto& eJson : sceneJson["entities"])
			{
				// エンティティ生成
				Entity entity = world.create_entity().id();
				Registry& reg = world.getRegistry();

				// コンポーネント復元
				// ※Serializer.h にある DeserializeComponent は private なので、
				//	 ここで同様の処理を書くか、Serializerを拡張して public ヘルパーを作るのが綺麗です。
				//	 今回は Serializer.h を修正せず、ここで直接実装します（Serializer.hの中身と同じことをする）。

				// Tag
				if (eJson.contains("Tag")) {
					Tag c; c.name = StringId(eJson["Tag"]["name"].get<std::string>());
					reg.emplace<Tag>(entity, c);
				}
				// Transform
				if (eJson.contains("Transform")) {
					Transform c;
					auto p = eJson["Transform"]["pos"]; c.position = { p[0], p[1], p[2] };
					auto r = eJson["Transform"]["rot"]; c.rotation = { r[0], r[1], r[2] };
					auto s = eJson["Transform"]["scl"]; c.scale = { s[0], s[1], s[2] };
					reg.emplace<Transform>(entity, c);
				}
				// Camera
				if (eJson.contains("Camera")) {
					Camera c;
					auto& j = eJson["Camera"];
					c.fov = j["fov"]; c.nearZ = j["near"]; c.farZ = j["far"]; c.aspect = j["aspect"];
					reg.emplace<Camera>(entity, c);
				}

				// Relationship
				if (eJson.contains("Relationship")) {
					Relationship c;
					auto& j = eJson["Relationship"];

					// Serializer::FromJson を使うか、直接書く
					if (j.contains("parent")) c.parent = (Entity)j["parent"].get<uint32_t>();
					if (j.contains("children")) {
						for (auto& childId : j["children"]) {
							c.children.push_back((Entity)childId.get<uint32_t>());
						}
					}
					reg.emplace<Relationship>(entity, c);
				}

				// MeshComponent
				if (eJson.contains("MeshComponent")) {
					MeshComponent c;
					auto& j = eJson["MeshComponent"];
					c.modelKey = StringId(j["key"].get<std::string>());
					auto s = j["scale"]; c.scaleOffset = { s[0], s[1], s[2] };
					auto col = j["color"]; c.color = { col[0], col[1], col[2], col[3] };
					reg.emplace<MeshComponent>(entity, c);
				}

				// SpriteComponent
				if (eJson.contains("SpriteComponent")) {
					SpriteComponent c;
					auto& j = eJson["SpriteComponent"];
					c.textureKey = StringId(j["key"].get<std::string>());
					auto col = j["col"]; c.color = { col[0], col[1], col[2], col[3] };
					reg.emplace<SpriteComponent>(entity, c);
				}

				// BillboardComponent
				if (eJson.contains("BillboardComponent")) {
					BillboardComponent c;
					auto& j = eJson["BillboardComponent"];
					c.textureKey = StringId(j["key"].get<std::string>());
					c.size = { j["w"], j["h"] };
					auto col = j["col"]; c.color = { col[0], col[1], col[2], col[3] };
					reg.emplace<BillboardComponent>(entity, c);
				}

				// Rigidbody
				if (eJson.contains("Rigidbody")) {
					Rigidbody c;
					auto& j = eJson["Rigidbody"];
					if (j.contains("type")) c.type = (BodyType)j["type"];
					if (j.contains("vel")) { auto v = j["vel"]; c.velocity = { v[0], v[1], v[2] }; }
					c.mass = j["mass"]; c.drag = j["drag"]; c.useGravity = j["grav"];
					reg.emplace<Rigidbody>(entity, c);
				}

				// Collider
				if (eJson.contains("Collider")) {
					Collider c;
					auto& j = eJson["Collider"];
					c.type = (ColliderType)j["type"];
					auto o = j["offset"]; c.offset = { o[0], o[1], o[2] };

					if (j.contains("layer")) c.layer = (Layer)j["layer"].get<uint32_t>();
					if (j.contains("mask"))	 c.mask = (Layer)j["mask"].get<uint32_t>();

					if (j.contains("boxSize")) { auto s = j["boxSize"]; c.boxSize = { s[0], s[1], s[2] }; }
					if (j.contains("radius")) c.sphere.radius = j["radius"];
					if (j.contains("radius") && j.contains("height")) { c.capsule.radius = j["radius"]; c.capsule.height = j["height"]; }
					if (j.contains("isTrigger")) c.isTrigger = j["isTrigger"];
					reg.emplace<Collider>(entity, c);
				}

				// AudioSource
				if (eJson.contains("AudioSource")) {
					AudioSource c;
					auto& j = eJson["AudioSource"];
					c.soundKey = StringId(j["key"].get<std::string>());
					c.volume = j["vol"]; c.range = j["range"];
					c.isLoop = j["loop"]; if (j.contains("awake")) c.playOnAwake = j["awake"];
					reg.emplace<AudioSource>(entity, c);
				}

				// その他 (PlayerInput, Lifetime, AudioListener)
				if (eJson.contains("PlayerInput")) {
					PlayerInput c; c.speed = eJson["PlayerInput"]["speed"]; c.jumpPower = eJson["PlayerInput"]["jump"];
					reg.emplace<PlayerInput>(entity, c);
				}
				if (eJson.contains("Lifetime")) {
					Lifetime c; c.time = eJson["Lifetime"]["time"];
					reg.emplace<Lifetime>(entity, c);
				}
				if (eJson.contains("AudioListener")) {
					reg.emplace<AudioListener>(entity);
				}
			}
		}

		Logger::Log("Scene Loaded: " + filepath);
	}

private:
	// 保存用ヘルパー
	template<typename T>
	static void SerializeIf(Registry& reg, Entity e, nlohmann::json& j, const std::string& key) {
		if (reg.has<T>(e)) {
			// SerializerクラスのToJsonはprivateなので呼べない。
			// 本来はSerializerをfriendにするかpublicにするべきだが、
			// ここでは「Serializer.h」の実装をコピーして使う形（上記SaveScene参照）をとるか、
			// Serializerクラスに「ToJsonを公開する」改修を入れるのが正しい。

			// ★今回は簡易的に「Serializer::SaveEntity」の中身をコピペして実装する前提で、
			// SaveScene関数内に直接 ToJson のロジックを書くか、
			// Serializerクラスを修正して ToJson を public static にすることを推奨します。

			// 暫定措置：Serializer.h を修正して ToJson を public にしてください。
			// それが一番早いです。
			j[key] = Serializer::ToJson(reg.get<T>(e));
		}
	}
};

#endif // !___SCENE_SERIALIZER_H___
