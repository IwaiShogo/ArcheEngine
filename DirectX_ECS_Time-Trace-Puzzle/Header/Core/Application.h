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
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>	// ComPtr用
#include <memory>
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"

// ImGuiのヘッダ
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// ライブラリのリンク指示
#pragma comment(lib, "d3d11.lib")

using Microsoft::WRL::ComPtr;

class Application
{
public:
	Application(HWND hwnd);
	~Application();

	void Initialize();
	void Run();	// 1フレームの処理（Update + Render）

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

	// ECS Registry（ゲームデータの実体）
	Registry m_registry;

	// シーンマネージャー
	SceneManager m_sceneManager;
};

#endif // !___APPLICATION_H___