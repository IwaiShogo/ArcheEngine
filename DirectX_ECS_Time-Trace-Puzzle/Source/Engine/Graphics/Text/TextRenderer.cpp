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

	// 1. D2D RenderTargetの取得
	ID2D1RenderTarget* d2dRT = GetD2DRenderTarget(rtv);
	if (!d2dRT) return;

	// 2. 描画開始
	d2dRT->BeginDraw();

	// 3. スケール計算（基準高さ1080pに対する現在の比率）
	// これにより、画面が小さくなっても文字が「画面に対して同じ大きさ」に見える
	float scale = viewportHeight / BASE_SCREEN_HEIGHT;

	registry.view<TextComponent, Transform>().each([&](Entity e, TextComponent& text, Transform& trans) {
		// フォント取得（なければデフォルト作成）
		auto format = FontManager::Instance().GetTextFormat(
			text.fontKey,
			L"Meiryo", // デフォルトはメイリオ
			text.fontSize * scale // ★ここでスケーリング適用
		);

		if (format) {
			// 配置設定
			format->SetTextAlignment(text.centerAlign ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING);
			format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

			// ブラシ色設定
			if (!m_brush) d2dRT->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &m_brush);
			m_brush->SetColor(D2D1::ColorF(text.color.x, text.color.y, text.color.z, text.color.w));

			// 座標計算 (スクリーン座標変換)
			// 本来は3D空間の座標をProjectしてスクリーン座標にする必要があるが、
			// ここでは簡易的に「Transform.position.x/y をスクリーン上の割合(0.0-1.0)またはピクセル」として扱うか、
			// 2D UIとして扱う設計にするのが一般的。

			// 今回は「Transform.position」を「スクリーン中心(0,0)からのピクセル座標」として扱ってみます。
			// 必要に応じて「3Dワールド座標から変換」する処理に変えてください。
			float screenX = (viewportWidth * 0.5f) + trans.position.x + (text.offset.x * scale);
			float screenY = (viewportHeight * 0.5f) - trans.position.y - (text.offset.y * scale);

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