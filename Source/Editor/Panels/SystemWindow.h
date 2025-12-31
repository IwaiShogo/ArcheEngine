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

#ifdef _DEBUG

// ===== インクルード =====
#include "Engine/pch.h"
#include "Editor/Core/Editor.h"
#include "Engine/Core/Time/Time.h"
#include "Engine/Core/Context.h"
#include "Engine/Scene/Serializer/SystemRegistry.h"

namespace Arche
{

	class SystemWindow
		: public EditorWindow
	{
	public:
		void Draw(World& world, Entity& selected, Context& ctx) override
		{
			Registry& reg = world.getRegistry();

			ImGui::Begin("System Monitor");	// ウィンドウ作成

			// 1. FPS / 統計情報
			// ------------------------------------------------------------
			//if (ctx.debugSettings.showFps)
			//{
			//	// ------------------------------------------------------------
			//	// FPS Graph
			//	// ------------------------------------------------------------
			//	static float values[90] = {};
			//	static int values_offset = 0;
			//	static float refresh_time = 0.0f;

			//	// 高速更新しすぎると見にくいので少し間引く
			//	if (refresh_time == 0.0f) refresh_time = static_cast<float>(ImGui::GetTime());
			//	while (refresh_time < ImGui::GetTime())
			//	{
			//		values[values_offset] = ImGui::GetIO().Framerate;
			//		values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
			//		refresh_time += 1.0f / 60.0f;
			//	}

			//	// グラフ描画
			//	// (ラベル, 配列, 数, オフセット, オーバーレイ文字, 最小Y, 最大Y, サイズ）
			//	ImGui::PlotLines("FPS", values, IM_ARRAYSIZE(values), values_offset, nullptr, 0.0f, 200.0f, ImVec2(0, 80));
			//	ImGui::Text("Avg: %.1f", ImGui::GetIO().Framerate);

			//	ImGui::Separator();

			//	ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
			//}
			ImGui::Text("FPS: %.1f  (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
			ImGui::SameLine(200);
			ImGui::Text("Time: %.2f s", Time::TotalTime());
			ImGui::Separator();

			// 2. システム管理リスト
			// ------------------------------------------------------------
			ImGui::Text("Active System: ");

			// 追加ボタン
			ImGui::SameLine(ImGui::GetWindowWidth() - 110);
			if (ImGui::Button("Add System..."))
			{
				ImGui::OpenPopup("AddSystemPopup");
			}

			// システム追加ポップアップ
			if (ImGui::BeginPopup("AddSystemPopup"))
			{
				ImGui::TextDisabled("Available Systems");
				ImGui::Separator();

				// 登録済み全システムを表示
				for (auto& [name, creator] : SystemRegistry::Instance().GetCreators())
				{
					// 既に存在するかチェック
					bool exists = false;
					for (const auto& sys : world.getSystems()) if (sys->m_systemName == name) exists = true;

					if (!exists)
					{
						if (ImGui::Selectable(name.c_str()))
						{
							SystemRegistry::Instance().CreateSystem(world, name, SystemGroup::PlayOnly);
						}
					}
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();

			// システム一覧表示（テーブル）
			if (ImGui::BeginTable("SystemsTable", 3, ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 80.0f);
				ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableHeadersRow();

				for (const auto& sys : world.getSystems())
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					// 名前
					ImGui::Text("%s", sys->m_systemName.c_str());

					// コンテキストメニュー（削除）
					if (ImGui::BeginPopupContextItem(sys->m_systemName.c_str()))
					{
						if (ImGui::MenuItem("Remove System"))
						{
							Logger::LogWarning("Removing systems at runtime requires restart/reload to be safe.");
							// TODO: 本来はここで削除処理を入れるが、vector<unique_ptr>からの削除は慎重に行う必要がある
							// 標準システムは削除しない
						}
						ImGui::EndPopup();
					}

					ImGui::TableNextColumn();
					// グループ
					if (sys->m_group == SystemGroup::Always) ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Always");
					else if (sys->m_group == SystemGroup::PlayOnly) ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "PlayOnly");
					else ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "EditOnly");

					ImGui::TableNextColumn();
					// 処理時間
					float timeMs = (float)sys->m_lastExecutionTime;
					ImVec4 timeCol = (timeMs > 2.0f) ? ImVec4(1, 0.5f, 0, 1) : ImVec4(1, 1, 1, 1);
					ImGui::TextColored(timeCol, "%.2f ms", timeMs);
				}
				ImGui::EndTable();
			}

			// Content BrowserからのD&Dによるシステム追加
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					std::string path = (const char*)payload->Data;
					std::filesystem::path fpath(path);
					std::string name = fpath.stem().string(); // "PhysicsSystem.h" -> "PhysicsSystem"

					// 登録名と一致するかチェック（スペースの有無などに注意）
					// 簡易的に、登録名の中にファイル名が含まれていればOKとする等の柔軟性を持たせても良い
					// ここでは完全一致または " System" 付加で試行
					if (SystemRegistry::Instance().CreateSystem(world, name, SystemGroup::PlayOnly))
					{
						Logger::Log("Added System: " + name);
					}
					else if (SystemRegistry::Instance().CreateSystem(world, name + " System", SystemGroup::PlayOnly))
					{
						Logger::Log("Added System: " + name + " System");
					}
					else
					{
						Logger::LogWarning("Could not find registered system for: " + name);
					}
				}
				ImGui::EndDragDropTarget();
			}

