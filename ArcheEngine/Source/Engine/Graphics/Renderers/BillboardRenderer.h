/*****************************************************************//**
 * @file	BillboardRenderer.h
 * @brief	3D空間に2D描画をする
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

#ifndef ___BILLBOARD_RENDERER_H___
#define ___BILLBOARD_RENDERER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Graphics/Core/Texture.h"

class BillboardRenderer
{
public:
	BillboardRenderer(ID3D11Device* device, ID3D11DeviceContext* context);
	void Initialize();

	void Begin(const XMMATRIX& view, const XMMATRIX& projection);

	// 中心座標、幅、高さ、色を指定して描画
	void Draw(Texture* texture, const XMFLOAT3& position, float width, float height, const XMFLOAT4& color = { 1,1,1,1 });

private:
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;

	ComPtr<ID3D11VertexShader> m_vs;
	ComPtr<ID3D11PixelShader> m_ps;
	ComPtr<ID3D11InputLayout> m_inputLayout;
	ComPtr<ID3D11Buffer> m_constantBuffer;
	ComPtr<ID3D11Buffer> m_vertexBuffer; // 四角形の頂点用
	ComPtr<ID3D11SamplerState> m_samplerState;
	ComPtr<ID3D11RasterizerState> m_rsBillboard;
	ComPtr<ID3D11BlendState> m_blendState; // 半透明用

	struct ConstantBufferData
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		XMFLOAT4 color;
	};
	ConstantBufferData m_cbData;
};

#endif // !___BILLBOARD_RENDERER_H___
