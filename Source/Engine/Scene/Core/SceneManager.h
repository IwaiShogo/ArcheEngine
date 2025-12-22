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
#include "Engine/pch.h"
#include "Engine/Scene/Core/IScene.h"

/**
 * @class	SceneManager
 * @brief	シーンの登録・切り替え・管理を行う（汎用化済み）
 */
class SceneManager
{
public:
	SceneManager();		// コンストラクタで自身を登録
	~SceneManager();	// デストラクタで解除

	// @brief	どこからでもアクセス可能
	static SceneManager& Instance();
	
	// @brief	初期化
	void Initialize();

	// @brief	更新
	void Update();

	// @brief	描画
	void Render();

	// @brief	Contextセット
	void SetContext(const Context& context) { m_context = context; }
	Context& GetContext() { return m_context; }

	// --- シーン操作 ---
	/**
	 * @brief	シーンを登録する（テンプレート）
	 * @tparam	T		ISceneを継承したシーンクラス
	 * @param	name	シーン名
	 */
	template<typename T>
	void RegisterScene(const std::string& name)
	{
		// ファクトリ関数（その場でnewする関数）を保存しておく
		m_factories[name] = []() {return std::make_shared<T>(); };
	}

	// @brief	シーン切り替え予約
	void ChangeScene(const std::string& name);

	// 現在のシーン情報を取得
	World& GetWorld();
	const std::string& GetCurrentSceneName() const { return m_currentSceneName; }

private:
	// 実際の切り替え処理
	void ProcessSceneChange();

private:
	// 自身の静的ポインタ
	static SceneManager* s_instance;

	// シーン生成関数のマップ（名前 -> 生成関数）
	std::unordered_map<std::string, std::function<std::shared_ptr<IScene>()>> m_factories;

	// 現在のシーン
	std::shared_ptr<IScene> m_currentScene;
	std::string m_currentSceneName = "";

	// 次のシーンへの予約
	std::string m_nextSceneRequest = "";

	// Context
	Context m_context;
};

#endif // !___SCENE_MANAGER_H___
