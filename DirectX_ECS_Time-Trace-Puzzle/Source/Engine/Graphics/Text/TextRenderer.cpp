/*****************************************************************//**
 * @file	TextRenderer.cpp
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

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Graphics/Text/TextRenderer.h"
#include "Engine/Components/Components.h"

TextRenderer::TextRenderer(ID3D11Device* device, ID3D11DeviceContext* context)
	: m_device(device), m_context(context)
{
	FontManager::Instance().Initialize();
}

TextRenderer::~TextRenderer()
{
	m_d2dTargets.clear();
}

void TextRenderer::Render(Registry& registry, ID3D11RenderTargetView* rtv, float viewportWidth, float viewportHeight)
{
	if (!rtv) return;

	// キャッシュをクリア
	m_d2dTargets.clear();

	// 1. D2D RenderTargetの取得
	ID2D1RenderTarget* d2dRT = GetD2DRenderTarget(rtv);
	if (!d2dRT) return;

	// 2. 描画開始
	d2dRT->BeginDraw();

	// 3. スケール計算（基準高さ1080pに対する現在の比率）
	// これにより、画面が小さくなっても文字が「画面に対して同じ大きさ」に見える
	float scale = viewportHeight / BASE_SCREEN_HEIGHT;

	registry.view<TextComponent, Transform>().each([&](Entity e, TextComponent& text, Transform& trans)
	{
		// フォントサイズを計算し、キャッシュキーに含める
		// これにより、サイズが変わるたびに別のフォントとしてキャッシュ・取得されるようになる。
		int scaledFontSize = (int)(text.fontSize * scale);
		if (scaledFontSize < 1) scaledFontSize = 1; // 0以下防止

		// キー生成: フォント名 + サイズ + 太字 + 斜体
		std::string uniqueKeyStr = text.fontKey.c_str();
		uniqueKeyStr += "_" + std::to_string(scaledFontSize);
		if (text.isBold) uniqueKeyStr += "_B";
		if (text.isItalic) uniqueKeyStr += "_I";
		StringId uniqueKey(uniqueKeyStr.c_str());

		// フォント名: コンポーネントの fontKey を使う
		// StringId -> std::string -> std::wstring 変換
		std::string fontNameStr = text.fontKey.c_str();
		std::wstring fontNameW(fontNameStr.begin(), fontNameStr.end());
		if (fontNameStr == "Default") fontNameW = L"Meiryo"; // デフォルト処理

		auto format = FontManager::Instance().GetTextFormat(
			uniqueKey,
			fontNameW, // 個別のフォント名
			(float)scaledFontSize,
			text.isBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, // 太字
			text.isItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL // 斜体
		);

		if (format) {
			// 配置設定
			format->SetTextAlignment(text.centerAlign ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING);
			format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

			// ブラシ色設定
			if (!m_brush) d2dRT->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &m_brush);
			m_brush->SetColor(D2D1::ColorF(text.color.x, text.color.y, text.color.z, text.color.w));

			// 座標計算
			float scaledPosX = trans.position.x * scale;
			float scaledPosY = trans.position.y * scale;

			float screenX = (viewportWidth * 0.5f) + scaledPosX + (text.offset.x * scale);
			float screenY = (viewportHeight * 0.5f) - scaledPosY - (text.offset.y * scale);

			// 矩形定義
			D2D1_RECT_F layoutRect = D2D1::RectF(
				screenX,
				screenY,
				screenX + (text.maxWidth > 0 ? text.maxWidth * scale : 2000.0f),
				screenY + (text.fontSize * scale * 2.0f) // 高さ概算
			);

			// 文字列変換
			std::wstring wText = std::wstring(text.text.begin(), text.text.end());

			// 描画
			d2dRT->DrawText(
				wText.c_str(),
				(UINT32)wText.length(),
				format.Get(),
				layoutRect,
				m_brush.Get()
			);
		}
	});

	// 4. 描画終了
	d2dRT->EndDraw();
}

ID2D1RenderTarget* TextRenderer::GetD2DRenderTarget(ID3D11RenderTargetView* rtv)
{
	// キャッシュにあれば返す
	if (m_d2dTargets.find(rtv) != m_d2dTargets.end()) {
		return m_d2dTargets[rtv].Get();
	}

	// なければ作成 (DXGI Surface経由)
	ComPtr<ID3D11Resource> resource;
	rtv->GetResource(&resource);
	if (!resource) return nullptr;

	ComPtr<IDXGISurface> surface;
	resource.As(&surface);
	if (!surface) return nullptr;

	// DXGIサーフェスのプロパティ
	D2D1_RENDER_TARGET_PROPERTIES props =
		D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
			96.0f, 96.0f // DPI
		);

	ComPtr<ID2D1RenderTarget> target;
	HRESULT hr = FontManager::Instance().GetD2DFactory()->CreateDxgiSurfaceRenderTarget(
		surface.Get(),
		&props,
		&target
	);

	if (SUCCEEDED(hr)) {
		m_d2dTargets[rtv] = target;
		return target.Get();
	}
	return nullptr;
}