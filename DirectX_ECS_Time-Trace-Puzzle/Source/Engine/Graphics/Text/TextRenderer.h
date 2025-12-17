/*****************************************************************//**
 * @file	TextRenderer.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/18	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___TEXT_RENDERER_H___
#define ___TEXT_RENDERER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/ECS/ECS.h"
#include "Engine/Graphics/Text/FontManager.h"

class TextRenderer
{
public:
	TextRenderer(ID3D11Device* device, ID3D11DeviceContext* context);
	~TextRenderer();

	// 描画実行
	// renderTargetView: 描画先のRTV（Debug時はシーンRT、Release時はBackBuffer）
	// viewportWidth/Height: 現在の描画領域サイズ
	void Render(Registry& registry, ID3D11RenderTargetView* rtv, float viewportWidth, float viewportHeight);

private:
	// D3D11テクスチャに関連付けられたD2Dレンダーターゲットを作成・取得
	ID2D1RenderTarget* GetD2DRenderTarget(ID3D11RenderTargetView* rtv);

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;

	// RTVポインタをキーにしてD2Dターゲットをキャッシュ
	// （Debug/Release切り替えやリサイズ時に対応）
	std::unordered_map<ID3D11RenderTargetView*, ComPtr<ID2D1RenderTarget>> m_d2dTargets;

	ComPtr<ID2D1SolidColorBrush> m_brush;

	// 基準解像度の高さ（この高さの時にfontSizeが等倍になる）
	const float BASE_SCREEN_HEIGHT = 1080.0f;
};

#endif // !___TEXT_RENDERER_H___