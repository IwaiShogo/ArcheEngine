/*****************************************************************//**
 * @file	SceneManager.cpp
 * @brief	シーンマネージャー
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/Core/SceneManager.h"
#include "Engine/Scene/Systems/Physics/CollisionSystem.h"
#include "Engine/Scene/Serializer/SceneSerializer.h"
#include "Engine/Scene/Serializer/SystemRegistry.h"

namespace Arche
{
	// 静的メンバ変数の実体
	SceneManager* SceneManager::s_instance = nullptr;

	SceneManager::SceneManager()
	{
		assert(s_instance == nullptr && "SceneManagerは既に存在します。");
		s_instance = this;
	}

	SceneManager::~SceneManager()
	{
		// システムのクリア
		m_world.clearSystems();

		if (s_instance == this)
		{
			s_instance = nullptr;
		}
	}

	SceneManager& SceneManager::Instance()
	{
		assert(s_instance != nullptr);
		return *s_instance;
	}

	void SceneManager::Initialize()
	{
		// 必要ならデフォルト設定など
	}

	void SceneManager::Update()
	{
		// シーン遷移リクエストがあれば実行
		if (!m_nextScenePath.empty())
		{
			ExecuteLoadScene();
		}

		// Worldの更新
		m_world.Tick(m_context.editorState);
	}

	void SceneManager::Render()
	{
		// Worldの描画
		m_world.Render(m_context);
	}

	void SceneManager::LoadScene(const std::string& filepath)
	{
		m_nextScenePath = filepath;
	}

	void SceneManager::ExecuteLoadScene()
	{
		// ファイルチェック
		std::ifstream f(m_nextScenePath);
		if (!f.good())
		{
			Logger::LogError("Scene not found: " + m_nextScenePath);
			m_nextScenePath = "";
			return;
		}
		f.close();

		// ロード実行
		SceneSerializer::LoadScene(m_world, m_nextScenePath);
		m_currentSceneName = m_nextScenePath;

		Logger::Log("Scene Loaded: " + m_nextScenePath);

		// システムがない場合の自動復旧
		if (m_world.getSystems().empty())
		{
			Logger::LogWarning("No systems found (or failed to load). Adding default systems.");
			auto& reg = SystemRegistry::Instance();

			// 必須システム群
			reg.CreateSystem(m_world, "Input System",			SystemGroup::Always);
			reg.CreateSystem(m_world, "Physics System",			SystemGroup::PlayOnly);
			reg.CreateSystem(m_world, "Collision System",		SystemGroup::PlayOnly);
			reg.CreateSystem(m_world, "UI System",				SystemGroup::Always);
			reg.CreateSystem(m_world, "Lifetime System",		SystemGroup::PlayOnly);
			reg.CreateSystem(m_world, "Hierarchy System",		SystemGroup::Always);
			reg.CreateSystem(m_world, "Render System",			SystemGroup::Always);
			reg.CreateSystem(m_world, "Model Render System",	SystemGroup::Always);
			reg.CreateSystem(m_world, "Billboard System",		SystemGroup::Always);
			reg.CreateSystem(m_world, "Sprite Render System",	SystemGroup::Always);
			reg.CreateSystem(m_world, "Text Render System",		SystemGroup::Always);
			reg.CreateSystem(m_world, "Audio System",			SystemGroup::Always);
		}

		// リクエストクリア
		m_nextScenePath = "";
	}

}	// namespace Arche