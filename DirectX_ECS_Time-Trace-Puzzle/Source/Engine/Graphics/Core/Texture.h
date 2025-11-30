/*****************************************************************//**
 * @file	Texture.h
 * @brief	読み込んだ画像データを表すクラス
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/25	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___TEXTURE_H___
#define ___TEXTURE_H___

// ===== インクルード =====
#include <d3d11.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

class Texture
{
public:
	// ファイルパスを保持（デバッグ用）
	std::string filepath;

	// 画像サイズ
	int width = 0;
	int height = 0;

	// DirectXリソース
	ComPtr<ID3D11ShaderResourceView> srv;

	// コンストラクタ
	Texture() = default;
	~Texture() = default;
};

#endif // !___TEXTURE_H___