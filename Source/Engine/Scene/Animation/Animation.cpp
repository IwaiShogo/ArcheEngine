/*****************************************************************//**
 * @file	Animation.cpp
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/Animation/Animation.h"

namespace Arche
{
	// ヘルパー: Assimp行列 -> DirectX行列
	static XMFLOAT4X4 AiMatrixToXM(const aiMatrix4x4& from)
	{
		XMFLOAT4X4 to;
		to._11 = from.a1; to._12 = from.b1; to._13 = from.c1; to._14 = from.d1;
		to._21 = from.a2; to._22 = from.b2; to._23 = from.c2; to._24 = from.d2;
		to._31 = from.a3; to._32 = from.b3; to._33 = from.c3; to._34 = from.d3;
		to._41 = from.a4; to._42 = from.b4; to._43 = from.c4; to._44 = from.d4;
		return to;
	}

	// ========================================================================
	// BoneChannel 実装 (補間ロジック)
	// ========================================================================

	float BoneChannel::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const
	{
		if (std::isnan(lastTimeStamp) || std::isinf(lastTimeStamp) ||
			std::isnan(nextTimeStamp) || std::isinf(nextTimeStamp) ||
			std::isnan(animationTime) || std::isinf(animationTime))
		{
			return 0.0f;
		}

		float framesDiff = nextTimeStamp - lastTimeStamp;

		// 1. ゼロ除算回避 (時間が全く進んでいない場合は0を返す)
		if (framesDiff <= 0.0f) return 0.0f;

		// 2. 係数計算
		float midWayLength = animationTime - lastTimeStamp;
		float factor = midWayLength / framesDiff;

		if (factor < 0.0f) factor = 0.0f;
		if (factor > 1.0f) factor = 1.0f;

		return factor;
	}

	int BoneChannel::GetPositionIndex(float animationTime) const
	{
		if (positions.size() < 2) return 0;

		for (int index = 0; index < positions.size() - 1; ++index)
		{
			if (animationTime < positions[index + 1].timeStamp)
				return index;
		}
		return (int)positions.size() - 2;
	}

	int BoneChannel::GetRotationIndex(float animationTime) const
	{
		if (rotations.size() < 2) return 0;

		for (int index = 0; index < rotations.size() - 1; ++index)
		{
			if (animationTime < rotations[index + 1].timeStamp)
				return index;
		}
		return (int)rotations.size() - 2;
	}

	int BoneChannel::GetScaleIndex(float animationTime) const
	{
		if (scales.size() < 2) return 0;

		for (int index = 0; index < scales.size() - 1; ++index)
		{
			if (animationTime < scales[index + 1].timeStamp)
				return index;
		}
		return (int)scales.size() - 2;
	}

	XMMATRIX BoneChannel::GetLocalTransform(float animationTime) const
	{
		XMMATRIX translationM = DirectX::XMMatrixIdentity();
		XMMATRIX rotationM = DirectX::XMMatrixIdentity();
		XMMATRIX scaleM = DirectX::XMMatrixIdentity();

		// 入力時間のサニタイズ
		if (std::isnan(animationTime) || std::isinf(animationTime)) animationTime = 0.0f;

		// 1. Translation Interpolation
		if (!positions.empty())
		{
			if (positions.size() == 1)
			{
				translationM = DirectX::XMMatrixTranslation(positions[0].position.x, positions[0].position.y, positions[0].position.z);
			}
			else
			{
				int p0 = GetPositionIndex(animationTime);
				int p1 = p0 + 1;
				// 境界チェック
				if (p0 >= positions.size()) p0 = 0;
				if (p1 >= positions.size()) p1 = 0;

				float scaleFactor = GetScaleFactor(positions[p0].timeStamp, positions[p1].timeStamp, animationTime);
				XMVECTOR start = DirectX::XMLoadFloat3(&positions[p0].position);
				XMVECTOR end = DirectX::XMLoadFloat3(&positions[p1].position);
				XMVECTOR finalV = DirectX::XMVectorLerp(start, end, scaleFactor);
				translationM = DirectX::XMMatrixTranslationFromVector(finalV);
			}
		}

		// 2. Rotation Interpolation
		if (!rotations.empty())
		{
			if (rotations.size() == 1)
			{
				XMVECTOR quat = DirectX::XMLoadFloat4(&rotations[0].orientation);
				// ゼロ・NaNチェック
				if (DirectX::XMQuaternionIsNaN(quat) || DirectX::XMVectorGetX(DirectX::XMQuaternionLengthSq(quat)) < 1.0e-6f)
				{
					quat = DirectX::XMQuaternionIdentity();
				}
				else
				{
					quat = DirectX::XMQuaternionNormalize(quat);
				}
				rotationM = DirectX::XMMatrixRotationQuaternion(quat);
			}
			else
			{
				int p0 = GetRotationIndex(animationTime);
				int p1 = p0 + 1;
				// 境界チェック
				if (p0 >= rotations.size()) p0 = 0;
				if (p1 >= rotations.size()) p1 = 0;

				float scaleFactor = GetScaleFactor(rotations[p0].timeStamp, rotations[p1].timeStamp, animationTime);

				// scaleFactorがNaNなら0にする（SlerpのAbort回避）
				if (std::isnan(scaleFactor) || std::isinf(scaleFactor)) scaleFactor = 0.0f;

				XMVECTOR start = DirectX::XMLoadFloat4(&rotations[p0].orientation);
				XMVECTOR end = DirectX::XMLoadFloat4(&rotations[p1].orientation);

				// start のサニタイズ
				if (DirectX::XMQuaternionIsNaN(start) || DirectX::XMVectorGetX(DirectX::XMQuaternionLengthSq(start)) < 1.0e-6f)
					start = DirectX::XMQuaternionIdentity();
				else
					start = DirectX::XMQuaternionNormalize(start);

				// end のサニタイズ
				if (DirectX::XMQuaternionIsNaN(end) || DirectX::XMVectorGetX(DirectX::XMQuaternionLengthSq(end)) < 1.0e-6f)
					end = DirectX::XMQuaternionIdentity();
				else
					end = DirectX::XMQuaternionNormalize(end);

				// Slerp実行（入力が正規化＆Validであることを保証済み）
				XMVECTOR finalQ = DirectX::XMQuaternionSlerp(start, end, scaleFactor);

				// 結果のサニタイズ
				if (DirectX::XMQuaternionIsNaN(finalQ) || DirectX::XMVectorGetX(DirectX::XMQuaternionLengthSq(finalQ)) < 1.0e-6f)
				{
					// 計算失敗時は前のフレーム(start)を採用するか、単位行列にする
					finalQ = start;
				}
				else
				{
					finalQ = DirectX::XMQuaternionNormalize(finalQ);
				}

				rotationM = DirectX::XMMatrixRotationQuaternion(finalQ);
			}
		}

		// 3. Scale Interpolation
		if (!scales.empty())
		{
			if (scales.size() == 1)
			{
				scaleM = DirectX::XMMatrixScaling(scales[0].scale.x, scales[0].scale.y, scales[0].scale.z);
			}
			else
			{
				int p0 = GetScaleIndex(animationTime);
				int p1 = p0 + 1;
				// 境界チェック
				if (p0 >= scales.size()) p0 = 0;
				if (p1 >= scales.size()) p1 = 0;

				float scaleFactor = GetScaleFactor(scales[p0].timeStamp, scales[p1].timeStamp, animationTime);
				XMVECTOR start = DirectX::XMLoadFloat3(&scales[p0].scale);
				XMVECTOR end = DirectX::XMLoadFloat3(&scales[p1].scale);
				XMVECTOR finalV = DirectX::XMVectorLerp(start, end, scaleFactor);
				scaleM = DirectX::XMMatrixScalingFromVector(finalV);
			}
		}

		// T * R * S
		return scaleM * rotationM * translationM;
	}

	// ========================================================================
	// AnimationClip 実装
	// ========================================================================

	AnimationClip::AnimationClip(const std::string& animationPath, std::shared_ptr<Model> model)
	{
		Assimp::Importer importer;
		// モデルと同じ設定でロード（階層構造を合わせるため）
		const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

		if (scene && scene->mRootNode && scene->mNumAnimations > 0)
		{
			auto animation = scene->mAnimations[0]; // 0番目のアニメーションを使用
			m_duration = (float)animation->mDuration;
			m_ticksPerSecond = (int)animation->mTicksPerSecond;
			if (m_ticksPerSecond == 0) m_ticksPerSecond = 25;

			// 階層構造の読み込み
			ReadHeirarchyData(m_rootNode, scene->mRootNode);

			// ボーンチャンネル（キーフレーム）の読み込み
			for (unsigned int i = 0; i < animation->mNumChannels; i++)
			{
				auto channel = animation->mChannels[i];
				BoneChannel boneChannel;
				boneChannel.name = channel->mNodeName.C_Str();

				// Position Keys
				for (unsigned int k = 0; k < channel->mNumPositionKeys; k++)
				{
					KeyPosition data;
					data.timeStamp = (float)channel->mPositionKeys[k].mTime;

					data.position.x = channel->mPositionKeys[k].mValue.x;
					data.position.y = channel->mPositionKeys[k].mValue.y;
					data.position.z = channel->mPositionKeys[k].mValue.z;

					boneChannel.positions.push_back(data);
				}

				// Rotation Keys
				for (unsigned int k = 0; k < channel->mNumRotationKeys; k++)
				{
					KeyRotation data;
					data.timeStamp = (float)channel->mRotationKeys[k].mTime;

					data.orientation.x = channel->mRotationKeys[k].mValue.x;
					data.orientation.y = channel->mRotationKeys[k].mValue.y;
					data.orientation.z = channel->mRotationKeys[k].mValue.z;
					data.orientation.w = channel->mRotationKeys[k].mValue.w;

					boneChannel.rotations.push_back(data);
				}

				// Scaling Keys
				for (unsigned int k = 0; k < channel->mNumScalingKeys; k++)
				{
					KeyScale data;
					data.timeStamp = (float)channel->mScalingKeys[k].mTime;
					data.scale.x = channel->mScalingKeys[k].mValue.x;
					data.scale.y = channel->mScalingKeys[k].mValue.y;
					data.scale.z = channel->mScalingKeys[k].mValue.z;
					boneChannel.scales.push_back(data);
				}

				m_boneChannels.push_back(boneChannel);
			}

			// ボーン情報のコピー（モデルから）
			m_boneInfoMap = model->boneInfoMap;
		}
	}

	void AnimationClip::ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
	{
		dest.name = src->mName.data;
		dest.transformation = AiMatrixToXM(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		for (unsigned int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHeirarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}
}
