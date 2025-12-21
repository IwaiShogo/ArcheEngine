/*****************************************************************//**
 * @file	RenderTarget.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/29	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Renderer/Core/RenderTarget.h"

RenderTarget::RenderTarget(ID3D11Device* device, int width, int height) {
	Resize(device, width, height);
}

void RenderTarget::Resize(ID3D11Device* device, int width, int height) {
	if (m_width == width && m_height == height) return;

	m_width = width;
	m_height = height;

	// 古いリソースを解放
	m_texture.Reset();
	m_rtv.Reset();
	m_srv.Reset();

	// 1. テクスチャ作成
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	device->CreateTexture2D(&desc, nullptr, &m_texture);

	// 2. RTV作成
	device->CreateRenderTargetView(m_texture.Get(), nullptr, &m_rtv);

	// 3. SRV作成 (ImGui用)
	device->CreateShaderResourceView(m_texture.Get(), nullptr, &m_srv);
}

void RenderTarget::Activate(ID3D11DeviceContext* context, ID3D11DepthStencilView* depthStencil) {
	// このテクスチャを描画先に設定
	ID3D11RenderTargetView* rtvList[] = { m_rtv.Get() };
	context->OMSetRenderTargets(1, rtvList, depthStencil); // 深度バッファは共有または別途用意

	D3D11_VIEWPORT vp = {};
	vp.Width = (float)m_width;
	vp.Height = (float)m_height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);
}

void RenderTarget::Clear(ID3D11DeviceContext* context, float r, float g, float b, float a) {
	float color[] = { r, g, b, a };
	context->ClearRenderTargetView(m_rtv.Get(), color);
}
