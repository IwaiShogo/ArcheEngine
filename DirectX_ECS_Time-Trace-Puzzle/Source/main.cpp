/*****************************************************************//**
 * @file	main.cpp
 * @brief	プログラムのエントリーポイント
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
#include "main.h"
#include "Engine/Core/Application.h"

// ゲーム固有のシーンをインクルード
#include "Game/Scenes/SceneTitle.h"
#include "Game/Scenes/SceneGame.h"

// ImGuiのハンドラを定義（前方宣言）
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_IME_CHAR)
	{
		auto& io = ImGui::GetIO();
		DWORD wChar = (DWORD)wParam;

		// 1バイト文字（英数など）はそのまま
		if (wChar <= 127)
		{
			io.AddInputCharacter((unsigned int)wChar);
		}
		else
		{
			// 2バイト文字（Shift-JIS）の場合
			// Windowsからの入力はリトルエンディアンで来るため、バイト順を調整して変換します
			BYTE low = (BYTE)(wChar & 0x00FF);
			BYTE high = (BYTE)((wChar & 0xFF00) >> 8);

			// メモリ上の配置を Shift-JIS の並び順 (上位->下位) に合わせる
			WORD sortedChar = MAKEWORD(high, low);

			// Shift-JIS (CP_ACP) -> UTF-16 (wchar_t) に変換
			wchar_t wBuf[2] = { 0 };
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&sortedChar, 2, wBuf, 2);

			// UTF-16としてImGuiに渡す（ImGui内部でUTF-8に変換される）
			io.AddInputCharacter(wBuf[0]);
		}
		return 0; // ImGuiのデフォルト処理と重複しないようにここで終了
	}
	// ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲

	// ImGuiへのメッセージを渡す
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) return true;

	if (uMsg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * @brief	プログラムのエントリーポイント
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// ウィンドウクラス登録
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "TimeTracePuzzleClass";
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);	// マウスカーソルを表示
	RegisterClass(&wc);

	// ウィンドウ生成
	// 実際のクライアント領域を調整
	RECT rc = { 0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindowEx(
		0,
		"TimeTracePuzzleClass",
		Config::WINDOW_TITLE,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		nullptr, nullptr, hInstance, nullptr
	);

	if (!hwnd) return -1;

	ShowWindow(hwnd, SW_SHOW);

	// Applicationクラスの初期化
	Application app(hwnd);
	try
	{
		app.Initialize();

		auto& sceneManager = app.GetSceneManager();
		sceneManager.RegisterScene<SceneTitle>("Title");
		sceneManager.RegisterScene<SceneGame>("Game");

		// 最初のシーンを開始
		sceneManager.ChangeScene("Title");
	}
	catch (const std::exception& e)
	{
		MessageBox(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
		return -1;
	}

	// メッセージループ
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// アプリケーションループ実行
			app.Run();
		}
	}

	return 0;
}
