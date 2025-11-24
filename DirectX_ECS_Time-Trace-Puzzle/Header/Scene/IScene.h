/*****************************************************************//**
 * @file	IScene.h
 * @brief	全てのシーンの親となるインターフェース
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

#ifndef ___ISCENE_H___
#define ___ISCENE_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include "Components/Components.h"
#include "Core/Context.h"
#include "imgui.h"

#include "Systems/RenderSystem.h"
#include "Systems/MovementSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/InputSystem.h"

/**
 * @enum	SceneType
 * @brief	シーンの種類
 */
enum class SceneType
{
	Title,			// タイトル
	StageSelect,	// ステージセレクト
	Game,			// インゲーム
	Result,			// リザルト
	None			// 終了時や無効な状態
};

/**
 * @class	IScene
 * @brief	全てのシーンの親クラス
 */
class IScene
{
public:
	virtual ~IScene() = default;

	// セットアップ：シーン生成直後にマネージャから呼ばれる
	void Setup(Context* context)
	{
		m_context = context;
	}

	// 初期化（リソース読み込み等）
	virtual void Initialize()
	{
		if (m_context->renderer)
		{
			m_world.registerSystem<RenderSystem>(m_context->renderer);
		}
	}

	// 終了処理（リソース解放等）
	virtual void Finalize() = 0;

	// 更新（次に行くべきシーンを返す。変更なしなら現在のシーンタイプを返す）
	virtual void Update()
	{
		m_world.Tick();
	}

	// 描画
	virtual void Render()
	{
		m_world.Render(*m_context);
	}

	// ImGuiデバッグ表示用
	virtual void OnInspector() {}

protected:
	World m_world;
	Context* m_context = nullptr;
};

#endif // !___ISCENE_H___