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
#include "Engine/pch.h"
#include "Engine/Core/Context.h"

// ===== 前方宣言 =====
class World;

/**
 * @class	IScene
 * @brief	シーンの基底クラス
 */
class IScene
{
public:
	virtual ~IScene() = default;

	// @brief	初期化（リソース読み込み等）
	virtual void Initialize() {}

	// @brief	終了処理（リソース解放等）
	virtual void Finalize() {}

	// @brief	更新
	virtual void Update() {}

	// @brief	描画
	virtual void Render() {}

	// @brief	ECSワールドの取得
	virtual World& GetWorld() = 0;

	// @brief	Context設定（エディタ連携用）
	virtual void Setup(Context* context) { m_context = context; }

protected:
	Context*	m_context = nullptr;
};

#endif // !___ISCENE_H___
