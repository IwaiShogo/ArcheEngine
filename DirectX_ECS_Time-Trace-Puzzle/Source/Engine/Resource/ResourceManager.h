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
#include "Engine/Graphics/Core/Texture.h"
#include "Engine/Graphics/Core/Model.h"
#include "Engine/Audio/Sound.h"

class ResourceManager
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

	// マニフェスト（JSON）を読み込んでパスを登録
	void LoadManifest(const std::string& jsonPath);

	void LoadAll();

	// テクスチャを取得
	std::shared_ptr<Texture> GetTexture(const std::string& key);
	// モデル取得
	std::shared_ptr<Model> GetModel(const std::string& key);
	// 音声取得
	std::shared_ptr<Sound> GetSound(const std::string& key);

	// デバッグ描画
	void OnInspector();

private:
	// 内部ロード関数（パス指定）
	std::shared_ptr<Texture> LoadTextureFromFile(const std::string& filepath);
	std::shared_ptr<Model> LoadModelFromFile(const std::string& filepath);
	std::shared_ptr<Sound> LoadWav(const std::string& filepath);

	// Assimpのノードを再帰的に処理する関数
	void ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<Model> model, const std::string& directory);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory);

	ResourceManager() = default;	// コンストラクタ隠蔽
	~ResourceManager() = default;

	ID3D11Device* m_device = nullptr;

	// テクスチャ用キャッシュ
	std::map<std::string, std::string> m_texturePaths;
	std::map<std::string, std::shared_ptr<Texture>> m_textures;

	// モデル用キャッシュ
	std::map<std::string, std::string> m_modelPaths;
	std::map<std::string, std::shared_ptr<Model>> m_models;

	// サウンド用キャッシュ
	std::map<std::string, std::string> m_soundPaths;
	std::map<std::string, std::shared_ptr<Sound>> m_sounds;
};

#endif // !___RESOURCE_MANAGER_H___