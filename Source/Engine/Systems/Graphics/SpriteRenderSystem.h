/*****************************************************************//**
 * @file	SpriteRenderSystem.h
 * @brief	SpriteRendrerを使って描画を行うシステム
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

#ifndef ___SPRITE_RENDERER_SYSTEM_H___
#define ___SPRITE_RENDERER_SYSTEM_H___

// ===== インクルード =====
#include "Engine/Scene/ECS/ECS.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Renderer/Renderers/SpriteRenderer.h"
#include "Engine/Resource/ResourceManager.h"

class SpriteRenderSystem
	: public ISystem
{
public:
	SpriteRenderSystem(SpriteRenderer* renderer)
		: m_renderer(renderer)
	{
		m_systemName = "Sprite Render System";
	}

	void Render(Registry& registry, const Context& context) override
	{
		if (!m_renderer) return;

		// 2D描画開始
		m_renderer->Begin();

		registry.view<SpriteComponent, Transform2D>().each([&](Entity e, SpriteComponent& s, Transform2D& t2d)
			{
				// テクスチャ取得
				auto tex = ResourceManager::Instance().GetTexture(s.textureKey);
				if (tex)
				{
					// 座標計算
					float width = t2d.calculatedRect.z - t2d.calculatedRect.x;
					float height = t2d.calculatedRect.w - t2d.calculatedRect.y;
					float centerX = t2d.calculatedRect.x + width * 0.5f;
					float centerY = t2d.calculatedRect.y + height * 0.5f;

					// 描画
					m_renderer->Draw(tex.get(), centerX - (width * 0.5f), centerY - (height * 0.5f), width, height, s.color);
				}
			});
	}

private:
	SpriteRenderer* m_renderer;
};

#endif // !___SPRITE_RENDERER_SYSTEM_H___
