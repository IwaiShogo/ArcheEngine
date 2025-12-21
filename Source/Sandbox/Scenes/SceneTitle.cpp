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
#include "Engine/pch.h"
#include "Sandbox/Scenes/SceneTitle.h"
#include "Engine/Scene/SceneManager.h"
#include "Engine/Core/Input.h"

void SceneTitle::Initialize()
{
	// 初期化処理
}

void SceneTitle::Finalize()
{
}

void SceneTitle::Update()
{
	if (Input::GetKeyDown(VK_SPACE))
	{
		SceneManager::Instance().ChangeScene("Game");
	}
}

void SceneTitle::Render()
{
}
