/*****************************************************************//**
 * @file	RenderSystem.h
 * @brief	描画処理
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
#include "Systems/RenderSystem.h"

using namespace DirectX;

void RenderSystem::Render(Registry& registry, const Context& context)
{
	if (!m_renderer) return;

	// ------------------------------------------------------------
	// 1. メインの描画
	// ------------------------------------------------------------

	// 1. カメラ探索
	XMMATRIX viewMatrix = XMMatrixIdentity();
	XMMATRIX projMatrix = XMMatrixIdentity();
	XMVECTOR eye = XMVectorSet(0, 0, -1, 0);
	XMVECTOR lookDir = XMVectorSet(0, 0, 0, 0);
	bool cameraFound = false;

	registry.view<Camera, Transform>([&](Entity e, Camera& cam, Transform& trans)
	{
		if (cameraFound) return;

		eye = XMLoadFloat3(&trans.position);

		// 回転行列を作成 (Pitch: X軸回転, Yaw: Y軸回転)
		// Transform.rotation.x を Pitch(上下)、y を Yaw(左右) として使います
		XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(trans.rotation.x, trans.rotation.y, 0.0f);

		// 前方ベクトル (0, 0, 1) を回転させる
		lookDir = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotationMatrix);

		// 上方向ベクトル (0, 1, 0) を回転させる
		XMVECTOR upDir = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rotationMatrix);

		// LookToLH: 位置、向き、上でビュー行列を作る
		viewMatrix = XMMatrixLookToLH(eye, lookDir, upDir);

		projMatrix = XMMatrixPerspectiveFovLH(cam.fov, cam.aspect, cam.nearZ, cam.farZ);
		cameraFound = true;
	});

	if (!cameraFound) return;

	// メインシーン描画開始
	m_renderer->Begin(viewMatrix, projMatrix);

	// グリッドと軸の描画（Collider設定に関わらず出す）
	if (context.debug.showGrid) m_renderer->DrawGrid();
	if (context.debug.showAxis) m_renderer->DrawAxis();

	// 3. オブジェクト描画
	if (context.debug.showColliders)
	{
		m_renderer->SetFillMode(context.debug.wireframeMode);

		registry.view<Transform, BoxCollider>([&](Entity e, Transform& trans, BoxCollider& box) {
			XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			if (registry.has<Tag>(e))
			{
				const char* name = registry.get<Tag>(e).name;
				if (strcmp(name, "Player") == 0) color = { 0.0f, 1.0f, 0.0f, 1.0f };
				if (strcmp(name, "Enemy") == 0) color = { 1.0f, 0.0f, 0.0f, 1.0f };
			}

			m_renderer->DrawBox(trans.position, box.size, color);
		});
	}

	// ------------------------------------------------------------
	// 2. シーンギズモの描画
	// ------------------------------------------------------------
	if (context.debug.showAxis)
	{
		// A. 現在のビューポートを保存
		UINT numViewports = 1;
		D3D11_VIEWPORT oldViewport;
		// 一旦簡易版
		oldViewport.Width = static_cast<float>(Config::SCREEN_WIDTH);
		oldViewport.Height = static_cast<float>(Config::SCREEN_HEIGHT);
		oldViewport.MinDepth = 0.0f;
		oldViewport.MaxDepth = 1.0f;
		oldViewport.TopLeftX = 0;
		oldViewport.TopLeftY = 0;

		// B. 右上用の新しいビューポートを作成
		float gizmoSize = 100.0f;	// 100x100ピクセル
		float padding = 20.0f;		// 画面端からの隙間

		D3D11_VIEWPORT gizmoViewport = {};
		gizmoViewport.Width = gizmoSize;
		gizmoViewport.Height = gizmoSize;
		gizmoViewport.MinDepth = 0.0f;
		gizmoViewport.MaxDepth = 1.0f;
		// 右上座標: (画面幅 - サイズ - 隙間, 隙間)
		gizmoViewport.TopLeftX = Config::SCREEN_WIDTH - gizmoSize - padding;
		gizmoViewport.TopLeftY = padding;

		// ビューポート適用
		m_renderer->GetDeviceContext()->RSSetViewports(1, &gizmoViewport);

		// C.ギズモ用のビュー行列
		// カメラの位置からターゲットへのベクトルを計算
		XMVECTOR dir = XMVectorSubtract(eye, lookDir);

		// 向きはそのまま、距離を一定に正規化して原点中心に配置
		dir = XMVector3Normalize(dir) * 5.0f;

		// ギズモカメラの位置 = 原点 + 方向ベクトル
		XMVECTOR gizmoEye = dir;
		XMVECTOR gizmoTarget = XMVectorSet(0, 0, 0, 0);
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);

		XMMATRIX gizmoView = XMMatrixLookAtLH(gizmoEye, gizmoTarget, up);

		// ギズモ用のプロジェクション
		XMMATRIX gizmoProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.0f, 0.1f, 100.0f);

		// D. ギズモ描画開始
		m_renderer->Begin(gizmoView, gizmoProj);
		m_renderer->SetFillMode(false);

		// 軸の描画 (X:赤, Y:緑, Z:青)
		float len = 1.5f;
		// X軸
		m_renderer->DrawArrow(XMFLOAT3(0, 0, 0), XMFLOAT3(len, 0, 0), XMFLOAT4(1, 0, 0, 1));
		// Y軸
		m_renderer->DrawArrow(XMFLOAT3(0, 0, 0), XMFLOAT3(0, len, 0), XMFLOAT4(0, 1, 0, 1));
		// Z軸
		m_renderer->DrawArrow(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, len), XMFLOAT4(0, 0, 1, 1));

		// 中央のボックス（ピボット）
		m_renderer->DrawBox(XMFLOAT3(0, 0, 0), XMFLOAT3(0.7f, 0.7f, 0.7f), XMFLOAT4(0.8f, 0.8f, 0.8f, 1));

		// E. ビューポートを元に戻す
		m_renderer->SetFillMode(true);
		m_renderer->GetDeviceContext()->RSSetViewports(1, &oldViewport);
	}
}