/*****************************************************************//**
 * @file	Animation.h
 * @brief	アニメーション再生に必要なクラス群
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___ANIMATION_H___
#define ___ANIMATION_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Renderer/Data/Model.h"

namespace Arche
{
	// 個別のボーンのアニメーションデータ（キーフレームの集合）
	struct BoneChannel
	{
		std::string name;
		int id;
		std::vector<KeyPosition> positions;
		std::vector<KeyRotation> rotations;
		std::vector<KeyScale> scales;

		// 補間計算
		XMMATRIX GetLocalTransform(float animationTime) const;

		// キーフレーム検索ヘルパー
		int GetPositionIndex(float animationTime) const;
		int GetRotationIndex(float animationTime) const;
		int GetScaleIndex(float animationTime) const;
		float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const;
	};

	// 階層構造ノード
	struct AssimpNodeData
	{
		std::string name;
		XMFLOAT4X4 transformation;
		int childrenCount;
		std::vector<AssimpNodeData> children;
	};

	// アニメーションクリップ
	class AnimationClip
	{
	public:
		AnimationClip() = default;
		AnimationClip(const std::string& animationPath, std::shared_ptr<Model> model); // ロード処理

		float GetDuration() const { return m_duration; }
		float GetTicksPerSecond() const { return m_ticksPerSecond; }
		const std::vector<BoneChannel>& GetBoneChannels() const { return m_boneChannels; }
		const AssimpNodeData& GetRootNode() const { return m_rootNode; }
		const std::map<std::string, BoneInfo>& GetBoneIDMap() const { return m_boneInfoMap; }

	private:
		// Assimp読み込みヘルパー
		void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src);
		void ReadMissingBones(const aiAnimation* animation, std::shared_ptr<Model> model);

		float m_duration;
		int m_ticksPerSecond;
		std::vector<BoneChannel> m_boneChannels; // ボーンごとの動き
		AssimpNodeData m_rootNode;				 // 階層構造
		std::map<std::string, BoneInfo> m_boneInfoMap;
	};
}

#endif // !___ANIMATION_H___