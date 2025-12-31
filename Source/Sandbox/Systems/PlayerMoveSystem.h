#pragma once

// ===== インクルード =====
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Core/Window/Input.h"
#include "Engine/Core/Time/Time.h"

#include "Sandbox/Components/PlayerMoveData.h"

namespace Arche
{
	class PlayerMoveSystem : public ISystem
	{
	public:
		PlayerMoveSystem()
		{
			m_systemName = "PlayerMoveSystem";
		}

		void Update(Registry& registry) override
		{
			// "PlayerMoveData" と "Transform" の両方を持っているエンティティを全て取得
			auto view = registry.view<PlayerMoveData, Transform>();

			for (auto entity : view)
			{
				// 参照（&）で取得して書き換えられるようにする
				auto& moveData = view.get<PlayerMoveData>(entity);
				auto& transform = view.get<Transform>(entity);

				// --- 移動ロジック ---
				float dt = Time::DeltaTime();
				float speed = moveData.speed;

				// Inputクラスを使って入力を取得
				if (Input::GetKey(VK_LEFT))	 transform.position.x -= speed * dt;
				if (Input::GetKey(VK_RIGHT)) transform.position.x += speed * dt;
				if (Input::GetKey(VK_UP))	 transform.position.y += speed * dt;
				if (Input::GetKey(VK_DOWN))	 transform.position.y -= speed * dt;
			}
		}
	};
}	// namespace Arche

#include "Engine/Scene/Serializer/SystemRegistry.h"
ARCHE_REGISTER_SYSTEM(Arche::PlayerMoveSystem, "PlayerMoveSystem")