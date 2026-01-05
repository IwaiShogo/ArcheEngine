/*****************************************************************//**
 * @file	AnimationSystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___ANIMATION_SYSTEM_H___
#define ___ANIMATION_SYSTEM_H___

// ===== インクルード =====
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Scene/Animation/Animation.h"
#include "Engine/Resource/ResourceManager.h"

namespace Arche
{
	class AnimationSystem : public ISystem
	{
	public:
		AnimationSystem()
		{
			m_systemName = "Animation System";
			m_group = SystemGroup::PlayOnly;
		}

		void Update(Registry& registry) override
		{
			float dt = Time::DeltaTime();

			bool debugLog = Input::GetKeyDown('L');

			auto view = registry.view<Animator, MeshComponent>();
			for (auto entity : view)
			{
				auto& animator = view.get<Animator>(entity);
				auto& mesh = view.get<MeshComponent>(entity);

				// 0. アニメーションデータの解決
				std::shared_ptr<Model> model = nullptr; // モデルの参照を保持

				if (!animator.currentAnimationName.empty())
				{
					model = ResourceManager::Instance().GetModel(mesh.modelKey);
					// まだクリップを持っていないならロード
					if (!animator.currentAnimation && model)
					{
						animator.currentAnimation = ResourceManager::Instance().GetAnimation(animator.currentAnimationName, model);
					}
				}

				// データがない、または再生オフならスキップ
				if (!animator.currentAnimation || !model) continue;
				if (!animator.isPlaying) continue;

				auto& clip = animator.currentAnimation;

				if (animator.finalBoneMatrices.size() < 100)
				{
					animator.finalBoneMatrices.resize(100);
					for (auto& m : animator.finalBoneMatrices)
					{
						XMStoreFloat4x4(&m, XMMatrixIdentity());
					}
				}

				// 1. 時間の進行
				animator.currentTime += clip->GetTicksPerSecond() * dt * animator.speed;

				// ループ処理
				float duration = clip->GetDuration();
				if (animator.loop)
				{
					if (duration > 0.0f)
						animator.currentTime = std::fmod(animator.currentTime, duration);
				}
				else if (animator.currentTime > duration)
				{
					animator.currentTime = duration;
				}

				// 2. 行列計算
				CalculateBoneTransform(clip.get(), animator.currentTime, animator.finalBoneMatrices, model->globalInverseTransform);

				if (debugLog && !animator.finalBoneMatrices.empty())
				{
					Logger::Log("===== Animation Debug Log (Entity ID: " + std::to_string((uint32_t)entity) + ") =====");
					Logger::Log("Time: " + std::to_string(animator.currentTime));

					// 最初の5本くらいのボーン行列を表示してみる
					int count = 0;
					for (const auto& m : animator.finalBoneMatrices)
					{
						if (count++ > 5) break; // 全部出すと多すぎるので5つだけ

						std::stringstream ss;
						ss << "Bone[" << (count - 1) << "]:\n";
						ss << std::fixed << std::setprecision(2);
						ss << "|" << m._11 << ", " << m._12 << ", " << m._13 << ", " << m._14 << "|\n";
						ss << "|" << m._21 << ", " << m._22 << ", " << m._23 << ", " << m._24 << "|\n";
						ss << "|" << m._31 << ", " << m._32 << ", " << m._33 << ", " << m._34 << "|\n";
						ss << "|" << m._41 << ", " << m._42 << ", " << m._43 << ", " << m._44 << "|";

						Logger::Log(ss.str());
					}
				}
			}
		}

	private:
		// 1. エントリーポイントの修正
		void CalculateBoneTransform(const AnimationClip* animation, float currentTime, std::vector<XMFLOAT4X4>& outputMatrices, const XMFLOAT4X4& globalInverseTransform)
		{
			XMMATRIX identity = XMMatrixIdentity();
			// グローバル逆行列をロード
			XMMATRIX globalInv = XMLoadFloat4x4(&globalInverseTransform);

			// globalInv を引数に追加して渡す
			CalculateBoneTransformRecursive(&animation->GetRootNode(), identity, animation, currentTime, outputMatrices, globalInv);
		}

		// 2. 再帰関数の修正（引数追加）
		void CalculateBoneTransformRecursive(const AssimpNodeData* node, XMMATRIX parentTransform, const AnimationClip* animation, float currentTime, std::vector<XMFLOAT4X4>& outputMatrices, XMMATRIX globalInverseTransform)
		{
			std::string nodeName = node->name;
			XMMATRIX nodeTransform = XMLoadFloat4x4(&node->transformation);

			const BoneChannel* channel = FindBoneChannel(animation, nodeName);
			if (channel)
			{
				nodeTransform = channel->GetLocalTransform(currentTime);
			}

			// 親行列 * 自分 = グローバル行列
			XMMATRIX globalTransformation = nodeTransform * parentTransform;

			auto& boneInfoMap = animation->GetBoneIDMap();
			if (boneInfoMap.find(nodeName) != boneInfoMap.end())
			{
				int index = boneInfoMap.at(nodeName).id;
				XMMATRIX offset = XMLoadFloat4x4(&boneInfoMap.at(nodeName).offset);

				XMMATRIX finalM = offset * globalTransformation * globalInverseTransform;

				if (index < outputMatrices.size())
				{
					XMStoreFloat4x4(&outputMatrices[index], finalM);
				}
			}

			for (const auto& child : node->children)
			{
				CalculateBoneTransformRecursive(&child, globalTransformation, animation, currentTime, outputMatrices, globalInverseTransform);
			}
		}

		const BoneChannel* FindBoneChannel(const AnimationClip* anim, const std::string& name)
		{
			for (const auto& ch : anim->GetBoneChannels())
			{
				if (ch.name == name) return &ch;
			}
			return nullptr;
		}
	};
}

#include "Engine/Scene/Serializer/SystemRegistry.h"
ARCHE_REGISTER_SYSTEM(Arche::AnimationSystem, "Animation System")

#endif // !___ANIMATION_SYSTEM_H___