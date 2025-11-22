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
#include "main.h"
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

	// レンダーターゲットをセット
	m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

	// ビューポート設定
	D3D11_VIEWPORT vp = {};
	vp.Width = static_cast<float>(Config::SCREEN_WIDTH);
	vp.Height = static_cast<float>(Config::SCREEN_HEIGHT);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_context->RSSetViewports(1, &vp);

	// --- ImGui ---
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	// キーボード操作有効化したい場合

	ImGui::StyleColorsDark();	// ダークテーマ適用
	
	// Win32 / DX11 バインディングの初期化
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(m_device.Get(), m_context.Get());

	// シーンマネージャ
	m_sceneManager.Initialize(SceneType::Title);
}

void Application::Update()
{
	// --- ImGui ---
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// デモウィンドウの表示（導入確認用）
	ImGui::ShowDemoWindow();

	// シーン更新
	m_sceneManager.Update();
}

void Application::Render()
{
	float color[] = { 0.1f, 0.1f, 0.3f, 1.0f };
	m_context->ClearRenderTargetView(m_renderTargetView.Get(), color);

	// シーン描画
	m_sceneManager.Render();

	// --- ImGui ---
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	m_swapChain->Present(Config::VSYNC_ENABLED ? 1: 0, 0);
}

void Application::Run()
{
	Update();
	Render();
}