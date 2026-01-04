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
#include "Engine/Scene/Serializer/SceneSerializer.h"
#include "Engine/Scene/Serializer/SystemRegistry.h"
#include "Engine/Core/Time/Time.h"

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
		// 必要なら
	}

	void SceneManager::Update()
	{
		float dt = Time::DeltaTime();

		// シーン更新
		if (m_transition == nullptr || m_transition->GetPhase() != ISceneTransition::Phase::WaitAsync)
		{
			m_world.Tick(m_context.editorState);
		}

		// 遷移エフェクト更新
		if (m_transition)
		{
			// フェード更新。trueが帰ってきたらそのフェーズ完了
			if (m_transition->Update(dt))
			{
				auto phase = m_transition->GetPhase();

				// 1. フェードアウト完了 -> ロード開始
				if (phase == ISceneTransition::Phase::Out)
				{
					if (m_isAsyncLoading)
					{
						// 非同期ロード開始
						m_transition->SetPhase(ISceneTransition::Phase::WaitAsync);
						// タイマーリセット
						m_transition->ResetTimer();
						// 別スレッドでファイルを読み込む
						m_loadFuture = std::async(std::launch::async, [this]()
						{
							// 一時的なワールド作成してロード
							m_tempWorld = std::make_unique<World>();
							// ファイル読み込み
							SceneSerializer::LoadScene(*m_tempWorld, m_nextScenePath);
						});
					}
					else
					{
						// 同期ロード
						m_transition->SetPhase(ISceneTransition::Phase::Loading);
						m_transition->ResetTimer();
						PerformLoad(m_nextScenePath);
						m_transition->SetPhase(ISceneTransition::Phase::In);
						m_transition->ResetTimer();
					}
				}
			}

			// 2. 非同期ロード待ち
			if (m_transition->GetPhase() == ISceneTransition::Phase::WaitAsync)
			{
				// スレッドが終わったか確認
				if (m_loadFuture.valid() && m_loadFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
				{
					m_loadFuture.get();

					// ワールド入れ替え
					SwapWorld();

					if (m_currentAsyncOp)
					{
						m_currentAsyncOp->progress = 1.0f;
						m_currentAsyncOp->isDone = true;
					}

					// フェードイン開始
					m_transition->SetPhase(ISceneTransition::Phase::In);
					m_transition->ResetTimer();
				}
			}

			// 3. 終了判定
			if (m_transition->GetPhase() == ISceneTransition::Phase::Finished)
			{
				m_transition = nullptr;
				m_isAsyncLoading = false;
				m_currentAsyncOp = nullptr;
				m_nextScenePath = "";
			}
		}
	}

	void SceneManager::Render()
	{
		// Worldの描画
		m_world.Render(m_context);

		// 遷移エフェクト描画
		if (m_transition)
		{
			m_transition->Render();
		}
	}

	void SceneManager::Render(World* targetWorld)
	{
		if (!targetWorld) return;

		targetWorld->Render(m_context);
	}

	// 同期ロード
	// ============================================================
	void SceneManager::LoadScene(const std::string& filepath, ISceneTransition* transition)
	{
		if (m_transition || m_isAsyncLoading) return;

		m_nextScenePath = filepath;
		m_isAsyncLoading = false;

		// 遷移エフェクトが無ければデフォルトのフェードを使用
		if (transition == nullptr)
			m_transition = std::make_unique<FadeTransition>(0.5f);
		else
			m_transition.reset(transition);

		m_transition->Start();
		Logger::Log("Transition Started: " + filepath);
	}

	// 非同期ロード
	// ============================================================
	std::shared_ptr<AsyncOperation> SceneManager::LoadSceneAsync(const std::string& filepath, ISceneTransition* transition)
	{
		if (m_transition || m_isAsyncLoading) return nullptr;

		m_nextScenePath = filepath;
		m_isAsyncLoading = true;
		m_currentAsyncOp = std::make_shared<AsyncOperation>();
		m_currentAsyncOp->progress = 0.0f;
		m_currentAsyncOp->isDone = false;

		// 遷移セット
		if (transition == nullptr)
			m_transition = std::make_unique<FadeTransition>(0.5f);
		else
			m_transition.reset(transition);

		m_transition->Start();
		Logger::Log("LoadSceneAsync Started: " + filepath);

		return m_currentAsyncOp;
	}

	// 同期ロード用ヘルパー
	void SceneManager::PerformLoad(const std::string& path)
	{
		m_world.clearSystems();
		m_world.clearEntities();
		SceneSerializer::LoadScene(m_world, path);
		m_currentSceneName = path;
	}

	// 非同期ロード完了後のワールド交換
	void SceneManager::SwapWorld()
	{
		if (m_tempWorld)
		{
			// 古いワールドのシステム
			m_world.clearSystems();
			m_world.clearEntities();

			// 中身をムーブ
			m_world = std::move(*m_tempWorld);

			m_tempWorld = nullptr;
			m_currentSceneName = m_nextScenePath;

			Logger::Log("Async Load Completed: " + m_currentSceneName);
		}
	}

}	// namespace Arche