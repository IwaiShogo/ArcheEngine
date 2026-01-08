/*****************************************************************//**
 * @file	ResourceManager.cpp
 * @brief	リソースマネージャー
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/25	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Core/Base/Logger.h"

namespace Arche
{
	void ResourceManager::Initialize(ID3D11Device* device)
	{
		m_device = device;
		CreateSystemTextures();
	}

	void ResourceManager::Clear()
	{
		m_textures.clear();
		m_models.clear();
		m_sounds.clear();
	}

	// システムリソース（While, Black, Normal 等）
	// ------------------------------------------------------------
	void ResourceManager::CreateSystemTextures()
	{
		// 1x1 While Texture
		{
			uint32_t pixel = 0xFFFFFFFF; // RGBA
			D3D11_SUBRESOURCE_DATA initData = { &pixel, sizeof(uint32_t), 0 };

			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = 1; desc.Height = 1; desc.MipLevels = 1; desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			ComPtr<ID3D11Texture2D> tex2D;
			m_device->CreateTexture2D(&desc, &initData, tex2D.GetAddressOf());

			ComPtr<ID3D11ShaderResourceView> srv;
			m_device->CreateShaderResourceView(tex2D.Get(), nullptr, srv.GetAddressOf());

			// 手動でTextureオブジェクトを構築
			auto tex = std::make_shared<Texture>();
			tex->filepath = "System::White";
			tex->width = 1;
			tex->height = 1;
			tex->srv = srv;

			m_textures["White"] = tex; // キーは小文字統一でも可
		}
	}

	// パス解決ロジック
	// ------------------------------------------------------------
	std::string ResourceManager::ResolvePath(const std::string& keyName, const std::vector<std::string>& directories, const std::vector<std::string>& extensions)
	{
		// 既にフルパスまたは拡張子付きで存在する場合
		if (std::filesystem::exists(keyName)) return keyName;

		// ディレクトリ x 拡張子 で探索
		for (const auto& dir : directories)
		{
			// 1. そのまま検索（拡張子込みで指定された場合など）
			std::string tryPath = dir + keyName;
			if (std::filesystem::exists(tryPath)) return tryPath;

			// 2. 拡張子を付与して検索
			for (const auto& ext : extensions)
			{
				std::string tryPathExt = tryPath + ext;
				if (std::filesystem::exists(tryPathExt)) return tryPathExt;
			}
		}

		return "";	// 見つからない
	}

	// Texture 取得
	// ------------------------------------------------------------
	std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& keyName)
	{
		if (m_textures.find(keyName) != m_textures.end()) return m_textures[keyName];

		std::string path = ResolvePath(keyName, m_textureDirs, m_imgExts);
		if (path.empty())
		{
			// 見つからない場合、絶対パスか確認、ダメならエラー
			if (std::filesystem::exists(keyName)) path = keyName;
			else
			{
				Logger::LogWarning("Texture not found: " + keyName + " -> Using White");
				return m_textures["White"]; // フォールバック
			}
		}

		auto tex = LoadTextureInternal(path);
		if (tex) m_textures[keyName] = tex; // キャッシュ登録
		else     m_textures[keyName] = m_textures["White"]; // 失敗時も登録して再試行を防ぐ

		return m_textures[keyName];
	}

	std::shared_ptr<Texture> ResourceManager::LoadTextureInternal(const std::string& path)
	{
		// DirectXTex を使用したロード処理
		std::wstring wpath = std::filesystem::path(path).wstring();
		DirectX::ScratchImage image;
		HRESULT hr;

		// 拡張子で判定
		std::string ext = std::filesystem::path(path).extension().string();
		if (ext == ".dds")
			hr = DirectX::LoadFromDDSFile(wpath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
		else if (ext == ".tga")
			hr = DirectX::LoadFromTGAFile(wpath.c_str(), nullptr, image);
		else
			hr = DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);

		if (FAILED(hr))
		{
			Logger::LogError("Failed to load texture file: " + path);
			return nullptr;
		}

		// SRV作成
		ComPtr<ID3D11ShaderResourceView> srv;
		hr = DirectX::CreateShaderResourceView(m_device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), srv.GetAddressOf());

		if (FAILED(hr)) return nullptr;

		auto tex = std::make_shared<Texture>();
		tex->filepath = path;
		tex->width = (int)image.GetMetadata().width;
		tex->height = (int)image.GetMetadata().height;
		tex->srv = srv;

		return tex;
	}

	void ResourceManager::ReloadTexture(const std::string& keyName)
	{
		// 既に存在する場合のみリロード
		if (m_textures.find(keyName) != m_textures.end())
		{
			// パス情報を取得して再ロード (WhiteなどはパスがSystem::なので弾く)
			std::string path = m_textures[keyName]->filepath;
			if (path.find("System::") == std::string::npos && std::filesystem::exists(path))
			{
				auto newTex = LoadTextureInternal(path);
				if (newTex) m_textures[keyName] = newTex;
				Logger::Log("Reloaded Texture: " + keyName);
			}
		}
	}

	void ResourceManager::AddResource(const std::string& key, std::shared_ptr<Texture> resource)
	{
		// マップ名は推測
		if (m_textures.find(key) == m_textures.end())
		{
			m_textures[key] = resource;
		}
		else
		{
			m_textures[key] = resource;
		}
	}

	// -------------------------------------------------------------------------
	// Model 取得
	// -------------------------------------------------------------------------
	std::shared_ptr<Model> ResourceManager::GetModel(const std::string& keyName)
	{
		static int callCount = 0;
		callCount++;
		if (callCount % 100 == 0) Logger::Log("GetModel called 100 times. Last key: " + keyName);

		// 1. キャッシュ確認
		auto it = m_models.find(keyName);
		if (it != m_models.end()) return it->second;

		// 2. パス解決
		std::string path = ResolvePath(keyName, m_modelDirs, m_modelExts);
		if (path.empty()) path = keyName; // 見つからなくても一旦そのまま試す

		// 3. 解決後のパスでもう一度キャッシュ確認
		it = m_models.find(path);
		if (it != m_models.end())
		{
			m_models[keyName] = it->second;
			return it->second;
		}

		// 4. ロード実行
		auto model = LoadModelInternal(path);
		if (model)
		{
			// ロード成功時
			m_models[keyName] = model;
		}
		return model;
	}

	std::shared_ptr<Model> ResourceManager::LoadModelInternal(const std::string& path)
	{
		// キャッシュ再確認（ResolvePathでフルパスになった場合など）
		if (m_models.find(path) != m_models.end()) return m_models[path];

		auto model = std::make_shared<Model>();

		// Model内部ロード
		if (!model->Load(path)) {
			Logger::LogError("Failed to load model: " + path);
			return nullptr;
		}

		// キャッシュ登録
		m_models[path] = model;

		// ファイル名単体でも引けるように登録（利便性のため）
		std::string fileName = std::filesystem::path(path).filename().string();
		if (fileName != path) {
			m_models[fileName] = model;
		}

		return model;
	}


	// -------------------------------------------------------------------------
	// Sound 取得
	// -------------------------------------------------------------------------
	std::shared_ptr<Sound> ResourceManager::GetSound(const std::string& keyName)
	{
		if (m_sounds.find(keyName) != m_sounds.end()) return m_sounds[keyName];

		std::string path = ResolvePath(keyName, m_soundDirs, m_soundExts);
		if (path.empty())
		{
			Logger::LogError("Sound not found: " + keyName);
			return nullptr;
		}

		auto sound = LoadSoundInternal(path);
		if (sound) m_sounds[keyName] = sound;
		return sound;
	}

	// WAVファイルパース用構造体
	struct RIFF_HEADER { char chunkId[4]; unsigned int chunkSize; char format[4]; };
	struct CHUNK_HEADER { char chunkId[4]; unsigned int chunkSize; };

	std::shared_ptr<Sound> ResourceManager::LoadSoundInternal(const std::string& path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) return nullptr;

		RIFF_HEADER riff;
		file.read((char*)&riff, sizeof(RIFF_HEADER));

		if (strncmp(riff.chunkId, "RIFF", 4) != 0 || strncmp(riff.format, "WAVE", 4) != 0) {
			Logger::LogError("Invalid WAV file: " + path);
			return nullptr;
		}

		auto sound = std::make_shared<Sound>();
		sound->filepath = path;

		// チャンク探索
		while (true)
		{
			CHUNK_HEADER chunk;
			file.read((char*)&chunk, sizeof(CHUNK_HEADER));
			if (file.eof()) break;

			// fmt チャンク (フォーマット情報)
			if (strncmp(chunk.chunkId, "fmt ", 4) == 0)
			{
				file.read((char*)&sound->wfx, sizeof(WAVEFORMATEX));

				// fmtチャンクの残りをスキップ (拡張領域がある場合など)
				int skip = chunk.chunkSize - sizeof(WAVEFORMATEX);
				if (skip > 0) file.seekg(skip, std::ios::cur);
			}
			// data チャンク (波形データ)
			else if (strncmp(chunk.chunkId, "data", 4) == 0)
			{
				sound->buffer.resize(chunk.chunkSize);
				file.read((char*)sound->buffer.data(), chunk.chunkSize);

				// XAudio2バッファ設定
				sound->xBuffer.pAudioData = sound->buffer.data();
				sound->xBuffer.Flags = XAUDIO2_END_OF_STREAM;
				sound->xBuffer.AudioBytes = chunk.chunkSize;
				// 再生時間計算 (Bytes / BytesPerSec)
				if (sound->wfx.nAvgBytesPerSec > 0)
					sound->duration = (float)chunk.chunkSize / sound->wfx.nAvgBytesPerSec;

				break; // dataを読んだら終了とする（必要ならループ継続）
			}
			else
			{
				// 未知のチャンクはスキップ
				file.seekg(chunk.chunkSize, std::ios::cur);
			}
		}

		return sound;
	}

}	// namespace Arche