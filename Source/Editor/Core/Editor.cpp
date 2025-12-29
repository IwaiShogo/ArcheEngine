/*****************************************************************//**
 * @file	Editor.cpp
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

	void Editor::Draw(World& world, Context& ctx)
	{
		bool ctrl = Input::GetKey(VK_CONTROL);

		if (ctrl && Input::GetKeyDown('Z'))
		{
			CommandHistory::Undo();
		}
		if (ctrl && Input::GetKeyDown('Y'))
		{
			CommandHistory::Redo();
		}

		ImGui::DockSpaceOverViewport(ImGui::GetID("MainDockSpace"), ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

		// ツールバー
		if (ImGui::BeginMainMenuBar())
		{
			if (ctx.editorState == EditorState::Edit)
			{
				if (ImGui::Button("Play"))
				{
					ctx.editorState = EditorState::Play;
				}
			}
			else
			{
				if (ImGui::Button("Stop"))
				{
					ctx.editorState = EditorState::Edit;
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

	void Editor::DrawGizmo(World& world, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj, float x, float y, float w, float h)
	{
		GizmoSystem::Draw(world.getRegistry(), m_selectedEntity, view, proj, x, y, w, h);
	}

}	// namespace Arche

#endif // _DEBUG