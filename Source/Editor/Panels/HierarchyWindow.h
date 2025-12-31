/*****************************************************************//**
 * @file	HierarchyWindow.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___HIERARCHY_WINDOW_H___
#define ___HIERARCHY_WINDOW_H___

#ifdef _DEBUG

// ===== インクルード =====
#include "Engine/pch.h"
#include "Editor/Core/Editor.h"
#include "Editor/Core/EditorCommands.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Scene/Serializer/SceneSerializer.h"

namespace Arche
{

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

			// 選択変更検知と親ノードの収集
			// ------------------------------------------------------------
			if (selected != m_lastSelected)
			{
				m_nodesToOpen.clear();
				if (selected != NullEntity && world.getRegistry().valid(selected))
				{
					// 親を辿って全てセットに追加
					Entity current = selected;
					Registry& reg = world.getRegistry();
					while (reg.has<Relationship>(current))
					{
						Entity parent = reg.get<Relationship>(current).parent;
						if (parent == NullEntity) break;

						m_nodesToOpen.insert(parent);
						current = parent;
					}
				}
				m_lastSelected = selected;
			}

			// Deleteキーでの削除処理
			if (ImGui::IsWindowFocused() && selected != NullEntity)
			{
				if (ImGui::IsKeyPressed(ImGuiKey_Delete))
				{
					CommandHistory::Execute(std::make_shared<DeleteEntityCommand>(world, selected));
					selected = NullEntity;
				}
			}

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

			// 余白クリックで選択解除
			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			{
				selected = NullEntity;
			}

			// 右クリック: 新規作成など
			if (ImGui::BeginPopupContextWindow(nullptr, ImGuiMouseButton_Right | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::MenuItem("Create Empty"))
				{
					Entity e = world.create_entity()
						.add<Tag>("GameObject")
						.add<Transform>().id();
					selected = 0;
				}
				ImGui::EndPopup();
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
					const char* droppedPath = (const char*)payload->Data;
					std::string relativePath = droppedPath;
					std::string rootPath = "Resources\\Game\\";

					std::string fullPath;
					// パスが既に "Resources/Game/" で始まっているかチェック
					if (relativePath.find(rootPath) == 0)
					{
						fullPath = relativePath;	// そのまま使う
					}
					else
					{
						fullPath = rootPath + relativePath;	// 結合する
					}

					// バックスラッシュをスラッシュに統一
					std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

					// 2. 拡張子の取得
					std::filesystem::path fpath = fullPath;
					std::string ext = fpath.extension().string();
					// 小文字に統一して判定しやすくする（簡易実装）
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					// --- 拡張子による分岐 ---

					// パターンA: プレファブ (.json)
					if (ext == ".json")
					{
						// シリアライザを使って丸ごと復元
						Entity prefabRoot = SceneSerializer::LoadPrefab(world, fullPath);

						if (prefabRoot != NullEntity)
						{
							// 必要なら名前を更新したり、選択状態にする
							selected = prefabRoot;
						}
					}
					// パターンB: 通常のアセット (モデル/画像/音)
					else
					{
						// 空のエンティティを新規作成
						Entity newEntity = world.create_entity().id();

						// 基本コンポーネント追加
						std::string entityName = fpath.stem().string();
						world.getRegistry().emplace<Tag>(newEntity, entityName);
						world.getRegistry().emplace<Transform>(newEntity);

						// アセットに応じたコンポーネント追加
						if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
						{
							StringId key(fullPath);
							ResourceManager::Instance().RegisterResource(key, fullPath, ResourceManager::ResourceType::Model);
							world.getRegistry().emplace<MeshComponent>(newEntity, key);
						}
						else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
						{
							StringId key(fullPath);
							ResourceManager::Instance().RegisterResource(key, fullPath, ResourceManager::ResourceType::Texture);
							world.getRegistry().emplace<SpriteComponent>(newEntity, key);
						}
						else if (ext == ".wav" || ext == ".mp3")
						{
							StringId key(fullPath);
							ResourceManager::Instance().RegisterResource(key, fullPath, ResourceManager::ResourceType::Sound);
							world.getRegistry().emplace<AudioSource>(newEntity, key);
						}

						selected = newEntity;
					}
				}

				ImGui::EndDragDropTarget();
			}
			// ---------------------------------------------------------

			ImGui::End(); // End Hierarchy
		}

	private:
		// エンティティとその子供を再帰的に削除する
		void DeleteEntityRecursively(World& world, Entity entity)
		{
			Registry& reg = world.getRegistry();
			if (!reg.valid(entity)) return;

			// 1. 親から自分を外す
			if (reg.has<Relationship>(entity))
			{
				Entity parent = reg.get<Relationship>(entity).parent;
				if (parent != NullEntity && reg.has<Relationship>(parent))
				{
					auto& siblings = reg.get<Relationship>(parent).children;
					siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());
				}
			}

			// 2. 子供を再帰的に削除するヘルパー
			auto recursiveDestroy = [&](auto&& self, Entity e) -> void
			{
				if(reg.has<Relationship>(e))
				{
					// コピーしてから回す
					std::vector<Entity> children = reg.get<Relationship>(e).children;
					for (Entity child : children)
					{
						self(self, child);
					}
				}

				// 削除実行
				reg.destroy(e);
			};

			recursiveDestroy(recursiveDestroy, entity);
		}

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
			Registry& reg = world.getRegistry();
			// 既に削除されている場合はスキップ
			if (!reg.valid(e)) return;

			Tag& tag = reg.get<Tag>(e);

			// ノードのフラグ設定
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (e == selected) flags |= ImGuiTreeNodeFlags_Selected;

			// 子がいるかチェック
			bool hasChildren = false;
			if (world.getRegistry().has<Relationship>(e)) {
				if (!world.getRegistry().get<Relationship>(e).children.empty()) hasChildren = true;
			}
			if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf; // 子がなければリーフ

			if (m_nodesToOpen.count(e))
			{
				ImGui::SetNextItemOpen(true);
			}

			// --- ノード描画 ---
			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)e, flags, "%s (ID:%d)", tag.name.c_str(), e);

			// クリック選択
			if (ImGui::IsItemClicked()) {
				selected = e;
			}

			// 右クリックメニュー
			if (ImGui::BeginPopupContextItem())
			{
				// エンティティ削除
				if (ImGui::MenuItem("Delete Entity"))
				{
					CommandHistory::Execute(std::make_shared<DeleteEntityCommand>(world, e));
					if (selected == e) selected = NullEntity;	// 選択中なら解除
					ImGui::EndPopup();
					if (opened) ImGui::TreePop();	// ツリーが開いていた場合の整合性
					return;
				}

				// プレファブ保存
				if (ImGui::MenuItem("Save as Prefab"))
				{
					// 保存先ディレクトリ確保
					std::filesystem::create_directories("Resources/Game/Prefabs");

					// ファイル名決定（エンティティ名.json）
					std::string filename = "Entity";
					if (reg.has<Tag>(e))
					{
						filename = reg.get<Tag>(e).name.c_str();
					}

					std::string path = "Resources/Game/Prefabs/" + filename + ".json";

					// 保存実行
					SceneSerializer::SavePrefab(reg, e, path);
				}

				ImGui::EndPopup();
			}

			// --- ドラッグ＆ドロップ処理 ---

			// 1. ドラッグ元 (Source)
			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("ENTITY_ID", &e, sizeof(Entity));
				ImGui::Text("Move %s", tag.name.c_str());
				ImGui::EndDragDropSource();
			}

			// 2. ドロップ先 (Target)
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID"))
				{
					Entity payloadEntity = *(const Entity*)payload->Data;
					// ドロップされたEntityを、こいつ(e)の子にする
					CommandHistory::Execute(std::make_shared<ReparentEntityCommand>(world, payloadEntity, e));
				}
				ImGui::EndDragDropTarget();
			}

			// --- 子要素の再帰描画 ---
			if (opened)
			{
				if (hasChildren)
				{
					auto children = reg.get<Relationship>(e).children;
					for (Entity child : children)
					{
						DrawEntityNode(world, child, selected);
					}
				}
				ImGui::TreePop();
			}
		}

	private:
		// 自動展開するノードIDを一時保存するセット
		std::set<Entity> m_nodesToOpen;
		Entity m_lastSelected = NullEntity;
	};

}	// namespace Arche

#endif // _DEBUG

#endif // !___HIERARCHY_WINDOW_H___