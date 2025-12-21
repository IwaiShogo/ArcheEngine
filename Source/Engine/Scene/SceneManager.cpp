/*****************************************************************//**
 * @file	SceneManager.cpp
 * @brief	シーンマネージャー
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/17	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/SceneManager.h"
#include "Engine/Scene/ECS/ECS.h"

// 静的メンバ変数の実体
SceneManager* SceneManager::s_instance = nullptr;

SceneManager::SceneManager()
{
	assert(s_instance == nullptr && "SceneManagerは既に存在します！重複生成禁止です。");
	s_instance = this;
}

SceneManager::~SceneManager()
{
	if (m_currentScene)
	{
		m_currentScene->Finalize();
	}

	if (s_instance == this)
	{
		s_instance = nullptr;
	}
}

SceneManager& SceneManager::Instance()
{
	assert(s_instance != nullptr && "SceneManagerがまだ生成されていません！Application初期化前に読んでいませんか？");
	return *s_instance;
}

void SceneManager::Initialize()
{
	
}

void SceneManager::Update()
{
	// シーン切り替えリクエストがあれば処理
	if (!m_nextSceneRequest.empty())
	{
		ProcessSceneChange();
	}

	if (m_currentScene)
	{
		m_currentScene->Update();
	}
}

void SceneManager::Render()
{
	if (m_currentScene)
	{
		m_currentScene->Render();
	}
}

void SceneManager::ChangeScene(const std::string& name)
{
	m_nextSceneRequest = name;
}

World& SceneManager::GetWorld()
{
	if (!m_currentScene)
	{
		// シーンが無い場合のダミーWorld（クラッシュ防止）
		static World emptyWorld;
		return emptyWorld;
	}
	return m_currentScene->GetWorld();
}

void SceneManager::ProcessSceneChange()
{
	std::string nextName = m_nextSceneRequest;
	m_nextSceneRequest = "";	// リクエスト消化

	// 登録されていないシーンならエラーログを出して無視
	auto it = m_factories.find(nextName);
	if (it == m_factories.end())
	{
		Logger::LogError("Error: Scene not found" + nextName + "\n");
		return;
	}

	// 現在のシーン終了
	if (m_currentScene)
	{
		m_currentScene->Finalize();
	}

	// 新しいシーンを生成（ファクトリ関数を実行）
	m_currentScene = it->second();
	m_currentSceneName = nextName;

	// 新しいシーンの初期化
	if (m_currentScene)
	{
		m_currentScene->Setup(&m_context);
		m_currentScene->Initialize();
	}
}
