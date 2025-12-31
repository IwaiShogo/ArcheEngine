/*****************************************************************//**
 * @file	main.cpp
 * @brief	サンドボックス（ゲーム）のエントリーポイント
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Base/Logger.h"


namespace Arche
{
	// エンジン側にアプリケーションの実体を渡す関数
	Application* CreateApplication()
	{
		return new Application("Arche Engine", Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
	}
}

// Windows エントリーポイント
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// メモリリークチェック
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // _DEBUG

	// アプリ生成
	auto app = Arche::CreateApplication();

	// 実行
	app->Run();

	// 終了
	delete app;
	return 0;
}