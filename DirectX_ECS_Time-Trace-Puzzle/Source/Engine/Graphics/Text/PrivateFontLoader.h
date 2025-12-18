/*****************************************************************//**
 * @file	PrivateFontLoader.h
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

#ifndef ___PRIVATE_FONT_LOADER_H___
#define ___PRIVATE_FONT_LOADER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Core/Logger.h"

// 1. フォントファイルを列挙するクラス
class PrivateFontFileEnumerator : public IDWriteFontFileEnumerator
{
	IDWriteFactory* m_factory;
	std::vector<std::wstring> m_filePaths;
	size_t m_currentIndex = static_cast<size_t>(-1);
	ComPtr<IDWriteFontFile> m_currentFile;

	ULONG m_refCount = 0;

public:
	PrivateFontFileEnumerator(IDWriteFactory* factory, const std::vector<std::wstring>& paths)
		: m_factory(factory), m_filePaths(paths)
	{
		if (m_factory) m_factory->AddRef();
	}

	~PrivateFontFileEnumerator() { if (m_factory) m_factory->Release(); }

	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject) override {
		if (iid == __uuidof(IUnknown) || iid == __uuidof(IDWriteFontFileEnumerator)) {
			*ppvObject = this; AddRef(); return S_OK;
		}
		*ppvObject = nullptr; return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
	ULONG STDMETHODCALLTYPE Release() override {
		ULONG newCount = InterlockedDecrement(&m_refCount);
		if (newCount == 0) delete this;
		return newCount;
	}

	// IDWriteFontFileEnumerator
	HRESULT STDMETHODCALLTYPE MoveNext(BOOL* hasCurrentFile) override {
		*hasCurrentFile = FALSE;

		// 有効なファイルが見つかるか、リストが終わるまでループする（スキップ機能）
		while (true)
		{
			m_currentIndex++;

			// リストの末尾に達したら終了
			if (m_currentIndex >= m_filePaths.size()) {
				return S_OK;
			}

			m_currentFile.Reset();

			// フォントファイル参照を作成
			HRESULT hr = m_factory->CreateFontFileReference(
				m_filePaths[m_currentIndex].c_str(),
				nullptr,
				&m_currentFile
			);

			if (SUCCEEDED(hr)) {
				// 成功したらこのファイルを採用してループを抜ける
				*hasCurrentFile = TRUE;
				return S_OK;
			}
			else {
				// 失敗した場合、ログを出力して次のファイルへ進む（ループ継続）
				// std::wstring を std::string に変換してログ出力
				std::wstring wpath = m_filePaths[m_currentIndex];
				int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wpath[0], (int)wpath.size(), NULL, 0, NULL, NULL);
				std::string strTo(size_needed, 0);
				WideCharToMultiByte(CP_UTF8, 0, &wpath[0], (int)wpath.size(), &strTo[0], size_needed, NULL, NULL);

				Logger::LogError("Skipping invalid font file: " + strTo);
			}
		}
	}

	HRESULT STDMETHODCALLTYPE GetCurrentFontFile(IDWriteFontFile** fontFile) override {
		*fontFile = m_currentFile.Get();
		if (*fontFile) (*fontFile)->AddRef();
		return S_OK;
	}
};

// 2. コレクションを作成するローダークラス
class PrivateFontCollectionLoader : public IDWriteFontCollectionLoader
{
	std::vector<std::wstring> m_fontPaths;
	ULONG m_refCount = 0;

public:
	// フォントパスのリストを受け取る
	void SetFontPaths(const std::vector<std::wstring>& paths) { m_fontPaths = paths; }

	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject) override {
		if (iid == __uuidof(IUnknown) || iid == __uuidof(IDWriteFontCollectionLoader)) {
			*ppvObject = this; AddRef(); return S_OK;
		}
		*ppvObject = nullptr; return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
	ULONG STDMETHODCALLTYPE Release() override {
		ULONG newCount = InterlockedDecrement(&m_refCount);
		if (newCount == 0) delete this;
		return newCount;
	}

	// IDWriteFontCollectionLoader
	HRESULT STDMETHODCALLTYPE CreateEnumeratorFromKey(
		IDWriteFactory* factory,
		void const* collectionKey,
		UINT32 collectionKeySize,
		IDWriteFontFileEnumerator** fontFileEnumerator) override
	{
		// 保持しているパスリストを使って列挙子を作成
		*fontFileEnumerator = new PrivateFontFileEnumerator(factory, m_fontPaths);
		(*fontFileEnumerator)->AddRef(); // Createで+1された状態で返す
		return S_OK;
	}
};

#endif // !___PRIVATE_FONT_LOADER_H___