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
#include "Engine/Core/Time/Time.h"

namespace Arche
{
	class AnimationSystem : public ISystem
	{
	public:
		AnimationSystem()
		{
			m_systemName = "Animation System";
			m_group = SystemGroup::Always;
		}

		void Update(Registry& registry) override
		{
			float dt = Time::DeltaTime();

			auto view = registry.view<MeshComponent, Animator>();

			for (auto entity : view)
			{
				auto& mesh = view.get<MeshComponent>(entity);
				auto& animator = view.get<Animator>(entity);

				if (!mesh.pModel) continue;

				if (animator.isPlaying)
				{
					// 名前で再生（Model側で検索・再生）
					if (!animator.currentAnimationName.empty()) {
						mesh.pModel->Play(animator.currentAnimationName, animator.loop, animator.speed);
					}

					// 時間進行
					mesh.pModel->Step(dt);
				}
			}
		}
	};
}

#include "Engine/Scene/Serializer/SystemRegistry.h"
ARCHE_REGISTER_SYSTEM(Arche::AnimationSystem, "Animation System")

#endif // !___ANIMATION_SYSTEM_H___