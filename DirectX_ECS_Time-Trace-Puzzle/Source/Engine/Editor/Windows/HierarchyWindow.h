/*****************************************************************//**
 * @file	HierarchyWindow.h
 * @brief	
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
 * @note	（省略可）
 *********************************************************************/

#ifndef ___HIERARCHY_WINDOW_H___
#define ___HIERARCHY_WINDOW_H___

// ===== インクルード =====
#include "Engine/Editor/Core/Editor.h"
#include "Game/Components/Components.h"
#include "imgui.h"
#include "imgui_internal.h"

class HierarchyWindow
	: public EditorWindow
{
public:
	void Draw(World& world, Entity& selected, Context& ctx) override
	{
		// ------------------------------------------------------------
		// Hierarchy Window
		// ------------------------------------------------------------
		ImGui::Begin("Hierarchy");

		// 「親を持たない（ルート）Entity」だけを起点に描画する
		// これをしないと、子が二重に表示されてしまいます
		world.getRegistry().view<Tag>([&](Entity e, Tag& tag) {
			bool isRoot = true;
			if (world.getRegistry().has<Relationship>(e)) {
				if (world.getRegistry().get<Relationship>(e).parent != NullEntity) isRoot = false;
			}

			if (isRoot) {
				DrawEntityNode(world, e, selected);
			}
			});

		// ウィンドウの余白部分へのドロップ（＝親子解除、ルート化）
		if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->Rect(), ImGui::GetID("Hierarchy"))) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID")) {
				Entity payloadEntity = *(const Entity*)payload->Data;
				// 親をNullにする（ルートに戻す）
				SetParent(world, payloadEntity, NullEntity);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::End();
	}

private:
	void SetParent(World& world, Entity child, Entity parent)
	{
		// 自分自身を親にはできない
		if (child == parent) return;

		// 1. 現在の親から離脱
		if (world.getRegistry().has<Relationship>(child))
		{
			Entity oldParent = world.getRegistry().get<Relationship>(child).parent;
			if (oldParent != NullEntity && world.getRegistry().has<Relationship>(oldParent))
			{
				auto& children = world.getRegistry().get<Relationship>(oldParent).children;
				// 子リストから自分を削除
				children.erase(std::remove(children.begin(), children.end(), child), children.end());
			}
		}

		// 2. 新しい親に所属
		// 子側の設定
		if (!world.getRegistry().has<Relationship>(child)) world.getRegistry().emplace<Relationship>(child);
		world.getRegistry().get<Relationship>(child).parent = parent;

		// 親側の設定
		if (parent != NullEntity)
		{
			if (!world.getRegistry().has<Relationship>(parent)) world.getRegistry().emplace<Relationship>(parent);
			world.getRegistry().get<Relationship>(parent).children.push_back(child);
		}
	}

	void DrawEntityNode(World& world, Entity e, Entity& selected)
	{
		Tag& tag = world.getRegistry().get<Tag>(e);

		// ノードのフラグ設定
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (e == selected) flags |= ImGuiTreeNodeFlags_Selected;

		// 子がいるかチェック
		bool hasChildren = false;
		if (world.getRegistry().has<Relationship>(e)) {
			if (!world.getRegistry().get<Relationship>(e).children.empty()) hasChildren = true;
		}
		if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf; // 子がなければリーフ（葉）

		// --- ノード描画 ---
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)e, flags, "%s (ID:%d)", tag.name.c_str(), e);

		// クリック選択
		if (ImGui::IsItemClicked()) {
			selected = e;
		}

		// --- ドラッグ＆ドロップ処理 ---

		// 1. ドラッグ元 (Source)
		if (ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload("ENTITY_ID", &e, sizeof(Entity));
			ImGui::Text("Move %s", tag.name);
			ImGui::EndDragDropSource();
		}

		// 2. ドロップ先 (Target)
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID")) {
				Entity payloadEntity = *(const Entity*)payload->Data;
				// ドロップされたEntityを、こいつ(e)の子にする
				SetParent(world, payloadEntity, e);
			}
			ImGui::EndDragDropTarget();
		}

		// --- 子要素の再帰描画 ---
		if (opened) {
			if (hasChildren) {
				for (Entity child : world.getRegistry().get<Relationship>(e).children) {
					DrawEntityNode(world, child, selected);
				}
			}
			ImGui::TreePop();
		}
	}
};

#endif // !___HIERARCHY_WINDOW_H___