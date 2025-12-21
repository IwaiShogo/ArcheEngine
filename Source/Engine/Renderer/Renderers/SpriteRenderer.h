/*****************************************************************//**
 * @file	SpriteRenderer.h
 * @brief	2D描画専用のレンダラークラス
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

#ifndef ___SPRITE_RENDERER_H___
#define ___SPRITE_RENDERER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Renderer/Core/Texture.h"

class SpriteRenderer
{
public:
	SpriteRenderer(ID3D11Device* device, ID3D11DeviceContext* context);
	~SpriteRenderer() = default;

	void Initialize();

	// 描画開始
	void Begin();

	/**
	 * @brief	スプライト描画
	 * 
	 * @param	[in] texture 描画する画像
	 * @param	[in] x 左上のX座標
	 * @param	[in] y 左上のY座標
	 * @param	[in] color 色合い
	 */
	void Draw(Texture* texture, float x, float y, const XMFLOAT4& color = { 1, 1, 1, 1 });

	// サイズ指定版
	void Draw(Texture* texture, float x, float y, float w, float h, const XMFLOAT4& color = { 1, 1, 1, 1 });

private:
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;

	ComPtr<ID3D11VertexShader>		m_vs;
	ComPtr<ID3D11PixelShader>		m_ps;
	ComPtr<ID3D11InputLayout>		m_inputLayout;
	ComPtr<ID3D11Buffer>			m_vertexBuffer;
	ComPtr<ID3D11Buffer>			m_constantBuffer;
	ComPtr<ID3D11RasterizerState>	m_rs2D;
	ComPtr<ID3D11DepthStencilState>	m_ds2D;

	// 透過処理用
	ComPtr<ID3D11BlendState> m_blendState;
	// サンプラー（テクスチャの貼り方設定）
	ComPtr<ID3D11SamplerState> m_samplerState;

	struct ConstantBufferData {
		XMMATRIX world;
		XMMATRIX projection;
		XMFLOAT4 color;
	};
	ConstantBufferData m_cbData;
};

#endif // !___SPRITE_RENDERER_H___
