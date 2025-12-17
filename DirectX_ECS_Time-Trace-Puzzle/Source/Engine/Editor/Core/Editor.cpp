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
#include "Engine/Editor/Core/Editor.h"
#include "Engine/Editor/Windows/HierarchyWindow.h"
#include "Engine/Editor/Windows/InspectorWindow.h"
#include "Engine/Editor/Windows/SystemWindow.h"
#include "Engine/Editor/Tools/GizmoSystem.h"
#include "Engine/Editor/Windows/CreatorWindow.h"
#include "Engine/Physics/CollisionSystem.h"

void Editor::Initialize()
{
	m_windows.clear();

	m_windows.push_back(std::make_unique<HierarchyWindow>());
	m_windows.push_back(std::make_unique<InspectorWindow>());
	m_windows.push_back(std::make_unique<SystemWindow>());
	m_windows.push_back(std::make_unique<CreatorWindow>());
}

void Editor::Draw(World& world, Context& ctx)
{	
	Registry& reg = world.getRegistry();

	// 各ウィンドウを描画
	for (auto& window : m_windows)
	{
		window->Draw(world, m_selectedEntity, ctx);
	}

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