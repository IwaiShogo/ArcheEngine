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
#include "Engine/Renderer/Data/Model.h" // Model定義

namespace Arche
{

	class ARCHE_API ModelRenderer
	{
	public:
		/**
		 * @brief	初期化
		 * @param	device	デバイス
		 * @param	context	コンテキスト
		 */
		static void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);

		static void Shutdown();
		
		/**
		 * @brief	描画開始（カメラ行列とライト情報をセット）
		 * @param	view		ビュー行列
		 * @param	projection	プロジェクション行列
		 * @param	lightDir	明かりの向き
		 * @param	lightColor	明かりの色
		 */
		static void Begin(const XMMATRIX& view, const XMMATRIX& projection,
			const XMFLOAT3& lightDir = { 1, -1, 1 },
			const XMFLOAT3& lightColor = { 1, 1, 1 });

		/**
		 * @brief	モデル描画
		 * @param	model	モデル
		 * @param	pos		位置
		 * @param	scale	スケール
		 * @param	rot		回転
		 */
		static void Draw(std::shared_ptr<Model> model, const XMFLOAT3& pos,
			const XMFLOAT3& scale = { 1,1,1 }, const XMFLOAT3& rot = { 0,0,0 });

		/**
		 * @brief	ワールド行列対応版
		 * @param	model		モデル
		 * @param	worldMatrix	ワールド行列
		 */
		static void Draw(std::shared_ptr<Model> model, const DirectX::XMMATRIX& worldMatrix, const std::vector<DirectX::XMFLOAT4X4>* boneMatrices = nullptr);

	private:
		static ID3D11Device* s_device;
		static ID3D11DeviceContext* s_context;

		static ComPtr<ID3D11VertexShader> s_vs;
		static ComPtr<ID3D11PixelShader> s_ps;
		static ComPtr<ID3D11InputLayout> s_inputLayout;
		static ComPtr<ID3D11Buffer> s_constantBuffer;
		static ComPtr<ID3D11SamplerState> s_samplerState;

		// ソリッド描画用ステート
		static ComPtr<ID3D11RasterizerState> s_rsSolid;

		struct CBData {
			XMMATRIX world;
			XMMATRIX view;
			XMMATRIX projection;
			XMFLOAT4 lightDir;
			XMFLOAT4 lightColor;
			XMFLOAT4 materialColor;
			// ボーン行列とフラグ
			XMMATRIX boneTransforms[100];
			int hasAnimation;
			float padding[3];	// アライメント調整
		};
		static CBData s_cbData;

		// デフォルトの白テクスチャ
		static ComPtr<ID3D11ShaderResourceView> s_whiteTexture;

		// 内部用：白テクスチャを作る関数
		static void CreateWhiteTexture();
	};

}	// namespace Arche

#endif // !___MODEL_RENDERER_H___