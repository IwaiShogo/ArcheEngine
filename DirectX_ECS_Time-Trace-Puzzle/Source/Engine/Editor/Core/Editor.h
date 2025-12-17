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

// ===== インクルード ====
#include "Engine/pch.h"
#include "Engine/ECS/ECS.h"
#include "Engine/Core/Context.h"

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

private:
	Editor() = default;
	~Editor() = default;

	std::vector<std::unique_ptr<EditorWindow>> m_windows;
	Entity m_selectedEntity = NullEntity;
};

#endif // !___EDITOR_H___