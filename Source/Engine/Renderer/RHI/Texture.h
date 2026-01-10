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
#include "Engine/pch.h"

namespace Arche
{

	class Texture
	{
	public:
		std::string filepath;
		int width = 0;
		int height = 0;

		// DirectXリソース
		ComPtr<ID3D11ShaderResourceView> srv;

		// 非同期ロード用の一時データ（CPUメモリ）
		DirectX::ScratchImage scratchImage;

		Texture() = default;
		~Texture() = default;

		// 1. CPUロード (スレッドセーフ・並列実行可能)
		bool LoadCPU(const std::string& path);

		// 2. GPUアップロード (メインスレッド専用)
		bool UploadGPU(ID3D11Device* device);
	};

}	// namespace Arche

#endif // !___TEXTURE_H___
