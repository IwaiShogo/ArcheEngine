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
#include "Engine/ECS/ECS.h"
#include "Engine/Components/Components.h"
#include "Engine/Graphics/Renderers/SpriteRenderer.h"
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

		registry.view<SpriteComponent, Transform>().each([&](Entity e, SpriteComponent& s, Transform& t)
			{
				// テクスチャ取得
				auto tex = ResourceManager::Instance().GetTexture(s.textureKey);
				if (tex)
				{
					// 座標計算（Pivot考慮）
					// Transform.position.x, y をスクリーン座標として扱います
					float x = t.position.x - (s.width * s.pivot.x);
					float y = t.position.y - (s.width * s.pivot.y);

					// 描画
					m_renderer->Draw(tex.get(), x, y, s.width, s.height, s.color);
				}
			});
	}

private:
	SpriteRenderer* m_renderer;
};

#endif // !___SPRITE_RENDERER_SYSTEM_H___