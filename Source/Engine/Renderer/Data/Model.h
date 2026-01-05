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
#include "Engine/pch.h"
#include "Engine/Renderer/RHI/Texture.h"
#include "AnimationData.h"

namespace Arche
{
	// ボーンの影響最大数
	#define MAX_BONE_INFLUENCE 4

	// 頂点データ（位置、法線、UV、ボーンID、ウェイト）
	struct ModelVertex
	{
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT2 uv;

		// スキニング情報
		int boneIDs[MAX_BONE_INFLUENCE];
		float weights[MAX_BONE_INFLUENCE];

		// 初期化コンストラクタ
		ModelVertex()
		{
			position = { 0, 0, 0 };
			normal = { 0, 0, 0 };
			uv = { 0, 0 };
			for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
			{
				boneIDs[i] = -1;
				weights[i] = 0.0f;
			}
		}

		// ウェイト追加ヘルパー
		void AddBoneData(int boneID, float weight)
		{
			for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
			{
				if (weights[i] == 0.0f)
				{
					boneIDs[i] = boneID;
					weights[i] = weight;
					return;
				}
			}
		}
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

		// ボーン情報マップ（名前 -> 情報）
		std::map<std::string, BoneInfo> boneInfoMap;
		int boneCounter = 0;

		// グローバル逆変換行列
		XMFLOAT4X4 globalInverseTransform;
	};

}	// namespace Arche

#endif // !___MODEL_H___