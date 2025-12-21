/*****************************************************************//**
 * @file	ContentBrowser.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/12/21	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___CONTENT_BROWSER_H___
#define ___CONTENT_BROWSER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Editor/Core/Editor.h"

class ContentBrowser
	: public EditorWindow
{
public:
	ContentBrowser();
	void Draw(World& world, Entity& selected, Context& ctx) override;

private:
	// 現在開いているディレクトリのパス
	std::filesystem::path m_currentDirectory;

	// プロジェクトのルートリソースパス
	std::filesystem::path m_baseDirectory;
};

#endif // !___CONTENT_BROWSER_H___