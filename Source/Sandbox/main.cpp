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

// Scenes
#include "SandBox/Scenes/SceneTitle.h"
#include "SandBox/Scenes/SceneGame.h"

using namespace Arche;

// アプリケーション設定
class SandboxApp
	: public Arche::Application
{
public:
	SandboxApp()
		: Arche::Application("Arche Engine Project", Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT) {}

	~SandboxApp() = default;

	// 初期化時にシーンを登録する
	void OnInitialize() override
	{
		auto& sm = SceneManager::Instance();
		
		sm.Initialize();

		sm.RegisterScene<SceneTitle>("Title");
		sm.RegisterScene<SceneGame>("Game");

		// 最初のシーンへ
		sm.ChangeScene("Title");

		Logger::Log("Sandbox Initialized.");
	}
};

// エンジン側にアプリケーションの実体を渡す関数
Arche::Application* Arche::CreateApplication()
{
	return new SandboxApp();
}

// Windows エントリーポイント
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// アプリ生成
	auto app = Arche::CreateApplication();

	// 実行
	app->Run();

	// 終了
	delete app;
	return 0;
}