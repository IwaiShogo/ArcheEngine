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

	// フォントディレクトリ内の全フォントをメモリにロード
	void LoadFonts(const std::string& directory);

	ComPtr<IDWriteTextFormat> GetTextFormat(StringId key, const std::wstring& fontFamily, float fontSize, DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE fontSysle = DWRITE_FONT_STYLE_NORMAL);

	IDWriteFactory* GetWriteFactory() const { return m_dwriteFactory.Get(); }
	ID2D1Factory* GetD2DFactory() const { return m_d2dFactory.Get(); }

private:
	FontManager() = default;
	~FontManager() = default;

	ComPtr<ID2D1Factory> m_d2dFactory;
	ComPtr<IDWriteFactory> m_dwriteFactory;

	// キャッシュ: キー(StringId) -> TextFormat
	// 同じ設定のフォントを何度も作らないようにする
	std::unordered_map<StringId, ComPtr<IDWriteTextFormat>> m_textFormats;

	// カスタムフォント管理用
	ComPtr<IDWriteFontCollection> m_customCollection;
	PrivateFontCollectionLoader* m_collectionLoader = nullptr;
};

#endif // !___FONT_MANAGER_H___