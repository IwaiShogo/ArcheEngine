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
#include "Engine/Core/StringId.h"
#include "Engine/Renderer/Core/Texture.h"
#include "Engine/Renderer/Core/Model.h"
#include "Engine/Audio/Sound.h"

class ResourceManager
{
public:
	// リソースの種類定義
	enum class ResourceType
	{
		Texture,
		Model,
		Sound,
		Font
	};

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
	std::shared_ptr<Texture> GetTexture(StringId key);
	// モデル取得
	std::shared_ptr<Model> GetModel(StringId key);
	// 音声取得
	std::shared_ptr<Sound> GetSound(StringId key);


	// --- エディタ連携用機能 ---

	// IDから元のファイルパスを取得（表示用）
	std::string GetPathByKey(StringId key, ResourceType type);

	// パスをキーとして動的に登録
	void RegisterResource(StringId key, const std::string& path, ResourceType type);

	// 指定した種類の登録済みパスリストを取得する
	std::vector<std::string> GetResourceList(ResourceType type);

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

	// パス管理用（StringId -> FilePath）
	std::unordered_map<StringId, std::string> m_texturePaths;
	std::unordered_map<StringId, std::string> m_modelPaths;
	std::unordered_map<StringId, std::string> m_soundPaths;

	// リソース本体（StringId -> Resource）
	std::unordered_map<StringId, std::shared_ptr<Texture>> m_textures;
	std::unordered_map<StringId, std::shared_ptr<Model>> m_models;
	std::unordered_map<StringId, std::shared_ptr<Sound>> m_sounds;
};

#endif // !___RESOURCE_MANAGER_H___
