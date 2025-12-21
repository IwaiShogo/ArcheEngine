/*****************************************************************//**
 * @file	ContentBrowser.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/12/21	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#include "Editor/Panels/ContentBrowser.h"

// リソースのルートディレクトリ
static const std::filesystem::path ASSET_PATH = "Resources/Game";

ContentBrowser::ContentBrowser()
	: m_currentDirectory(ASSET_PATH)
	, m_baseDirectory(ASSET_PATH)
{
	// ディレクトリが存在しない場合は作成しておく
	if (!std::filesystem::exists(m_baseDirectory))
	{
		std::filesystem::create_directories(m_baseDirectory);
	}
}

void ContentBrowser::Draw(World& world, Entity& selected, Context& ctx)
{
	ImGui::Begin("Content Browser");

	// --- 以前と同じ中身 ---
	// 戻るボタン
	if (m_currentDirectory != std::filesystem::path(ASSET_PATH))
	{
		if (ImGui::Button("<- Back"))
		{
			m_currentDirectory = m_currentDirectory.parent_path();
		}
		ImGui::Separator();
	}

	// ファイル一覧
	float padding = 16.0f;
	float thumbnailSize = 64.0f;
	float cellSize = thumbnailSize + padding;
	float panelWidth = ImGui::GetContentRegionAvail().x;
	int columnCount = (int)(panelWidth / cellSize);
	if (columnCount < 1) columnCount = 1;

	if (ImGui::BeginTable("ContentBrowserTable", columnCount))
	{
		for (auto& directoryEntry : std::filesystem::directory_iterator(m_currentDirectory))
		{
			const auto& path = directoryEntry.path();
			auto relativePath = std::filesystem::relative(path, ASSET_PATH);
			std::string filenameString = path.filename().string();

			ImGui::TableNextColumn();
			ImGui::PushID(filenameString.c_str());

			if (directoryEntry.is_directory())
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
				if (ImGui::Button("DIR", { thumbnailSize, thumbnailSize }))
				{
					m_currentDirectory /= path.filename();
				}
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
				ImGui::Button("FILE", { thumbnailSize, thumbnailSize });

				if (ImGui::BeginDragDropSource())
				{
					std::string itemPath = relativePath.string();
					std::replace(itemPath.begin(), itemPath.end(), '\\', '/');
					ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(), itemPath.size() + 1);
					ImGui::Text("%s", filenameString.c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::PopStyleColor();
			}

			ImGui::TextWrapped("%s", filenameString.c_str());
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	ImGui::End();
}