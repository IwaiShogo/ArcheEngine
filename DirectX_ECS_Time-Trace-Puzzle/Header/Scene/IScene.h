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

	// 初期化（リソース読み込み等）
	virtual void Initialize() = 0;

	// 終了処理（リソース解放等）
	virtual void Finalize() = 0;

	// 更新（次に行くべきシーンを返す。変更なしなら現在のシーンタイプを返す）
	virtual SceneType Update() = 0;

	// 描画
	virtual void Render() = 0;

	// ImGuiデバッグ表示用
	virtual void OnInspector() {}
};

#endif // !___ISCENE_H___