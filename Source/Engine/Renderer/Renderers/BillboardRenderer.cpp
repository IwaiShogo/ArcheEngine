/*****************************************************************//**
 * @file	BillboardRenderer.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/27	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Renderer/Renderers/BillboardRenderer.h"

struct BillboardVertex {
	XMFLOAT3 pos; // オフセット位置 (-0.5 ~ 0.5)
	XMFLOAT2 uv;
};

BillboardRenderer::BillboardRenderer(ID3D11Device* device, ID3D11DeviceContext* context)
	: m_device(device), m_context(context) {
}

void BillboardRenderer::Initialize() {
	// 1. シェーダーコンパイル (Billboard.hlsl)
	ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

	HRESULT hr = D3DCompileFromFile(L"Resources/Engine/Shaders/Billboard.hlsl", nullptr, nullptr, "VS", "vs_5_0", flags, 0, &vsBlob, &errorBlob);
	if (FAILED(hr)) throw std::runtime_error("Failed to compile Billboard VS");
	m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);

	hr = D3DCompileFromFile(L"Resources/Engine/Shaders/Billboard.hlsl", nullptr, nullptr, "PS", "ps_5_0", flags, 0, &psBlob, &errorBlob);
	if (FAILED(hr)) throw std::runtime_error("Failed to compile Billboard PS");
	m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

	// 2. 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);

	// 3. 定数バッファ
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBufferData);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	m_device->CreateBuffer(&bd, nullptr, &m_constantBuffer);

	// 4. 頂点バッファ (単位正方形: 中心基準)
	BillboardVertex vertices[] = {
		{ XMFLOAT3(-0.5f, 0.5f, 0),	 XMFLOAT2(0, 0) }, // 左上
		{ XMFLOAT3(0.5f, 0.5f, 0),	 XMFLOAT2(1, 0) }, // 右上
		{ XMFLOAT3(-0.5f, -0.5f, 0), XMFLOAT2(0, 1) }, // 左下
		{ XMFLOAT3(0.5f, -0.5f, 0), XMFLOAT2(1, 1) }, // 右下
	};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(BillboardVertex) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices;
	m_device->CreateBuffer(&bd, &initData, &m_vertexBuffer);

	// 5. サンプラー & ラスタライザ & ブレンド
	D3D11_SAMPLER_DESC sd = {};
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	m_device->CreateSamplerState(&sd, &m_samplerState);

	D3D11_RASTERIZER_DESC rd = {};
	rd.CullMode = D3D11_CULL_NONE; // 両面描画
	rd.FillMode = D3D11_FILL_SOLID;
	m_device->CreateRasterizerState(&rd, &m_rsBillboard);

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
}

void BillboardRenderer::Begin(const XMMATRIX& view, const XMMATRIX& projection) {
	m_context->IASetInputLayout(m_inputLayout.Get());
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	UINT stride = sizeof(BillboardVertex);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

	m_context->VSSetShader(m_vs.Get(), nullptr, 0);
	m_context->PSSetShader(m_ps.Get(), nullptr, 0);

	m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

	m_context->RSSetState(m_rsBillboard.Get());

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xffffffff);

	// ビュー・プロジェクション行列セット
	m_cbData.view = XMMatrixTranspose(view);
	m_cbData.projection = XMMatrixTranspose(projection);
}

void BillboardRenderer::Draw(Texture* texture, const XMFLOAT3& position, float width, float height, const XMFLOAT4& color) {
	if (!texture) return;

	// 1. ワールド行列（位置とサイズのみ。回転はシェーダーがやる）
	// スケール行列で幅と高さを設定
	XMMATRIX S = XMMatrixScaling(width, height, 1.0f);
	XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX world = S * T;

	m_cbData.world = XMMatrixTranspose(world);
	m_cbData.color = color;
	m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &m_cbData, 0, 0);

	// 2. テクスチャセット
	m_context->PSSetShaderResources(0, 1, texture->srv.GetAddressOf());

	// 3. 描画
	m_context->Draw(4, 0);
}
