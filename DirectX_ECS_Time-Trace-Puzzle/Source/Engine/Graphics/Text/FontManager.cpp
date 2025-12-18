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
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));

	if (!m_collectionLoader)
	{
		m_collectionLoader = new PrivateFontCollectionLoader();
		m_collectionLoader->AddRef();
		m_dwriteFactory->RegisterFontCollectionLoader(m_collectionLoader);
	}

	// .exe基準の絶対パス
	char buffer[MAX_PATH]; 
	GetModuleFileNameA(NULL, buffer, MAX_PATH);	// exeのフルパス取得
	std::string exePath(buffer);
	// exe名の部分を削ってディレクトリだけにする
	std::string exeDir = exePath.substr(0, exePath.find_last_of("\\/"));

	// 探索候補リスト
	std::vector<std::string> searchPaths =
	{
		exeDir + "\\Resources\\Fonts",	// 1. exeと同じ場所 (Release/配布時)
		"Resources/Fonts",				// 2. カレントディレクトリ（VSデバッグ時の標準）
		"../../Resources/Fonts",		// 3. プロジェクトルート（x64/Debugから見た位置）
		"../../../Resources/Fonts"		// 4. 念のためもう一つ上
	};

	std::string validPath = "";
	for (const auto& path : searchPaths)
	{
		if (std::filesystem::exists(path))
		{
			validPath = path;
			break;	// 見つかったら終了
		}
	}

	if (!validPath.empty())
	{
		Logger::Log("Fonts found at: " + validPath);
		LoadFonts(validPath);
	}
	else
	{
		Logger::LogError("FAILED to find Resources/Fonts directory!");
		LoadFonts("Resources/Fonts");
	}
}

void FontManager::LoadFonts(const std::string& directory)
{
	namespace fs = std::filesystem;
	if (!fs::exists(directory)) return;

	// 1. ファイルパスを収集
	std::vector<std::wstring> fontPaths;
	for (const auto& entry : fs::recursive_directory_iterator(directory))
	{
		if (entry.is_regular_file())
		{
			std::string ext = entry.path().extension().string();
			if (ext == ".ttf" || ext == ".otf" || ext == ".TTF" || ext == ".OTF")
			{
				// 絶対パスを取得してリストに追加
				std::filesystem::path absPath = std::filesystem::absolute(entry.path());
				fontPaths.push_back(absPath.wstring());
				Logger::Log("Found Font File: " + absPath.string());
			}
		}
	}

	if (fontPaths.empty()) return;

	// 2. ローダーにパスを渡して、カスタムコレクションを作成
	m_collectionLoader->SetFontPaths(fontPaths);

	// キーは何でもいいが、ローダーに渡される
	const char* key = "CustomFonts";
	HRESULT hr = m_dwriteFactory->CreateCustomFontCollection(
		m_collectionLoader,
		key,
		(UINT32)strlen(key),
		&m_customCollection
	);

	if (FAILED(hr)) {
		Logger::LogError("Failed to create custom font collection.");
		return;
	}

	// 3. デバッグ: カスタムコレクションの中身を確認
	UINT32 count = m_customCollection->GetFontFamilyCount();
	for (UINT32 i = 0; i < count; ++i)
	{
		ComPtr<IDWriteFontFamily> family;
		m_customCollection->GetFontFamily(i, &family);
		ComPtr<IDWriteLocalizedStrings> names;
		family->GetFamilyNames(&names);

		UINT32 length = 0;
		names->GetStringLength(0, &length);
		std::wstring wname; wname.resize(length + 1);
		names->GetString(0, &wname[0], length + 1);

		// このログに出た名前を使ってください！
		std::string name(wname.begin(), wname.end() - 1);
		Logger::Log(">>> Custom Font Loaded: " + name);
	}
}

ComPtr<IDWriteTextFormat> FontManager::GetTextFormat(StringId key, const std::wstring& fontFamily, float fontSize, DWRITE_FONT_WEIGHT fontWeight, DWRITE_FONT_STYLE fontStyle)
{
	if (m_textFormats.find(key) != m_textFormats.end()) {
		return m_textFormats[key];
	}

	// 重要: 指定されたフォントが「カスタムコレクション」にあるか探す
	IDWriteFontCollection* targetCollection = nullptr; // nullptrならシステムフォントを使う

	if (m_customCollection) {
		UINT32 index;
		BOOL exists;
		m_customCollection->FindFamilyName(fontFamily.c_str(), &index, &exists);
		if (exists) {
			targetCollection = m_customCollection.Get(); // カスタムフォントにあった！
		}
	}

	ComPtr<IDWriteTextFormat> format;
	HRESULT hr = m_dwriteFactory->CreateTextFormat(
		fontFamily.c_str(),
		targetCollection, // ★ここでコレクションを切り替える
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
	return nullptr;
}