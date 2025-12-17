/*****************************************************************//**
 * @file	PrimitiveRenderer.h
 * @brief	箱を描画するための簡易クラス
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

#ifndef ___PRIMITIVE_RENDERER_H___
#define ___PRIMITIVE_RENDERER_H___

// ===== インクルード =====
#include "Engine/pch.h"

class PrimitiveRenderer
{
public:
	PrimitiveRenderer(ID3D11Device* device, ID3D11DeviceContext* context);
	~PrimitiveRenderer() = default;

	// 初期化（シェーダー読み込み等）
	void Initialize();

	// 描画開始（カメラ行列をリセット）
	void Begin(const XMMATRIX& view, const XMMATRIX& projection);

	// ボックス描画
	void DrawBox(const XMFLOAT3& position, const XMFLOAT3& size, const XMFLOAT4& rotation, const XMFLOAT4& color);
	// 球体描画
	void DrawSphere(const XMFLOAT3& position, float radius, const XMFLOAT4& color);
	// カプセル描画
	void DrawCapsule(const XMFLOAT3& position, float radius, float height, const XMFLOAT4& rotation, const XMFLOAT4& color);
	// 円柱描画
	void DrawCylinder(const XMFLOAT3& position, float radius, float height, const XMFLOAT4& rotation, const XMFLOAT4& color);
	// ライン描画
	void DrawLine(const XMFLOAT3& p1, const XMFLOAT3& p2, const XMFLOAT4& color);

	// ギズモ用の矢印描画
	void DrawArrow(const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT4& color);

	// 描画モード変更
	void SetFillMode(bool wireframe);
	// グリッドと軸を描画
	void DrawGrid(float spacing = 1.0f, int lines = 10);
	void DrawAxis(float length = 5.0f);

	ID3D11DeviceContext* GetDeviceContext() { return m_context; }

private:
	// 円を描くヘルパー
	void DrawCircle(const XMFLOAT3& center, float radius, const XMFLOAT4& color);
	// メッシュ用ヘルパー
	void CreateCylinderMesh();
	void CreateCapsuleMesh();

private:
	struct ConstantBufferData
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		XMFLOAT4 color;
	};

	ID3D11Device*			m_device;
	ID3D11DeviceContext*	m_context;

	ComPtr<ID3D11VertexShader>	m_vs;
	ComPtr<ID3D11PixelShader>	m_ps;
	ComPtr<ID3D11InputLayout>	m_inputLayout;
	ComPtr<ID3D11Buffer>		m_vertexBuffer;
	ComPtr<ID3D11Buffer>		m_indexBuffer;
	ComPtr<ID3D11Buffer>		m_constantBuffer;
	ComPtr<ID3D11Buffer>		m_lineVertexBuffer;
	ComPtr<ID3D11DepthStencilState> m_depthState;
	// 球体用のバッファ
	ComPtr<ID3D11Buffer>		m_sphereVB;
	ComPtr<ID3D11Buffer>		m_sphereIB;
	UINT						m_sphereIndexCount = 0;
	// 円柱用バッファ
	ComPtr<ID3D11Buffer> m_cylinderVB, m_cylinderIB;
	UINT m_cylinderIndexCount = 0;
	// カプセル用バッファ
	ComPtr<ID3D11Buffer> m_capsuleVB, m_capsuleIB;
	UINT m_capsuleIndexCount = 0;

	ConstantBufferData	m_cbData;

	ComPtr<ID3D11RasterizerState>	m_rsWireframe;
	ComPtr<ID3D11RasterizerState>	m_rsSolid;
};

#endif // !___PRIMITIVE_RENDERER_H___