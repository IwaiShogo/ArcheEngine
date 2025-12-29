/*****************************************************************//**
 * @file	ContentBrowser.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___CONTENT_BROWSER_H___
#define ___CONTENT_BROWSER_H___

#ifdef _DEBUG

// ===== インクルード =====
#include "Engine/pch.h"
#include "Editor/Core/Editor.h"

namespace Arche
{

	class ContentBrowser
		: public EditorWindow
	{
	public:
		ContentBrowser()
			: m_currentDirectory("Resources")	// 初期ディレクトリ
		{
			// 必要ならテクスチャアイコンのロードなど
		}
		void Draw(World& world, Entity& selected, Context& ctx) override
		{
			ImGui::Begin("Content Browser");

			// 1. 戻るボタン（ルートでなければ表示）
			if (m_currentDirectory != std::filesystem::path("Resources"))
			{
				if (ImGui::Button("<-"))
				{
					m_currentDirectory = m_currentDirectory.parent_path();
				}
				ImGui::SameLine();
			}
			ImGui::Text("Current: %s", m_currentDirectory.string().c_str());
			ImGui::Separator();

			// 2. カラムレイアウト設定
			float padding = 16.0f;
			float thumbnailSize = 80.0f;
			float cellSize = thumbnailSize + padding;

			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columnCount = (int)(panelWidth / cellSize);
			if (columnCount < 1) columnCount = 1;

			ImGui::Columns(columnCount, 0, false);

			// 3. ファイル走査と描画
			for (auto& directoryEntry : std::filesystem::directory_iterator(m_currentDirectory))
			{
				const auto& path = directoryEntry.path();
				std::string filename = path.filename().string();

				// IDの衝突を防ぐ
				ImGui::PushID(filename.c_str());

				// アイコン（フォルダ or ファイル で色分け）
				if (directoryEntry.is_directory())
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f)); // フォルダ色
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // ファイル色
				}

				ImGui::Button(filename.c_str(), ImVec2(thumbnailSize, thumbnailSize));
				ImGui::PopStyleColor();

				// ドラッグ & ドロップ対応
				// ------------------------------------------------------------
				if (ImGui::BeginDragDropSource())
				{
					// パスをペイロードとして渡す
					std::string itemPath = path.string();
					// ワイド文字変換
					ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(), itemPath.size() + 1);
					ImGui::Text("%s", filename.c_str());
					ImGui::EndDragDropSource();
				}

				// ダブルクリック処理
				// ------------------------------------------------------------
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (directoryEntry.is_directory())
					{
						// フォルダなら移動
						m_currentDirectory /= path.filename();
					}
					else
					{
						// ファイルなら拡張子判定
						if (path.extension() == ".json")
						{
							// シーンのロード実行
							SceneSerializer::LoadScene(world, path.string());

							// 選択解除
							selected = NullEntity;
						}
					}
				}

				ImGui::TextWrapped("%s", filename.c_str());

				ImGui::NextColumn();
				ImGui::PopID();
			}

			ImGui::Columns(1);
			ImGui::End();
		}

	private:
		// 現在開いているディレクトリのパス
		std::filesystem::path m_currentDirectory;
	};

}	// namespace Arche

#endif // _DEBUG

#endif // !___CONTENT_BROWSER_H___