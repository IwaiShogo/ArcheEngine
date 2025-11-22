/*****************************************************************//**
 * @file	SceneTitle.h
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

#ifndef ___SCENE_TITLE_H___
#define ___SCENE_TITLE_H___

// ===== インクルード =====
#include "Scene/IScene.h"

class SceneTitle
	: public IScene
{
public:
	void Initialize() override;
	void Finalize() override;
	SceneType Update() override;
	void Render() override;
	void OnInspector() override;
};

#endif // !___SCENE_TITLE_H___