/*****************************************************************//**
 * @file	Context.h
 * @brief	共有データをまとめる構造体
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

#ifndef ___CONTEXT_H___
#define ___CONTEXT_H___

// ===== 前方宣言 =====
class PrimitiveRenderer;
// class InputManager;
// class AudioManager;

//  デバッグ設定（全シーン共有）
struct DebugSettings
{
	bool showFps = true;			// FPS表示
	bool showColliders = true;		// 当たり判定（Box）表示
	bool showDemoWindow = false;	// ImGuiデモ画面
	bool pauseGame = false;			// 一時停止
	bool wireframeMode = true;		// true: 線、false: 面
	bool showGrid = true;			// グリッド線
	bool showAxis = true;			// 座標軸
	bool useDebugCamera = false;	// デバッグカメラ
};

struct Context
{
	PrimitiveRenderer* renderer = nullptr;
	DebugSettings debug;
};

#endif // !___CONTEXT_H___