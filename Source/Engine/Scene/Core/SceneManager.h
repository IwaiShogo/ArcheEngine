/*****************************************************************//**
 * @file	SceneManager.h
 * @brief	現在のシーン（World）を保持し、管理を行うクラス
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___SCENE_MANAGER_H___
#define ___SCENE_MANAGER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/Core/ECS/ECS.h"

namespace Arche
{
	/**
	 * @class	SceneManager
	 * @brief	Worldの実体を持ち、更新と描画を行う
	 */
	class SceneManager
	{
	public:
		SceneManager();
		~SceneManager();

		// @brief	どこからでもアクセス可能
		static SceneManager& Instance();

		// @brief	初期化
		void Initialize();

		// @brief	更新
		void Update();

		// @brief	描画
		void Render();

		// @brief	Contextアクセス
		void SetContext(const Context& context) { m_context = context; }
		Context& GetContext() { return m_context; }

		// @brief	ワールドへのアクセス
		World& GetWorld() { return m_world; }

		// @brief	現在のシーン名
		const std::string& GetCurrentSceneName() const { return m_currentSceneName; }

		// @brief	シーンロード予約
		void LoadScene(const std::string& filepath);

	private:
		// 実際にロードを行う関数
		void ExecuteLoadScene();

	private:
		// 自身の静的ポインタ
		static SceneManager* s_instance;

		World m_world;
		std::string m_currentSceneName = "Untitled";
		Context m_context;
		std::string m_nextScenePath = "";
	};

}	// namespace Arche

#endif // !___SCENE_MANAGER_H___