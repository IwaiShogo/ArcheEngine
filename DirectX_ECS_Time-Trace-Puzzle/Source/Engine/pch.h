// ======================================================================
// Procompiled Header for Engine
// ここに追加されたヘッダーは一度だけコンパイルされ、使い回されます。
// 頻繁に変更されるファイルはここに追加しないでください。
// ======================================================================

#ifndef ___PCH_H___
#define ___PCH_H___

// Windowsのmin/maxマクロと標準ライブラリの競合を防ぐ
#define NOMINMAX

// Windows APIの軽量化（不要なAPIを除外）
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Timeクラスで使用するマルチメディアタイマーAPI用
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Inputクラス
#include <Xinput.h>
#include <array>
#pragma comment(lib, "xinput.lib")

// ResourceManager
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <DirectXTex.h>

// --- DirectX ---
#include <d3d11.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>	// ComPtr
#pragma comment(lib, "d3dcompiler.lib")

// --- Standard Library ---
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <typeindex>
#include <type_traits>
#include <chrono>
#include <thread>
#include <mutex>
#include <cmath>
#include <limits>
#include <cassert>
#include <stdexcept>
#include <set>
#include <random>
#include <cstdint>

// --- Libraries ---
#include <json.hpp>	// nlohmann/json
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"

// --- Common Engine Definitions ---
// よく使うComPtrやDirectXMathのnamespce省略など
using Microsoft::WRL::ComPtr;
using namespace DirectX;

// メモリリーク検出用（Debug用）
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif // _DEBUG

#endif // !___PCH_H___