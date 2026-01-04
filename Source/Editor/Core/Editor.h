/*****************************************************************//**
 * @file	Editor.h
 * @brief	デバッグGUI全体の管理者
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

#ifndef ___EDITOR_H___
#define ___EDITOR_H___

#ifdef _DEBUG

// ===== インクルード ====
#include "Engine/pch.h"
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Core/Context.h"

// パネル群
#include "Editor/Panels/SceneViewPanel.h"
#include "Editor/Panels/GameViewPanel.h"

namespace Arche
{
	// 前方宣言
	class HierarchyWindow;

	// 各ウィンドウの親クラス
	class EditorWindow
	{
	public:
		virtual ~EditorWindow() = default;
		virtual void Draw(World& world, Entity& selected, Context& ctx) = 0;
	};

	// エディタの管理者
	class Editor
	{
	public:
		static Editor& Instance()
		{
			static Editor instance;
			return instance;
		}

		void Initialize();
		void Draw(World& world, Context& ctx);

		void SetSelectedEntity(Entity e) { m_selectedEntity = e; }
		Entity& GetSelectedEntity() { return m_selectedEntity; }
		
		SceneViewPanel& GetSceneViewPanel() { return m_sceneViewPanel; }

		enum class EditorMode { Scene, Prefab };

		// 現在のモード取得
		EditorMode GetMode() const { return m_editorMode; }

		// プレファブモードで編集中のワールドを取得
		World* GetActiveWorld() { return (m_editorMode == EditorMode::Scene) ? &SceneManager::Instance().GetWorld() : m_prefabWorld.get(); }

		// プレファブを開く
		void OpenPrefab(const std::string& path);

		// プレファブを保存してシーンに戻る
		void SavePrefabAndExit();

		// 変更を破棄してシーンに戻る
		void ExitPrefabMode();

	private:
		Editor() = default;
		~Editor() = default;

		Entity m_selectedEntity = NullEntity;

		// ウィンドウ間利用リスト
		std::vector<std::unique_ptr<EditorWindow>> m_windows;

		HierarchyWindow* m_hierarchyPanel = nullptr;

		SceneViewPanel m_sceneViewPanel;
		GameViewPanel m_gameViewPanel;

		EditorMode m_editorMode = EditorMode::Scene;
		std::unique_ptr<World> m_prefabWorld;			// プレファブ編集用の一時ワールド
		std::string m_currentPrefabPath;				// 編集中のパス

		// 内部処理: メインシーン内の全インスタンスを更新
		void PropagateChangesToScene();
	};
}	// namespace Arche

#endif // _DEBUG

#endif // !___EDITOR_H___