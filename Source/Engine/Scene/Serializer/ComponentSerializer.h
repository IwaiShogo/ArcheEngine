/*****************************************************************//**
 * @file	ComponentSerializer.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/20	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COMPONENT_SERIALIZER_H___
#define ___COMPONENT_SERIALIZER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/ECS/ECS.h"
#include "Engine/Core/Reflection.h"
#include "Engine/Scene/Components/Components.h"

// -----------------------------------------------------------------------------
// [Visitor] データを JSON に書き込む (Save)
// -----------------------------------------------------------------------------
struct SerializeVisitor {
	json& j;

	// 基本型
	template <typename T>
	void operator()(const T& val, const char* name) {
		j[name] = val;
	}

	// DirectX Math系 (配列として保存)
	void operator()(const DirectX::XMFLOAT2& val, const char* name) {
		j[name] = { val.x, val.y };
	}
	void operator()(const DirectX::XMFLOAT3& val, const char* name) {
		j[name] = { val.x, val.y, val.z };
	}
	void operator()(const DirectX::XMFLOAT4& val, const char* name) {
		j[name] = { val.x, val.y, val.z, val.w };
	}

	// StringId (文字列として保存)
	void operator()(const StringId& val, const char* name) {
		j[name] = val.c_str();
	}

	// Entity型 (uint32_tとして保存)
	void operator()(const Entity& val, const char* name) {
		j[name] = (uint32_t)val;
	}

	// td::vector<Entity>型 (配列として保存)
	void operator()(const std::vector<Entity>& val, const char* name) {
		std::vector<uint32_t> ids;
		for (auto e : val) ids.push_back((uint32_t)e);
		j[name] = ids;
	}

	// Enum型の汎用対応（intキャスト）
	template<typename T>
		requires std::is_enum_v<T>
	void operator()(const T& val, const char* name) {
		j[name] = static_cast<int>(val);
	}
};

// -----------------------------------------------------------------------------
// [Visitor] JSON からデータを読み込む (Load)
// -----------------------------------------------------------------------------
struct DeserializeVisitor {
	const json& j;

	// 基本型
	template <typename T>
	void operator()(T& val, const char* name) {
		if (j.contains(name)) val = j[name].get<T>();
	}

	// DirectX Math系
	void operator()(DirectX::XMFLOAT2& val, const char* name) {
		if (j.contains(name)) {
			auto arr = j[name];
			if (arr.size() >= 2) { val.x = arr[0]; val.y = arr[1]; }
		}
	}
	void operator()(DirectX::XMFLOAT3& val, const char* name) {
		if (j.contains(name)) {
			auto arr = j[name];
			if (arr.size() >= 3) { val.x = arr[0]; val.y = arr[1]; val.z = arr[2]; }
		}
	}
	void operator()(DirectX::XMFLOAT4& val, const char* name) {
		if (j.contains(name)) {
			auto arr = j[name];
			if (arr.size() >= 4) { val.x = arr[0]; val.y = arr[1]; val.z = arr[2]; val.w = arr[3]; }
		}
	}

	// StringId
	void operator()(StringId& val, const char* name) {
		if (j.contains(name)) {
			std::string s = j[name].get<std::string>();
			val = StringId(s.c_str());
		}
	}

	// Entity型
	void operator()(Entity& val, const char* name) {
		if (j.contains(name)) val = (Entity)j[name].get<uint32_t>();
	}

	// std::vector<Entity>型
	void operator()(std::vector<Entity>& val, const char* name) {
		if (j.contains(name)) {
			val.clear();
			for (auto& id : j[name]) {
				val.push_back((Entity)id.get<uint32_t>());
			}
		}
	}

	// Enum型
	template<typename T>
		requires std::is_enum_v<T>
	void operator()(T& val, const char* name) {
		if (j.contains(name)) val = static_cast<T>(j[name].get<int>());
	}
};

// -----------------------------------------------------------------------------
// [Utility] エンティティ全体のシリアライズ処理
// -----------------------------------------------------------------------------
class ComponentSerializer {
public:
	// エンティティが持っている全コンポーネントをJSONに変換
	static void SerializeEntity(Registry& reg, Entity entity, json& outJson) {
		// ComponentList (Tuple) を使って全型を走査
		DrawAll<ComponentList>(reg, entity, outJson);
	}

	// JSONからコンポーネントを復元
	static void DeserializeEntity(Registry& reg, Entity entity, const json& inJson) {
		LoadAll<ComponentList>(reg, entity, inJson);
	}

private:
	// メタプログラミング: 全コンポーネントループ (Save)
	template <typename TupleT>
	static void DrawAll(Registry& reg, Entity entity, json& outJson) {
		auto view = [&]<typename... Ts>(std::tuple<Ts...>*) {
			(..., [&](void) {
				using T = Ts;

				if constexpr (std::is_same_v<T, DummyComponent>) return;

				if (reg.has<T>(entity)) {
					json compJson;
					// Reflectionを使ってメンバを保存
					Reflection::VisitMembers(reg.get<T>(entity), SerializeVisitor{ compJson });
					// コンポーネント名をキーにして登録
					outJson[Reflection::Meta<T>::Name] = compJson;
				}
				}());
		};
		view((TupleT*)nullptr);
	}

	// メタプログラミング: 全コンポーネントループ (Load)
	template <typename TupleT>
	static void LoadAll(Registry& reg, Entity entity, const json& inJson) {
		auto view = [&]<typename... Ts>(std::tuple<Ts...>*) {
			(..., [&](void) {
				using T = Ts;

				if constexpr (std::is_same_v<T, DummyComponent>) return;

				const char* name = Reflection::Meta<T>::Name;
				// JSONにこのコンポーネントのデータがあれば
				if (inJson.contains(name)) {
					// コンポーネントを追加 (既にあれば取得)
					T& comp = reg.has<T>(entity) ? reg.get<T>(entity) : reg.emplace<T>(entity);
					// Reflectionを使ってメンバを復元
					Reflection::VisitMembers(comp, DeserializeVisitor{ inJson[name] });
				}
				}());
		};
		view((TupleT*)nullptr);
	}
};

#endif // !___COMPONENT_SERIALIZER_H___