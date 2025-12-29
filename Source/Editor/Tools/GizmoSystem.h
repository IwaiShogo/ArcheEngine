/*****************************************************************//**
 * @file	GizmoSystem.h
 * @brief	ギズモの描画ロジック（2D / 3D）
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___GIZMO_SYSTEM_H___
#define ___GIZMO_SYSTEM_H___

#ifdef _DEBUG

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Core/Window/Input.h"
#include "Engine/Core/Context.h" // Config::SCREEN_WIDTH等用
#include "Engine/Config.h"

namespace Arche
{

	class GizmoSystem
	{
	public:
		static void Draw(Registry& reg, Entity& selected, const XMMATRIX& view, const XMMATRIX& proj, float x, float y, float width, float height)
		{
			if (selected == NullEntity) return;

			// =================================================================================
			// 1. Transform2D (UI / 2D) の場合
			// =================================================================================
			if (reg.has<Transform2D>(selected))
			{
				ImGuizmo::SetOrthographic(true); // 2Dモード
				ImGuizmo::SetDrawlist();
				ImGuizmo::SetRect(x, y, width, height);

				// --------------------------------------------------------
				// カメラ設定 (Z=-10から原点を見る)
				// --------------------------------------------------------
				XMVECTOR eye	= XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
				XMVECTOR at		= XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
				XMVECTOR up		= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				XMMATRIX view2D = XMMatrixLookAtLH(eye, at, up);

				float screenW = (float)Config::SCREEN_WIDTH;
				float screenH = (float)Config::SCREEN_HEIGHT;
				XMMATRIX proj2D = XMMatrixOrthographicLH(screenW, screenH, 0.1f, 1000.0f);

				float viewM[16], projM[16], worldM[16];
				MatrixToFloat16(view2D, viewM);
				MatrixToFloat16(proj2D, projM);

				auto& t2 = reg.get<Transform2D>(selected);

				// --------------------------------------------------------
				// ギズモに渡す行列の作成 (D2D WorldMatrix -> DirectX 4x4 Matrix)
				// --------------------------------------------------------
				// UISystemですでに計算された「正しい表示位置（ワールド行列）」を使います
				// これにより、ギズモがオブジェクトの見た目通りの場所に表示されます
				auto& m = t2.worldMatrix;
				XMMATRIX matWorld = XMMatrixSet(
					m._11, m._12, 0.0f, 0.0f,
					m._21, m._22, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					m._31, m._32, 0.0f, 1.0f
				);
				MatrixToFloat16(matWorld, worldM);

				// --------------------------------------------------------
				// 操作モード制御
				// --------------------------------------------------------
				static ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
				if (Input::GetKeyDown('1')) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
				if (Input::GetKeyDown('2')) mCurrentGizmoOperation = ImGuizmo::ROTATE;
				if (Input::GetKeyDown('3')) mCurrentGizmoOperation = ImGuizmo::SCALE;

				// --------------------------------------------------------
				// ギズモ操作実行
				// --------------------------------------------------------
				// 2D回転が軸ブレしないように LOCAL モード推奨ですが、UIなら WORLD でもOK
				// ここでは操作しやすさ優先で WORLD にします
				if (ImGuizmo::Manipulate(viewM, projM, mCurrentGizmoOperation, ImGuizmo::WORLD, worldM))
				{
					// 操作後の新しいワールド行列
					XMMATRIX newWorldMat = XMLoadFloat4x4((XMFLOAT4X4*)worldM);

					// ----------------------------------------------------
					// 親の行列を考慮して「ローカル座標」に戻す
					// ----------------------------------------------------
					XMMATRIX parentMat = XMMatrixIdentity();
					float anchorX = 0.0f;
					float anchorY = 0.0f;

					// 親矩形のデフォルト
					float halfW = screenW * 0.5f;
					float halfH = screenH * 0.5f;
					D2D1_RECT_F parentRect = { -halfW, halfH, halfW, -halfH };

					// 親がいるか確認
					if (reg.has<Relationship>(selected))
					{
						Entity parent = reg.get<Relationship>(selected).parent;
						if (parent != NullEntity && reg.has<Transform2D>(parent))
						{
							// 親のワールド行列を取得して 4x4 に変換
							auto& pm = reg.get<Transform2D>(parent).worldMatrix;
							parentMat = XMMatrixSet(
								pm._11, pm._12, 0.0f, 0.0f,
								pm._21, pm._22, 0.0f, 0.0f,
								0.0f, 0.0f, 1.0f, 0.0f,
								pm._31, pm._32, 0.0f, 1.0f
							);

							// 親のローカル矩形を計算 (UISystemと同じロジック)
							auto& pt = reg.get<Transform2D>(parent);
							float pl = -pt.size.x * pt.pivot.x;
							float pr = pl + pt.size.x;
							float pb = -pt.size.y * pt.pivot.y;
							float pt_top = pb + pt.size.y;
							parentRect = { pl, pt_top, pr, pb };
						}
					}

					// アンカー位置計算
					float pW = parentRect.right - parentRect.left;
					float pH = parentRect.bottom - parentRect.top;
					anchorX = parentRect.left + (pW * (t2.anchorMin.x + t2.anchorMax.x) * 0.5f);
					anchorY = parentRect.top + (pH * (t2.anchorMin.y + t2.anchorMax.y) * 0.5f);

					// --------------------------------------------------------
					// 変換: World -> Local -> Position
					// --------------------------------------------------------
					// 親の逆行列で「ローカル座標（アンカー込み）」に変換
					DirectX::XMVECTOR det;
					DirectX::XMMATRIX invParent = DirectX::XMMatrixInverse(&det, parentMat);
					DirectX::XMMATRIX localMat = newWorldMat * invParent;

					float localM[16];
					MatrixToFloat16(localMat, localM);

					float translation[3], rotation[3], scale[3];
					ImGuizmo::DecomposeMatrixToComponents(localM, translation, rotation, scale);

					// 分解された translation は「アンカー + position」なので、アンカーを引く
					t2.position.x = translation[0] - anchorX;
					t2.position.y = translation[1] - anchorY;

					t2.rotation.z = rotation[2];
					t2.scale.x = scale[0];
					t2.scale.y = scale[1];
				}
			}
			// 2. Transform（3D）の場合
			else if (reg.has<Transform>(selected))
			{
				ImGuizmo::SetOrthographic(false);	// 3Dモード（透視投影）
				ImGuizmo::SetDrawlist();
				ImGuizmo::SetRect(x, y, width, height);

				// 行列をfloat配列に変換 (Transpose含む)
				float viewM[16], projM[16], worldM[16];
				MatrixToFloat16(view, viewM);
				MatrixToFloat16(proj, projM);

				// 対象のTransformを取得
				Transform& t = reg.get<Transform>(selected);
				MatrixToFloat16(t.GetWorldMatrix(), worldM);

				// 操作モード
				static ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
				if (Input::GetKeyDown('1')) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
				if (Input::GetKeyDown('2')) mCurrentGizmoOperation = ImGuizmo::ROTATE;
				if (Input::GetKeyDown('3')) mCurrentGizmoOperation = ImGuizmo::SCALE;

				// ギズモ描画と操作判定
				if (ImGuizmo::Manipulate(viewM, projM, mCurrentGizmoOperation, ImGuizmo::LOCAL, worldM))
				{
					float translation[3], rotation[3], scale[3];
					ImGuizmo::DecomposeMatrixToComponents(worldM, translation, rotation, scale);

					t.position = { translation[0], translation[1], translation[2] };
					t.scale = { scale[0], scale[1], scale[2] };

					t.rotation.x = rotation[0];
					t.rotation.y = rotation[1];
					t.rotation.z = rotation[2];
				}
			}

		}

	private:
		// ヘルパー: XMMATRIX -> float[16]
		static void MatrixToFloat16(const XMMATRIX& m, float* out) {
			XMStoreFloat4x4((XMFLOAT4X4*)out, m);
		}
	};

}	// namespace Arche

#endif // _DEBUG

#endif // !___GIZMO_SYSTEM_H___