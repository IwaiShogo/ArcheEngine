/*****************************************************************//**
 * @file	SystemWindow.h
 * @brief	システムウィンドウ
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/27	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SYSTEM_WINDOW_H___
#define ___SYSTEM_WINDOW_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Editor/Core/Editor.h"
#include "Engine/Core/Time.h"
#include "Engine/Core/Context.h"
#include "Engine/Core/Input.h"
#include "Engine/Scene/SceneManager.h"

class SystemWindow
	: public EditorWindow
{
public:
	void Draw(World& world, Entity& selected, Context& ctx) override
	{
		Registry& reg = world.getRegistry();
		
		ImGui::Begin("System");	// ウィンドウ作成

		ImGui::Checkbox("Show FPS", &ctx.debug.showFps);

		// 1. FPSと時間
		if (ctx.debug.showFps)
		{
			// ------------------------------------------------------------
			// FPS Graph
			// ------------------------------------------------------------
			static float values[90] = {};
			static int values_offset = 0;
			static float refresh_time = 0.0f;

			// 高速更新しすぎると見にくいので少し間引く
			if (refresh_time == 0.0f) refresh_time = static_cast<float>(ImGui::GetTime());
			while (refresh_time < ImGui::GetTime())
			{
				values[values_offset] = ImGui::GetIO().Framerate;
				values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
				refresh_time += 1.0f / 60.0f;
			}

			// グラフ描画
			// (ラベル, 配列, 数, オフセット, オーバーレイ文字, 最小Y, 最大Y, サイズ）
			ImGui::PlotLines("FPS", values, IM_ARRAYSIZE(values), values_offset, nullptr, 0.0f, 200.0f, ImVec2(0, 80));
			ImGui::Text("Avg: %.1f", ImGui::GetIO().Framerate);

			ImGui::Separator();

			ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
		}
		ImGui::Text("Total Time: %.2f s", Time::TotalTime());
		ImGui::Separator();

		// 2. 表示切替
		if (ImGui::CollapsingHeader("Display Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Separator();
			ImGui::Checkbox("Show Grid", &ctx.debug.showGrid);
			ImGui::Checkbox("Show Axis", &ctx.debug.showAxis);
			ImGui::Checkbox("Show Sound Events", &ctx.debug.showSoundLocation);
			ImGui::Checkbox("Enable Mouse Picking", &ctx.debug.enableMousePicking);

			ImGui::Separator();
			ImGui::Checkbox("Debug Camera Mode", &ctx.debug.useDebugCamera);
			ImGui::Checkbox("Show Colliders", &ctx.debug.showColliders);
			// コライダー表示中のみ有効なサブ設定
			if (ctx.debug.showColliders)
			{
				ImGui::Indent();
				if (ImGui::RadioButton("Wireframe", ctx.debug.wireframeMode)) ctx.debug.wireframeMode = true;
				ImGui::SameLine();
				if (ImGui::RadioButton("Solid", !ctx.debug.wireframeMode)) ctx.debug.wireframeMode = false;
				ImGui::Unindent();
			}
		}

		// 入力デバッグ情報の表示
		if (ImGui::CollapsingHeader("Input Visualizer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 1. 接続状態
			bool connected = Input::IsControllerConnected();
			ImGui::Text("Controller: %s", connected ? "Connected" : "Disconnected");

			if (connected)
			{
				// ------------------------------------------------------------
				// スティック描画用ヘルパー関数
				// ------------------------------------------------------------
				auto DrawStick = [](const char* label, float x, float y)
					{
						ImGui::BeginGroup();	// グループ化して横並びにしやすくする
						ImGui::Text("%s", label);

						// 枠
						ImDrawList* drawList = ImGui::GetWindowDrawList();
						ImVec2 p = ImGui::GetCursorScreenPos();
						float size = 80.0f;

						drawList->AddRect(p, ImVec2(p.x + size, p.y + size), IM_COL32(255, 255, 255, 255));

						// 十字線
						ImVec2 center(p.x + size * 0.5f, p.y + size * 0.5f);
						drawList->AddLine(ImVec2(center.x, p.y), ImVec2(center.x, p.y + size), IM_COL32(100, 100, 100, 100));
						drawList->AddLine(ImVec2(p.x, center.y), ImVec2(p.x + size, center.y), IM_COL32(100, 100, 100, 100));

						// 現在の位置の点（赤色）
						float dotX = center.x + (x * (size * 0.5f));
						float dotY = center.y - (y * (size * 0.5f));
						drawList->AddCircleFilled(ImVec2(dotX, dotY), 4.0f, IM_COL32(255, 0, 0, 255));

						// スペース確保
						ImGui::Dummy(ImVec2(size, size));

						// 数値表示
						ImGui::Text("X: % .2f", x);
						ImGui::Text("Y: % .2f", y);
						ImGui::EndGroup();
					};

				// ------------------------------------------------------------
				// 描画実行
				// ------------------------------------------------------------
				float lx = Input::GetAxis(Axis::Horizontal);
				float ly = Input::GetAxis(Axis::Vertical);
				float rx = Input::GetAxis(Axis::RightHorizontal);
				float ry = Input::GetAxis(Axis::RightVertical);

				DrawStick("L Stick", lx, ly);
				ImGui::SameLine(0, 20);	// 横に並べる
				DrawStick("R Stick", rx, ry);

				ImGui::Separator();

				// 3. ボタン入力の可視化
				ImGui::Text("Buttons:");
				// ヘルパーラムダ式
				auto DrawBtn = [&](const char* label, Button btn)
					{
						if (Input::GetButton(btn)) ImGui::TextColored(ImVec4(0, 1, 0, 1), "[%s]", label);
						else ImGui::TextDisabled("[%s]", label);
						ImGui::SameLine();
					};

				DrawBtn("A", Button::A);
				DrawBtn("B", Button::B);
				DrawBtn("X", Button::X);
				DrawBtn("Y", Button::Y);
				ImGui::NewLine();
				DrawBtn("LB", Button::LShoulder);
				DrawBtn("RB", Button::RShoulder);
				DrawBtn("START", Button::Start);
				ImGui::NewLine();
			}
		}

		// 3. ゲーム進行制御
		if (ImGui::CollapsingHeader("Game Control", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// タイムスケール操作
			ImGui::Text("Time Scale");
			ImGui::SliderFloat("##TimeScale", &Time::timeScale, 0.0f, 15.0f, "%.1fx");

			// --- 一時停止 / 再開 ---
			if (Time::isPaused)
			{
				// 停止中なので「再開」ボタン
				if (ImGui::Button("Resume"))
				{
					Time::isPaused = false;
				}
				ImGui::SameLine();

				// --- コマ送り（停止中のみ表示）---
				if (ImGui::Button("Step Frame (+1F)"))
				{
					Time::StepFrame();
				}
			}
			else
			{
				// 動作中なので「一時停止」ボタン
				if (ImGui::Button("Pause"))
				{
					Time::isPaused = true;
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Reset Speed"))
			{
				Time::timeScale = 1.0f;
			}

			// リスタート
			if (ImGui::Button("Restart Scene", ImVec2(-1, 0)))
			{
				auto sceneManager = SceneManager::Instance();
				sceneManager.ChangeScene(sceneManager.GetCurrentSceneName());
			}
		}

		// 4. シーン遷移
		if (ImGui::CollapsingHeader("Scene Transition"))
		{
			if (ImGui::Button("Title", ImVec2(100, 0)))
			{
				SceneManager::Instance().ChangeScene("Title");
			}
			ImGui::SameLine();
			if (ImGui::Button("Game", ImVec2(100, 0)))
			{
				SceneManager::Instance().ChangeScene("Game");
			}
		}

		ImGui::Separator();

		// --------------------------------------------------------
		// システム稼働状況 (System Monitor)
		// --------------------------------------------------------
		if (ImGui::CollapsingHeader("System Monitor", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Columns(2, "systems");
			ImGui::Text("System Name"); ImGui::NextColumn();
			ImGui::Text("Load (16ms)"); ImGui::NextColumn();
			ImGui::Separator();

			// world.getSystems() でシステム一覧を取得
			for (const auto& sys : world.getSystems())
			{
				ImGui::Text("%s", sys->m_systemName.c_str());
				ImGui::NextColumn();

				float timeMs = (float)sys->m_lastExecutionTime;
				float fraction = timeMs / 16.0f;
				char buf[32];
				sprintf_s(buf, "%.3f ms", timeMs);

				ImVec4 col = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
				if (timeMs > 2.0f) col = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
				if (timeMs > 10.0f) col = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
				ImGui::ProgressBar(fraction, ImVec2(-1, 0), buf);
				ImGui::PopStyleColor();

				ImGui::NextColumn();
			}
			ImGui::Columns(1);
		}

		// --------------------------------------------------------
		// エンティティリスト (Entity List - Flat View)
		// ※階層構造(Hierarchy)があるので必須ではないですが、デバッグ用に残します
		// --------------------------------------------------------
		if (ImGui::CollapsingHeader("Entity List (Flat)", ImGuiTreeNodeFlags_DefaultOpen))
		{
			int count = 0;
			reg.view<Tag>().each([&](Entity e, Tag& tag) {
				count++;
				// クリックで選択可能にする
				if (ImGui::Selectable((std::to_string(e) + ": " + tag.name.c_str()).c_str(), selected == e)) {
					selected = e;
				}
				});
			ImGui::Separator();
			ImGui::Text("Total Entities: %d", count);
		}

		// --------------------------------------------------------
		// プレイヤー詳細情報 (Player Watcher)
		// --------------------------------------------------------
		if (ImGui::CollapsingHeader("Player Watcher", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool playerFound = false;
			reg.view<Tag>().each([&](Entity e, Tag& tag) {
				if (!playerFound && tag.name == "Player") {
					playerFound = true;

					ImGui::Text("ID: %d", e);
					if (reg.has<Transform>(e)) {
						auto& t = reg.get<Transform>(e);
						ImGui::Text("Pos: (%.2f, %.2f, %.2f)", t.position.x, t.position.y, t.position.z);
					}
					if (reg.has<Rigidbody>(e)) {
						auto& rb = reg.get<Rigidbody>(e);
						float speed = std::sqrt(rb.velocity.x * rb.velocity.x + rb.velocity.z * rb.velocity.z);
						ImGui::ProgressBar(speed / 10.0f, ImVec2(0, 0), "Speed");
					}
				}
				});

			if (!playerFound) ImGui::TextDisabled("Player Not Found");
		}

		// --------------------------------------------------------
		// デバッグカメラ制御 (ここに追加)
		// --------------------------------------------------------
		if (ctx.debug.useDebugCamera)
		{
			// MainCameraを探す
			reg.view<Tag, Transform, Camera>().each([&](Entity e, Tag& tag, Transform& t, Camera& c) {
				if (tag.name == "MainCamera")
				{
					bool isRightClicking = Input::GetMouseRightButton();

					// ImGui操作中は動かさない
					if (isRightClicking ||!ImGui::GetIO().WantCaptureMouse)
					{
						// 1. マウス回転 (右クリック)
						if (Input::GetMouseRightButton()) {
							float rotSpeed = 0.005f;
							t.rotation.y += Input::GetMouseDeltaX() * rotSpeed;
							t.rotation.x += Input::GetMouseDeltaY() * rotSpeed;
						}

						// 2. 移動 (WASD)
						float moveSpeed = 10.0f * Time::DeltaTime();
						if (Input::GetKey(VK_SHIFT)) moveSpeed *= 3.0f;

						using namespace DirectX;
						XMMATRIX rotM = XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, 0.0f);
						XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotM);
						XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotM);
						//XMVECTOR up = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rotM); // カメラ基準の上
						XMVECTOR up = XMVectorSet(0, 1, 0, 0);

						XMVECTOR pos = XMLoadFloat3(&t.position);

						if (Input::GetKey('W')) pos += forward * moveSpeed;
						if (Input::GetKey('S')) pos -= forward * moveSpeed;
						if (Input::GetKey('D')) pos += right * moveSpeed;
						if (Input::GetKey('A')) pos -= right * moveSpeed;
						if (Input::GetKey('E')) pos += up * moveSpeed;	   // 上昇
						if (Input::GetKey('Q')) pos -= up * moveSpeed;	   // 下降

						XMStoreFloat3(&t.position, pos);
					}
				}
				});
		}

		ImGui::End();
	}
};

#endif // !___SYSTEM_WINDOW_H___