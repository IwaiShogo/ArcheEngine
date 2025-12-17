/*****************************************************************//**
 * @file	FontManager.cpp
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
#include "Engine/Graphics/Text/FontManager.h"

void FontManager::Initialize()
{
	// D2D Factory作成
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());

	// DirectWrite Factory作成
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));
}

ComPtr<IDWriteTextFormat> FontManager::GetTextFormat(StringId key, const std::wstring& fontFamily, float fontSize, DWRITE_FONT_WEIGHT fontWeight)
{
	// キャッシュチェック
	if (m_textFormats.find(key) != m_textFormats.end()) {
		return m_textFormats[key];
	}

	// 新規作成
	ComPtr<IDWriteTextFormat> format;
	HRESULT hr = m_dwriteFactory->CreateTextFormat(
		fontFamily.c_str(),
		nullptr,
		fontWeight,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		L"ja-jp", // 日本語ロケール
		&format
	);

	if (SUCCEEDED(hr)) {
		m_textFormats[key] = format;
		return format;
	}

	return nullptr;
}