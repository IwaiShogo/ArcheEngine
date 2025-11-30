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
class SpriteRenderer;
class ModelRenderer;
class BillboardRenderer;
// class InputManager;
// class AudioManager;

//  デバッグ設定（全シーン共有）
struct DebugSettings
{
	// ビルド構成によってデフォルト値を切り替える定数
#ifdef _DEBUG
	static constexpr bool DefaultOn = true;	 // Debug時は True
#else
	static constexpr bool DefaultOn = false; // Release時は False
#endif

	// 各フラグを定数で初期化
	bool showFps = DefaultOn;
	bool showGrid = DefaultOn;
	bool showAxis = DefaultOn;
	bool showColliders = DefaultOn;
	bool showSoundLocation = DefaultOn;
	bool enableMousePicking = DefaultOn;
	bool useDebugCamera = DefaultOn;

	// 以下の設定はDebug時でも最初はOFFにしておくのが一般的（お好みで DefaultOn にしてもOK）
	bool wireframeMode = false;
	bool showDemoWindow = false;
	bool pauseGame = false;
};

struct Context
{
	PrimitiveRenderer*	renderer = nullptr; 
	SpriteRenderer*		spriteRenderer = nullptr;
	ModelRenderer*		modelRenderer = nullptr;
	BillboardRenderer*	billboardRenderer = nullptr;

	DebugSettings debug;
};

#endif // !___CONTEXT_H___