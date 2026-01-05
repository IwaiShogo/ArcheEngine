/*****************************************************************//**
 * @file	AnimationData.h
 * @brief	アニメーションに必要なデータ
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___ANIMAMTION_DATA_H___
#define ___ANIMAMTION_DATA_H___

// ===== インクルード =====
#include "Engine/pch.h"

namespace Arche
{
	// ボーン情報
	struct BoneInfo
	{
		int id;
		XMFLOAT4X4 offset;	// モデル空間 -> ボーンローカル空間への変換行列
	};

	// キーフレーム（位置）
	struct KeyPosition
	{
		XMFLOAT3 position;
		float timeStamp;
	};

	// キーフレーム（回転）
	struct KeyRotation
	{
		XMFLOAT4 orientation;
		float timeStamp;
	};

	// キーフレーム（スケール）
	struct KeyScale
	{
		XMFLOAT3 scale;
		float timeStamp;
	};

	// アニメーションデータ
	struct Animation
	{
		std::string name;
		float duration;
		float ticksPerSecond;
	};
}

#endif // !___ANIMAMTION_DATA_H___