/*****************************************************************//**
 * @file	ModelRenderer.cpp
 * @brief	モデルを描画するクラス
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/26	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Renderer/Renderers/ModelRenderer.h"

namespace Arche
{
	ID3D11Device* ModelRenderer::s_device = nullptr;
	ID3D11DeviceContext* ModelRenderer::s_context = nullptr;
	ComPtr<ID3D11VertexShader> ModelRenderer::s_vs = nullptr;
	ComPtr<ID3D11PixelShader> ModelRenderer::s_ps = nullptr;
	ComPtr<ID3D11InputLayout> ModelRenderer::s_inputLayout = nullptr;
	ComPtr<ID3D11Buffer> ModelRenderer::s_constantBuffer = nullptr;
	ComPtr<ID3D11SamplerState> ModelRenderer::s_samplerState = nullptr;
	ComPtr<ID3D11RasterizerState> ModelRenderer::s_rsSolid = nullptr;
	ModelRenderer::CBData ModelRenderer::s_cbData = {};
	ComPtr<ID3D11ShaderResourceView> ModelRenderer::s_whiteTexture = nullptr;

	void ModelRenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		s_device = device;
		s_context = context;

		// 1. シェーダーコンパイル
		ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

		// Standard.hlsl を読み込む
		HRESULT hr = D3DCompileFromFile(L"Resources/Engine/Shaders/Standard.hlsl", nullptr, nullptr, "VS", "vs_5_0", flags, 0, &vsBlob, &errorBlob);
		if (FAILED(hr)) throw std::runtime_error("Failed to compile Standard VS");
		s_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &s_vs);

		hr = D3DCompileFromFile(L"Resources/Engine/Shaders/Standard.hlsl", nullptr, nullptr, "PS", "ps_5_0", flags, 0, &psBlob, &errorBlob);
		if (FAILED(hr)) throw std::runtime_error("Failed to compile Standard PS");
		s_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &s_ps);

		// 2. 入力レイアウト (Pos, Normal, UV)
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		s_device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &s_inputLayout);

		// 3. 定数バッファ
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(CBData);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		s_device->CreateBuffer(&bd, nullptr, &s_constantBuffer);

		// 4. サンプラー
		D3D11_SAMPLER_DESC sd = {};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		s_device->CreateSamplerState(&sd, &s_samplerState);

		// 5. ラスタライザ
		D3D11_RASTERIZER_DESC rd = {};
		rd.CullMode = D3D11_CULL_BACK; // 裏面カリング有効
		rd.FillMode = D3D11_FILL_SOLID;
		rd.DepthClipEnable = TRUE;
		s_device->CreateRasterizerState(&rd, &s_rsSolid);

		CreateWhiteTexture();
	}

	void ModelRenderer::Shutdown()
	{
		s_vs.Reset();
		s_ps.Reset();
		s_inputLayout.Reset();
		s_constantBuffer.Reset();
		s_samplerState.Reset();
		s_rsSolid.Reset();

		s_whiteTexture.Reset(); // これを忘れるとテクスチャが残ります

		s_device = nullptr;
		s_context = nullptr;
	}

	void ModelRenderer::Begin(const XMMATRIX& view, const XMMATRIX& projection, const XMFLOAT3& lightDir, const XMFLOAT3& lightColor)
	{
		s_context->IASetInputLayout(s_inputLayout.Get());
		s_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		s_context->RSSetState(s_rsSolid.Get());

		s_context->VSSetShader(s_vs.Get(), nullptr, 0);
		s_context->PSSetShader(s_ps.Get(), nullptr, 0);

		s_context->VSSetConstantBuffers(0, 1, s_constantBuffer.GetAddressOf());
		s_context->PSSetConstantBuffers(0, 1, s_constantBuffer.GetAddressOf());
		s_context->PSSetSamplers(0, 1, s_samplerState.GetAddressOf());

		// 行列とライト設定
		s_cbData.view = XMMatrixTranspose(view);
		s_cbData.projection = XMMatrixTranspose(projection);
		s_cbData.lightDir = XMFLOAT4(lightDir.x, lightDir.y, lightDir.z, 0);
		s_cbData.lightColor = XMFLOAT4(lightColor.x, lightColor.y, lightColor.z, 1);
	}

	void ModelRenderer::Draw(std::shared_ptr<Model> model, const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot)
	{
		// 行列を作って、上の関数に丸投げする
		XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX R = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		XMMATRIX T = XMMatrixTranslation(pos.x, pos.y, pos.z);

		XMMATRIX world = S * R * T;

		// 新しいDrawを呼ぶ
		Draw(model, world);
	}

	void ModelRenderer::Draw(std::shared_ptr<Model> model, const DirectX::XMMATRIX& worldMatrix)
	{
		if (!model) return;

		// 1. 定数バッファの更新
		// 受け取った行列を転置してセット
		s_cbData.world = XMMatrixTranspose(worldMatrix);
		s_cbData.materialColor = { 1, 1, 1, 1 }; // デフォルト白

		// シェーダーに送信
		s_context->UpdateSubresource(s_constantBuffer.Get(), 0, nullptr, &s_cbData, 0, 0);

		// 2. メッシュごとの描画ループ
		for (const auto& mesh : model->meshes)
		{
			// 頂点バッファ・インデックスバッファセット
			UINT stride = sizeof(ModelVertex);
			UINT offset = 0;
			s_context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
			s_context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

			// テクスチャセット (なければ白テクスチャ)
			ID3D11ShaderResourceView* srv = nullptr;
			if (mesh.texture) {
				srv = mesh.texture->srv.Get();
			}
			else {
				srv = s_whiteTexture.Get();
			}
			s_context->PSSetShaderResources(0, 1, &srv);

			// 描画実行
			s_context->DrawIndexed(mesh.indexCount, 0, 0);
		}
	}

	void ModelRenderer::CreateWhiteTexture()
	{
		// 1x1 の白ピクセルデータ (R,G,B,A = 255,255,255,255)
		uint32_t pixel = 0xFFFFFFFF;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = &pixel;
		initData.SysMemPitch = sizeof(uint32_t);

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = 1;
		desc.Height = 1;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		ComPtr<ID3D11Texture2D> tex;
		s_device->CreateTexture2D(&desc, &initData, &tex);

		s_device->CreateShaderResourceView(tex.Get(), nullptr, &s_whiteTexture);
	}

}	// namespace Arche