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
#include "Engine/ECS/ECS.h"
#include "Game/Components/Components.h"
#include "Engine/Core/Context.h"
#include "imgui.h"

// Systems
#include "Game/Systems/Graphics/RenderSystem.h"
#include "Game/Systems/Physics/CollisionSystem.h"
#include "Game/Systems/Logic/InputSystem.h"
#include "Game/Systems/Graphics/SpriteRenderSystem.h"
#include "Game/Systems/Graphics/ModelRenderSystem.h"
#include "Game/Systems/Audio/AudioSystem.h"
#include "Game/Systems/Logic/LifetimeSystem.h"
#include "Game/Systems/Logic/HierarchySystem.h"
#include "Game/Systems/Graphics/BillboardSystem.h"

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

	World& GetWorld() { return m_world; }

protected:
	World		m_world;
	Context*	m_context = nullptr;
};

#endif // !___ISCENE_H___