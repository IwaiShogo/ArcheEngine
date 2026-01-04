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
#include "Engine/Scene/Core/SceneTransition.h"

namespace Arche
{
	struct AsyncOperation
	{
		bool isDone = false;
		float progress = 0.0f;
	};

	/**
	 * @class	SceneManager
	 * @brief	Worldの実体を持ち、更新と描画を行う
	 */
	class ARCHE_API SceneManager
	{
	public:
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;

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

		// @brief	指定したワールドを描画する
		void Render(World* targetWorld);

		// シーンロード API
		// ------------------------------------------------------------

		// 1. 同期ロード（基本）
		void LoadScene(const std::string& filepath, ISceneTransition* transition);

		// 2. 非同期ロード
		std::shared_ptr<AsyncOperation> LoadSceneAsync(const std::string& filepath, ISceneTransition* transition = nullptr);

		// ------------------------------------------------------------

		// @brief	Contextアクセス
		void SetContext(const Context& context) { m_context = context; }
		Context& GetContext() { return m_context; }

		// @brief	ワールドへのアクセス
		World& GetWorld() { return m_world; }

		// @brief	現在のシーン名
		const std::string& GetCurrentSceneName() const { return m_currentSceneName; }

	private:
		// 自身の静的ポインタ
		static SceneManager* s_instance;
		World m_world;
		Context m_context;
		std::string m_currentSceneName = "Untitled";

		// 遷移管理
		std::unique_ptr<ISceneTransition> m_transition = nullptr;
		std::string m_nextScenePath = "";
		
		// 非同期ロード管理
		bool m_isAsyncLoading = false;
		std::future<void> m_loadFuture;		// ロードスレッドの状態
		std::unique_ptr<World> m_tempWorld;	// ロード中の新しいワールド
		std::shared_ptr<AsyncOperation> m_currentAsyncOp;

		// 内部処理
		void PerformLoad(const std::string& path);	// 実際にファイルを読んでWorldを作る
		void SwapWorld();							// メインスレッドでワールドを入れ替える
	};

}	// namespace Arche

#endif // !___SCENE_MANAGER_H___