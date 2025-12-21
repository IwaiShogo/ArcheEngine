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
#include "Engine/pch.h"
#define NOMINMAX
#include "Engine/Systems/Graphics/RenderSystem.h"
#include "Engine/Audio/AudioManager.h"
#include "Engine/Renderer/Renderers/SpriteRenderer.h"
#include "Engine/Renderer/Renderers/BillboardRenderer.h"

void RenderSystem::Render(Registry& registry, const Context& context)
{
	if (!m_renderer) return;

	// ------------------------------------------------------------
	// 1. メインの描画
	// ------------------------------------------------------------

	// 1. カメラ探索
	XMMATRIX viewMatrix = XMMatrixIdentity();
	XMMATRIX projMatrix = XMMatrixIdentity();
	static XMFLOAT3 savedRotation = { 0, 0, 0 };
	bool cameraFound = false;

	registry.view<Camera, Transform>().each([&](Entity e, Camera& cam, Transform& trans)
	{
		if (cameraFound) return;

		savedRotation = trans.rotation;

		XMVECTOR eye = XMLoadFloat3(&trans.position);
		// 回転行列を作成 (Pitch: X軸回転, Yaw: Y軸回転)
		// Transform.rotation.x を Pitch(上下)、y を Yaw(左右) として使います
		XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(trans.rotation.x, trans.rotation.y, 0.0f);
		// 前方ベクトル (0, 0, 1) を回転させる
		XMVECTOR lookDir = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotationMatrix);
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

		registry.view<Transform, Collider>().each([&](Entity e, Transform& t, Collider& c)
			{
				XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
				if (registry.has<Tag>(e))
				{
					std::string name = registry.get<Tag>(e).name.c_str();
					if (name == "Player") color = { 0.0f, 1.0f, 0.0f, 1.0f };
					if (name == "Enemy") color = { 1.0f, 0.0f, 0.0f, 1.0f };
				}

				// ワールド座標計算
				XMVECTOR scale, rot, pos;
				XMMatrixDecompose(&scale, &rot, &pos, t.GetWorldMatrix());

				XMFLOAT3 gPos, gScale;
				XMStoreFloat3(&gPos, pos);
				XMStoreFloat3(&gScale, scale);
				XMFLOAT4 gRot;
				XMStoreFloat4(&gRot, rot);
				
				// オフセット運用
				XMVECTOR offsetVec = XMLoadFloat3(&c.offset);
				XMVECTOR centerVec = XMVector3Transform(offsetVec, t.GetWorldMatrix());
				XMFLOAT3 center;
				XMStoreFloat3(&center, centerVec);

				// タイプ別描画
				if (c.type == ColliderType::Box)
				{
					XMFLOAT3 size = { c.boxSize.x * gScale.x, c.boxSize.y * gScale.y, c.boxSize.z * gScale.z };
					m_renderer->DrawBox(center, size, gRot, color);
				}
				else if (c.type == ColliderType::Sphere)
				{
					// スケールの最大値を半径に適用
					float maxScale = std::max({ gScale.x, gScale.y, gScale.z });
					m_renderer->DrawSphere(center, c.radius * maxScale, color);
				}
				else if (c.type == ColliderType::Capsule) {
					// 半径: スケールのXZ最大値, 高さ: Yスケール
					float maxScaleXZ = std::max(gScale.x, gScale.z);
					m_renderer->DrawCapsule(center, c.radius * maxScaleXZ, c.height * gScale.y, gRot, color);
				}
				else if (c.type == ColliderType::Cylinder) {
					float maxScaleXZ = std::max(gScale.x, gScale.z);
					m_renderer->DrawCylinder(center, c.radius * maxScaleXZ, c.height * gScale.y, gRot, color);
				}
			});
	}

	// -------------------------------------------------------
	// 音源の可視化 (Billboard版)
	// -------------------------------------------------------
	if (context.debug.showSoundLocation && context.billboardRenderer) {

		// ビルボード描画開始 (カメラ行列を渡す)
		context.billboardRenderer->Begin(viewMatrix, projMatrix);

		// アイコン画像を取得 (assets.json または LoadTexture で指定したキー)
		// ※とりあえず "player" か、用意した "icon_sound" を指定
		auto iconTex = ResourceManager::Instance().GetTexture("star");

		// 画像がなければ "player" などで代用
		if (!iconTex) iconTex = ResourceManager::Instance().GetTexture("star");

		if (iconTex) {
			// 記録されている音イベントをループ
			for (const auto& evt : AudioManager::Instance().GetSoundEvents()) {

				// ビルボードを描画
				// 位置: evt.position
				// サイズ: 1.0f x 1.0f (3D空間での1メートル四方)
				// 色: 赤っぽくして目立たせる
				context.billboardRenderer->Draw(
					iconTex.get(),
					evt.position,
					1.0f, 1.0f,
					{ 1.0f, 0.5f, 0.5f, 1.0f }
				);
			}
		}

		// ステートの後始末（深度書き込みなどを元に戻す必要がある場合）
		// BillboardRendererは半透明(Blend)を使うので、最後にBlendStateを切ると安全です
		m_renderer->GetDeviceContext()->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}

	// ------------------------------------------------------------
	// 2. シーンギズモの描画
	// ------------------------------------------------------------
	if (context.debug.showAxis)
	{
		// A. 現在のビューポートを保存
		UINT numViewports = 1;
		D3D11_VIEWPORT oldViewport;
		m_renderer->GetDeviceContext()->RSGetViewports(&numViewports, &oldViewport);

		// B. 右上用の新しいビューポートを作成
		float gizmoSize = 100.0f;	// 100x100ピクセル
		float padding = 20.0f;		// 画面端からの隙間

		D3D11_VIEWPORT gizmoViewport = {};
		gizmoViewport.Width = gizmoSize;
		gizmoViewport.Height = gizmoSize;
		gizmoViewport.MinDepth = 0.0f;
		gizmoViewport.MaxDepth = 1.0f;

		// Config::SCREEN_WIDTH ではなく、現在のビューポート（oldViewport）の幅を使う！
		// これにより、Sceneビューのサイズが変わっても、その中の右上に正しく表示されます
		gizmoViewport.TopLeftX = oldViewport.Width - gizmoSize - padding;
		gizmoViewport.TopLeftY = padding;

		// ビューポート適用
		m_renderer->GetDeviceContext()->RSSetViewports(1, &gizmoViewport);

		// C.ギズモ用のビュー行列
		XMMATRIX gizmoRotMatrix = XMMatrixRotationRollPitchYaw(savedRotation.x, savedRotation.y, 0.0f);

		// 2. ギズモカメラの位置計算
		XMVECTOR offset = XMVector3TransformCoord(XMVectorSet(0, 0, -5.0f, 0), gizmoRotMatrix);
		XMVECTOR gizmoEye = offset;
		XMVECTOR gizmoTarget = XMVectorSet(0, 0, 0, 0);

		// 上方向も回転させる
		XMVECTOR gizmoUp = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), gizmoRotMatrix);
		XMMATRIX gizmoView = XMMatrixLookAtLH(gizmoEye, gizmoTarget, gizmoUp);
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
		m_renderer->DrawBox(XMFLOAT3(0, 0, 0), XMFLOAT3(0.7f, 0.7f, 0.7f), XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0.8f, 0.8f, 0.8f, 1));

		// E. ビューポートを元に戻す
		m_renderer->SetFillMode(true);
		m_renderer->GetDeviceContext()->RSSetViewports(1, &oldViewport);
	}
}
