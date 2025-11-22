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
#include "Scene/IScene.h"
#include "SceneTitle.h"
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
		: m_currenType(SceneType::None) {}

	~SceneManager()
	{
		if (m_currentScene)
		{
			m_currentScene->Finalize();
		}
	}

	// 最初のシーンを設定して開始
	void Initialize(SceneType startScene)
	{
		ChangeScene(startScene);
	}

	// 毎フレーム呼ぶ
	void Update()
	{
		if (!m_currentScene) return;

		// 現在のシーンを実行し、次のシーン要求を受けとる
		SceneType next = m_currentScene->Update();

		// デバッグ情報（ImGui）
		m_currentScene->OnInspector();

		// シーンが変わっていたら切り替え
		if (next != m_currenType)
		{
			ChangeScene(next);
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
	// シーン切り替え処理
	void ChangeScene(SceneType nextScene)
	{
		// 古いシーンの終了
		if (m_currentScene)
		{
			m_currentScene->Finalize();
		}

		// 新しいシーンの生成
		m_currentScene = CreateScene(nextScene);
		m_currenType = nextScene;

		// 新しいシーンの初期化
		if (m_currentScene)
		{
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
		default:
			return nullptr;
		}
	}

private:
	std::shared_ptr<IScene> m_currentScene;
	SceneType m_currenType;
};

#endif // !___SCENE_MANAGER_H___