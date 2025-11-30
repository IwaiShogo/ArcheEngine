/*****************************************************************//**
 * @file	SpriteRenderer.cpp
 * @brief	2D専用のレンダラークラスの実装
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/25	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Engine/Graphics/Renderers/SpriteRenderer.h"
#include "main.h"
#include <d3dcompiler.h>
#include <stdexcept>

#pragma comment(lib, "d3dcompiler.lib")

struct Vertex2D {
	XMFLOAT3 pos;
	XMFLOAT2 uv;
};

SpriteRenderer::SpriteRenderer(ID3D11Device* device, ID3D11DeviceContext* context)
	: m_device(device), m_context(context) {}

void SpriteRenderer::Initialize() {
	// 1. シェーダーコンパイル (パスに注意)
	ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

	HRESULT hr = D3DCompileFromFile(L"Resources/Shaders/Sprite.hlsl", nullptr, nullptr, "VS", "vs_5_0", flags, 0, &vsBlob, &errorBlob);
	if (FAILED(hr)) throw std::runtime_error("Failed to compile Sprite VS");
	m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);

	hr = D3DCompileFromFile(L"Resources/Shaders/Sprite.hlsl", nullptr, nullptr, "PS", "ps_5_0", flags, 0, &psBlob, &errorBlob);
	if (FAILED(hr)) throw std::runtime_error("Failed to compile Sprite PS");
	m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

	// 2. 入力レイアウト (Pos + UV)
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);

	// 3. 頂点バッファ (動的に書き換えるのでDYNAMIC)
	// 四角形 (TriangleStripなら4頂点で済む)
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex2D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	m_device->CreateBuffer(&bd, nullptr, &m_vertexBuffer);

	// 4. 定数バッファ
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBufferData);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	m_device->CreateBuffer(&bd, nullptr, &m_constantBuffer);

	// 5. ブレンドステート (透過有効)
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	m_device->CreateBlendState(&blendDesc, &m_blendState);

	// 6. サンプラー (リニア補間)
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	m_device->CreateSamplerState(&samplerDesc, &m_samplerState);

	// 7. ラスタライザー
	D3D11_RASTERIZER_DESC rd = {};
	rd.CullMode = D3D11_CULL_NONE;
	rd.FillMode = D3D11_FILL_SOLID;
	rd.DepthClipEnable = FALSE;
	m_device->CreateRasterizerState(&rd, &m_rs2D);

	// 深度ステンシルステート
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = FALSE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_ALWAYS;

	hr = m_device->CreateDepthStencilState(&dsd, &m_ds2D);
	if (FAILED(hr)) throw std::runtime_error("Failed to create 2D DepthStencil State");
}

void SpriteRenderer::Begin() {
	// 2D用のパイプライン設定
	m_context->IASetInputLayout(m_inputLayout.Get());
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); // ストリップ

	UINT stride = sizeof(Vertex2D);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

	m_context->VSSetShader(m_vs.Get(), nullptr, 0);
	m_context->PSSetShader(m_ps.Get(), nullptr, 0);

	m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	// サンプラーセット
	m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

	// ブレンドステートセット
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xffffffff);

	// 2D正射影行列 (左上0,0 ～ 右下W,H)
	// Z範囲は 0.0～1.0
	float w = static_cast<float>(Config::SCREEN_WIDTH);
	float h = static_cast<float>(Config::SCREEN_HEIGHT);
	m_cbData.projection = XMMatrixTranspose(XMMatrixOrthographicOffCenterLH(0, w, h, 0, 0.0f, 1.0f));

	m_context->RSSetState(m_rs2D.Get());
	m_context->OMSetDepthStencilState(m_ds2D.Get(), 0);
}

void SpriteRenderer::Draw(Texture* texture, float x, float y, const XMFLOAT4& color) {
	if (!texture) return;
	Draw(texture, x, y, (float)texture->width, (float)texture->height, color);
}

void SpriteRenderer::Draw(Texture* texture, float x, float y, float w, float h, const XMFLOAT4& color) {
	if (!texture) return;

	// 頂点データの更新 (4頂点)
	D3D11_MAPPED_SUBRESOURCE ms;
	if (SUCCEEDED(m_context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
		Vertex2D* v = (Vertex2D*)ms.pData;
		// TriangleStripの順序: 左上 -> 右上 -> 左下 -> 右下
		v[0] = { XMFLOAT3(x,	 y,		0), XMFLOAT2(0, 0) }; // 左上
		v[1] = { XMFLOAT3(x + w, y,		0), XMFLOAT2(1, 0) }; // 右上
		v[2] = { XMFLOAT3(x,	 y + h, 0), XMFLOAT2(0, 1) }; // 左下
		v[3] = { XMFLOAT3(x + w, y + h, 0), XMFLOAT2(1, 1) }; // 右下
		m_context->Unmap(m_vertexBuffer.Get(), 0);
	}

	// テクスチャセット
	m_context->PSSetShaderResources(0, 1, texture->srv.GetAddressOf());

	// 定数バッファ更新
	m_cbData.world = XMMatrixIdentity(); // 頂点座標で位置を決めたのでワールドは単位行列
	m_cbData.color = color;
	m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &m_cbData, 0, 0);

	// 描画
	m_context->Draw(4, 0);
}