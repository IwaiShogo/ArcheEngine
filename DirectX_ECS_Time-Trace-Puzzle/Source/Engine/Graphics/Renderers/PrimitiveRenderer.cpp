/*****************************************************************//**
 * @file	PrimitiveRenderer.cpp
 * @brief	箱を描画するための簡易クラスの実装
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

// ===== インクルード =====
#define NOMINMAX
#include "Engine/Graphics/Renderers/PrimitiveRenderer.h"
#include <d3dcompiler.h>
#include <stdexcept>

#pragma comment(lib, "d3dcompiler.lib")

// 立方体の頂点データ
struct Vertex
{
	XMFLOAT3 position;
};

PrimitiveRenderer::PrimitiveRenderer(ID3D11Device* device, ID3D11DeviceContext* context)
	: m_device(device)
	, m_context(context)
{
}

void PrimitiveRenderer::Initialize()
{
	// 1. シェーダーコンパイル
	ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

	// Vertex Shader
	HRESULT hr = D3DCompileFromFile(L"Resources/Shaders/DebugPrimitive.hlsl", nullptr, nullptr, "VS", "vs_5_0", compileFlags, 0, &vsBlob, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob)OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		throw std::runtime_error("Failed to compile VS");
	}
	m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vs);

	// Pixel Shader
	hr = D3DCompileFromFile(L"Resources/Shaders/DebugPrimitive.hlsl", nullptr, nullptr, "PS", "ps_5_0", compileFlags, 0, &psBlob, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob)OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		throw std::runtime_error("Failed to compile PS");
	}
	m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_ps);

	// 2. 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	m_device->CreateInputLayout(layout, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);

	// ============================================================
	// ボックスのメッシュ生成
	// ============================================================
	float hs = 0.5f;
	Vertex vertices[] = {
		{ XMFLOAT3(-hs, -hs, -hs) }, { XMFLOAT3(-hs, +hs, -hs) },
		{ XMFLOAT3(+hs, +hs, -hs) }, { XMFLOAT3(+hs, -hs, -hs) },
		{ XMFLOAT3(-hs, -hs, +hs) }, { XMFLOAT3(-hs, +hs, +hs) },
		{ XMFLOAT3(+hs, +hs, +hs) }, { XMFLOAT3(+hs, -hs, +hs) },
	};
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex) * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices;
	m_device->CreateBuffer(&bd, &initData, &m_vertexBuffer);

	// 4. インデックスバッファ（ワイヤーフレーム用 LineList）
	// 立方体は6面、1面当たり三角形2枚、計12枚の三角形 = 36インデックス
	uint16_t indices[] = {
		// 手前（Z-）
		0, 1, 2,   0, 2, 3,
		// 奥（Z+）
		4, 6, 5,   4, 7, 6,
		// 上（Y+）
		1, 5, 6,   1, 6, 2,
		// 下（Y-）
		0, 3, 7,   0, 7, 4,
		// 左（X-）
		0, 4, 5,   0, 5, 1,
		// 右（X+）
		3, 2, 6,   3, 6, 7,
	};
	bd.ByteWidth = sizeof(indices);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	initData.pSysMem = indices;
	m_device->CreateBuffer(&bd, &initData, &m_indexBuffer);

	// 5. 定数バッファ
	bd.ByteWidth = sizeof(ConstantBufferData);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	m_device->CreateBuffer(&bd, nullptr, &m_constantBuffer);

	// 線描画用の動的頂点バッファ
	bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * 2;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	m_device->CreateBuffer(&bd, nullptr, &m_lineVertexBuffer);

	// ============================================================
	// 球体のメッシュ生成
	// ============================================================
	std::vector<Vertex> sphereVertices;
	std::vector<uint16_t> sphereIndices;

	int div = 16; // 分割数 (増やすと滑らかになりますが重くなります)
	float radius = 0.5f; // 直径1.0の単位球を作る

	// 頂点生成
	for (int y = 0; y <= div; ++y) {
		float v = (float)y / div;
		float pitch = (v - 0.5f) * XM_PI;
		float cosPitch = cos(pitch);
		float sinPitch = sin(pitch);

		for (int x = 0; x <= div; ++x) {
			float u = (float)x / div;
			float yaw = u * XM_2PI;
			float cosYaw = cos(yaw);
			float sinYaw = sin(yaw);

			Vertex vert;
			vert.position = { radius * cosPitch * cosYaw, radius * sinPitch, radius * cosPitch * sinYaw };
			sphereVertices.push_back(vert);
		}
	}

	// インデックス生成
	for (int y = 0; y < div; ++y) {
		for (int x = 0; x < div; ++x) {
			int i0 = y * (div + 1) + x;
			int i1 = i0 + 1;
			int i2 = (y + 1) * (div + 1) + x;
			int i3 = i2 + 1;

			// 三角形1 (左上 -> 右上 -> 左下)
			sphereIndices.push_back(i0);
			sphereIndices.push_back(i1);
			sphereIndices.push_back(i2);

			// 三角形2 (左下 -> 右上 -> 右下)
			sphereIndices.push_back(i2);
			sphereIndices.push_back(i1);
			sphereIndices.push_back(i3);
		}
	}
	m_sphereIndexCount = (UINT)sphereIndices.size();

	// バッファ作成
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.ByteWidth = sizeof(Vertex) * sphereVertices.size();
	initData = {};
	initData.pSysMem = sphereVertices.data();
	m_device->CreateBuffer(&bd, &initData, &m_sphereVB);

	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.ByteWidth = sizeof(uint16_t) * sphereIndices.size();
	initData.pSysMem = sphereIndices.data();
	m_device->CreateBuffer(&bd, &initData, &m_sphereIB);

	// カプセル、円柱の作成
	CreateCapsuleMesh();
	CreateCylinderMesh();

	// 深度ステンシルステート作成
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = TRUE;								// 深度テストを行う
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;	// 深度バッファに書き込む
	dsd.DepthFunc = D3D11_COMPARISON_LESS;				// 手前にあるものを描画する

	hr = m_device->CreateDepthStencilState(&dsd, &m_depthState);
	if (FAILED(hr)) throw std::runtime_error("Failed to create Primitive DepthState");

	// ラスタライザステート作成
	D3D11_RASTERIZER_DESC rd = {};
	rd.CullMode = D3D11_CULL_NONE;	// 両面描画
	rd.FrontCounterClockwise = FALSE;
	rd.DepthClipEnable = TRUE;

	// ワイヤーフレーム用
	rd.FillMode = D3D11_FILL_WIREFRAME;
	m_device->CreateRasterizerState(&rd, &m_rsWireframe);

	// ソリッド用
	rd.FillMode = D3D11_FILL_SOLID;
	m_device->CreateRasterizerState(&rd, &m_rsSolid);
}

void PrimitiveRenderer::Begin(const XMMATRIX& view, const XMMATRIX& projection)
{
	// 共通設定
	m_context->IASetInputLayout(m_inputLayout.Get());
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_context->OMSetDepthStencilState(m_depthState.Get(), 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	m_context->VSSetShader(m_vs.Get(), nullptr, 0);
	m_context->PSSetShader(m_ps.Get(), nullptr, 0);
	m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	// ビュー・プロジェクション行列を保存
	m_cbData.view = XMMatrixTranspose(view);
	m_cbData.projection = XMMatrixTranspose(projection);
}

// ボックス
void PrimitiveRenderer::DrawBox(const XMFLOAT3& position, const XMFLOAT3& size, const XMFLOAT4& rotation, const XMFLOAT4& color)
{
	// 1. パイプライン設定 (ステート漏れを防ぐため毎回セットする)
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // ワイヤーフレーム

	// 2. 定数バッファ更新
	XMVECTOR q = XMLoadFloat4(&rotation);
	XMMATRIX world = XMMatrixScaling(size.x, size.y, size.z) * XMMatrixRotationQuaternion(q) * XMMatrixTranslation(position.x, position.y, position.z);
	m_cbData.world = XMMatrixTranspose(world);
	m_cbData.color = color;
	m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &m_cbData, 0, 0);

	// 3. 描画
	m_context->DrawIndexed(36, 0, 0);
}

// 球体
void PrimitiveRenderer::DrawSphere(const XMFLOAT3& position, float radius, const XMFLOAT4& color)
{
	// ワールド行列 (半径に合わせてスケーリング)
	// 単位球(半径0.5)を作ったので、radius * 2 倍する
	float scale = radius * 2.0f;
	XMMATRIX world = XMMatrixScaling(scale, scale, scale) * XMMatrixTranslation(position.x, position.y, position.z);

	m_cbData.world = XMMatrixTranspose(world);
	m_cbData.color = color;
	m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &m_cbData, 0, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, m_sphereVB.GetAddressOf(), &stride, &offset);
	m_context->IASetIndexBuffer(m_sphereIB.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_context->DrawIndexed(m_sphereIndexCount, 0, 0);
}

// カプセル
void PrimitiveRenderer::DrawCapsule(const XMFLOAT3& position, float radius, float height, const XMFLOAT4& rotation, const XMFLOAT4& color)
{
	// カプセルは「円柱」＋「上下の半球」で構成される
	// height は全体の高さ。円柱部分の高さは height - 2*radius
	float cylinderH = std::max(0.0f, height - 2.0f * radius);

	XMVECTOR posV = XMLoadFloat3(&position);
	XMVECTOR rotQ = XMLoadFloat4(&rotation);

	// Y軸向きのローカル座標系で考える
	XMVECTOR up = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), rotQ);

	// 1. 中央の円柱部分
	if (cylinderH > 0.0f) {
		DrawCylinder(position, radius, cylinderH, rotation, color);
	}

	// 2. 上の半球 (球体を描画して代用)
	XMVECTOR topPos = posV + up * (cylinderH * 0.5f);
	XMFLOAT3 topPosF; XMStoreFloat3(&topPosF, topPos);
	DrawSphere(topPosF, radius, color);

	// 3. 下の半球
	XMVECTOR bottomPos = posV - up * (cylinderH * 0.5f);
	XMFLOAT3 bottomPosF; XMStoreFloat3(&bottomPosF, bottomPos);
	DrawSphere(bottomPosF, radius, color);
}

// 円柱
void PrimitiveRenderer::DrawCylinder(const XMFLOAT3& position, float radius, float height, const XMFLOAT4& rotation, const XMFLOAT4& color)
{
	// 半径と高さでスケーリング (基本メッシュは半径1, 高さ1)
	XMVECTOR q = XMLoadFloat4(&rotation);
	XMMATRIX world = XMMatrixScaling(radius * 2.0f, height, radius * 2.0f) * XMMatrixRotationQuaternion(q) * XMMatrixTranslation(position.x, position.y, position.z);
	m_cbData.world = XMMatrixTranspose(world);
	m_cbData.color = color;
	m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &m_cbData, 0, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, m_cylinderVB.GetAddressOf(), &stride, &offset);
	m_context->IASetIndexBuffer(m_cylinderIB.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_context->DrawIndexed(m_cylinderIndexCount, 0, 0);
}

void PrimitiveRenderer::DrawLine(const XMFLOAT3& p1, const XMFLOAT3& p2, const XMFLOAT4& color)
{
	// 1. 定数バッファ更新（ワールド行列は単位行列にする）
	m_cbData.world = XMMatrixIdentity();
	m_cbData.color = color;
	m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &m_cbData, 0, 0);

	// 2. 動的頂点バッファの書き換え
	D3D11_MAPPED_SUBRESOURCE ms;
	if (SUCCEEDED(m_context->Map(m_lineVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms)))
	{
		Vertex* v = (Vertex*)ms.pData;
		v[0].position = p1;
		v[1].position = p2;
		m_context->Unmap(m_lineVertexBuffer.Get(), 0);
	}

	// 3. パイプライン設定変更
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_context->IASetVertexBuffers(0, 1, m_lineVertexBuffer.GetAddressOf(), &stride, &offset);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 4.描画
	m_context->Draw(2, 0);

	// 5. バッファをBox用のものに戻しておく
	m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void PrimitiveRenderer::DrawArrow(const XMFLOAT3& start, const XMFLOAT3& end, const XMFLOAT4& color)
{
	// 1. 軸の線を書く
	DrawLine(start, end, color);

	// 2. 先端に小さな箱を書く
	float boxSize = 0.2f;
	DrawBox(end, XMFLOAT3(boxSize, boxSize, boxSize), XMFLOAT4(0, 0, 0, 0), color);
}

void PrimitiveRenderer::SetFillMode(bool wireframe)
{
	if (wireframe)
	{
		m_context->RSSetState(m_rsWireframe.Get());
	}
	else
	{
		m_context->RSSetState(m_rsSolid.Get());
	}
}

void PrimitiveRenderer::DrawGrid(float spacing, int lines)
{
	// グリッドは常にワイヤーフレームで書く
	m_context->RSSetState(m_rsWireframe.Get());

	float size = static_cast<float>(lines) * spacing;
	XMFLOAT4 color = { 0.5f, 0.5f, 0.5f, 1.0f };
	
	// X軸に平行な線
	for (int i = -lines; i <= lines; ++i)
	{
		float pos = static_cast<float>(i) * spacing;

		// 横線
		DrawLine(XMFLOAT3(-size, 0, pos), XMFLOAT3(size, 0, pos), color);
		// 縦線
		DrawLine(XMFLOAT3(pos, 0, -size), XMFLOAT3(pos, 0, size), color);
	}
}

// 座標軸描画
void PrimitiveRenderer::DrawAxis(float length)
{
	// X軸（赤）
	DrawLine(XMFLOAT3(0, 0, 0), XMFLOAT3(length, 0, 0), XMFLOAT4(1, 0, 0, 1));
	// Y軸（緑）
	DrawLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, length, 0), XMFLOAT4(0, 1, 0, 1));
	// Z軸（青）
	DrawLine(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, length), XMFLOAT4(0, 0, 1, 1));
}

void PrimitiveRenderer::DrawCircle(const XMFLOAT3& center, float radius, const XMFLOAT4& color)
{
	const int div = 16;
	const float dAngle = XM_2PI / div;

	for (int i = 0; i < div; ++i)
	{
		float a1 = i * dAngle;
		float a2 = (i + 1) * dAngle;

		XMFLOAT3 p1 = { center.x + cosf(a1) * radius, center.y, center.z + sinf(a1) * radius };
		XMFLOAT3 p2 = { center.x + cosf(a2) * radius, center.y, center.z + sinf(a2) * radius };

		DrawLine(p1, p2, color);
	}
}

void PrimitiveRenderer::CreateCylinderMesh()
{
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	int slices = 16; // 円の分割数

	// --- 頂点生成 ---
	// 上面の中心 (Index 0)
	vertices.push_back({ {0, 0.5f, 0} });
	// 上面の円周 (Index 1 ~ slices)
	for (int i = 0; i <= slices; ++i) {
		float theta = (float)i / slices * XM_2PI;
		vertices.push_back({ {cosf(theta), 0.5f, sinf(theta)} });
	}
	// 下面の中心 (Index slices+2)
	int bottomCenterIndex = (int)vertices.size();
	vertices.push_back({ {0, -0.5f, 0} });
	// 下面の円周
	for (int i = 0; i <= slices; ++i) {
		float theta = (float)i / slices * XM_2PI;
		vertices.push_back({ {cosf(theta), -0.5f, sinf(theta)} });
	}

	// --- インデックス生成 ---
	// 上面の蓋 (Triangle Fan -> List)
	for (int i = 1; i <= slices; ++i) {
		indices.push_back(0);
		indices.push_back(i + 1);
		indices.push_back(i);
	}
	// 下面の蓋
	int bottomStart = bottomCenterIndex + 1;
	for (int i = 0; i < slices; ++i) {
		indices.push_back(bottomCenterIndex);
		indices.push_back(bottomStart + i);
		indices.push_back(bottomStart + i + 1);
	}
	// 側面 (Quad -> 2 Triangles)
	for (int i = 1; i <= slices; ++i) {
		int top1 = i;
		int top2 = i + 1;
		int bot1 = bottomStart + (i - 1);
		int bot2 = bottomStart + i;

		// トライアングル1
		indices.push_back(top1); indices.push_back(bot1); indices.push_back(top2);
		// トライアングル2
		indices.push_back(top2); indices.push_back(bot1); indices.push_back(bot2);
	}

	m_cylinderIndexCount = (UINT)indices.size();

	// バッファ作成 (定型文なので省略可ですが念のため)
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Vertex) * (UINT)vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vInit = { vertices.data(), 0, 0 };
	m_device->CreateBuffer(&vbd, &vInit, &m_cylinderVB);

	D3D11_BUFFER_DESC ibd = {};
	ibd.ByteWidth = sizeof(uint16_t) * (UINT)indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA iInit = { indices.data(), 0, 0 };
	m_device->CreateBuffer(&ibd, &iInit, &m_cylinderIB);
}

void PrimitiveRenderer::CreateCapsuleMesh()
{
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	int slices = 16; // 円周分割
	int stacks = 16; // 縦分割
	float radius = 0.5f;
	float height = 1.0f; // 全高 (cylinderHeight + 2*radius)
	float cylinderHeight = 0.5f; // 円柱部分

	// 頂点生成
	for (int j = 0; j <= stacks; ++j) {
		float v = (float)j / stacks;
		float phi = v * XM_PI; // 0 to PI

		// Y座標の計算 (半球を上下に引き離す)
		float y = -cosf(phi) * radius;

		// 下半球
		if (j < stacks / 2) {
			y -= cylinderHeight * 0.5f;
		}
		// 上半球
		else if (j > stacks / 2) {
			y += cylinderHeight * 0.5f;
		}
		// 真ん中のリングは重複させるか、スキップして繋ぐ

		float r = sinf(phi) * radius; // その高さでの半径

		for (int i = 0; i <= slices; ++i) {
			float u = (float)i / slices;
			float theta = u * XM_2PI;

			Vertex vert;
			vert.position = { r * cosf(theta), y, r * sinf(theta) };
			vertices.push_back(vert);
		}
	}

	// インデックス生成 (Grid状にトライアングルを貼る)
	int ringVertexCount = slices + 1;
	for (int j = 0; j < stacks; ++j) {
		for (int i = 0; i < slices; ++i) {
			int i0 = j * ringVertexCount + i;
			int i1 = i0 + 1;
			int i2 = (j + 1) * ringVertexCount + i;
			int i3 = i2 + 1;

			indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
			indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
		}
	}

	m_capsuleIndexCount = (UINT)indices.size();

	// バッファ作成 (省略: Cylinderと同じ手順で m_capsuleVB/IB を作成)
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Vertex) * (UINT)vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vInit = { vertices.data(), 0, 0 };
	m_device->CreateBuffer(&vbd, &vInit, &m_capsuleVB);

	D3D11_BUFFER_DESC ibd = {};
	ibd.ByteWidth = sizeof(uint16_t) * (UINT)indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA iInit = { indices.data(), 0, 0 };
	m_device->CreateBuffer(&ibd, &iInit, &m_capsuleIB);
}