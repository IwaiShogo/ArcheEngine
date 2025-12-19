/*****************************************************************//**
 * @file	CreatorWindow.h
 * @brief	エンティティ作成ウィンドウ
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

#ifndef ___CREATOR_WINDOW_H___
#define ___CREATOR_WINDOW_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Editor/Core/Editor.h"
#include "Engine/Editor/Tools/ThumbnailGenerator.h"
#include "Engine/Resource/Serializer.h"

class CreatorWindow : public EditorWindow {
public:
	void Draw(World& world, Entity& selected, Context& ctx) override {
		ImGui::Begin("Asset Browser"); // 名前変更

		Registry& reg = world.getRegistry();

		// アイコン画像の取得 (なければ白テクスチャ)
		auto iconTex = ResourceManager::Instance().GetTexture("star"); // ※要ロード
		void* iconID = iconTex ? (void*)iconTex->srv.Get() : nullptr;

		// 1. 新規作成
		if (ImGui::Button("Create Empty")) {
			Entity e = world.create_entity().add<Tag>("New Entity").add<Transform>().id();
			selected = e;
		}
		ImGui::Separator();

		// 2. プレハブ一覧 (グリッド表示)
		ImGui::Text("Prefabs:");

		namespace fs = std::filesystem;
		std::string prefabDir = "Resources/Prefabs";

		if (fs::exists(prefabDir)) {
			float padding = 16.0f;
			float thumbnailSize = 64.0f;
			float cellSize = thumbnailSize + padding;

			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnCount = (int)(panelWidth / cellSize);
			if (columnCount < 1) columnCount = 1;

			if (ImGui::BeginTable("PrefabGrid", columnCount)) {
				for (const auto& entry : fs::directory_iterator(prefabDir)) {
					if (entry.path().extension() == ".json") {
						ImGui::TableNextColumn();

						std::string filename = entry.path().filename().string();
						std::string id = "btn_" + filename;

						// サムネイルを取得
						void* thumbID = ThumbnailGenerator::Instance().GetThumbnailID(filename);

						if (thumbID) {
							if (ImGui::ImageButton(id.c_str(), (ImTextureID)thumbID, ImVec2(thumbnailSize, thumbnailSize))) {
								Entity e = Serializer::LoadEntity(world, entry.path().string());
								selected = e;
								Logger::Log("Spawned: " + filename);
							}
						}
						else {
							if (ImGui::Button(filename.c_str(), ImVec2(thumbnailSize, thumbnailSize))) {
								Entity e = Serializer::LoadEntity(world, entry.path().string());
								selected = e;
							}
						}

						// ファイル名 (中央揃え風)
						//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (cellSize - ImGui::CalcTextSize(filename.c_str()).x) * 0.5f);
						ImGui::TextWrapped("%s", filename.c_str());
					}
				}
				ImGui::EndTable();
			}
		}

		ImGui::End();
	}
};

#endif // !___CREATOR_WINDOW_H___
