/*****************************************************************//**
 * @file	main.h
 * @brief	定数の外部定義
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

#ifndef ___MAIN_H___
#define ___MAIN_H___

namespace Config
{
	// ウィンドウ設定
	static const int SCREEN_WIDTH = 1440;
	static const int SCREEN_HEIGHT = 810;
	static const char* WINDOW_TITLE = "Time Trace Puzzle (DX11 + ECS)";

	// レンダリング設定
	static const unsigned int FRAME_RATE = 60;	// 目標FPS（リフレッシュレートの分子）
	static const bool VSYNC_ENABLED = false;		// 垂直同期（true: 60fps固定、false: 無制限）
}

#endif // !___MAIN_H___