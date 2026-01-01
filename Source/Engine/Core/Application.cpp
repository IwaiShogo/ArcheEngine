/*****************************************************************//**
 * @file	Application.cpp
 * @brief	アプリケーション実装（マルチビューレンダリング対応）
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
#include "Engine/Core/Window/Input.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Audio/AudioManager.h"
#include "Engine/Core/Base/Logger.h"
#include "Engine/Scene/Serializer/SceneSerializer.h"

// Renderer（静的初期化用）
#include "Engine/Renderer/Renderers/PrimitiveRenderer.h"
#include "Engine/Renderer/Renderers/SpriteRenderer.h"
#include "Engine/Renderer/Renderers/ModelRenderer.h"
#include "Engine/Renderer/Renderers/BillboardRenderer.h"
#include "Engine/Renderer/Text/TextRenderer.h"

#include "Engine/EngineLoader.h"
#include "Sandbox/GameLoader.h"

#ifdef _DEBUG
#include "Editor/Core/Editor.h"
#include "Editor/Core/GameCommands.h"
#endif // _DEBUG

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Arche
{
	Application* Application::s_instance = nullptr;

	HMODULE GetCurrentModuleHandle()
	{
		HMODULE hModule = nullptr;
		GetModuleHandleEx(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)GetCurrentModuleHandle, // この関数(Application::Run)のアドレスを使ってDLLを特定
			&hModule);
		return hModule;
	}

	Application::Application(const std::string& title, uint32_t width, uint32_t height)
		: m_title(title), m_width(width), m_height(height)
	{
		if (!s_instance) s_instance = this;

		m_windowClassName = "ArcheEngineWindowClass_" + std::to_string((unsigned long long)this);

		if (!InitializeWindow()) abort();
		if (!InitializeGraphics()) abort();

		// --- サブシステム初期化 ---
		// シーンマネージャー
		new SceneManager();
		// 入力
		Input::Initialize();
		// リソースマネージャー
		ResourceManager::Instance().Initialize(m_device.Get());
		ResourceManager::Instance().LoadManifest("Resources/Game/resources.json");
		// オーディオマネージャー
		AudioManager::Instance().Initialize();
		// FPS制御
		Time::Initialize();
		Time::SetFrameRate(Config::FRAME_RATE);

		// レンダラー静的初期化
		PrimitiveRenderer::Initialize(m_device.Get(), m_context.Get());
		SpriteRenderer::Initialize(m_device.Get(), m_context.Get(), m_width, m_height);
		ModelRenderer::Initialize(m_device.Get(), m_context.Get());
		BillboardRenderer::Initialize(m_device.Get(), m_context.Get());
		TextRenderer::Initialize(m_device.Get(), m_context.Get());

#ifdef _DEBUG
		// --- ImGui & Editor 初期化 ---
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;	// ドッキング有効化
		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(m_hwnd);
		ImGui_ImplDX11_Init(m_device.Get(), m_context.Get());
		m_isImguiInitialized = true;

		// Editor初期化
		Editor::Instance().Initialize();

		// レンダーターゲット作成（初期サイズはウィンドウサイズ）
		m_sceneRT = std::make_unique<RenderTarget>();
		m_sceneRT->Create(m_device.Get(), m_width, m_height);

		m_gameRT = std::make_unique<RenderTarget>();
		m_gameRT->Create(m_device.Get(), m_width, m_height);
#endif // _DEBUG
	}

	Application::~Application()
	{
		Finalize();
	}

	void Application::Finalize()
	{
#ifdef _DEBUG
		if (m_isImguiInitialized)
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}
#endif // _DEBUG
		AudioManager::Instance().Finalize();

		ResourceManager::Instance().Clear();

		// レンダラーの静的リソース開放
		SpriteRenderer::Shutdown();
		BillboardRenderer::Shutdown();
		PrimitiveRenderer::Shutdown();
		ModelRenderer::Shutdown();
		TextRenderer::Shutdown();

		SceneManager* sm = &SceneManager::Instance();
		if (sm) delete sm;

		// ウィンドウ破棄
		if (m_hwnd)
		{
			DestroyWindow(m_hwnd);
			m_hwnd = nullptr;
		}

		// クラス登録解除
		UnregisterClass(m_windowClassName.c_str(), GetCurrentModuleHandle());

		s_instance = nullptr;
	}

	void Application::Run()
	{
		OnInitialize();
		SceneManager::Instance().Initialize();

#ifdef _DEBUG
		Context& ctx = SceneManager::Instance().GetContext();
		GameCommands::RegisterAll(SceneManager::Instance().GetWorld(), ctx);
#endif // _DEBUG

		// シーンロードの分岐
		// 「一時ファイルがあるなら続きから、なければ通常起動」と判断します。
		std::string tempPath = "temp_hotreload.json";
		if (std::filesystem::exists(tempPath))
		{
			// ホットリロード復帰: 続きからロード
			LoadState();

			// 読み込んだら消す（次回通常起動時に誤爆しないように）
			std::filesystem::remove(tempPath);
		}
		else
		{
			// 通常起動: スタートアップシーンのロード
			std::string startScene = "Resources/Game/Scenes/GameScene.json";
			std::ifstream f(startScene);
			if (f.good())
			{
				f.close();
				SceneSerializer::LoadScene(SceneManager::Instance().GetWorld(), startScene);
				Logger::Log("Startup: Loaded: " + startScene);
			}
			else
			{
				Logger::LogWarning("Startup: Scene file not found.");
			}
		}

		// 残留メッセージ
		MSG cleanupMsg = {};
		while (PeekMessage(&cleanupMsg, nullptr, 0, 0, PM_REMOVE))
		{
			if (cleanupMsg.message == WM_QUIT) continue;
			TranslateMessage(&cleanupMsg);
			DispatchMessage(&cleanupMsg);
		}

		MSG msg = {};
		while (msg.message != WM_QUIT && !m_reloadRequested)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				// 1. 更新処理
				Update();

				// 2. 描画処理
				Render();

				// 3. フレームレート調整
				Time::WaitFrame();
			}
		}
	}

	void Application::Update()
	{
		// FPS制御
		Time::Update();
		// 入力
		Input::Update();
		// オーディオ
		AudioManager::Instance().Update();

#ifdef _DEBUG
		// ImGui開始
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
#endif // _DEBUG

		// シーン更新
		SceneManager::Instance().Update();
	}

	// ======================================================================
	// 描画パイプライン（SceneRT -> GameRT -> BackBuffer）
	// ======================================================================
	void Application::Render()
	{
#ifdef _DEBUG
		// ----------------------------------------------------
		// 1. Scene View Pass (エディタカメラ)
		// ----------------------------------------------------
		if (m_sceneRT)
		{
			m_sceneRT->Clear(m_context.Get(), 0.15f, 0.15f, 0.15f, 1.0f);	// グレー背景
			m_sceneRT->Bind(m_context.Get());

			// デバッグカメラ情報をContextにセット
			Context& ctx = SceneManager::Instance().GetContext();

			// エディタカメラを取得
			EditorCamera& cam = Editor::Instance().GetSceneViewPanel().GetCamera();

			// 行列を保存
			XMStoreFloat4x4(&ctx.renderCamera.viewMatrix, cam.GetViewMatrix());
			XMStoreFloat4x4(&ctx.renderCamera.projMatrix, cam.GetProjectionMatrix());
			XMStoreFloat3(&ctx.renderCamera.position, cam.GetPosition());
			ctx.renderCamera.useOverride = true;

			SceneManager::Instance().Render();
		}

		// ----------------------------------------------------
		// 2. Game View Pass（ゲーム内カメラ）
		// ----------------------------------------------------
		if (m_gameRT)
		{
			m_gameRT->Clear(m_context.Get(), 0.0f, 0.0f, 0.0f, 1.0f);	// 黒背景
			m_gameRT->Bind(m_context.Get());

			// ゲームビュー用にContextのデバッグ設定を一時的にOFFにする
			Context& ctx = SceneManager::Instance().GetContext();
			auto backSettings = ctx.debugSettings;	// 設定をバッグアップ

			// ゲームビューではデバッグ表示を無効化
			ctx.debugSettings.showGrid = false;
			ctx.debugSettings.showAxis = false;
			ctx.debugSettings.showColliders = false;
			ctx.debugSettings.showSoundLocation = false;
			ctx.debugSettings.useDebugCamera = false;
			ctx.renderCamera.useOverride = false;

			SceneManager::Instance().Render();

			// 設定を元に戻す
			ctx.debugSettings = backSettings;
		}

		// ----------------------------------------------------
		// 3. ImGui Pass (BackBuffer)
		// ----------------------------------------------------
		// レンダーターゲットをバックバッファに戻す
		m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

		// バックバッファクリア
		static float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
		m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		
		// エディタUI構築 & 描画
		Context& ctx = SceneManager::Instance().GetContext();
		Editor::Instance().Draw(SceneManager::Instance().GetWorld(), ctx);
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
#else
		// ====================================================
		// Release Mode (Game Only)
		// ====================================================
		m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

		float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_context->ClearRenderTargetView(m_renderTargetView.Get(), color);
		m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// ゲーム画面描画
		SceneManager::Instance().Render();
#endif // _DEBUG

		// フリップ
		m_swapChain->Present(Config::VSYNC_ENABLED ? 1 : 0, 0);
	}

#ifdef _DEBUG
	void Application::ResizeSceneRenderTarget(float width, float height)
	{
		if (m_sceneRT && (width > 0 && height > 0))
		{
			// サイズが変わった時だけ再生成などの処理を入れる
			// 現在のRenderTargetクラスの実装に合わせて調整してくだい
			m_sceneRT->Create(m_device.Get(), (uint32_t)width, (uint32_t)height);

			TextRenderer::ClearCache();
		}
	}

	void Application::ResizeGameRenderTarget(float width, float height)
	{
		if (m_gameRT && (width > 0 && height > 0))
		{
			m_gameRT->Create(m_device.Get(), (uint32_t)width, (uint32_t)height);

			TextRenderer::ClearCache();
		}
	}
#endif // _DEBUG

	void Application::SaveState()
	{
		SceneSerializer::SaveScene(SceneManager::Instance().GetWorld(), "temp_hotreload.json");
		Logger::Log("HotReload: State Saved.");
	}

	void Application::LoadState()
	{
		std::string tempPath = "temp_hotreload.json";
		std::ifstream f(tempPath);
		if (f.good())
		{
			f.close();
			// 現在のシーンをクリアしてからロード
			SceneSerializer::LoadScene(SceneManager::Instance().GetWorld(), tempPath);
			Logger::Log("HotReload: State Loaded.");
		}
	}

	// ======================================================================
	// ウィンドウ関連の実装
	// ======================================================================
	bool Application::InitializeWindow()
	{
		HINSTANCE hInstance = GetCurrentModuleHandle();

		WNDCLASS wc = {};
		wc.lpfnWndProc = StaticWndProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = m_windowClassName.c_str();
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		RegisterClass(&wc);

		RECT rc = { 0, 0, (LONG)m_width, (LONG)m_height };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

		// ウィンドウ作成
		m_hwnd = CreateWindowEx(
			0, m_windowClassName.c_str(), m_title.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top,
			nullptr, nullptr, wc.hInstance, nullptr
		);

		if (!m_hwnd) return false;

		ShowWindow(m_hwnd, SW_SHOW);
		return true;
	}

	bool Application::InitializeGraphics()
	{
		// DX11デバイス生成処理（既存コードと同様）
		// ※長くなるため、アップロードされたコードのロジックをここに移植してください
		// 基本的な CreateDeviceAndSwapChain -> RTV/DSV 作成の流れです。

		// 簡易実装（本来は詳細なエラーチェックが必要）
		DXGI_SWAP_CHAIN_DESC scd = {};
		scd.BufferCount = 1;
		scd.BufferDesc.Width = m_width;
		scd.BufferDesc.Height = m_height;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.RefreshRate.Numerator = 60;
		scd.BufferDesc.RefreshRate.Denominator = 1;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.OutputWindow = m_hwnd;
		scd.SampleDesc.Count = 1;
		scd.SampleDesc.Quality = 0;
		scd.Windowed = TRUE;

		D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
		D3D_FEATURE_LEVEL featureLevel;

		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags,
			featureLevels, 1, D3D11_SDK_VERSION, &scd,
			&m_swapChain, &m_device, &featureLevel, &m_context
		);

		if (FAILED(hr)) return false;

		// RTV作成
		ComPtr<ID3D11Texture2D> backBuffer;
		m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
		m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);

		// DSV作成
		D3D11_TEXTURE2D_DESC dsd = {};
		dsd.Width = m_width;
		dsd.Height = m_height;
		dsd.MipLevels = 1;
		dsd.ArraySize = 1;
		dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsd.SampleDesc.Count = 1;
		dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		ComPtr<ID3D11Texture2D> depthBuffer;
		m_device->CreateTexture2D(&dsd, nullptr, &depthBuffer);
		m_device->CreateDepthStencilView(depthBuffer.Get(), nullptr, &m_depthStencilView);

		// ビューポート
		D3D11_VIEWPORT vp = {};
		vp.Width = (float)m_width;
		vp.Height = (float)m_height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_context->RSSetViewports(1, &vp);

		return true;
	}

	LRESULT CALLBACK Application::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		// ImGui用ハンドラ
		if (s_instance && s_instance->m_isImguiInitialized)
		{
			extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
			if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;
		}

		switch (msg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_MOUSEWHEEL:
		{
			float wheelDelta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
			Input::UpdateMouseWheel(wheelDelta);
			return 0;
		}

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

}	// namespace Arche