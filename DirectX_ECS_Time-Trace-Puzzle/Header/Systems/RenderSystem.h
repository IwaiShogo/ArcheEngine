/*****************************************************************//**
 * @file	RenderSystem.h
 * @brief	描画処理
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___RENDER_SYSTEM_H___
#define ___RENDER_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include "Graphics/PrimitiveRenderer.h"
#include "Components/Components.h"

class RenderSystem
	: public ISystem
{
public:
	RenderSystem(PrimitiveRenderer* rendererPtr)
		: m_renderer(rendererPtr) {
		m_systemName = "Render System";
	}

	void Render(Registry& registry, const Context& context) override;

private:
	PrimitiveRenderer* m_renderer;
};

#endif // !___RENDER_SYSTEM_H___