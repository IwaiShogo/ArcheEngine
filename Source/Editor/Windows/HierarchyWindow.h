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
#include "Engine/pch.h"
#include "Editor/Core/Editor.h"
#include "Engine/Scene/Components/Components.h"

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
		world.getRegistry().view<Tag>().each([&](Entity e, Tag& tag) {
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

		// リストの下に余白を作る（ここにもドロップできるように）
		ImGui::Dummy(ImGui::GetContentRegionAvail());

		// ---------------------------------------------------------
		// ドラッグ＆ドロップの「受信」処理
		// ---------------------------------------------------------
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				// 1. パスの取得と補完
				const char* droppedRelativePath = (const char*)payload->Data;
				std::string rootPath = "Resources/Game/";
				std::string fullPath = rootPath + droppedRelativePath; // 例: Resources/Game/Textures/hero.png

				// パスの整形（Windowsの\を/に）
				std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

				// 2. 拡張子の取得
				std::filesystem::path fpath = fullPath;
				std::string ext = fpath.extension().string();
				// 小文字に統一して判定しやすくする（簡易実装）
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

				// 3. エンティティ生成
				Entity newEntity = world.create_entity().id();

				// 名前設定 (ファイル名をエンティティ名に)
				std::string entityName = fpath.stem().string();
				world.getRegistry().emplace<Tag>(newEntity, entityName);
				world.getRegistry().emplace<Transform>(newEntity);

				// --- 拡張子による分岐 ---

				// A. モデル (.fbx, .obj, .gltf)
				if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
				{
					// リソース登録
					StringId key(fullPath);
					ResourceManager::Instance().RegisterResource(key, fullPath, ResourceManager::ResourceType::Model);

					// MeshComponent追加
					world.getRegistry().emplace<MeshComponent>(newEntity, key);
					OutputDebugStringA(("Created Model Entity: " + entityName + "\n").c_str());
				}
				// B. テクスチャ (.png, .jpg, .bmp, .tga) -> Sprite
				else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
				{
					StringId key(fullPath);
					ResourceManager::Instance().RegisterResource(key, fullPath, ResourceManager::ResourceType::Texture);

					// SpriteComponent追加
					world.getRegistry().emplace<SpriteComponent>(newEntity, key);
					OutputDebugStringA(("Created Sprite Entity: " + entityName + "\n").c_str());
				}
				// C. サウンド (.wav, .mp3) -> AudioSource
				else if (ext == ".wav" || ext == ".mp3")
				{
					StringId key(fullPath);
					ResourceManager::Instance().RegisterResource(key, fullPath, ResourceManager::ResourceType::Sound);

					// AudioSource追加
					world.getRegistry().emplace<AudioSource>(newEntity, key);
					OutputDebugStringA(("Created Audio Entity: " + entityName + "\n").c_str());
				}
				// D. プレハブ (.json) -> 将来対応
				else if (ext == ".json")
				{
					// SceneSerializer::DeserializeEntity(...) を呼べば復元できます
					OutputDebugStringA("Prefab dropping is not implemented yet.\n");
				}
				else
				{
					// 未対応ファイルは、空のエンティティとして残すか、削除する
					OutputDebugStringA(("Unknown file type dropped: " + fullPath + "\n").c_str());
				}
			}

			ImGui::EndDragDropTarget();
		}
		// ---------------------------------------------------------

		ImGui::End(); // End Hierarchy
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
