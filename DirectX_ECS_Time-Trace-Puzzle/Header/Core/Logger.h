/*****************************************************************//**
 * @file	Logger.h
 * @brief	ImGuiウィンドウに表示されるロガークラス
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/25	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___LOGGER_H___
#define ___LOGGER_H___

// ===== インクルード =====
#include <vector>
#include <string>
#include "imgui.h"

struct LogEntry
{
	std::string message;
	ImVec4 color;
};

class Logger
{
public:
	// 通常ログ
	static void Log(const std::string& message)
	{
		s_logs.push_back({ message, ImVec4(1, 1, 1, 1) });
		s_scrollToBottom = true;
	}

	// エラーログ（赤色）
	static void LogError(const std::string& message)
	{
		s_logs.push_back({ "[Error] " + message, ImVec4(1, 0.5f, 0.5f, 1) });
		s_scrollToBottom = true;
	}

	// ImGuiでの描画
	static void Draw(const char* title = "Log Console")
	{
		ImGui::Begin(title);

		if (ImGui::Button("Clear")) s_logs.clear();

		ImGui::Separator();

		// スクロール領域の作成
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		for (const auto& log : s_logs)
		{
			ImGui::TextColored(log.color, "%s", log.message.c_str());
		}

		// 自動スクロール
		if (s_scrollToBottom)
		{
			ImGui::SetScrollHereX(1.0f);
			s_scrollToBottom = false;
		}

		ImGui::EndChild();
		ImGui::End();
	}

private:
	inline static std::vector<LogEntry> s_logs;
	inline static bool s_scrollToBottom = false;
};

#endif // !___LOGGER_H___