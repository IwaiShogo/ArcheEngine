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

// パネル群
#include "Editor/Panels/HierarchyWindow.h"
#include "Editor/Panels/InspectorWindow.h"
#include "Editor/Panels/ContentBrowser.h"
#include "Editor/Panels/ProjectSettingsWindow.h"
#include "Editor/Panels/ResourceInspectorWindow.h"
#include "Editor/Panels/SystemMonitorWindow.h"
#include "Editor/Panels/GameControlWindow.h"
#include "Editor/Panels/InputVisualizerWindow.h"

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
		m_windows.push_back(std::make_unique<ProjectSettingsWindow>());
		m_windows.push_back(std::make_unique<ResourceInspectorWindow>());
		m_windows.push_back(std::make_unique<SystemMonitorWindow>());
		m_windows.push_back(std::make_unique<GameControlWindow>());
		m_windows.push_back(std::make_unique<InputVisualizerWindow>());

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
					SceneSerializer::SaveScene(world, s_currentScenePath);
					EditorPrefs::Instance().lastScenePath = s_currentScenePath;
					EditorPrefs::Instance().Save();
				}
				ImGui::EndMenu();
			}

			// ウィンドウメニュー
			if (ImGui::BeginMenu("Window"))
			{
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
						reg.CreateSystem(world, "Input System", SystemGroup::Always);
						reg.CreateSystem(world, "Physics System", SystemGroup::PlayOnly);
						reg.CreateSystem(world, "Collision System", SystemGroup::PlayOnly);
						reg.CreateSystem(world, "UI System", SystemGroup::Always);
						reg.CreateSystem(world, "Lifetime System", SystemGroup::PlayOnly);
						reg.CreateSystem(world, "Hierarchy System", SystemGroup::Always);
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

			ImGui::EndMainMenuBar();
		}

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
		sysReg.CreateSystem(*m_prefabWorld, "Hierarchy System", SystemGroup::Always);

		// 3. プレファブをロード
		Entity root = SceneSerializer::LoadPrefab(*m_prefabWorld, path);

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

		// 1. プレファブのルートエンティティを探す
		// (LoadPrefabで作ったルート。通常はRegistryの先頭付近にあるはず)
		// ここでは簡易的に、RelationshipのParentがNullEntityであるものをルートとする
		Entity root = NullEntity;
		m_prefabWorld->getRegistry().each([&](Entity e) {
			if (m_prefabWorld->getRegistry().has<Relationship>(e)) {
				if (m_prefabWorld->getRegistry().get<Relationship>(e).parent == NullEntity) {
					root = e;
				}
			}
			});

		if (root != NullEntity)
		{
			// 2. ファイルへ書き出し
			SceneSerializer::SavePrefab(m_prefabWorld->getRegistry(), root, m_currentPrefabPath);
			Logger::Log("Prefab Saved: " + m_currentPrefabPath);

			// 3. メインシーンへの反映 (Propagation)
			PropagateChangesToScene();
		}

		// 4. モード終了
		ExitPrefabMode();
	}

	void Editor::ExitPrefabMode()
	{
		m_editorMode = EditorMode::Scene;
		m_prefabWorld.reset(); // 一時ワールド破棄
		m_currentPrefabPath = "";

		// 選択解除
		SetSelectedEntity(NullEntity);
	}

	// メインシーンにあるインスタンスを更新する処理
	void Editor::PropagateChangesToScene()
	{
		World& mainWorld = SceneManager::Instance().GetWorld();
		Registry& mainReg = mainWorld.getRegistry();

		// 1. 更新されたプレファブデータをJSONとしてメモリにロード
		// (ファイルから読むのが一番確実)
		std::ifstream fin(m_currentPrefabPath);
		json prefabJson;
		fin >> prefabJson;
		fin.close();

		// 2. シーン内から、このプレファブを使っているエンティティを探す
		std::vector<Entity> targetEntities;

		// Viewを使って走査
		auto view = mainReg.view<PrefabInstance>();
		for (auto entity : view)
		{
			const auto& prefab = view.get<PrefabInstance>(entity);
			if (prefab.prefabPath == m_currentPrefabPath)
			{
				targetEntities.push_back(entity);
			}
		}

		// 3. 各エンティティを更新（再構築）
		for (auto target : targetEntities)
		{
			// A. 現在のトランスフォームをバックアップ（位置がリセットされないように）
			Transform backupTransform;
			if (mainReg.has<Transform>(target)) backupTransform = mainReg.get<Transform>(target);

			// 親子関係のバックアップ
			Entity parent = NullEntity;
			if (mainReg.has<Relationship>(target)) parent = mainReg.get<Relationship>(target).parent;

			// B. 子エンティティをすべて削除 (プレファブ構造が変わっている可能性があるため)
			// 再帰的な削除が必要
			std::function<void(Entity)> deleteChildren = [&](Entity e) {
				if (mainReg.has<Relationship>(e)) {
					for (auto c : mainReg.get<Relationship>(e).children) {
						deleteChildren(c); // 再帰
						mainReg.destroy(c);
					}
				}
				};
			deleteChildren(target);

			// C. ターゲット自身のコンポーネントを全クリア (IDとTag, PrefabInstance以外)
			// EnTTの remove_all 相当の処理が必要ですが、簡易的に主要なものを消すか、
			// もしくは「ターゲット自体も消して、同じIDで作り直す」のが理想ですがID維持は難しい。
			// ここでは「古いコンポーネントを上書きロードする」方針をとります。

			// PrefabJson[0] はルート要素
			// ComponentSerializer::DeserializeEntity は "追記/上書き" を行う
			if (prefabJson.is_array() && !prefabJson.empty())
			{
				// ルートのJSONデータ
				json rootJson = prefabJson[0];

				// 上書きロード (Transformなどもプレファブの値になる)
				ComponentSerializer::DeserializeEntity(mainReg, target, rootJson);

				// D. バックアップしていた位置情報などを復元
				// これにより「プレファブの中身は更新されるが、シーンでの配置場所は維持される」
				if (mainReg.has<Transform>(target))
				{
					auto& t = mainReg.get<Transform>(target);
					t.position = backupTransform.position;
					t.rotation = backupTransform.rotation;
					t.scale = backupTransform.scale;
				}

				// E. PrefabInstanceコンポーネントが消えていたら付け直す
				if (!mainReg.has<PrefabInstance>(target)) {
					mainReg.emplace<PrefabInstance>(target, m_currentPrefabPath);
				}

				// F. 子要素の再生成 (JSONの2つ目以降が子要素)
				// SceneSerializer::LoadPrefab のロジックの一部を再利用して
				// target を親として子を生成する必要がある。
				// ※ここが少し複雑になるため、SceneSerializerにヘルパー関数を作ると良い
				SceneSerializer::ReconstructPrefabChildren(mainWorld, target, prefabJson);
			}
		}

		Logger::Log("Propagated changes to " + std::to_string(targetEntities.size()) + " instances.");
	}

}	// namespace Arche

#endif // _DEBUG