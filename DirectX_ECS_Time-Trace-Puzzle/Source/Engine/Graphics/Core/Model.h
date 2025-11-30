/*****************************************************************//**
 * @file	Model.h
 * @brief	3Dモデルのデータを保持するクラス
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/11/26	初回作成日
 * 			作業内容：	- 追加：
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#ifndef ___MODEL_H___
#define ___MODEL_H___

 // ===== インクルード =====
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <memory>
#include "Engine/Graphics/Core/Texture.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// 頂点データ（位置、法線、UV）
struct ModelVertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
};

// 1つのメッシュ（モデルの構成部品）
struct Mesh
{
	ComPtr<ID3D11Buffer> vertexBuffer;
	ComPtr<ID3D11Buffer> indexBuffer;
	unsigned int indexCount = 0;

	// マテリアル情報
	std::shared_ptr<Texture> texture;
};

// モデル全体（複数のメッシュを持つ）
class Model
{
public:
	std::vector<Mesh> meshes;
	std::string filepath;	// デバッグ用
};

#endif // !___MODEL_H___