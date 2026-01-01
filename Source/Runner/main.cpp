#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include "Engine/Core/Application.h"

// 関数ポインタの型定義
typedef Arche::Application* (*CreateAppFunc)();

// グローバル変数
HMODULE g_sandboxModule = nullptr;
Arche::Application* g_app = nullptr;
std::filesystem::file_time_type g_lastWriteTime;	// 最終更新日時

// パス設定
const std::string DLL_NAME = "Sandbox.dll";
const std::string DLL_COPY_NAME = "Sandbox_Loaded.dll";
const std::string PDB_NAME = "Sandbox.pdb";
const std::string PDB_COPY_NAME = "Sandbox_Loaded.pdb";

// ヘルパー: 実行ファイル（EXE）のあるディレクトリパスを取得
// ============================================================
std::filesystem::path GetExeDirectory()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	return std::filesystem::path(buffer).parent_path();
}

// DLLのロード処理
// ============================================================
bool LoadGameDLL()
{
	// 1. 安全のため、古いDLLがあれば解放
	if (g_sandboxModule)
	{
		FreeLibrary(g_sandboxModule);
		g_sandboxModule = nullptr;
	}

	// パスの構築
	std::filesystem::path exeDir = GetExeDirectory();
	std::filesystem::path sourceDll = exeDir / DLL_NAME;
	std::filesystem::path copyDll = exeDir / DLL_COPY_NAME;
	std::filesystem::path sourcePdb = exeDir / PDB_NAME;
	std::filesystem::path copyPdb = exeDir / PDB_COPY_NAME;

	// 2. DLLをコピー
	int retries = 0;
	while (retries < 50)
	{
		try {
			// DLLのコピー
			std::filesystem::copy_file(sourceDll, copyDll, std::filesystem::copy_options::overwrite_existing);

			// PDBのコピーは失敗しても無視する (エラーで止めない)
			try {
				if (std::filesystem::exists(sourcePdb)) {
					std::filesystem::copy_file(sourcePdb, copyPdb, std::filesystem::copy_options::overwrite_existing);
				}
			}
			catch (...) {
				std::cout << "[Runner] Warning: PDB copy failed (Ignored)." << std::endl;
			}

			// 成功したら時刻を記録して抜ける
			g_lastWriteTime = std::filesystem::last_write_time(sourceDll);
			break;
		}
		catch (std::exception& e) {
			// 失敗したらちょっと待つ
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			retries++;

			// ログを少し詳細に（デバッグ用）
			if (retries % 10 == 0) {
				std::cout << "[Runner] Waiting for file lock... (" << retries << "/50) " << e.what() << std::endl;
			}
		}
	}

	if (retries == 50) // 50回失敗したら諦める
	{
		std::cout << "[Runner] Failed to copy DLL after retries (File locked?)" << std::endl;
		return false;
	}

	g_sandboxModule = LoadLibraryA(copyDll.string().c_str());
	if (!g_sandboxModule) return false;

	// 4. 関数を取り出す
	CreateAppFunc createApp = (CreateAppFunc)GetProcAddress(g_sandboxModule, "CreateApplication");
	if (!createApp)
	{
		std::cout << "[Runner] Could not find 'CreateApplication' function!" << std::endl;
		return false;
	}

	// 5. アプリ生成
	g_app = createApp();
	std::cout << "[Runner] Game Loaded Successfully!" << std::endl;
	return true;
}

// DLLのアンロード処理
// ============================================================
void UnloadGameDLL()
{
	if (g_app)
	{
		g_app->SaveState();

		delete g_app;	// アプリの終了処理
		g_app = nullptr;
	}

	if (g_sandboxModule)
	{
		FreeLibrary(g_sandboxModule);	// DLLのロック解除
		g_sandboxModule = nullptr;
	}
	std::cout << "[Runner] Game Unloaded." << std::endl;
}

// ファイル監視チェック
// ============================================================
bool CheckForDllUpdate()
{
	try
	{
		std::filesystem::path dllPath = GetExeDirectory() / DLL_NAME;
		auto currentWriteTime = std::filesystem::last_write_time(dllPath);

		// 時刻が進んでいたら「更新あり」とみなす
		if (currentWriteTime > g_lastWriteTime)
		{
			return true;
		}
	}
	catch (...)
	{
		// ビルド中はファイルにアクセスできないことがあるので無視
	}
	return false;
}

// メインループ
// ============================================================
int main()
{
	// 最初のロード
	if (!LoadGameDLL()) return -1;

	while (true)
	{
		std::atomic<bool> stopWatcher = false;
		std::thread watcher([&]()
		{
			while (!stopWatcher)
			{
				if (CheckForDllUpdate())
				{
					// 更新検知
					if (g_app) g_app->RequestReload();
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		});

		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				// 何もしない
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		// ゲーム実行（リロードされるまでここで止まる）
		g_app->Run();

		// Runから戻ってきたら監視スレッドを止める
		stopWatcher = true;
		if (watcher.joinable()) watcher.join();

		// リロード要求か、単なる終了か？
		if (g_app && g_app->IsReloadRequested())
		{
			std::cout << "[Runner] Detected changes! Reloading..." << std::endl;

			// アンロード (内部で SaveState される)
			UnloadGameDLL();

			// 再ロード
			if (!LoadGameDLL()) break;

			std::cout << "[Runner] Game Loaded Successfully!" << std::endl;
		}
		else
		{
			break; // 終了
		}
	}

	UnloadGameDLL();
	return 0;
}