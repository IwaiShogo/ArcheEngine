/*****************************************************************//**
 * @file	Application.cpp
 * @brief	
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

// ===== インクルード =====
#include "Core/Application.h"
#include "Core/Time.h"
#include "Core/Input.h"
#include "main.h"
#include <string>
#include <stdexcept>

Application::Application(HWND hwnd)
	: m_hwnd(hwnd)
{
}

Application::~Application()
{
	// ImGui 終了処理
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// ComPtrを使用しているため、明示的なReleaseは不要
}

void Application::Initialize()
{
	// 1. スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferCount = 1;
	scd.BufferDesc.Width = Config::SCREEN_WIDTH;
	scd.BufferDesc.Height = Config::SCREEN_HEIGHT;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = Config::FRAME_RATE;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = m_hwnd;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.Windowed = TRUE;

	// 2. デバイスとスワップチェーンの作成
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevel;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		featureLevels,
		1,
		D3D11_SDK_VERSION,
		&scd,
		&m_swapChain,
		&m_device,
		&featureLevel,
		&m_context
	);

	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create D3D11 device");
	}

	// 3. レンダーターゲットビュー（RTV）の作成
	ComPtr<ID3D11Texture2D> backBuffer;
	hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to get back buffer");
	}

	hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create RTV");
	}

	// 4. 深度バッファ（Z-Buffer）の作成
	D3D11_TEXTURE2D_DESC depthBufferDesc = {};
	depthBufferDesc.Width = Config::SCREEN_WIDTH;
	depthBufferDesc.Height = Config::SCREEN_HEIGHT;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// テクスチャ作成
	ComPtr<ID3D11Texture2D> depthStencilBuffer;
	hr = m_device->CreateTexture2D(&depthBufferDesc, nullptr, &depthStencilBuffer);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create Depth Stencil Buffer");
	}

	// ビュー作成
	hr = m_device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, &m_depthStencilView);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create Depth Stencil View");
	}

	// レンダーターゲットをセット
	m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// ビューポート設定
	D3D11_VIEWPORT vp = {};
	vp.Width = static_cast<float>(Config::SCREEN_WIDTH);
	vp.Height = static_cast<float>(Config::SCREEN_HEIGHT);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_context->RSSetViewports(1, &vp);

	// 時間管理の初期化
	Time::Initialize();
	Time::SetFrameRate(Config::FRAME_RATE);

	// 入力
	Input::Initialize();

	// --- ImGui ---
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// ドッキングとマルチビューポートを有効化
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	// キーボード操作有効
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// ウィンドウドッキング有効
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;		// 別ウィンドウ化を有効

	// スタイル調整
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowBorderHoverPadding = 1.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	
	// Win32 / DX11 バインディングの初期化
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(m_device.Get(), m_context.Get());

	m_primitiveRenderer = std::make_unique<PrimitiveRenderer>(m_device.Get(), m_context.Get());
	m_primitiveRenderer->Initialize();

	Context context;
	context.renderer = m_primitiveRenderer.get();

	// シーンマネージャ
	m_sceneManager.SetContext(context);
	m_appContext = context;
	m_sceneManager.SetContext(m_appContext);
	m_sceneManager.Initialize(SceneType::Title);
}

// 全シーン共通のデバッグメニュー
void Application::DrawDebugUI()
{
	// Contextの参照を取得
	Context& ctx = m_sceneManager.GetContext();

	ImGui::Begin("Debug Menu");	// ウィンドウ作成

	ImGui::Checkbox("Show FPS", &ctx.debug.showFps);

	// 1. FPSと時間
	if (ctx.debug.showFps)
	{
		// ------------------------------------------------------------
		// FPS Graph
		// ------------------------------------------------------------
		static float values[90] = {};
		static int values_offset = 0;
		static float refresh_time = 0.0f;

		// 高速更新しすぎると見にくいので少し間引く
		if (refresh_time == 0.0f) refresh_time = ImGui::GetTime();
		while (refresh_time < ImGui::GetTime())
		{
			values[values_offset] = ImGui::GetIO().Framerate;
			values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
			refresh_time += 1.0f / 60.0f;
		}
		
		// グラフ描画
		// (ラベル, 配列, 数, オフセット, オーバーレイ文字, 最小Y, 最大Y, サイズ）
		ImGui::PlotLines("FPS", values, IM_ARRAYSIZE(values), values_offset, nullptr, 0.0f, 200.0f, ImVec2(0, 80));
		ImGui::Text("Avg: %.1f", ImGui::GetIO().Framerate);

		ImGui::Separator();

		ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
	}
	ImGui::Text("Total Time: %.2f s", Time::TotalTime());
	ImGui::Separator();

	// 2. 表示切替
	if (ImGui::CollapsingHeader("Display Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Separator();
		ImGui::Checkbox("Show Grid", &ctx.debug.showGrid);
		ImGui::Checkbox("Show Axis", &ctx.debug.showAxis);

		ImGui::Separator();
		ImGui::Checkbox("Debug Camera Mode", &ctx.debug.useDebugCamera);
		ImGui::Checkbox("Show Colliders", &ctx.debug.showColliders);
		// コライダー表示中のみ有効なサブ設定
		if (ctx.debug.showColliders)
		{
			ImGui::Indent();
			if (ImGui::RadioButton("Wireframe", ctx.debug.wireframeMode)) ctx.debug.wireframeMode = true;
			ImGui::SameLine();
			if (ImGui::RadioButton("Solid", !ctx.debug.wireframeMode)) ctx.debug.wireframeMode = false;
			ImGui::Unindent();
		}
	}

	// 入力デバッグ情報の表示
	if (ImGui::CollapsingHeader("Input Visualizer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 1. 接続状態
		bool connected = Input::IsControllerConnected();
		ImGui::Text("Controller: %s", connected ? "Connected" : "Disconnected");

		if (connected)
		{
			// ------------------------------------------------------------
			// スティック描画用ヘルパー関数
			// ------------------------------------------------------------
			auto DrawStick = [](const char* label, float x, float y)
				{
					ImGui::BeginGroup();	// グループ化して横並びにしやすくする
					ImGui::Text("%s", label);

					// 枠
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					ImVec2 p = ImGui::GetCursorScreenPos();
					float size = 80.0f;

					drawList->AddRect(p, ImVec2(p.x + size, p.y + size), IM_COL32(255, 255, 255, 255));

					// 十字線
					ImVec2 center(p.x + size * 0.5f, p.y + size * 0.5f);
					drawList->AddLine(ImVec2(center.x, p.y), ImVec2(center.x, p.y + size), IM_COL32(100, 100, 100, 100));
					drawList->AddLine(ImVec2(p.x, center.y), ImVec2(p.x + size, center.y), IM_COL32(100, 100, 100, 100));

					// 現在の位置の点（赤色）
					float dotX = center.x + (x * (size * 0.5f));
					float dotY = center.y - (y * (size * 0.5f));
					drawList->AddCircleFilled(ImVec2(dotX, dotY), 4.0f, IM_COL32(255, 0, 0, 255));

					// スペース確保
					ImGui::Dummy(ImVec2(size, size));

					// 数値表示
					ImGui::Text("X: % .2f", x);
					ImGui::Text("Y: % .2f", y);
					ImGui::EndGroup();
				};
			
			// ------------------------------------------------------------
			// 描画実行
			// ------------------------------------------------------------
			float lx = Input::GetAxis(Axis::Horizontal);
			float ly = Input::GetAxis(Axis::Vertical);
			float rx = Input::GetAxis(Axis::RightHorizontal);
			float ry = Input::GetAxis(Axis::RightVertical);

			DrawStick("L Stick", lx, ly);
			ImGui::SameLine(0, 20);	// 横に並べる
			DrawStick("R Stick", rx, ry);

			ImGui::Separator();

			// 3. ボタン入力の可視化
			ImGui::Text("Buttons:");
			// ヘルパーラムダ式
			auto DrawBtn = [&](const char* label, Button btn)
				{
					if (Input::GetButton(btn)) ImGui::TextColored(ImVec4(0, 1, 0, 1), "[%s]", label);
					else ImGui::TextDisabled("[%s]", label);
					ImGui::SameLine();
				};

			DrawBtn("A", Button::A);
			DrawBtn("B", Button::B);
			DrawBtn("X", Button::X);
			DrawBtn("Y", Button::Y);
			ImGui::NewLine();
			DrawBtn("LB", Button::LShoulder);
			DrawBtn("RB", Button::RShoulder);
			DrawBtn("START", Button::Start);
			ImGui::NewLine();
		}
	}

	// 3. ゲーム進行制御
	if (ImGui::CollapsingHeader("Game Control", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// タイムスケール操作
		ImGui::Text("Time Scale");
		ImGui::SliderFloat("##TimeScale", &Time::timeScale, 0.0f, 15.0f, "%.1fx");

		// --- 一時停止 / 再開 ---
		if (Time::isPaused)
		{
			// 停止中なので「再開」ボタン
			if (ImGui::Button("Resume"))
			{
				Time::isPaused = false;
			}
			ImGui::SameLine();

			// --- コマ送り（停止中のみ表示）---
			if (ImGui::Button("Step Frame (+1F)"))
			{
				Time::StepFrame();
			}
		}
		else
		{
			// 動作中なので「一時停止」ボタン
			if (ImGui::Button("Pause"))
			{
				Time::isPaused = true;
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset Speed"))
		{
			Time::timeScale = 1.0f;
		}

		// リスタート
		if (ImGui::Button("Restart Scene", ImVec2(-1, 0)))
		{
			SceneManager::ChangeScene(m_sceneManager.GetCurrentType());
		}
	}

	// 4. シーン遷移
	if (ImGui::CollapsingHeader("Scene Transition"))
	{
		if (ImGui::Button("Title", ImVec2(100, 0)))
		{
			SceneManager::ChangeScene(SceneType::Title);
		}
		ImGui::SameLine();
		if (ImGui::Button("Game", ImVec2(100, 0)))
		{
			SceneManager::ChangeScene(SceneType::Game);
		}
	}

	ImGui::End();

	if (ctx.debug.showDemoWindow) ImGui::ShowDemoWindow();
}

void Application::Update()
{
	// 毎フレーム時間を更新
	Time::Update();

	// 入力
	Input::Update();

	// --- ImGui ---
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// デバッグUI
	DrawDebugUI();

	// ログウィンドウの描画
	Logger::Draw("Debug Logger");

	// シーン更新
	m_sceneManager.Update();
}

void Application::Render()
{
	// 1. レンダーターゲットをセット
	m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// 2. ビューポート設定
	D3D11_VIEWPORT vp = {};
	vp.Width = static_cast<float>(Config::SCREEN_WIDTH);
	vp.Height = static_cast<float>(Config::SCREEN_HEIGHT);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_context->RSSetViewports(1, &vp);

	float color[] = { 0.1f, 0.1f, 0.3f, 1.0f };
	m_context->ClearRenderTargetView(m_renderTargetView.Get(), color);

	// 深度バッファのクリア
	if (m_depthStencilView)
	{
		m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	// シーン描画
	m_sceneManager.Render();

	// --- ImGui ---
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	m_swapChain->Present(Config::VSYNC_ENABLED ? 1: 0, 0);
}

void Application::Run()
{
	// 1. 更新と描画
	Update();
	Render();

	// 2. フレームレート調整（待機）
	// Timeクラスが自動で残りの時間を計算して待ってくれます
	Time::WaitFrame();
}