			// 3. ゲームコントロール（Play中のみ表示）
			// ------------------------------------------------------------
			if (ctx.editorState == EditorState::Play)
			{
				ImGui::Separator();
				if (ImGui::CollapsingHeader("Game Control", ImGuiTreeNodeFlags_DefaultOpen))
				{
					// タイムスケール
					ImGui::SliderFloat("Time Scale", &Time::timeScale, 0.0f, 20.0f, "%.1fx");

					// 一時停止
					if (Time::isPaused) {
						if (ImGui::Button("Resume")) Time::isPaused = false;
						ImGui::SameLine();
						if (ImGui::Button("Step")) Time::StepFrame();
					}
					else {
						if (ImGui::Button("Pause")) Time::isPaused = true;
					}
				}
			}

			// 4. デバッグ表示設定
			// ------------------------------------------------------------
			if (ImGui::CollapsingHeader("Debug View"))
			{
				ImGui::Checkbox("Show Grid", &ctx.debugSettings.showGrid);
				ImGui::Checkbox("Show Axis", &ctx.debugSettings.showAxis);
				ImGui::Checkbox("Show Colliders", &ctx.debugSettings.showColliders);
				if (ctx.debugSettings.showColliders)
				{
					ImGui::SameLine();
					ImGui::Checkbox("Wireframe", &ctx.debugSettings.wireframeMode);
				}
				ImGui::Checkbox("Show Input Visualizer", &m_showInputDebug);
			}

			// --------------------------------------------------------
			// スクリプト プロファイラー
			// --------------------------------------------------------
			/*if (ImGui::CollapsingHeader("Script Profiler", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (NativeScriptSystem::s_scriptExecutionTimes.empty())
				{
					ImGui::TextDisabled("No scripts running.");
				}
				else
				{
					ImGui::Columns(2, "scripts");
					ImGui::Text("Script Name"); ImGui::NextColumn();
					ImGui::Text("Time (ms)"); ImGui::NextColumn();
					ImGui::Separator();

					for (auto& [name, time] : NativeScriptSystem::s_scriptExecutionTimes)
					{
						ImGui::Text("%s", name.c_str());
						ImGui::NextColumn();
						ImGui::Text("%.4f ms", time);
						ImGui::NextColumn();
					}
					ImGui::Columns(1);
				}
			}*/

			// 5. InputVisualizer
			if (m_showInputDebug)
			{
				DrawInputVisualizer();
			}
			ImGui::End();

			// --------------------------------------------------------
			// システム稼働状況 (System Monitor)
			// --------------------------------------------------------
			//if (ImGui::CollapsingHeader("System Monitor", ImGuiTreeNodeFlags_DefaultOpen))
			//{
			//	ImGui::Columns(2, "systems");
			//	ImGui::Text("System Name"); ImGui::NextColumn();
			//	ImGui::Text("Load (16ms)"); ImGui::NextColumn();
			//	ImGui::Separator();

			//	// world.getSystems() でシステム一覧を取得
			//	for (const auto& sys : world.getSystems())
			//	{
			//		ImGui::Text("%s", sys->m_systemName.c_str());
			//		ImGui::NextColumn();

			//		float timeMs = (float)sys->m_lastExecutionTime;
			//		float fraction = timeMs / 16.0f;
			//		char buf[32];
			//		sprintf_s(buf, "%.3f ms", timeMs);

			//		ImVec4 col = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
			//		if (timeMs > 2.0f) col = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
			//		if (timeMs > 10.0f) col = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

			//		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
			//		ImGui::ProgressBar(fraction, ImVec2(-1, 0), buf);
			//		ImGui::PopStyleColor();

			//		ImGui::NextColumn();
			//	}
			//	ImGui::Columns(1);
			//}
		}

	private:
		bool m_showInputDebug = false;

		void DrawInputVisualizer()
		{
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
		}
	};

}	// namespace Arche

#endif // _DEBUG

#endif // !___SYSTEM_WINDOW_H___