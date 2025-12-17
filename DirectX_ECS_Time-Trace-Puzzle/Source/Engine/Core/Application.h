/*****************************************************************//**
 * @file	Application.h
 * @brief	ゲームループ
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

#ifndef ___APPLICATION_H___
#define ___APPLICATION_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Config.h"
#include "Engine/ECS/ECS.h"
#include "Engine/Scene/SceneManager.h"

// Renderer
#include "Engine/Graphics/Renderers/PrimitiveRenderer.h"
#include "Engine/Graphics/Renderers/SpriteRenderer.h"
#include "Engine/Graphics/Renderers/ModelRenderer.h"
#include "Engine/Graphics/Renderers/BillboardRenderer.h"
#include "Engine/Graphics/Core/RenderTarget.h"

// ImGuiのヘッダ
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// ライブラリのリンク指示
#pragma comment(lib, "d3d11.lib")

class Application
{
public:
	Application(HWND hwnd);
	~Application();

	void Initialize();
	void Run();	// 1フレームの処理（Update + Render）

	SceneManager& GetSceneManager() { return m_sceneManager; }

private:
	void Update();
	void Render();

private:
	HWND m_hwnd;

	// DirectX 11 Resources
	ComPtr<ID3D11Device>			m_device;
	ComPtr<ID3D11DeviceContext>		m_context;
	ComPtr<IDXGISwapChain>			m_swapChain;
	ComPtr<ID3D11RenderTargetView>	m_renderTargetView;
	ComPtr<ID3D11DepthStencilView>	m_depthStencilView;

	std::unique_ptr<PrimitiveRenderer>	m_primitiveRenderer;
	std::unique_ptr<SpriteRenderer>		m_spriteRenderer;
	std::unique_ptr<ModelRenderer>		m_modelRenderer;
	std::unique_ptr<BillboardRenderer>	m_billboardRenderer;
	Context m_appContext;

	// エディタ用レンダーターゲット
	std::unique_ptr<RenderTarget> m_sceneRT;
	std::unique_ptr<RenderTarget> m_gameRT;

	// シーンマネージャー
	SceneManager m_sceneManager;

	ImVec2 m_sceneWindowSize = { (float)Config::SCREEN_WIDTH, (float)Config::SCREEN_HEIGHT };
};

#endif // !___APPLICATION_H___