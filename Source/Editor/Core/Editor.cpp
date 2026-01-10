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
#include "Engine/Scene/Serializer/SceneSerializer.h"
#include "Engine/Scene/Serializer/ComponentSerializer.h"
#include "Engine/Scene/Serializer/SystemRegistry.h"
#include "Engine/Scene/Core/SceneManager.h"

// パネル群
#include "Editor/Panels/HierarchyWindow.h"
#include "Editor/Panels/InspectorWindow.h"
#include "Editor/Panels/ContentBrowser.h"
#include "Editor/Panels/PhysicsSettingsWindow.h"
#include "Editor/Panels/ResourceInspectorWindow.h"
#include "Editor/Panels/SystemMonitorWindow.h"
#include "Editor/Panels/GameControlWindow.h"
#include "Editor/Panels/InputVisualizerWindow.h"
#include "Editor/Panels/BuildSettingsWindow.h"
#include "Editor/Panels/AnimatorGraphWindow.h"

namespace Arche
{
	void Editor::Initialize()
	{
		// 1. パネルの生成
		m_windows.clear();

		auto hierarchy = std::make_unique<HierarchyWindow>();
		m_hierarchyPanel = hierarchy.get();
		m_windows.push_back(std::move(hierarchy));
		m_windows.push_back(std::make_unique<InspectorWindow>());
		m_windows.push_back(std::make_unique<ContentBrowser>());
		m_windows.push_back(std::make_unique<PhysicsSettingsWindow>());
		m_windows.push_back(std::make_unique<ResourceInspectorWindow>());
		m_windows.push_back(std::make_unique<SystemMonitorWindow>());
		m_windows.push_back(std::make_unique<GameControlWindow>());
		m_windows.push_back(std::make_unique<InputVisualizerWindow>());
		m_windows.push_back(std::make_unique<BuildSettingsWindow>());
		m_windows.push_back(std::make_unique<AnimatorGraphWindow>());

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

	void Editor::Shutdown()
	{
		// プレファブ編集モードらな強制終了してワールドを破棄
		if (m_editorMode == EditorMode::Prefab)
		{
			ExitPrefabMode();
		}

		// ウィンドウ群の破棄
		m_windows.clear();

		// プレファブワールドの明示的リセット
		m_prefabWorld.reset();

		Logger::Log("Editor Shutdown Completed.");
	}

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
				std::string path = SceneManager::Instance().GetCurrentScenePath();
				if (path.empty()) path = "Resources/Game/Scenes/Untitled.json";

				SceneSerializer::SaveScene(world, path);
				SceneManager::Instance().SetDirty(false);
				EditorPrefs::Instance().lastScenePath = path;
				EditorPrefs::Instance().Save();
			}
		}

		// Play / Edit 切り替え
		if (ctrl && Input::GetKeyDown('P'))
		{
			if (ctx.editorState == EditorState::Edit)
			{
				SceneSerializer::SaveScene(world, "Resources/Engine/Cache/SceneCache.json");
				ctx.editorState = EditorState::Play;
			}
			else
			{
				ctx.editorState = EditorState::Edit;

				// 1. まず現在のワールドを空にする
				world.clearSystems();
				world.clearEntities();

				// 復元
				SceneSerializer::LoadScene(world, "Resources/Engine/Cache/SceneCache.json");
			}
		}

		// 1. 全画面を覆うウィンドウの設定
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		// スタイル設定（角丸や枠線を消す）
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		// ウィンドウフラグ設定
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		window_flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

		// 2. ドッキング用ルートウィンドウの開始
		ImGui::Begin("MainDockSpace", nullptr, window_flags);
		ImGui::PopStyleVar(3); // スタイルの復元

		// 3. ドッキングスペースの提出
		ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

		// ツールバー
		if (ImGui::BeginMainMenuBar())
		{
			// ファイルメニュー
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save Scene", "Ctrl + S"))
				{
					std::string path = SceneManager::Instance().GetCurrentScenePath();
					if (path.empty()) path = "Resources/Game/Scenes/Untitled.json";

					SceneSerializer::SaveScene(world, path);
					SceneManager::Instance().SetDirty(false);
					EditorPrefs::Instance().lastScenePath = path;
					EditorPrefs::Instance().Save();
				}

				if (ImGui::MenuItem("Exit"))
				{
					RequestCloseEngine();
				}

				ImGui::EndMenu();
			}

			// ウィンドウメニュー
			if (ImGui::BeginMenu("Window"))
			{
				for (auto& window : m_windows)
				{
					if (ImGui::MenuItem(window->m_windowName.c_str(), nullptr, window->m_isOpen))
					{
						window->m_isOpen = !window->m_isOpen;
					}
				}

				ImGui::Separator();
				if (ImGui::MenuItem("Reset Layout"))
				{
					std::filesystem::remove("imgui.ini");
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

					// 1. まず現在のワールドを空にする
					world.clearSystems();
					world.clearEntities();

					// 復元
					SceneSerializer::LoadScene(world, "Resources/Engine/Cache/SceneCache.json");

					// 3. システムの復旧 (重要)
					// シーンファイルにシステム情報が含まれていない場合、描画などが止まってしまうためデフォルトを追加
					if (world.getSystems().empty())
					{
						// Logger::Log("Restoring default systems...");
						auto& reg = SystemRegistry::Instance();
						reg.CreateSystem(world, "Physics System", SystemGroup::PlayOnly);
						reg.CreateSystem(world, "Collision System", SystemGroup::PlayOnly);
						reg.CreateSystem(world, "UI System", SystemGroup::Always);
						reg.CreateSystem(world, "Lifetime System", SystemGroup::PlayOnly);
						reg.CreateSystem(world, "Hierarchy System", SystemGroup::Always);
						reg.CreateSystem(world, "Animation System", SystemGroup::PlayOnly);
						reg.CreateSystem(world, "Render System", SystemGroup::Always);
						reg.CreateSystem(world, "Model Render System", SystemGroup::Always);
						reg.CreateSystem(world, "Sprite Render System", SystemGroup::Always);
						reg.CreateSystem(world, "Billboard System", SystemGroup::Always);
						reg.CreateSystem(world, "Text Render System", SystemGroup::Always);
						reg.CreateSystem(world, "Audio System", SystemGroup::Always);
						reg.CreateSystem(world, "Button System", SystemGroup::Always);
					}
				}
			}

			// 現在のシーン名を表示
			std::string currentScene = SceneManager::Instance().GetCurrentScenePath();
			if (currentScene.empty()) currentScene = "Untitled";
			else currentScene = std::filesystem::path(currentScene).filename().string();

			if (SceneManager::Instance().IsDirty()) currentScene += "*";

			// 画面中央に表示するための計算
			float textWidth = ImGui::CalcTextSize(currentScene.c_str()).x;
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", currentScene.c_str());

			ImGui::EndMainMenuBar();
		}

		// ポップアップの描画
		DrawSavePopup();

		ImGuizmo::BeginFrame();

		// 描画対象のワールドを切り替える
		World* activeWorld = &world;
		if (m_editorMode == EditorMode::Prefab && m_prefabWorld)
		{
			activeWorld = m_prefabWorld.get();
		}

		// 各ウィンドウを描画
		for (auto& window : m_windows)
		{
			window->Draw(*activeWorld, m_selectedEntity, ctx);
		}

		m_sceneViewPanel.Draw(*activeWorld, m_selectedEntity);

		m_gameViewPanel.Draw();

		// デバッグUI
		AudioManager::Instance().OnInspector();

		// ログウィンドウの描画
		Logger::Draw("Debug Logger");

		ImGui::End();
	}

	void Editor::OpenPrefab(const std::string& path)
	{
		// 1. プレファブモードへ移行
		m_editorMode = EditorMode::Prefab;
		m_currentPrefabPath = path;
		
		// 履歴をクリア
		CommandHistory::Clear();

		// 2. 編集用の一時ワールドを作成
		m_prefabWorld = std::make_unique<World>();

		// 描画システム
		auto& sysReg = SystemRegistry::Instance();

		// 描画に関連するシステム
		sysReg.CreateSystem(*m_prefabWorld, "Render System", SystemGroup::Always);
		sysReg.CreateSystem(*m_prefabWorld, "Model Render System", SystemGroup::Always);
		sysReg.CreateSystem(*m_prefabWorld, "Sprite Render System", SystemGroup::Always);
		sysReg.CreateSystem(*m_prefabWorld, "Billboard System", SystemGroup::Always);
		sysReg.CreateSystem(*m_prefabWorld, "Text Render System", SystemGroup::Always);
		sysReg.CreateSystem(*m_prefabWorld, "UI System", SystemGroup::Always);
		sysReg.CreateSystem(*m_prefabWorld, "Hierarchy System", SystemGroup::Always);

		// 3. プレファブをロード
		Entity root = SceneSerializer::LoadPrefab(*m_prefabWorld, path);
		m_prefabRoot = root;

		if (m_prefabRoot != NullEntity && m_prefabWorld->getRegistry().has<PrefabInstance>(m_prefabRoot))
		{
			m_prefabWorld->getRegistry().remove<PrefabInstance>(m_prefabRoot);
		}

		// 4. カメラのフォーカスを合わせる
		if (root != NullEntity)
		{
			m_sceneViewPanel.FocusEntity(root, *m_prefabWorld);
		}

		// 5. 選択状態をリセット
		SetSelectedEntity(root);

		Logger::Log("Opened Prefab Mode: " + path);
	}

	void Editor::SavePrefabAndExit()
	{
		if (m_editorMode != EditorMode::Prefab || !m_prefabWorld) return;

		Entity root = m_prefabRoot;

		if (root != NullEntity && m_prefabWorld->getRegistry().valid(root))
		{
			// 2. ファイルへ書き出し
			SceneSerializer::SavePrefab(m_prefabWorld->getRegistry(), root, m_currentPrefabPath);
			Logger::Log("Prefab Saved: " + m_currentPrefabPath);

			// 3. メインシーンへの反映 (Propagation)
			SceneSerializer::ReloadPrefabInstances(
				SceneManager::Instance().GetWorld(),
				m_currentPrefabPath
			);
		}
		else
		{
			Logger::LogError("Failed to save prefab: Root entity not found.");
		}

		// 4. モード終了
		ExitPrefabMode();
	}

	void Editor::ExitPrefabMode()
	{
		// 履歴をクリア
		CommandHistory::Clear();

		m_editorMode = EditorMode::Scene;
		m_prefabWorld.reset(); // 一時ワールド破棄
		m_currentPrefabPath = "";
		m_prefabRoot = NullEntity;

		// 選択解除
		SetSelectedEntity(NullEntity);
	}

	void Editor::RequestOpenScene(const std::string& path)
	{
		if (SceneManager::Instance().IsDirty())
		{
			// 変更があれば確認ポップアップを出す
			m_pendingAction = PendingAction::LoadScene;
			m_pendingScenePath = path;
			m_showSavePopup = true;
		}
		else
		{
			// 変更が無ければ即ロード
			SceneManager::Instance().LoadSceneAsync(path, new ImmediateTransition());
		}
	}

	void Editor::RequestCloseEngine()
	{
		if (SceneManager::Instance().IsDirty())
		{
			m_pendingAction = PendingAction::CloseEngine;
			m_showSavePopup = true;
		}
		else
		{
			PostQuitMessage(0);
		}
	}

	void Editor::DrawSavePopup()
	{
		if (m_showSavePopup)
		{
			ImGui::OpenPopup("Save Changes?");
			m_showSavePopup = false; // OpenPopupは1回呼べばいいので戻す
		}

		// モーダルウィンドウ（画面中央に表示、後ろを触れなくする）
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("Save Changes?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Current scene has unsaved changes.\nDo you want to save before continuing?");
			ImGui::Separator();

			// --- SAVE ---
			if (ImGui::Button("Save", ImVec2(120, 0)))
			{
				// 現在のシーンを保存
				std::string currentPath = SceneManager::Instance().GetCurrentScenePath();
				if (currentPath.empty()) currentPath = "Resources/Game/Scenes/Untitled.json";

				SceneSerializer::SaveScene(SceneManager::Instance().GetWorld(), currentPath);
				SceneManager::Instance().SetDirty(false); // 保存したのでDirty解除
				Logger::Log("Saved: " + currentPath);

				// 保留していたアクションを実行
				if (m_pendingAction == PendingAction::LoadScene)
				{
					SceneManager::Instance().LoadSceneAsync(m_pendingScenePath);
				}
				else if (m_pendingAction == PendingAction::CloseEngine)
				{
					PostQuitMessage(0);
				}

				m_pendingAction = PendingAction::None;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();

			// --- DON'T SAVE ---
			if (ImGui::Button("Don't Save", ImVec2(120, 0)))
			{
				// 保存せずにアクション実行
				if (m_pendingAction == PendingAction::LoadScene)
				{
					SceneManager::Instance().LoadSceneAsync(m_pendingScenePath);
				}
				else if (m_pendingAction == PendingAction::CloseEngine)
				{
					PostQuitMessage(0);
				}

				m_pendingAction = PendingAction::None;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			// --- CANCEL ---
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				// 何もしない
				m_pendingAction = PendingAction::None;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

}	// namespace Arche

#endif // _DEBUG