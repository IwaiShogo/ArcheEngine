/*****************************************************************//**
 * @file	InputSystem.h
 * @brief	入力処理
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/24	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___INPUT_SYSTEM_H___
#define ___INPUT_SYSTEM_H___

// ===== インクルード =====
#include "Engine/ECS/ECS.h"
#include "Engine/Components/Components.h"
#include "Engine/Core/Input.h"

class InputSystem
	: public ISystem
{
public:
	InputSystem() { m_systemName = "Input System"; }

	void Update(Registry& registry) override
	{
		// デバッグカメラモードなら受け付けない
		if (m_context && m_context->debug.useDebugCamera) return;

		// Inputクラスから値を取得
		float x = Input::GetAxis(Axis::Horizontal);
		float z = Input::GetAxis(Axis::Vertical);

		// 斜め移動の正規化
		if (x != 0.0f || z != 0.0f)
		{
			float length = std::sqrt(x * x + z * z);
			if (length > 1.0f)
			{
				x /= length;
				z /= length;
			}
		}

		registry.view<PlayerInput, Rigidbody>().each([&](Entity e, PlayerInput& input, Rigidbody& rb)
			{
				rb.velocity.x = x * input.speed;
				rb.velocity.z = z * input.speed;

				if (Input::GetButtonDown(Button::A))
				{
					// 上方向（Y）に速度を与える
					rb.velocity.y = input.jumpPower;
				}
			});
	}

	void SetContext(Context* ctx) { m_context = ctx; }

private:
	Context* m_context = nullptr;
};

#endif // !___INPUT_SYSTEM_H___
