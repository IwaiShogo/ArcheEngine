/*****************************************************************//**
 * @file	FontManager.h
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

#ifndef ___FONT_MANAGER_H___
#define ___FONT_MANAGER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Core/StringId.h"
#include "Engine/Graphics/Text/PrivateFontLoader.h"

class FontManager
{
public:
	static FontManager& Instance() { static FontManager i; return i; }

	void Initialize();

	ComPtr<IDWriteTextFormat> GetTextFormat(StringId key, const std::wstring& fontFamily, float fontSize, DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL);

	ID2D1Factory* GetD2DFactory() const { return m_d2dFactory.Get(); }

	// ロードされたフォント名のリストを取得
	const std::vector<std::string>& GetLoadedFontNames() const { return m_loadedFontNames; }

private:
	// フォントディレクトリ内の全フォントをメモリにロード
	void LoadFonts(const std::string& directory);

	FontManager() = default;
	~FontManager();

	ComPtr<ID2D1Factory> m_d2dFactory;
	ComPtr<IDWriteFactory> m_dwriteFactory;

	// キャッシュ: キー(StringId) -> TextFormat
	// 同じ設定のフォントを何度も作らないようにする
	std::unordered_map<StringId, ComPtr<IDWriteTextFormat>> m_textFormats;

	// カスタムフォント管理用
	std::vector<std::string> m_registeredFontFiles;

	// GUI表示用
	std::vector<std::string> m_loadedFontNames;
};

#endif // !___FONT_MANAGER_H___