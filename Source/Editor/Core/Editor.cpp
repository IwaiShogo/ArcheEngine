/*****************************************************************//**
 * @file	Editor.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#ifdef _DEBUG
#include "Editor/Core/Editor.h"
#include "Editor/Core/CommandHistory.h"
#include "Editor/Tools/GizmoSystem.h"
#include "Engine/Audio/AudioManager.h"

// パネル群
#include "Editor/Panels/HierarchyWindow.h"
#include "Editor/Panels/InspectorWindow.h"
#include "Editor/Panels/SystemWindow.h"
#include "Editor/Panels/ContentBrowser.h"
#include "Editor/Panels/ProjectSettingsWindow.h"

namespace Arche
{

	void Editor::Initialize()
	{
		// 1. パネルの生成
		m_windows.clear();
		m_windows.push_back(std::make_unique<HierarchyWindow>());
		m_windows.push_back(std::make_unique<InspectorWindow>());
		m_windows.push_back(std::make_unique<SystemWindow>());
		m_windows.push_back(std::make_unique<ContentBrowser>());
		m_windows.push_back(std::make_unique<ProjectSettingsWindow>());

		// 2. ImGuiの設定
		ImGuiIO& io = ImGui::GetIO();

		// マルチビューポート
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	// キーボード操作有効
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// ウィンドウドッキング
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		// 別ウィンドウ化を有効

		// 3. スタイル設定
		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// 4. フォント読み込み
		// Windows標準「メイリオ」
		static const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesJapanese();
		// フォントサイズ 18.0f で読み込み
		ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\meiryo.ttc", 18.0f, nullptr, glyphRanges);

		// 読み込みに失敗した場合の保険
		if (font == nullptr)
		{
			io.Fonts->AddFontDefault();
		}
	}

	static std::string s_currentScenePath = "Resources/Game/Scenes/GameScene.json";

	void Editor::Draw(World& world, Context& ctx)
	{
		bool ctrl = Input::GetKey(VK_CONTROL);

		// Undo / Redo
		if (ctrl && Input::GetKeyDown('Z'))
		{
			CommandHistory::Undo();
		}
		if (ctrl && Input::GetKeyDown('Y'))
		{
			CommandHistory::Redo();
		}

		// シーン保存
		if (ctrl && Input::GetKeyDown('S'))
		{
			// Editモードの時だけ保存可能
			if (ctx.editorState == EditorState::Edit)
			{
				SceneSerializer::SaveScene(world, s_currentScenePath);
			}
		}

		// Play / Edit 切り替え
		if (ctrl && Input::GetKeyDown('P'))
		{
			if (ctx.editorState == EditorState::Edit)
				ctx.editorState = EditorState::Play;
			else
				ctx.editorState = EditorState::Edit;
		}

		ImGui::DockSpaceOverViewport(ImGui::GetID("MainDockSpace"), ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

		// ツールバー
		if (ImGui::BeginMainMenuBar())
		{
			// ファイルメニュー
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save Scene", "Ctrl + S"))
				{
					SceneSerializer::SaveScene(world, s_currentScenePath);
				}
				ImGui::EndMenu();
			}

			// Play / Stop 切り替え
			if (ctx.editorState == EditorState::Edit)
			{
				if (ImGui::Button("Play"))
				{
					// 一時保存
					SceneSerializer::SaveScene(world, "Resources/Engine/Cache/SceneCache.json");

					ctx.editorState = EditorState::Play;
				}
			}
			else
			{
				if (ImGui::Button("Stop"))
				{
					ctx.editorState = EditorState::Edit;

					// 復元
					SceneSerializer::LoadScene(world, "Resources/Engine/Cache/SceneCache.json");
				}
			}

			ImGui::EndMainMenuBar();
		}

		ImGuizmo::BeginFrame();

		// 各ウィンドウを描画
		for (auto& window : m_windows)
		{
			window->Draw(world, m_selectedEntity, ctx);
		}

		m_sceneViewPanel.Draw(world, m_selectedEntity);

		m_gameViewPanel.Draw();

		// デバッグUI
		ResourceManager::Instance().OnInspector();
		AudioManager::Instance().OnInspector();

		// ログウィンドウの描画
		Logger::Draw("Debug Logger");
	}

}	// namespace Arche

#endif // _DEBUG