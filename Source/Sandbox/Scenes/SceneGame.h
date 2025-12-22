/*****************************************************************//**
 * @file	SceneGame.h
 * @brief	ゲームシーン
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

#ifndef ___SCENE_GAME_H___
#define ___SCENE_GAME_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/Core/IScene.h"
#include "Engine/Scene/Core/ECS/ECS.h"

/**
 * @class	SceneGame
 * @brief	ゲームシーン
 */
class SceneGame
	: public IScene
{
public:
	SceneGame() = default;
	~SceneGame() = default;

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Render() override;

	World& GetWorld() override { return m_world; }

private:
	World m_world;
};

#endif // !___SCENE_GAME_H___
