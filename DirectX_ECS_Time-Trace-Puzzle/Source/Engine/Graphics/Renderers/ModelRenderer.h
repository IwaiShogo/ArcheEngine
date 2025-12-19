/*****************************************************************//**
 * @file	ModelRenderer.h
 * @brief	モデルクラスが持つ複数のメッシュをループして描画するレンダラー
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

#ifndef ___MODEL_RENDERER_H___
#define ___MODEL_RENDERER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Graphics/Core/Model.h" // Model定義

class ModelRenderer {
public:
	ModelRenderer(ID3D11Device* device, ID3D11DeviceContext* context);

	void Initialize();

	// 描画開始 (カメラ行列とライト情報をセット)
	void Begin(const XMMATRIX& view, const XMMATRIX& projection,
		const XMFLOAT3& lightDir = { 1, -1, 1 },
		const XMFLOAT3& lightColor = { 1, 1, 1 });

	// モデル描画
	void Draw(std::shared_ptr<Model> model, const XMFLOAT3& pos,
		const XMFLOAT3& scale = { 1,1,1 }, const XMFLOAT3& rot = { 0,0,0 });

	// ワールド行列
	void Draw(std::shared_ptr<Model> model, const DirectX::XMMATRIX& worldMatrix);

private:
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;

	ComPtr<ID3D11VertexShader> m_vs;
	ComPtr<ID3D11PixelShader> m_ps;
	ComPtr<ID3D11InputLayout> m_inputLayout;
	ComPtr<ID3D11Buffer> m_constantBuffer;
	ComPtr<ID3D11SamplerState> m_samplerState;

	// ソリッド描画用ステート
	ComPtr<ID3D11RasterizerState> m_rsSolid;

	struct CBData {
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		XMFLOAT4 lightDir;
		XMFLOAT4 lightColor;
		XMFLOAT4 materialColor;
	};
	CBData m_cbData;

	// デフォルトの白テクスチャ
	ComPtr<ID3D11ShaderResourceView> m_whiteTexture;

	// 内部用：白テクスチャを作る関数
	void CreateWhiteTexture();
};

#endif // !___MODEL_RENDERER_H___
