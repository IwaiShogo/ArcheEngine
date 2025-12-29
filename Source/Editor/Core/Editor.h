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

		void DrawGizmo(World& world, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj, float x, float y, float w, float h);

		void SetSelectedEntity(Entity e) { m_selectedEntity = e; }
		Entity& GetSelectedEntity() { return m_selectedEntity; }
		
		SceneViewPanel& GetSceneViewPanel() { return m_sceneViewPanel; }

	private:
		Editor() = default;
		~Editor() = default;

		Entity m_selectedEntity = NullEntity;

		// ウィンドウ間利用リスト
		std::vector<std::unique_ptr<EditorWindow>> m_windows;

		SceneViewPanel m_sceneViewPanel;
		GameViewPanel m_gameViewPanel;
	};
}	// namespace Arche

#endif // _DEBUG

#endif // !___EDITOR_H___