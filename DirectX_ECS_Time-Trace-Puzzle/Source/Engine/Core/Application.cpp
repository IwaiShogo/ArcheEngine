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
#include "Engine/pch.h"
#include "Engine/Core/Application.h"
#include "Engine/Config.h"
#include "Engine/Core/Time.h"
#include "Engine/Core/Input.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Audio/AudioManager.h"
#include "Engine/Editor/Core/Editor.h"
#include "Engine/Editor/Tools/ThumbnailGenerator.h"

#include "Engine/Components/Components.h"
#include "Engine/Physics/CollisionSystem.h"

Application::Application(HWND hwnd)
	: m_hwnd(hwnd)
{
}

Application::~Application()
{
#ifdef _DEBUG
	// ImGui 終了処理
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // _DEBUG

	// オーディオ
	AudioManager::Instance().Finalize();
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

#ifdef _DEBUG
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

	// エディタ用の画面サイズで初期化
	m_sceneRT = std::make_unique<RenderTarget>(m_device.Get(), Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
	m_gameRT = std::make_unique<RenderTarget>(m_device.Get(), Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
#endif // _DEBUG

	// マネージャー
	ResourceManager::Instance().Initialize(m_device.Get());
	AudioManager::Instance().Initialize();	// オーディオ初期化
	ResourceManager::Instance().LoadManifest("Resources/resources.json");
	ResourceManager::Instance().LoadAll();

	// 3D描画作成
	m_primitiveRenderer = std::make_unique<PrimitiveRenderer>(m_device.Get(), m_context.Get());
	m_primitiveRenderer->Initialize();

	// 2D描画作成
	m_spriteRenderer = std::make_unique<SpriteRenderer>(m_device.Get(), m_context.Get());
	m_spriteRenderer->Initialize();

	// モデル描画作成
	m_modelRenderer = std::make_unique<ModelRenderer>(m_device.Get(), m_context.Get());
	m_modelRenderer->Initialize();

	// ビルボードレンダラー作成
	m_billboardRenderer = std::make_unique<BillboardRenderer>(m_device.Get(), m_context.Get());
	m_billboardRenderer->Initialize();

	Context context;
	context.renderer = m_primitiveRenderer.get();
	context.spriteRenderer = m_spriteRenderer.get();
	context.modelRenderer = m_modelRenderer.get();
	context.billboardRenderer = m_billboardRenderer.get();

	// サムネイル生成器の初期化
	ThumbnailGenerator::Instance().Initialize(m_device.Get(), m_context.Get(), m_modelRenderer.get());
	// 全プレファブのサムネイルを作る
	ThumbnailGenerator::Instance().GenerateAll("Resources/Prefabs");

	// シーンマネージャ
	m_sceneManager.SetContext(context);
	m_appContext = context;
	m_sceneManager.SetContext(m_appContext);
	m_sceneManager.Initialize();
}

void Application::Update()
{
	// 毎フレーム時間を更新
	Time::Update();

	// 入力
	Input::Update();
	// オーディオ
	AudioManager::Instance().Update();

	// シーン更新
	m_sceneManager.SetContext(m_appContext);
	m_sceneManager.Update();
}

void Application::Render()
{
#ifdef _DEBUG
	// ====================================================
	// Debug (Editor) Mode
	// ====================================================

	// ----------------------------------------------------
	// 1. ImGui フレーム開始 (一番最初！)
	// ----------------------------------------------------
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

	if (m_sceneWindowSize.y > 0)
	{
		float aspect = m_sceneWindowSize.x / m_sceneWindowSize.y;
		auto& reg = m_sceneManager.GetWorld().getRegistry();

		reg.view<Tag, Camera>().each([&](Entity e, Tag& tag, Camera& cam)
			{
				if (tag.name == "MainCamera")
				{
					cam.aspect = aspect;
				}
			});

		m_sceneRT->Resize(m_device.Get(), (int)m_sceneWindowSize.x, (int)m_sceneWindowSize.y);
	}

	// ----------------------------------------------------
	// 2. Scene View (RT) への描画
	// ----------------------------------------------------
	m_sceneRT->Activate(m_context.Get(), m_depthStencilView.Get());
	m_sceneRT->Clear(m_context.Get(), 0.2f, 0.2f, 0.2f, 1.0f);
	m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// エディタ用の設定（全てON）
	{
		Context sceneCtx = m_appContext;	// コピー作成
		sceneCtx.debug.useDebugCamera = true;
		sceneCtx.debug.showGrid = true;
		sceneCtx.debug.showAxis = true;
		sceneCtx.debug.showColliders = true;
		sceneCtx.debug.showSoundLocation = true;

		// シーンマネージャに一時的にセットして描画
		m_sceneManager.SetContext(sceneCtx);
		m_sceneManager.Render();
	}

	// ----------------------------------------------------
	// 3. Game View (RT) への描画
	// ----------------------------------------------------
	m_gameRT->Activate(m_context.Get(), m_depthStencilView.Get());
	m_gameRT->Clear(m_context.Get(), 0.0f, 0.0f, 0.0f, 1.0f);
	m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ゲーム用の設定（デバッグ系を全て強制OFF）
	{
		Context gameCtx = m_appContext;	// コピー作成
		gameCtx.debug.useDebugCamera = false;
		gameCtx.debug.showGrid = false;
		gameCtx.debug.showAxis = false;
		gameCtx.debug.showColliders = false;
		gameCtx.debug.showSoundLocation = false;
		gameCtx.debug.wireframeMode = false;

		// シーンマネージャにセットして描画
		m_sceneManager.SetContext(gameCtx);
		m_sceneManager.Render();
	}

	// 設定を元に戻す（Editor表示用）
	m_sceneManager.SetContext(m_appContext);

	// ----------------------------------------------------
	// 4. バックバッファに戻して ImGui 描画
	// ----------------------------------------------------
	m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

	// Scene Window (RT画像を表示)
	ImGui::Begin("Scene");
	{
		// 今のウィンドウサイズを取得
		ImVec2 size = ImGui::GetContentRegionAvail();
		m_sceneWindowSize = size;

		// レンダーターゲットのリサイズと描画
		ImGui::Image(m_sceneRT->GetID(), size);

		// 画像の左座標を取得
		ImVec2 imageMin = ImGui::GetItemRectMin();

		// カメラ行列の行列
		XMMATRIX view = XMMatrixIdentity();
		XMMATRIX proj = XMMatrixIdentity();
		bool cameraFound = false;

		World& world = m_sceneManager.GetWorld();
 		m_sceneManager.GetWorld().getRegistry().view<Tag, Camera, Transform>().each([&](Entity e, Tag& tag, Camera& cam, Transform& t) {
			if (!cameraFound && tag.name == "MainCamera")
			{
				XMVECTOR eye = XMLoadFloat3(&t.position);
				XMMATRIX rot = XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, 0.0f);
				XMVECTOR look = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rot);
				XMVECTOR up = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rot);

				// ギズモ用に RH (右手系) で計算！
				view = XMMatrixLookToLH(eye, look, up);
				proj = XMMatrixPerspectiveFovLH(cam.fov, cam.aspect, cam.nearZ, cam.farZ);

				cameraFound = true;
			}
			});

	if (cameraFound)
	{
		Editor::Instance().DrawGizmo(world, view, proj, imageMin.x, imageMin.y, size.x, size.y);
	}

		if (!ImGuizmo::IsUsing() && ImGui::IsItemClicked(ImGuiMouseButton_Left) && m_sceneManager.GetContext().debug.enableMousePicking)
		{
			// 1. マウスの相対座標を計算 (画像左上からの位置)
			ImVec2 mousePos = ImGui::GetMousePos();		 // マウスの絶対座標
			ImVec2 imageMin = ImGui::GetItemRectMin();	 // 画像の左上絶対座標

			float x = mousePos.x - imageMin.x;
			float y = mousePos.y - imageMin.y;

			// 2. NDC座標 (-1.0 ~ 1.0) に変換
			// Y軸は下向きがプラスなので、3D空間(上向きプラス)に合わせて反転
			float ndcX = (x / size.x) * 2.0f - 1.0f;
			float ndcY = ((y / size.y) * 2.0f - 1.0f) * -1.0f;
			
			// 3. カメラ行列の取得 (MainCameraを探す)
			XMMATRIX view = XMMatrixIdentity();
			XMMATRIX proj = XMMatrixIdentity();
			XMVECTOR camPos = XMVectorZero();
			bool cameraFound = false;

			// 簡易的にメインカメラを探す (デバッグカメラ対応は後ほど)
			// ※シーンマネージャ経由でWorldにアクセス
			World& world = m_sceneManager.GetWorld();
			world.getRegistry().view<Tag, Camera, Transform>().each([&](Entity e, Tag& tag, Camera& cam, Transform& t) {
				if (!cameraFound && strcmp(tag.name.c_str(), "MainCamera") == 0) {
					camPos = XMLoadFloat3(&t.position);

					// 行列計算 (RenderSystemと同じロジック)
					XMMATRIX rot = XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, 0.0f);
					XMVECTOR look = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rot);
					XMVECTOR up = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rot);

					view = XMMatrixLookToLH(camPos, look, up);
					proj = XMMatrixPerspectiveFovLH(cam.fov, cam.aspect, cam.nearZ, cam.farZ);
					cameraFound = true;
				}
				});

			if (cameraFound) {
				// 4. レイの作成 (Unproject)
				// スクリーン座標(ndcX, ndcY) から ワールド空間へ
				XMVECTOR rayOrigin = camPos;
				XMVECTOR rayTarget = XMVector3Unproject(
					XMVectorSet(x, y, 1.0f, 0.0f), // Z=1 (Far)
					0, 0, size.x, size.y, 0.0f, 1.0f, // Viewportは画像のサイズ
					proj, view, XMMatrixIdentity()
				);
				XMVECTOR rayDir = XMVector3Normalize(rayTarget - rayOrigin);

				XMFLOAT3 origin, dir;
				XMStoreFloat3(&origin, rayOrigin);
				XMStoreFloat3(&dir, rayDir);

				// 5. レイキャスト実行
				float dist;
				Entity hit = CollisionSystem::Raycast(world.getRegistry(), origin, dir, dist);

				// 6. 選択状態の更新 (Editorのメンバを更新したい)
				// Editorに「選択する関数」を追加するか、publicメンバにする必要がありますが、
				// ここでは Editor::Instance().SelectEntity(hit) のような関数を作るのがベストです。
				// 簡易的に Editor::Draw の引数で渡しているならそこで更新できますが、
				// Editor::Draw はここより後で呼ばれるため、Editor側にセッターを作りましょう。

				Editor::Instance().SetSelectedEntity(hit); // ★これを作成してください
				if (hit != NullEntity) Logger::Log("Selected: " + std::to_string(hit));
			}
		}
	}
	ImGui::End();

	// Game Window
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // 余白なし
	ImGui::Begin("Game");
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		m_gameRT->Resize(m_device.Get(), (int)size.x, (int)size.y);
		ImGui::Image(m_gameRT->GetID(), size);
	}
	ImGui::End();
	ImGui::PopStyleVar();

	// その他のウィンドウ (Inspector, Hierarchy...)
	Editor::Instance().Draw(m_sceneManager.GetWorld(), m_appContext);

	// 描画終了
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	// ビューポート復帰
	D3D11_VIEWPORT vp = {};
	vp.Width = static_cast<float>(Config::SCREEN_WIDTH);
	vp.Height = static_cast<float>(Config::SCREEN_HEIGHT);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_context->RSSetViewports(1, &vp);

#else
	// ====================================================
	// Release Mode (Game Only)
	// ====================================================
	m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
	// ... (ビューポート設定) ...
	D3D11_VIEWPORT vp = {};
	vp.Width = static_cast<float>(Config::SCREEN_WIDTH);
	vp.Height = static_cast<float>(Config::SCREEN_HEIGHT);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	m_context->RSSetViewports(1, &vp);

	float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_context->ClearRenderTargetView(m_renderTargetView.Get(), color);
	m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// 強制的にゲーム設定
	m_sceneManager.GetContext().debug.useDebugCamera = false;
	m_sceneManager.Render();

#endif

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