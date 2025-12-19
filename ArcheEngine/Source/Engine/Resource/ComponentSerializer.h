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
#include "Engine/ECS/ECS.h"
#include "Engine/Core/Reflection.h"
#include "Engine/Components/Components.h"

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