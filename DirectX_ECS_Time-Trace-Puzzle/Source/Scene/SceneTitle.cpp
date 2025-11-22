/*****************************************************************//**
 * @file	SceneTitle.cpp
 * @brief	タイトルシーン
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Scene/SceneTitle.h"
#include "imgui.h"
#include <iostream>

void SceneTitle::Initialize()
{
	std::cout << "Title Scene Initialized" << std::endl;
}

void SceneTitle::Finalize()
{
	std::cout << "Title Scene Finalized" << std::endl;
}

SceneType SceneTitle::Update()
{
	return SceneType::Title;
}

void SceneTitle::Render()
{
}

void SceneTitle::OnInspector()
{
	ImGui::Begin("Title Menu");
	ImGui::Text("Press Enter to Start");
	if (ImGui::Button("Go to Game Scene"))
	{

	}
	ImGui::End();
}