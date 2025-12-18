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

FontManager::~FontManager()
{
	// 終了時に登録したフォントを削除
	for (const auto& file : m_registeredFontFiles)
	{
		RemoveFontResourceExA(file.c_str(), FR_PRIVATE, 0);
	}
}

void FontManager::Initialize()
{
	// ---------------------------------------------------------
	// 1. カレントディレクトリをexeの場所に強制移動 (Release対策)
	// ---------------------------------------------------------
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string exePath(buffer);
	std::string exeDir = std::filesystem::path(exePath).parent_path().string();

	// これにより "Resources/Fonts" などの相対パスが確実に通るようになります
	std::filesystem::current_path(exeDir);
	Logger::Log("Current Directory set to: " + exeDir);

	// ---------------------------------------------------------
	// 2. Factory作成
	// ---------------------------------------------------------
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
	if (FAILED(hr)) Logger::LogError("Failed to create D2D Factory!");

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));
	if (FAILED(hr)) Logger::LogError("Failed to create DWrite Factory!");

	// ---------------------------------------------------------
	// 3. フォントロード
	// ---------------------------------------------------------
	// カレントディレクトリ基準で探す
	if (std::filesystem::exists("Resources/Fonts")) {
		LoadFonts("Resources/Fonts");
	}
	else {
		Logger::LogError("Resources/Fonts directory NOT FOUND at: " + std::filesystem::absolute("Resources/Fonts").string());
	}

	// 最後にシステムフォント一覧を取得して、使える名前をリスト化する
	m_loadedFontNames.clear();
	ComPtr<IDWriteFontCollection> sysCollection;
	m_dwriteFactory->GetSystemFontCollection(&sysCollection);

	if (sysCollection) {
		UINT32 count = sysCollection->GetFontFamilyCount();
		for (UINT32 i = 0; i < count; ++i) {
			ComPtr<IDWriteFontFamily> family;
			sysCollection->GetFontFamily(i, &family);

			// 名前取得
			ComPtr<IDWriteLocalizedStrings> names;
			family->GetFamilyNames(&names);
			UINT32 index = 0; BOOL exists = false;
			names->FindLocaleName(L"en-us", &index, &exists);
			if (!exists) index = 0; // fallback

			UINT32 length = 0;
			names->GetStringLength(index, &length);
			std::wstring wname; wname.resize(length + 1);
			names->GetString(index, &wname[0], length + 1);
			wname.pop_back(); // null除去

			// string変換
			int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wname[0], (int)wname.size(), NULL, 0, NULL, NULL);
			std::string name(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wname[0], (int)wname.size(), &name[0], size_needed, NULL, NULL);

			// 独自に追加したフォントかどうかは判別しにくいが、
			// とりあえず全リストがあればユーザーはそこから選べる。
			// 特にResourcesにあるファイル名に近いものはログに出すなどしてもよい。
			m_loadedFontNames.push_back(name);
		}
	}
}

void FontManager::LoadFonts(const std::string& directory)
{
	namespace fs = std::filesystem;

	Logger::Log("Loading fonts from: " + directory);

	for (const auto& entry : fs::recursive_directory_iterator(directory))
	{
		if (entry.is_regular_file())
		{
			std::string ext = entry.path().extension().string();
			// 小文字変換
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			if (ext == ".ttf" || ext == ".otf")
			{
				// 絶対パスを取得
				std::string absPath = fs::absolute(entry.path()).string();

				// Windows APIでフォントを一時インストール
				int result = AddFontResourceExA(absPath.c_str(), FR_PRIVATE, 0);

				if (result > 0) {
					m_registeredFontFiles.push_back(absPath);
					Logger::Log("Registered Font File: " + entry.path().filename().string());
				}
				else {
					Logger::LogError("Failed to register font: " + absPath);
				}
			}
		}
	}
}

ComPtr<IDWriteTextFormat> FontManager::GetTextFormat(StringId key, const std::wstring& fontFamily, float fontSize, DWRITE_FONT_WEIGHT fontWeight, DWRITE_FONT_STYLE fontStyle)
{
	if (m_textFormats.find(key) != m_textFormats.end()) {
		return m_textFormats[key];
	}

	ComPtr<IDWriteTextFormat> format;

	// 第二引数は nullptr (システムコレクション) でOKになる！
	// AddFontResourceExで登録したフォントはシステムコレクションに含まれるため。
	HRESULT hr = m_dwriteFactory->CreateTextFormat(
		fontFamily.c_str(),
		nullptr,
		fontWeight,
		fontStyle,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		L"ja-jp",
		&format
	);

	if (SUCCEEDED(hr)) {
		m_textFormats[key] = format;
		return format;
	}
	else {
		Logger::LogError("Failed to create TextFormat for: " + std::string(fontFamily.begin(), fontFamily.end()));
	}
	return nullptr;
}