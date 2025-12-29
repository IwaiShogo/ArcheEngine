/*****************************************************************//**
 * @file	EditorCommands.h
 * @brief	具体的な編集コマンド定義
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___EDITOR_COMMANDS_H___
#define ___EDITOR_COMMANDS_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Editor/Core/CommandHistory.h"
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Scene/Serializer/SceneSerializer.h"

namespace Arche
{
	// エンティティ削除コマンド
	// ------------------------------------------------------------
	class DeleteEntityCommand
		: public ICommand
	{
	public:
		DeleteEntityCommand(World& world, Entity entity)
			: m_world(world), m_entity(entity), m_parent(NullEntity)
		{
			// 親を覚えておく
			Registry& reg = m_world.getRegistry();
			if (reg.has<Relationship>(entity))
			{
				m_parent = reg.get<Relationship>(entity).parent;
			}
			// データをバックアップ
			SceneSerializer::SerializeEntityToJson(reg, entity, m_backup);
		}

		void Execute() override
		{
			if (m_world.getRegistry().valid(m_entity))
			{
				m_world.getRegistry().destroy(m_entity);
			}
		}

		void Undo() override
		{
			// 復元実行
			Registry& reg = m_world.getRegistry();
			m_entity = SceneSerializer::DeserializeEntityFromJson(reg, m_backup);

			// 親子関係の修復
			if (m_parent != NullEntity && reg.valid(m_parent))
			{
				if (!reg.has<Relationship>(m_entity)) reg.emplace<Relationship>(m_entity);
				reg.get<Relationship>(m_entity).parent = m_parent;

				if (!reg.has<Relationship>(m_parent)) reg.emplace<Relationship>(m_parent);
				reg.get<Relationship>(m_parent).children.push_back(m_entity);
			}
		}

	private:
		World& m_world;
		Entity m_entity;	// 削除対象
		json m_backup;		// 復元用データ
		Entity m_parent;	// 親ID
	};

	// エンティティ生成コマンド
	// ------------------------------------------------------------
	class CreateEntityCommand
		: public ICommand
	{
	public:
		CreateEntityCommand(World& world) : m_world(world) {}

		void Execute() override
		{
			// 生成
			m_createdEntity = m_world.create_entity()
				.add<Tag>("New Entity")
				.add<Transform>().id();
		}

		void Undo() override
		{
			// 生成の取り消し = 削除
			if (m_world.getRegistry().valid(m_createdEntity))
			{
				m_world.getRegistry().destroy(m_createdEntity);
			}
		}
		
	private:
		World& m_world;
		Entity m_createdEntity = NullEntity;
	};

	// 親子関係変更コマンド
	// ------------------------------------------------------------
	class ReparentEntityCommand
		: public ICommand
	{
	public:
		ReparentEntityCommand(World& world, Entity child, Entity newParent)
			: m_world(world), m_child(child), m_newParent(newParent), m_oldParent(NullEntity)
		{
			Registry& reg = m_world.getRegistry();
			if (reg.has<Relationship>(child))
			{
				m_oldParent = reg.get<Relationship>(child).parent;
			}
		}

		void Execute() override
		{
			SetParent(m_child, m_newParent);
		}

		void Undo() override
		{
			SetParent(m_child, m_oldParent);
		}

	private:
		void SetParent(Entity child, Entity parent)
		{
			Registry& reg = m_world.getRegistry();
			if (!reg.valid(child)) return;

			// 1. 旧親から離脱
			if (!reg.has<Relationship>(child))
			{
				Entity currParent = reg.get<Relationship>(child).parent;
				if (currParent != NullEntity && reg.valid(currParent) && reg.has<Relationship>(currParent));
			}

			// 2. 新規へ所属
			if (!reg.has<Relationship>(child)) reg.emplace<Relationship>(child);
			reg.get<Relationship>(child).parent = parent;

			if (parent != NullEntity && reg.valid(parent))
			{
				if (!reg.has<Relationship>(parent)) reg.emplace<Relationship>(parent);
				reg.get<Relationship>(parent).children.push_back(child);
			}
		}

	private:
		World& m_world;
		Entity m_child;
		Entity m_newParent;
		Entity m_oldParent;
	};

	// コンポーネント追加コマンド
	// ------------------------------------------------------------
	class AddComponentCommand
		: public ICommand
	{
	public:
		AddComponentCommand(World& world, Entity entity, const std::string& compName)
			: m_world(world), m_entity(entity), m_componentName(compName) {}

		void Execute() override
		{
			Registry& reg = m_world.getRegistry();
			auto& interfaces = ComponentRegistry::Instance().GetInterfaces();
			if (interfaces.count(m_componentName))
			{
				interfaces.at(m_componentName).add(reg, m_entity);
			}
		}

		void Undo()
		{
			Registry& reg = m_world.getRegistry();
			auto& interfaces = ComponentRegistry::Instance().GetInterfaces();
			if (interfaces.count(m_componentName))
			{
				interfaces.at(m_componentName).remove(reg, m_entity);
			}
		}

	private:
		World& m_world;
		Entity m_entity;
		std::string m_componentName;
	};

	// コンポーネント削除コマンド
	// ------------------------------------------------------------
	class RemoveComponentCommand
		: public ICommand
	{
	public:
		RemoveComponentCommand(World& world, Entity entity, const std::string& compName)
			: m_world(world), m_entity(entity), m_componentName(compName)
		{
			// 削除前に現在の状態を保存
			Registry& reg = m_world.getRegistry();
			auto& interfaces = ComponentRegistry::Instance().GetInterfaces();
			if (interfaces.count(m_componentName))
			{
				interfaces.at(m_componentName).serialize(reg, m_entity, m_backup);
			}
		}

		void Execute() override
		{
			Registry& reg = m_world.getRegistry();
			auto& interfaces = ComponentRegistry::Instance().GetInterfaces();
			if (interfaces.count(m_componentName))
			{
				interfaces.at(m_componentName).remove(reg, m_entity);
			}
		}

		void Undo() override
		{
			Registry& reg = m_world.getRegistry();
			auto& interfaces = ComponentRegistry::Instance().GetInterfaces();
			if (interfaces.count(m_componentName))
			{
				interfaces.at(m_componentName).deserialize(reg, m_entity, m_backup);
			}
		}

	private:
		World& m_world;
		Entity m_entity;
		std::string m_componentName;
		json m_backup;
	};

	// コンポーネント値変更コマンド
	// ------------------------------------------------------------
	class ChangeComponentValueCommand
		: public ICommand
	{
	public:
		ChangeComponentValueCommand(World& world, Entity entity, const std::string& compName, const json& oldVal, const json& newVal)
			: m_world(world), m_entity(entity), m_componentName(compName), m_oldState(oldVal), m_newState(newVal) {}

		void Execute() override
		{
			ApplyState(m_newState);
		}

		void Undo() override
		{
			ApplyState(m_oldState);
		}

	private:
		void ApplyState(const json& state)
		{
			Registry& reg = m_world.getRegistry();
			auto& interfaces = ComponentRegistry::Instance().GetInterfaces();
			if (interfaces.count(m_componentName))
			{
				interfaces.at(m_componentName).deserialize(reg, m_entity, state);
			}
		}

	private:
		World& m_world;
		Entity m_entity;
		std::string m_componentName;
		json m_oldState;
		json m_newState;
	};

}	// namespace Arche

#endif // !___EDITOR_COMMANDS_H___