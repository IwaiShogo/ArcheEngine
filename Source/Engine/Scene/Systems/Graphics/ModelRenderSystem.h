/*****************************************************************//**
 * @file	ModelRenderSystem.h
 * @brief	ModelRendererを使って描画を行うシステム
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

#ifndef ___MODEL_RENDER_SYSTEM_H___
#define ___MODEL_RENDER_SYSTEM_H___

// ===== インクルード =====
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Renderer/Renderers/ModelRenderer.h"
#include "Engine/Resource/ResourceManager.h"

namespace Arche
{

	class ModelRenderSystem
		: public ISystem
	{
	public:
		ModelRenderSystem()
		{
			m_systemName = "Model Render System";
		}

		void Render(Registry& registry, const Context& context) override
		{
			// 1. カメラ探索
			XMMATRIX viewMatrix = XMMatrixIdentity();
			XMMATRIX projMatrix = XMMatrixIdentity();
			XMVECTOR eye = XMVectorZero();
			XMFLOAT3 lightDir = { 0.5f, -1.0f, 0.5f };
			bool cameraFound = false;

			if (context.renderCamera.useOverride)
			{
				viewMatrix = XMLoadFloat4x4(&context.renderCamera.viewMatrix);
				projMatrix = XMLoadFloat4x4(&context.renderCamera.projMatrix);
				eye = XMLoadFloat3(&context.renderCamera.position);
				cameraFound = true;
			}
			else
			{
				registry.view<Camera, Transform>().each([&](Entity e, Camera& cam, Transform& trans)
					{
						if (cameraFound) return;

						eye = XMLoadFloat3(&trans.position);
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
			}

			if (!cameraFound) return;

			// 2. 描画開始
			ModelRenderer::Begin(viewMatrix, projMatrix, lightDir);
			// 3. MeshComponentとTransformを持つEntityを描画
			registry.view<MeshComponent, Transform>().each([&](Entity e, MeshComponent& m, Transform& t)
				{
					auto model = ResourceManager::Instance().GetModel(m.modelKey);
					if (model)
					{
						// 計算済みの GetWorldMatrix() を取得
						XMMATRIX world = t.GetWorldMatrix();

						// モデル固有のスケール補正 * Transformのスケール
						if (m.scaleOffset.x != 1.0f || m.scaleOffset.y != 1.0f || m.scaleOffset.z != 1.0f)
						{
							XMMATRIX preScale = XMMatrixScaling(m.scaleOffset.x, m.scaleOffset.y, m.scaleOffset.z);
							world = preScale * world;
						}

						// 描画
						ModelRenderer::Draw(model, world);
					}
				});
		}
	};

}	// namespace Arche

#include "Engine/Scene/Serializer/SystemRegistry.h"
ARCHE_REGISTER_SYSTEM(Arche::ModelRenderSystem, "Model Render System")

#endif // !___MODEL_RENDER_SYSTEM_H___