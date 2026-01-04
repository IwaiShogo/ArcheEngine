/*****************************************************************//**
 * @file	ResourceManager.h
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

#ifndef ___RESOURCE_MANAGER_H___
#define ___RESOURCE_MANAGER_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Core/Base/StringId.h"
#include "Engine/Renderer/RHI/Texture.h"
#include "Engine/Renderer/Data/Model.h"
#include "Engine/Audio/Sound.h"

#include "Engine/Audio/AudioManager.h"

namespace Arche
{

	class ARCHE_API ResourceManager
	{
	public:
		// シングルトン取得
		static ResourceManager& Instance()
		{
			static ResourceManager instance;
			return instance;
		}

		// 初期化（Deviceが必要）
		void Initialize(ID3D11Device* device);

		// 終了処理
		void Clear();

		// リソース取得（ファイル名のみでOK、拡張子省略可）
		// 例: GetTexture("Player") -> Resources/Game/Textures/Player.png
		// ------------------------------------------------------------
		std::shared_ptr<Texture> GetTexture(const std::string& keyName);
		std::shared_ptr<Model>   GetModel(const std::string& keyName);
		std::shared_ptr<Sound>   GetSound(const std::string& keyName);

		// ユーティリティ
		// ------------------------------------------------------------
		// 現在ロードされているリソース情報の取得（Resource Inspector用）
		const std::unordered_map<std::string, std::shared_ptr<Texture>>& GetTextureMap() const { return m_textures; }
		const std::unordered_map<std::string, std::shared_ptr<Model>>& GetModelMap() const { return m_models; }
		const std::unordered_map<std::string, std::shared_ptr<Sound>>& GetSoundMap() const { return m_sounds; }

		// 強制リロード
		void ReloadTexture(const std::string& keyName);

		void AddResource(const std::string& key, std::shared_ptr<Texture> resource);

	private:
		ResourceManager() = default;	// コンストラクタ隠蔽
		~ResourceManager() = default;

		// 内部: システムテクスチャ生成
		void CreateSystemTextures();

		// 内部: パス解決ロジック
		std::string ResolvePath(const std::string& keyName, const std::vector<std::string>& directories, const std::vector<std::string>& extensions);

		// ロード関数群
		std::shared_ptr<Texture> LoadTextureInternal(const std::string& path);
		std::shared_ptr<Model>   LoadModelInternal(const std::string& path);
		std::shared_ptr<Sound>   LoadSoundInternal(const std::string& path);

	private:
		ID3D11Device* m_device = nullptr;

		// リソースキャッシュ（Keyはファイル名 / 識別名）
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
		std::unordered_map<std::string, std::shared_ptr<Model>>   m_models;
		std::unordered_map<std::string, std::shared_ptr<Sound>>   m_sounds;

		// 検索パス設定
		const std::vector<std::string> m_textureDirs = { "Resources/Game/Textures/", "Resources/Engine/Textures/" };
		const std::vector<std::string> m_modelDirs = { "Resources/Game/Models/",   "Resources/Engine/Models/" };
		const std::vector<std::string> m_soundDirs = { "Resources/Game/Sounds/",   "Resources/Engine/Sounds/" };

		const std::vector<std::string> m_imgExts = { ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds" };
		const std::vector<std::string> m_modelExts = { ".fbx", ".obj", ".gltf", ".glb" };
		const std::vector<std::string> m_soundExts = { ".wav", ".mp3", ".ogg" };
	};

}	// namespace Arche

#endif // !___RESOURCE_MANAGER_H___