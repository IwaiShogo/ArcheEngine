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
#include "Engine/Core/Logger.h"

void FontManager::Initialize()
{
	// D2D Factory作成
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());

	// DirectWrite Factory作成
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));

	// フォント読み込み
	LoadFonts("Resources/Fonts");
}

void FontManager::LoadFonts(const std::string& directory)
{
	namespace fs = std::filesystem;
	if (!fs::exists(directory)) return;

	for (const auto& entry : fs::recursive_directory_iterator(directory))
	{
		if (entry.is_regular_file())
		{
			std::string ext = entry.path().extension().string();
			if (ext == ".ttf" || ext == ".otf" || ext == ".TTF" || ext == ".OTF")
			{
				std::string path = entry.path().string();

				// 絶対パスに変換
				std::filesystem::path absPath = std::filesystem::absolute(entry.path());
				std::string absPathStr = absPath.string();

				// Windows API: フォントを現在のプロセスのみで使用可能にする
				// FR_PRIVATE: 他のアプリからは見えない。アプリ終了時に自動解除。
				// FR_NOT_ENUM: 列挙しない（必要に応じて外してもOK）
				int result = AddFontResourceExA(absPathStr.c_str(), FR_PRIVATE | FR_NOT_ENUM, 0);

				if (result > 0)
				{
					Logger::Log("Loaded Font: " + path);
				}
				else
				{
					Logger::LogError("Failed to load font: " + path);
				}
			}
		}
	}

	// デバッグ用: 読み込まれている "Pixel" を含むフォント名を列挙してログに出す
	ComPtr<IDWriteFontCollection> collection;
	m_dwriteFactory->GetSystemFontCollection(&collection, TRUE); // TRUE=更新を確認
	UINT32 count = collection->GetFontFamilyCount();

	for (UINT32 i = 0; i < count; ++i)
	{
		ComPtr<IDWriteFontFamily> family;
		collection->GetFontFamily(i, &family);

		ComPtr<IDWriteLocalizedStrings> names;
		family->GetFamilyNames(&names);

		// ロケール0番目（英語など）の名前を取得
		UINT32 length = 0;
		names->GetStringLength(0, &length);
		std::wstring wname;
		wname.resize(length + 1);
		names->GetString(0, &wname[0], length + 1);

		// ワイド文字をstringに変換
		std::string name(wname.begin(), wname.end() - 1); // null文字除去

		// "Pixel" が含まれていたらログに出す
		if (name.find("Pixel") != std::string::npos) {
			Logger::Log("Found Font Family: " + name);
		}
	}
}

ComPtr<IDWriteTextFormat> FontManager::GetTextFormat(StringId key, const std::wstring& fontFamily, float fontSize, DWRITE_FONT_WEIGHT fontWeight, DWRITE_FONT_STYLE fontStyle)
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
		fontStyle,
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