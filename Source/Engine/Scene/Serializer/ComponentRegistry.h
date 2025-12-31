/*****************************************************************//**
 * @file	ComponentRegistry.h
 * @brief	コンポーネントの自動登録・管理・シリアライズ機能
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___COMPONENT_REGISTRY_H___
#define ___COMPONENT_REGISTRY_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Core/Base/Reflection.h"

// エディタ機能（InspectorGui）を利用可能にする
#ifdef _DEBUG
#include "Editor/Core/InspectorGui.h"
#endif // _DEBUG

namespace Arche
{
	// シリアライズ用ビジター（保存）
	// ------------------------------------------------------------
	struct SerializeVisitor
	{
		json& j;
		template<typename T>
		void operator()(const T& val, const char* name) { j[name] = val; }
		void operator()(const XMFLOAT2& v, const char* name) { j[name] = { v.x, v.y }; }
		void operator()(const XMFLOAT3& v, const char* name) { j[name] = { v.x, v.y, v.z }; }
		void operator()(const XMFLOAT4& v, const char* name) { j[name] = { v.x, v.y, v.z, v.w }; }
		void operator()(const StringId& v, const char* name) { j[name] = v.c_str(); }
		void operator()(const Entity& v, const char* name) { j[name] = (uint32_t)v; }
		template<typename T> requires std::is_enum_v<T> void operator()(const T& v, const char* name) { j[name] = (int)v; }
		void operator()(const std::vector<Entity>& v, const char* name)
		{
			std::vector<uint32_t> ids; for (auto e : v) ids.push_back((uint32_t)e); j[name] = ids;
		}
	};

	// デシリアライズ用ビジター
	// ------------------------------------------------------------
	struct DeserializeVisitor
	{
		const json& j;
		template<typename T>
		void operator()(T& val, const char* name) { if (j.contains(name)) val = j[name].get<T>(); }
		void operator()(DirectX::XMFLOAT2& v, const char* name) { if (j.contains(name)) { auto a = j[name]; if (a.size() >= 2) { v.x = a[0]; v.y = a[1]; } } }
		void operator()(DirectX::XMFLOAT3& v, const char* name) { if (j.contains(name)) { auto a = j[name]; if (a.size() >= 3) { v.x = a[0]; v.y = a[1]; v.z = a[2]; } } }
		void operator()(DirectX::XMFLOAT4& v, const char* name) { if (j.contains(name)) { auto a = j[name]; if (a.size() >= 4) { v.x = a[0]; v.y = a[1]; v.z = a[2]; v.w = a[3]; } } }
		void operator()(StringId& v, const char* name) { if (j.contains(name)) v = StringId(j[name].get<std::string>().c_str()); }
		void operator()(Entity& v, const char* name) { if (j.contains(name)) v = (Entity)j[name].get<uint32_t>(); }
		template<typename T>requires std::is_enum_v<T>
		void operator()(T& v, const char* name) { if (j.contains(name)) v = (T)j[name].get<int>(); }
		void operator()(std::vector<Entity>& v, const char* name)
		{
			if (j.contains(name)) { v.clear(); for (auto& id : j[name]) v.push_back((Entity)id.get<uint32_t>()); }
		}
	};

	// コンポーネントレジストリ
	// ------------------------------------------------------------
	class ComponentRegistry
	{
	public:
		// コマンド発行用コールバック
		using CommandCallback = std::function<void(json oldVal, json newVal)>;

		struct Interface
		{
			std::string name;
			std::function<void(Registry&, Entity, json&)> serialize;
			std::function<void(Registry&, Entity, const json&)> deserialize;
			std::function<void(Registry&, Entity)> add;
			std::function<void(Registry&, Entity)> remove;
			std::function<bool(Registry&, Entity)> has;
			std::function<void(Registry&, Entity, CommandCallback)> drawInspector;
			std::function<void(Registry&, Entity, int, std::function<void(int, int)>, std::function<void()>, CommandCallback)> drawInspectorDnD;
		};

		static ComponentRegistry& Instance() { static ComponentRegistry s; return s; }

		// コンポーネント登録関数
		template<typename T>
		void Register(const char* name)
		{
			std::string nameStr = name;

			// 既に登録済みならスキップ
			if (m_interfaces.find(nameStr) != m_interfaces.end()) return;

			Interface iface;
			iface.name = nameStr;

			// Serialize
			iface.serialize = [nameStr](Registry& reg, Entity e, json& j)
			{
				if (reg.has<T>(e))
				{
					json compJ;
					Reflection::VisitMembers(reg.get<T>(e), SerializeVisitor{ compJ });
					j[nameStr] = compJ;
				}
			};

			// Deserialize
			iface.deserialize = [nameStr](Registry& reg, Entity e, const json& j)
			{
				if (j.contains(nameStr))
				{
					// 持っていなければ追加、あれば取得
					T& comp = reg.has<T>(e) ? reg.get<T>(e) : reg.emplace<T>(e);
					Reflection::VisitMembers(comp, DeserializeVisitor{ j[nameStr] });
				}
			};

			// Add
			iface.add = [](Registry& reg, Entity e)
			{
					if (!reg.has<T>(e)) reg.emplace<T>(e);
			};

			// Remove
			iface.remove = [](Registry& reg, Entity e)
			{
				if (reg.has<T>(e)) reg.remove<T>(e);
			};

			// Has
			iface.has = [](Registry& reg, Entity e)
			{
					return reg.has<T>(e);
			};

			// DrawInspector（Debugのみ有効）
			iface.drawInspectorDnD = [nameStr, iface](Registry& reg, Entity e, int index, std::function<void(int, int)> onReorder, std::function<void()> onRemove, CommandCallback onCommand)
			{
#ifdef _DEBUG
				if(reg.has<T>(e))
				{
					bool removed = false;

					// Visitorに必要なシリアライズ用関数を作成
					auto serializeFunc = [&](json& outJson)
					{
						iface.serialize(reg, e, outJson);
					};

					// シリアライズ用の関数を作成
					DrawComponent(nameStr.c_str(), reg.get<T>(e), removed, serializeFunc, onCommand, index, onReorder);

					if (removed && onRemove)
					{
						onRemove();
					}
				}
#endif // _DEBUG
			};

			m_interfaces[nameStr] = iface;
		}

		const std::map<std::string, Interface>& GetInterfaces() const { return m_interfaces; }

	private:
		std::map<std::string, Interface> m_interfaces;
	};

}	// namespace Arche

#endif // !___COMPONENT_REGISTRY_H___