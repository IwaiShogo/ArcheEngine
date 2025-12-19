/*****************************************************************//**
 * @file	TextRenderSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/12/18	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___TEXT_RENDER_SYSTEM_H___
#define ___TEXT_RENDER_SYSTEM_H___

// ===== インクルード =====
#include "Engine/ECS/ECS.h"
#include "Engine/Graphics/Text/TextRenderer.h"

class TextRenderSystem
	: public ISystem
{
public:
	TextRenderSystem(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		// Rendererの実体を作成
		m_renderer = std::make_unique<TextRenderer>(device, context);
		m_context = context;	// RTV取得用に保持
	}

	void Update(Registry& registry) override {}

	void Render(Registry& registry, const Context& ctx) override
	{
		// 1. 現在設定されている RTV を取得する
		ComPtr<ID3D11RenderTargetView> currentRTV;
		ComPtr<ID3D11DepthStencilView> currentDSV;
		m_context->OMGetRenderTargets(1, currentRTV.GetAddressOf(), currentDSV.GetAddressOf());

		if (!currentRTV) return;

		// 2. 現在のビューポート（画面サイズ）を取得する
		UINT numViewports = 1;
		D3D11_VIEWPORT vp;
		m_context->RSGetViewports(&numViewports, &vp);

		// 取得した情報を使って描画
		m_renderer->Render(registry, currentRTV.Get(), vp.Width, vp.Height);
	}

private:
	std::unique_ptr<TextRenderer> m_renderer;
	ID3D11DeviceContext* m_context = nullptr;
};

#endif // !___TEXT_RENDER_SYSTEM_H___
