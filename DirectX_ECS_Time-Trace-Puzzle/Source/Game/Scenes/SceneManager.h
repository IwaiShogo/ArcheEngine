/*****************************************************************//**
 * @file	SceneManager.h
 * @brief	現在のシーンを保持し、切り替えを行うクラス
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

#ifndef ___SCENE_MANAGER_H___
#define ___SCENE_MANAGER_H___

// ===== インクルード =====
#include "Game/Scenes/IScene.h"
#include "Game/Scenes/SceneTitle.h"
#include "Game/Scenes/SceneGame.h"
#include "Engine/Core/Context.h"

#include <memory>
#include <map>
#include <functional>

/**
 * @class	SceneManager
 * @brief	現在のシーンを保持し、切り替えを行うクラス
 */
class SceneManager
{
public:
	SceneManager()
		: m_currentType(SceneType::None) {}

	~SceneManager()
	{
		if (m_currentScene)
		{
			m_currentScene->Finalize();
		}
	}
	
	// 静的メソッド：シーン切り替え
	static void ChangeScene(SceneType nextScene)
	{
		m_nextSceneRequest = nextScene;
	}

	// ApplicationからContextをセット
	void SetContext(const Context& context)
	{
		m_context = context;
	}

	Context& GetContext()
	{
		return m_context;
	}

	SceneType GetCurrentType() const
	{
		return m_currentType;
	}

	// 現在のシーンからワールドを取得
	World& GetWorld()
	{
		if (!m_currentScene)
		{
			static World emptyWorld;
			return emptyWorld;
		}
		return m_currentScene->GetWorld();
	}

	// 最初のシーンを設定して開始
	void Initialize(SceneType startScene)
	{
		ChangeScene(startScene);
		ProcessSceneChange();
	}

	// 毎フレーム呼ぶ
	void Update()
	{
		// シーン切り替えリクエストがあれば処理
		if (m_nextSceneRequest != SceneType::None)
		{
			ProcessSceneChange();
		}

		if (m_currentScene)
		{
			m_currentScene->Update();
		}
	}

	void Render()
	{
		if (m_currentScene)
		{
			m_currentScene->Render();
		}
	}

private:
	void ProcessSceneChange()
	{
		// リクエスト消化
		SceneType next = m_nextSceneRequest;
		m_nextSceneRequest = SceneType::None;

		// 同じシーンなら無視
		if (next == m_currentType) return;
		if (m_currentScene) m_currentScene->Finalize();

		m_currentScene = CreateScene(next);
		m_currentType = next;

		if (m_currentScene)
		{
			m_currentScene->Setup(&m_context);

			m_currentScene->Initialize();
		}
	}

	// シーン生成用ファクトリ関数
	std::shared_ptr<IScene> CreateScene(SceneType type)
	{
		switch (type)
		{
		case SceneType::Title:
			return std::make_shared<SceneTitle>();
		case SceneType::Game:
			return std::make_shared<SceneGame>();
		default:
			return nullptr;
		}
	}

private:
	std::shared_ptr<IScene> m_currentScene;
	SceneType m_currentType;

	// 次のシーンへの予約
	inline static SceneType m_nextSceneRequest;

	Context m_context;
};

#endif // !___SCENE_MANAGER_H___