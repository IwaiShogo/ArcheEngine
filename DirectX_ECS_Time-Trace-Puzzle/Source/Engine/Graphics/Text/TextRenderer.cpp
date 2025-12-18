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
#include "Engine/Config.h"

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

	// 基準解像度
	float baseWidth = static_cast<float>(Config::SCREEN_WIDTH);
	float baseHeight = static_cast<float>(Config::SCREEN_HEIGHT);
	if (baseWidth == 0) baseWidth = 1920.0f;
	if (baseHeight == 0) baseHeight = 1080.0f;

	// 現在のビューポートとの比率を計算
	float scaleX = viewportWidth / baseWidth;
	float scaleY = viewportHeight / baseHeight;
	float uniformScale = (scaleX < scaleY) ? scaleX : scaleY;

	// 画面全体をフィットさせるための行列
	D2D1_MATRIX_3X2_F screenScaleMatrix = D2D1::Matrix3x2F::Scale(uniformScale, uniformScale);

	registry.view<TextComponent, Transform2D>().each([&](Entity e, TextComponent& text, Transform2D& trans)
	{
		// フォントサイズを計算し、キャッシュキーに含める
		// これにより、サイズが変わるたびに別のフォントとしてキャッシュ・取得されるようになる。
		int scaledFontSize = (int)(text.fontSize * uniformScale);
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
			format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

			// ブラシ色設定
			if (!m_brush) d2dRT->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &m_brush);
			m_brush->SetColor(D2D1::ColorF(text.color.x, text.color.y, text.color.z, text.color.w));

			// 矩形定義
			D2D1_RECT_F layoutRect = D2D1::RectF(
				trans.calculatedRect.x,
				trans.calculatedRect.y,
				trans.calculatedRect.z,
				trans.calculatedRect.w
			);

			// 文字列変換
			std::wstring wText = std::wstring(text.text.begin(), text.text.end());

			d2dRT->SetTransform(trans.worldMatrix * screenScaleMatrix);

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

	if (SUCCEEDED(hr))
	{
		target->SetDpi(96.0f, 96.0f);

		m_d2dTargets[rtv] = target;
		return target.Get();
	}
	return nullptr;
}