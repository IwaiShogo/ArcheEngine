/*****************************************************************//**
 * @file	Time.h
 * @brief	FPS制御
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/24	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___TIME_H___
#define ___TIME_H___

// ===== インクルード =====
#include <Windows.h>
#include <thread>	// Sleep用

#pragma comment(lib, "winmm.lib")

class Time
{
public:
	// アプリケーション初期化時に呼ぶ
	static void Initialize()
	{
		// CPUの高精度タイマー周波数を取得
		QueryPerformanceFrequency(&s_cpuFreq);
		QueryPerformanceCounter(&s_startTime);
		s_lastTime = s_startTime;

		// Sleepの精度を1msにする
		timeBeginPeriod(1);
	}

	// 毎フレーム呼ぶ
	static void Update()
	{
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		// 経過時間を計算（現在のカウント - 前回のカウント） / 周波数
		long long diff = currentTime.QuadPart - s_lastTime.QuadPart;

		// 秒単位に変換
		s_deltaTime = static_cast<double>(diff) / static_cast<double>(s_cpuFreq.QuadPart);

		// 現在時刻を保存
		s_lastTime = currentTime;
	}

	inline static void StepFrame()
	{
		s_isStepNext = true;	// 次のフレームだけ動かすフラグを立てる
	}

	// 前のフレームからの経過時間（秒）を取得
	static float DeltaTime()
	{
		// コマ送り要求がある場合
		if (s_isStepNext)
		{
			s_isStepNext = false;	// フラグを下す
			return 1.0f / 60.0f;	// 強制的に1/60秒進んだことにする
		}

		// 一時停止中の場合
		if (isPaused)
		{
			return 0.0f;	// 時間を経過させない
		}

		return static_cast<float>(s_deltaTime) * timeScale;
	}

	// ゲーム開始からの総経過時間（秒）
	static float TotalTime()
	{
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);
		long long diff = currentTime.QuadPart - s_startTime.QuadPart;
		return static_cast<float>(static_cast<double>(diff) / static_cast<double>(s_cpuFreq.QuadPart));
	}

	// 目標フレームレートを設定
	static void SetFrameRate(int fps)
	{
		if (fps > 0)
		{
			s_targetFrameTime = 1.0f / static_cast<double>(fps);
		}
	}

	// 設定したフレームレートになるように待機する
	static void WaitFrame()
	{
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		// Update()が呼ばれた時刻（s_lastTime）からの経過時間を計算
		double elapsed = static_cast<double>(currentTime.QuadPart - s_lastTime.QuadPart) / static_cast<double>(s_cpuFreq.QuadPart);

		// 目標時間に達するまで待機
		while (elapsed < s_targetFrameTime)
		{
			double remaining = s_targetFrameTime - elapsed;

			// 残り1ms以上あればOSのSleepで休む（CPU負荷対策）
			if (remaining > 0.001)
			{
				Sleep(static_cast<DWORD>(remaining * 1000.0));
			}

			// 再計測
			QueryPerformanceCounter(&currentTime);
			elapsed = static_cast<double>(currentTime.QuadPart - s_lastTime.QuadPart) / static_cast<double>(s_cpuFreq.QuadPart);
		}
	}

	inline static float timeScale = 1.0f;			// タイムスケール

	inline static bool isPaused = false;

private:
	inline static LARGE_INTEGER s_cpuFreq = {};		// CPUの周波数
	inline static LARGE_INTEGER s_lastTime = {};	// 前回の時間
	inline static LARGE_INTEGER s_startTime = {};	// 開始時間
	inline static double s_deltaTime = 0.0;			// 経過時間
	inline static bool s_isStepNext = false;
	inline static double s_targetFrameTime = 1.0 / 60.0;
};

#endif // !___TIME_H___