/*****************************************************************//**
 * @file	SceneViewPanel.h
 * @brief	シーンビューの表示とマウスピック処理
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___SCENE_VIEW_PANEL_H___
#define ___SCENE_VIEW_PANEL_H___

#ifdef _DEBUG

// ===== インクルード =====
#include "Engine/pch.h"
#include "Editor/Core/Editor.h"
#include "Editor/Tools/GizmoSystem.h"
#include "Editor/Tools/EditorCamera.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Window/Input.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Scene/Systems/Physics/CollisionSystem.h"

namespace Arche
{
	class SceneViewPanel
	{
	public:
		// コンストラクタ：カメラ初期化
		SceneViewPanel()
		{
			m_camera.Initialize(XM_PIDIV4, (float)Config::SCREEN_WIDTH / (float)Config::SCREEN_HEIGHT, 0.1f, 1000.0f);
		}

		void Draw(World& world, Entity& selectedEntity)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			bool open = ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

			if (!open)
			{
				ImGui::End();
				ImGui::PopStyleVar();
				return;
			}

			// 1. サイズチェック、必要ならリサイズ
			// ------------------------------------------------------------
			ImVec2 panelSize = ImGui::GetContentRegionAvail();

			// 最小サイズガード
			if (panelSize.x < 1.0f || panelSize.y < 1.0f)
			{
				ImGui::End();
				ImGui::PopStyleVar();
				return;
			}

			// 利用可能サイズを整数化し、誤差によるはみだしを防ぐ
			panelSize.x = std::floor(panelSize.x);
			panelSize.y = std::floor(panelSize.y);

			float targetAspect = (float)Config::SCREEN_WIDTH / (float)Config::SCREEN_HEIGHT;
			// 描画する画像サイズ
			ImVec2 imageSize = panelSize;

			// パネルの方が横長なら、横幅を制限（左右に黒帯）
			if (panelSize.x / panelSize.y > targetAspect)
			{
				imageSize.x = std::floor(panelSize.y * targetAspect);
				imageSize.y = panelSize.y;
			}
			// パネルの方が縦長なら、高さを制限（上下に黒帯）
			else
			{
				imageSize.x = panelSize.x;
				imageSize.y = std::floor(panelSize.x / targetAspect);
			}
			
			// 中央揃えのためのオフセット計算
			ImVec2 cursorStart = ImGui::GetCursorScreenPos();
			float offsetX = (panelSize.x - imageSize.x) * 0.5f;
			float offsetY = (panelSize.y - imageSize.y) * 0.5f;

			// 実際に画像を描画する左上座標
			ImVec2 imageStart = ImVec2(cursorStart.x + offsetX, cursorStart.y + offsetY);

			// カーソル位置を移動（これでImageが中央に出る）
			ImGui::SetCursorScreenPos(imageStart);

			// 2. リサイズ処理（計算したimageSizeに合わせる）
			// ------------------------------------------------------------
			static ImVec2 lastSize = { 0, 0 };
			if ((int)imageSize.x != (int)lastSize.x || (int)imageSize.y != (int)lastSize.y)
			{
				// 作り直す
				Application::Instance().ResizeSceneRenderTarget((float)imageSize.x, (float)imageSize.y);
				lastSize = imageSize;
			}

			// ウィンドウがホバーされている時のみ操作可能にする
			if (ImGui::IsWindowHovered())
			{
				m_camera.SetAspect(imageSize.x / imageSize.y);
				m_camera.Update();
			}

			// 3. 画像描画
			// ------------------------------------------------------------
			auto srv = Application::Instance().GetSceneSRV();
			if (srv) {
				ImGui::Image((void*)srv, imageSize);
			}

			// 描画直後のアイテム（画像）の情報を取得
			ImVec2 imageMin = ImGui::GetItemRectMin();	// 画像の左上（絶対座標）

			// 4. カメラ情報の取得
			// ------------------------------------------------------------
			XMMATRIX viewMat = m_camera.GetViewMatrix();
			XMMATRIX projMat = m_camera.GetProjectionMatrix();
			bool cameraFound = true;	// デバッグカメラは常に存在する

			// 5. ギズモ描画
			// ------------------------------------------------------------
			if (cameraFound && selectedEntity != NullEntity)
			{
				// ImGuizmoの準備
				ImGuizmo::SetOrthographic(false);
				ImGuizmo::SetDrawlist();

				// 計算したスクリーン座標とサイズをセット
				ImGuizmo::SetRect(imageStart.x, imageStart.y, imageSize.x, imageSize.y);

				// ギズモ描画実行
				GizmoSystem::Draw(world.getRegistry(), selectedEntity, viewMat, projMat, imageStart.x, imageStart.y, imageSize.x, imageSize.y);
			}

			// 6. マウスピッキング
			// ------------------------------------------------------------
			if (!ImGuizmo::IsUsing() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				// 1. マウスの相対座標
				ImVec2 mousePos = ImGui::GetMousePos();
				float x = mousePos.x - imageMin.x;
				float y = mousePos.y - imageMin.y;

				// 画像内かチェック
				if (x >= 0 && x < imageSize.x && y >= 0 && y < imageSize.y)
				{
					// ====================================================
					// 2D UI Picking
					// ====================================================
					// マウス座標を中心基準(Y-Up)に変換
					float halfW = imageSize.x * 0.5f;
					float halfH = imageSize.y * 0.5f;

					// スクリーン座標系(左上0,0) -> 中心基準
					float uiMouseX = x - halfW;
					float uiMouseY = halfH - y; // Y反転

					// スケール補正（実際の画面解像度と、シーンビュー表示サイズの比率）
					float scaleX = (float)Config::SCREEN_WIDTH / imageSize.x;
					float scaleY = (float)Config::SCREEN_HEIGHT / imageSize.y;
					uiMouseX *= scaleX;
					uiMouseY *= scaleY;

					bool uiHit = false;
					Entity uiHitEntity = NullEntity;

					// 手前（ZIndexが大きい、または階層が後）から判定したい
					// viewは順方向なので、vectorに入れて逆順で回すのが確実
					std::vector<Entity> uiEntities;
					world.getRegistry().view<Transform2D>().each([&](Entity e, Transform2D& t) {
						uiEntities.push_back(e);
						});

					// 逆順ループ
					for (auto it = uiEntities.rbegin(); it != uiEntities.rend(); ++it)
					{
						Entity e = *it;
						// アクティブ・有効チェック
						//if (world.getRegistry().has<Active>(e) && !world.getRegistry().get<Active>(e).isActive) continue;
						//if (!world.getRegistry().get<Transform2D>(e).enabled) continue;
						if (world.getRegistry().has<Canvas>(e)) continue;
						auto& t = world.getRegistry().get<Transform2D>(e);
						auto& r = t.calculatedRect; // {left, top, right, bottom} (Center/Y-Up Coordinates)

						// 矩形判定 (Y-Upなので Top > Bottom)
						// calculatedRectはUISystemで「Y-Up」で計算されている前提
						if (uiMouseX >= r.x && uiMouseX <= r.z &&
							uiMouseY <= r.y && uiMouseY >= r.w) // Top(y) > MouseY > Bottom(w)
						{
							uiHitEntity = e;
							uiHit = true;
							break; // 手前でヒットしたら終了
						}
					}

					// 結果反映
					if (uiHit)
					{
						selectedEntity = uiHitEntity;
						Logger::Log("Selected UI: " + std::to_string(uiHitEntity));
					}
					else
					{
						// UIがなければ3D判定へ
						// 2. Unproject用のレイ作成
						XMVECTOR rayOrigin = m_camera.GetPosition();

						// Viewport行列（Unproject用）: X, Y, W, H, MinZ, MaxZ
						XMVECTOR rayTarget = XMVector3Unproject(
							XMVectorSet(x, y, 1.0f, 0.0f), // スクリーン座標 (Z=1: Far平面)
							0, 0, imageSize.x, imageSize.y, 0.0f, 1.0f,
							projMat, viewMat, XMMatrixIdentity()
						);

						XMVECTOR rayDir = XMVector3Normalize(rayTarget - rayOrigin);

						XMFLOAT3 origin, dir;
						XMStoreFloat3(&origin, rayOrigin);
						XMStoreFloat3(&dir, rayDir);


						// 3. レイキャスト実行
						float dist = 0.0f;
						Entity hit = CollisionSystem::Raycast(world.getRegistry(), origin, dir, dist);

						if (hit != NullEntity) {
							selectedEntity = hit;
						}
						else {
							//selectedEntity = NullEntity;
						}
					}
				}
			}

			ImGui::End();
			ImGui::PopStyleVar();
		}

		EditorCamera& GetCamera() { return m_camera; }

	private:
		EditorCamera m_camera;
	};
}

#endif // _DEBUG

#endif // !___SCENE_VIEW_PANEL_H___