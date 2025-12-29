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

// Renderer（静的初期化用）
#include "Engine/Renderer/Renderers/PrimitiveRenderer.h"
#include "Engine/Renderer/Renderers/SpriteRenderer.h"
#include "Engine/Renderer/Renderers/ModelRenderer.h"
#include "Engine/Renderer/Renderers/BillboardRenderer.h"
#include "Engine/Renderer/Text/TextRenderer.h"

#ifdef _DEBUG
#include "Editor/Core/Editor.h"
#endif // _DEBUG

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Arche
{
	Application* Application::s_instance = nullptr;

	Application::Application(const std::string& title, uint32_t width, uint32_t height)
		: m_title(title), m_width(width), m_height(height)
	{
		if (!s_instance) s_instance = this;

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

		SceneManager* sm = &SceneManager::Instance();
		if (sm) delete sm;

		s_instance = nullptr;
	}


	void Application::Run()
	{
		OnInitialize();
		SceneManager::Instance().Initialize();

		MSG msg = {};
		while (msg.message != WM_QUIT)
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

//	void Application::Initialize()
//	{
//		// 1. スワップチェーンの設定
//		DXGI_SWAP_CHAIN_DESC scd = {};
//		scd.BufferCount = 1;
//		scd.BufferDesc.Width = Config::SCREEN_WIDTH;
//		scd.BufferDesc.Height = Config::SCREEN_HEIGHT;
//		scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
//		scd.BufferDesc.RefreshRate.Numerator = Config::FRAME_RATE;
//		scd.BufferDesc.RefreshRate.Denominator = 1;
//		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//		scd.OutputWindow = m_hwnd;
//		scd.SampleDesc.Count = 1;
//		scd.SampleDesc.Quality = 0;
//		scd.Windowed = TRUE;
//
//		// フラグ設定
//		UINT creationFlags = 0;
//#ifdef _DEBUG
//		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif // _DEBUG
//		creationFlags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
//
//		// 2. デバイスとスワップチェーンの作成
//		D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
//		D3D_FEATURE_LEVEL featureLevel;
//
//		HRESULT hr = D3D11CreateDeviceAndSwapChain(
//			nullptr,
//			D3D_DRIVER_TYPE_HARDWARE,
//			nullptr,
//			creationFlags,
//			featureLevels,
//			1,
//			D3D11_SDK_VERSION,
//			&scd,
//			&m_swapChain,
//			&m_device,
//			&featureLevel,
//			&m_context
//		);
//
//		if (FAILED(hr))
//		{
//			throw std::runtime_error("Failed to create D3D11 device");
//		}
//
//		// 3. レンダーターゲットビュー（RTV）の作成
//		ComPtr<ID3D11Texture2D> backBuffer;
//		hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
//		if (FAILED(hr))
//		{
//			throw std::runtime_error("Failed to get back buffer");
//		}
//
//		hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
//		if (FAILED(hr))
//		{
//			throw std::runtime_error("Failed to create RTV");
//		}
//
//		// 4. 深度バッファ（Z-Buffer）の作成
//		D3D11_TEXTURE2D_DESC depthBufferDesc = {};
//		depthBufferDesc.Width = Config::SCREEN_WIDTH;
//		depthBufferDesc.Height = Config::SCREEN_HEIGHT;
//		depthBufferDesc.MipLevels = 1;
//		depthBufferDesc.ArraySize = 1;
//		depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//		depthBufferDesc.SampleDesc.Count = 1;
//		depthBufferDesc.SampleDesc.Quality = 0;
//		depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
//		depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
//		depthBufferDesc.CPUAccessFlags = 0;
//		depthBufferDesc.MiscFlags = 0;
//
//		// テクスチャ作成
//		ComPtr<ID3D11Texture2D> depthStencilBuffer;
//		hr = m_device->CreateTexture2D(&depthBufferDesc, nullptr, &depthStencilBuffer);
//		if (FAILED(hr))
//		{
//			throw std::runtime_error("Failed to create Depth Stencil Buffer");
//		}
//
//		// ビュー作成
//		hr = m_device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, &m_depthStencilView);
//		if (FAILED(hr))
//		{
//			throw std::runtime_error("Failed to create Depth Stencil View");
//		}
//
//		// レンダーターゲットをセット
//		m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
//
//		// ビューポート設定
//		D3D11_VIEWPORT vp = {};
//		vp.Width = static_cast<float>(Config::SCREEN_WIDTH);
//		vp.Height = static_cast<float>(Config::SCREEN_HEIGHT);
//		vp.MinDepth = 0.0f;
//		vp.MaxDepth = 1.0f;
//		vp.TopLeftX = 0;
//		vp.TopLeftY = 0;
//		m_context->RSSetViewports(1, &vp);
//
//		// 時間管理の初期化
//		Time::Initialize();
//		Time::SetFrameRate(Config::FRAME_RATE);
//
//		// 入力
//		Input::Initialize();
//
//		FontManager::Instance().Initialize();
//
//#ifdef _DEBUG
//		// --- ImGui ---
//		IMGUI_CHECKVERSION();
//		ImGui::CreateContext();
//		ImGuiIO& io = ImGui::GetIO(); (void)io;
//
//		// ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼
//		// 日本語フォントの読み込み
//		// ------------------------------------------------------------
//		// Windows標準の「メイリオ」などを読み込む
//		static const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesJapanese();
//
//		// フォントサイズ 18.0f で読み込み
//		ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\meiryo.ttc", 18.0f, nullptr, glyphRanges);
//
//		// もし読み込みに失敗した場合の保険
//		if (font == nullptr)
//		{
//			io.Fonts->AddFontDefault();
//		}
//		// ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲
//
//		// ドッキングとマルチビューポートを有効化
//		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	// キーボード操作有効
//		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// ウィンドウドッキング有効
//		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		// 別ウィンドウ化を有効
//
//		// スタイル調整
//		ImGui::StyleColorsDark();
//		ImGuiStyle& style = ImGui::GetStyle();
//
//		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
//		{
//			style.WindowBorderHoverPadding = 1.0f;
//			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
//		}
//
//		// Win32 / DX11 バインディングの初期化
//		ImGui_ImplWin32_Init(m_hwnd);
//		ImGui_ImplDX11_Init(m_device.Get(), m_context.Get());
//
//		// エディタ用の画面サイズで初期化
//		m_sceneRT = std::make_unique<RenderTarget>(m_device.Get(), Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
//		m_gameRT = std::make_unique<RenderTarget>(m_device.Get(), Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
//#endif // _DEBUG
//
//		// マネージャー
//		ResourceManager::Instance().Initialize(m_device.Get());
//		AudioManager::Instance().Initialize();	// オーディオ初期化
//		ResourceManager::Instance().LoadManifest("Resources/Game/resources.json");
//		ResourceManager::Instance().LoadAll();
//
//		// 3D描画作成
//		m_primitiveRenderer = std::make_unique<PrimitiveRenderer>(m_device.Get(), m_context.Get());
//		m_primitiveRenderer->Initialize();
//
//		// 2D描画作成
//		m_spriteRenderer = std::make_unique<SpriteRenderer>(m_device.Get(), m_context.Get());
//		m_spriteRenderer->Initialize();
//
//		// モデル描画作成
//		m_modelRenderer = std::make_unique<ModelRenderer>(m_device.Get(), m_context.Get());
//		m_modelRenderer->Initialize();
//
//		// ビルボードレンダラー作成
//		m_billboardRenderer = std::make_unique<BillboardRenderer>(m_device.Get(), m_context.Get());
//		m_billboardRenderer->Initialize();
//
//		// テキストレンダラー
//		m_textRenderer = std::make_unique<TextRenderer>(m_device.Get(), m_context.Get());
//
//		Context context;
//		context.renderer = m_primitiveRenderer.get();
//		context.spriteRenderer = m_spriteRenderer.get();
//		context.modelRenderer = m_modelRenderer.get();
//		context.billboardRenderer = m_billboardRenderer.get();
//		context.device = m_device.Get();
//		context.context = m_context.Get();
//
//		// サムネイル生成器の初期化
//		ThumbnailGenerator::Instance().Initialize(m_device.Get(), m_context.Get(), m_modelRenderer.get());
//		// 全プレファブのサムネイルを作る
//		ThumbnailGenerator::Instance().GenerateAll("Resources/Game/Prefabs");
//
//		// シーンマネージャ
//		SceneManager::Instance().SetContext(context);
//		m_appContext = context;
//		SceneManager::Instance().SetContext(m_appContext);
//		SceneManager::Instance().Initialize();
//	}

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

	// ======================================================================
	// ウィンドウ関連の実装
	// ======================================================================
	bool Application::InitializeWindow()
	{
		WNDCLASS wc = {};
		wc.lpfnWndProc = StaticWndProc;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.lpszClassName = "ArcheEngineWindowClass";
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		RegisterClass(&wc);

		RECT rc = { 0, 0, (LONG)m_width, (LONG)m_height };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

		// ウィンドウ作成
		m_hwnd = CreateWindowEx(
			0, "ArcheEngineWindowClass", m_title.c_str(),
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
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;

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