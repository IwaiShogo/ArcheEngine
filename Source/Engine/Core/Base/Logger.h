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
#include "Engine/Core/Time/Time.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Audio/AudioManager.h"

// ログの種類
enum class LogType
{
	Info,
	Warning,
	Error,
	Command,
};

struct LogEntry
{
	std::string message;
	LogType type;
	ImVec4 color;
};

class Logger
{
public:
	// --- ログ出力 ---
	static void Log(const std::string& message) {
		AddLog("[Info] " + message, LogType::Info, ImVec4(1, 1, 1, 1));
	}
	static void LogWarning(const std::string& message) {
		AddLog("[Warn] " + message, LogType::Warning, ImVec4(1, 1, 0, 1));
	}
	static void LogError(const std::string& message) {
		AddLog("[Error] " + message, LogType::Error, ImVec4(1, 0.5f, 0.5f, 1));
	}

	// コマンドハンドラ
	using CommandHandler = std::function<void(const std::vector<std::string>& args)>;

	static void RegisterCommand(const std::string& name, CommandHandler handler)
	{
		s_commands[name] = handler;
	}

	// --- 描画 & コマンド処理 ---
	static void Draw(const char* title = "Console") {
		ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin(title)) {
			ImGui::End();
			return;
		}

		// 1. ツールバー (フィルタリング & クリア)
		if (ImGui::Button("Clear")) s_logs.clear();
		ImGui::SameLine();
		ImGui::Checkbox("Info", &s_showInfo); ImGui::SameLine();
		ImGui::Checkbox("Warn", &s_showWarn); ImGui::SameLine();
		ImGui::Checkbox("Error", &s_showError);

		ImGui::Separator();

		// 2. ログ表示エリア (スクロール)
		// 下にコマンド入力欄を作るため、少し高さを残す
		float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

		for (const auto& log : s_logs) {
			if (log.type == LogType::Info && !s_showInfo) continue;
			if (log.type == LogType::Warning && !s_showWarn) continue;
			if (log.type == LogType::Error && !s_showError) continue;

			ImGui::TextColored(log.color, "%s", log.message.c_str());
		}

		// 自動スクロール
		if (s_scrollToBottom || (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
			ImGui::SetScrollHereY(1.0f);
			s_scrollToBottom = false;
		}

		ImGui::EndChild();
		ImGui::Separator();

		// 3. コマンド入力エリア
		bool reclaim_focus = false;
		ImGui::PushItemWidth(-1); // 幅いっぱい
		if (ImGui::InputText("##Input", s_inputBuf, IM_ARRAYSIZE(s_inputBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
			ExecuteCommand(s_inputBuf);
			s_inputBuf[0] = 0; // 入力欄クリア
			reclaim_focus = true;
		}
		ImGui::PopItemWidth();

		// フォーカスを維持（連続入力用）
		if (reclaim_focus) {
			ImGui::SetKeyboardFocusHere(-1);
		}

		ImGui::End();
	}

	static void ClearCommands() { s_commands.clear(); }

private:
	static void AddLog(const std::string& msg, LogType type, const ImVec4& color) {
		s_logs.push_back({ msg, type, color });
		s_scrollToBottom = true;
	}

	// --- コマンド実行ロジック ---
	static void ExecuteCommand(const std::string& commandLine) {
		AddLog("# " + commandLine, LogType::Command, ImVec4(0.7f, 0.7f, 0.7f, 1));

		std::stringstream ss(commandLine);
		std::string cmdName;
		ss >> cmdName;

		std::vector<std::string> args;
		std::string arg;
		while (ss >> arg) args.push_back(arg);

		// 登録されたコマンドを探して実行
		if (s_commands.count(cmdName)) {
			s_commands[cmdName](args);
		}
		else if (cmdName == "help") {
			Log("Available commands:");
			for (const auto& pair : s_commands) {
				Log(" - " + pair.first);
			}
		}
		else {
			LogError("Unknown command: " + cmdName);
		}
	}

	// コマンドリスト
	inline static std::map<std::string, CommandHandler> s_commands;

private:
	inline static std::vector<LogEntry> s_logs;
	inline static bool s_scrollToBottom = false;
	inline static bool s_showInfo = true;
	inline static bool s_showWarn = true;
	inline static bool s_showError = true;
	inline static char s_inputBuf[256] = {};
};

#endif // !___LOGGER_H___
