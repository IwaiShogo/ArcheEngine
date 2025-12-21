/*****************************************************************//**
 * @file	InspectorWindow.h
 * @brief	インスペクターウィンドウ
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
 * 1. Draw()内、コンポーネント一覧に追加
 * 2. AddComponentPopupに追加
 *********************************************************************/

#ifndef ___INSPECTOR_WINDOW_H___
#define ___INSPECTOR_WINDOW_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Editor/Core/Editor.h"
#include "Engine/Components/Components.h"
#include "Engine/Audio/AudioManager.h"
#include "Engine/Editor/Core/InspectorGui.h"
#include "Engine/Resource/SceneSerializer.h"

class InspectorWindow : public EditorWindow
{
public:
	void Draw(World& world, Entity& selected, Context& ctx) override
	{
		Registry& reg = world.getRegistry();

		if (selected == NullEntity || !reg.has<Tag>(selected)) return;

		ImGui::Begin("Inspector");

		// --------------------------------------------------------
		// ヘッダー (ID & Name)
		// --------------------------------------------------------
		ImGui::Text("ID: %d", selected);
		
		ImGui::Separator();

		// --------------------------------------------------------
		// コンポーネント一覧
		// --------------------------------------------------------

		DrawAllComponents<ComponentList>(reg, selected);

		ImGui::Separator();

		// --------------------------------------------------------
		// Add Component
		// --------------------------------------------------------
		if (ImGui::Button("Add Component", ImVec2(-1, 30))) {
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup")) {
			// ここに全てのコンポーネントを追加
			if (!reg.has<MeshComponent>(selected) && ImGui::Selectable("Mesh")) reg.emplace<MeshComponent>(selected, "hero");
			if (!reg.has<SpriteComponent>(selected) && ImGui::Selectable("Sprite")) reg.emplace<SpriteComponent>(selected, "player");
			if (!reg.has<BillboardComponent>(selected) && ImGui::Selectable("Billboard")) reg.emplace<BillboardComponent>(selected, "star");
			if (!reg.has<Collider>(selected) && ImGui::Selectable("Collider")) reg.emplace<Collider>(selected);
			if (!reg.has<AudioSource>(selected) && ImGui::Selectable("Audio Source")) reg.emplace<AudioSource>(selected, "jump");
			if (!reg.has<Camera>(selected) && ImGui::Selectable("Camera")) reg.emplace<Camera>(selected);

			if (!reg.has<Rigidbody>(selected) && ImGui::Selectable("Rigidbody")) reg.emplace<Rigidbody>(selected);
			if (!reg.has<PlayerInput>(selected) && ImGui::Selectable("Player Input")) reg.emplace<PlayerInput>(selected);
			if (!reg.has<Lifetime>(selected) && ImGui::Selectable("Lifetime")) reg.emplace<Lifetime>(selected, 10.0f);

			ImGui::EndPopup();
		}

		ImGui::Separator();

		// Save Prefab
		if (ImGui::Button("Save as Prefab")) {
			std::string path = std::string("Resources/Prefabs/") + reg.get<Tag>(selected).name.c_str() + ".json";
			SceneSerializer::SaveEntity(reg, selected, path);
			Logger::Log("Saved Prefab: " + path);
		}

		// Destroy
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
		if (ImGui::Button("Destroy Entity", ImVec2(-1, 30))) {
			reg.destroy(selected);
			selected = NullEntity;
		}
		ImGui::PopStyleColor();

		ImGui::End();
	}

private:
	// --------------------------------------------------------
	// リソース選択用ヘルパー (StringId & ResourceType対応)
	// --------------------------------------------------------
	void FileSelector(const char* label, StringId& currentId, const std::string& dir, const std::string& filterExt, ResourceManager::ResourceType type)
	{
		// 1. 表示名の取得
		// StringIdからパスを取得を試みる
		std::string preview = ResourceManager::Instance().GetPathByKey(currentId, type);

		// 見つからない場合（未登録 or ID=0）
		if (preview.empty()) {
			if (currentId.GetHash() == 0) preview = "None";
			else preview = currentId.c_str(); // Debug時のみハッシュ値または元文字列
		}

		// 2. コンボボックス描画
		if (ImGui::BeginCombo(label, preview.c_str()))
		{
			namespace fs = std::filesystem;
			if (fs::exists(dir)) {
				for (const auto& entry : fs::recursive_directory_iterator(dir)) {
					if (!entry.is_regular_file()) continue;

					// 拡張子フィルタ
					std::string ext = entry.path().extension().string();
					if (ext == filterExt) { // 大文字小文字無視は省略
						std::string path = entry.path().string();
						std::replace(path.begin(), path.end(), '\\', '/');

						StringId pathId(path);
						bool isSelected = (currentId == pathId);

						if (ImGui::Selectable(path.c_str(), isSelected)) {
							// ★重要: 選択されたらResourceManagerに「パスをキーとして」登録する
							ResourceManager::Instance().RegisterResource(pathId, path, type);
							currentId = pathId;
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
				}
			}
			ImGui::EndCombo();
		}

		// 3. ドラッグ＆ドロップ受け入れ
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
			{
				const char* droppedPath = (const char*)payload->Data;
				StringId newId(droppedPath);

				// ドロップされたらResourceManagerに登録する
				ResourceManager::Instance().RegisterResource(newId, droppedPath, type);

				currentId = newId;
			}
			ImGui::EndDragDropTarget();
		}
	}

	// タプル内の全型に対して処理を行う
	template<typename TupleT>
	void DrawAllComponents(Registry& registry, Entity entity)
	{
		auto view = [&]<typename... Ts>(std::tuple<Ts...>*)
		{
			(..., [&](void)
			{
				using ComponentType = Ts;

				// ダミーの int 型はスキップする！
				if constexpr (std::is_same_v<ComponentType, DummyComponent>) return;

				// そのコンポーネントを持っているか？
				if (registry.has<ComponentType>(entity))
				{
					bool removed = false;

					// リフレクション情報から名前を取得して描画
					DrawComponent(
						Reflection::Meta<ComponentType>::Name,
						registry.get<ComponentType>(entity),
						removed
					);

					// 削除ボタンが押されたら削除
					if (removed)
					{
						registry.remove<ComponentType>(entity);
					}
				}
			}());
		};

		// 定義したタプル型を使って実行
		view(static_cast<TupleT*>(nullptr));
	}
};

#endif	//!___INSPECTOR_WINDOW_H___
