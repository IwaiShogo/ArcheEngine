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
	// 内部ヘルパー (Assimp処理用)
	// ====================================================================================
	namespace
	{
		// 行列変換ヘルパー (Assimp -> DirectX)
		inline XMFLOAT4X4 AiMatrixToXM(const aiMatrix4x4& from)
		{
			XMFLOAT4X4 to;
			to._11 = from.a1; to._12 = from.b1; to._13 = from.c1; to._14 = from.d1;
			to._21 = from.a2; to._22 = from.b2; to._23 = from.c2; to._24 = from.d2;
			to._31 = from.a3; to._32 = from.b3; to._33 = from.c3; to._34 = from.d3;
			to._41 = from.a4; to._42 = from.b4; to._43 = from.c4; to._44 = from.d4;
			return to;
		}

		// ウェイト設定ヘルパー
		void SetVertexBoneData(ModelVertex& vertex, int boneID, float weight)
		{
			vertex.AddBoneData(boneID, weight);
		}

		// ボーン情報の抽出
		void ExtractBoneWeightForVertices(std::vector<ModelVertex>& vertices, aiMesh* mesh, std::shared_ptr<Model> model)
		{
			for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
			{
				int boneID = -1;
				std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

				// 新しいボーンなら登録
				if (model->boneInfoMap.find(boneName) == model->boneInfoMap.end())
				{
					BoneInfo newBoneInfo;
					newBoneInfo.id = model->boneCounter;
					newBoneInfo.offset = AiMatrixToXM(mesh->mBones[boneIndex]->mOffsetMatrix);
					model->boneInfoMap[boneName] = newBoneInfo;
					boneID = model->boneCounter;
					model->boneCounter++;
				}
				else
				{
					boneID = model->boneInfoMap[boneName].id;
				}

				assert(boneID != -1);

				// ウェイト情報の取得
				aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;
				int numWeights = mesh->mBones[boneIndex]->mNumWeights;

				for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
				{
					int vertexId = weights[weightIndex].mVertexId;
					float weight = weights[weightIndex].mWeight;
					SetVertexBoneData(vertices[vertexId], boneID, weight);
				}
			}
		}

		// メッシュ単体のロード処理
		Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, std::shared_ptr<Model> model, const std::string& directory, ID3D11Device* device)
		{
			Mesh myMesh;
			std::vector<ModelVertex> vertices;
			std::vector<uint32_t> indices;

			// 1. 頂点データ取得
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				ModelVertex vertex;
				// 位置
				vertex.position.x = mesh->mVertices[i].x;
				vertex.position.y = mesh->mVertices[i].y;
				vertex.position.z = mesh->mVertices[i].z;

				// 法線
				if (mesh->HasNormals())
				{
					vertex.normal.x = mesh->mNormals[i].x;
					vertex.normal.y = mesh->mNormals[i].y;
					vertex.normal.z = mesh->mNormals[i].z;
				}

				// UV座標 (0番目のチャンネルのみ)
				if (mesh->mTextureCoords[0])
				{
					vertex.uv.x = mesh->mTextureCoords[0][i].x;
					vertex.uv.y = mesh->mTextureCoords[0][i].y;
				}

				vertices.push_back(vertex);
			}

			// 2. ボーンウェイトの抽出
			ExtractBoneWeightForVertices(vertices, mesh, model);

			// ウェイトの正規化
			for (auto& v : vertices)
			{
				float sum = 0.0f;
				for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
				{
					if (v.boneIDs[i] != -1)
						sum += v.weights[i];
				}

				// 合計が0より大きければ、合計で割って1.0にする
				if (sum > 0.0f)
				{
					for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
					{
						if (v.boneIDs[i] != -1)
							v.weights[i] /= sum;
					}
				}

				if (vertices.size() > 0)
				{
					static bool s_logged = false;
					if (!s_logged)
					{
						s_logged = true; // 1回だけ出す
						auto& v = vertices[0];
						std::string log = "Vertex[0] BoneIDs: "
							+ std::to_string(v.boneIDs[0]) + ", "
							+ std::to_string(v.boneIDs[1]) + ", "
							+ std::to_string(v.boneIDs[2]) + ", "
							+ std::to_string(v.boneIDs[3]);
						Logger::Log(log);

						std::string wlog = "Vertex[0] Weights: "
							+ std::to_string(v.weights[0]) + ", "
							+ std::to_string(v.weights[1]) + ", "
							+ std::to_string(v.weights[2]) + ", "
							+ std::to_string(v.weights[3]);
						Logger::Log(wlog);
					}
				}
			}

			// 3. インデックスデータ取得
			for (unsigned int i = 0; i < mesh->mNumFaces; i++)
			{
				aiFace face = mesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; j++)
				{
					indices.push_back(face.mIndices[j]);
				}
			}
			myMesh.indexCount = (unsigned int)indices.size();

			// 4. バッファ作成
			// Vertex Buffer
			D3D11_BUFFER_DESC vbd = {};
			vbd.Usage = D3D11_USAGE_DEFAULT;
			vbd.ByteWidth = sizeof(ModelVertex) * (UINT)vertices.size();
			vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			D3D11_SUBRESOURCE_DATA vInit = {};
			vInit.pSysMem = vertices.data();
			device->CreateBuffer(&vbd, &vInit, myMesh.vertexBuffer.GetAddressOf());

			// Index Buffer
			D3D11_BUFFER_DESC ibd = {};
			ibd.Usage = D3D11_USAGE_DEFAULT;
			ibd.ByteWidth = sizeof(uint32_t) * (UINT)indices.size(); // 32bit Index
			ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			D3D11_SUBRESOURCE_DATA iInit = {};
			iInit.pSysMem = indices.data();
			device->CreateBuffer(&ibd, &iInit, myMesh.indexBuffer.GetAddressOf());

			// 5. マテリアル（テクスチャ）ロード
			if (mesh->mMaterialIndex >= 0)
			{
				aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
				if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
				{
					aiString str;
					material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
					std::string filename = std::filesystem::path(str.C_Str()).filename().string();
					std::string localPath = directory + "/" + filename;
					std::replace(localPath.begin(), localPath.end(), '\\', '/');

					if (std::filesystem::exists(localPath))
						myMesh.texture = ResourceManager::Instance().GetTexture(localPath);
					else
						myMesh.texture = ResourceManager::Instance().GetTexture(filename);
				}
			}
			if (!myMesh.texture) myMesh.texture = ResourceManager::Instance().GetTexture("White");

			return myMesh;
		}

		// ノードを再帰的に処理
		void ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<Model> model, const std::string& directory, ID3D11Device* device)
		{
			// このノードのメッシュを処理
			for (unsigned int i = 0; i < node->mNumMeshes; i++)
			{
				aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				model->meshes.push_back(ProcessMesh(mesh, scene, model, directory, device));
			}

			// 子ノードへ
			for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
				ProcessNode(node->mChildren[i], scene, model, directory, device);
			}
		}
	}

	void ResourceManager::Initialize(ID3D11Device* device)
	{
		m_device = device;
		CreateSystemTextures();
		Logger::Log("ResourceManager Initialized");
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
		// (文字コード変換などが必要ですが、簡易的に記述します)
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
		if (m_models.find(keyName) != m_models.end()) return m_models[keyName];

		std::string path = ResolvePath(keyName, m_modelDirs, m_modelExts);
		if (path.empty())
		{
			Logger::LogError("Model not found: " + keyName);
			return nullptr;
		}

		auto model = LoadModelInternal(path);
		if (model) m_models[keyName] = model;
		return model;
	}

	std::shared_ptr<Model> ResourceManager::LoadModelInternal(const std::string& path)
	{
		Assimp::Importer importer;
		// 読み込みフラグ: 三角形化、UVフリップ(DirectX用)、タンジェント計算
		const aiScene* scene = importer.ReadFile(path,
			aiProcess_Triangulate |
			aiProcess_FlipUVs |
			aiProcess_GenNormals |
			aiProcess_LimitBoneWeights |
			aiProcess_ConvertToLeftHanded
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			Logger::LogError("Assimp Error: " + std::string(importer.GetErrorString()));
			return nullptr;
		}

		auto model = std::make_shared<Model>();
		model->filepath = path;

		aiMatrix4x4 transform = scene->mRootNode->mTransformation;
		transform.Inverse();
		model->globalInverseTransform = AiMatrixToXM(transform);

		std::string directory = std::filesystem::path(path).parent_path().string();

		// ノードを再帰的に処理してメッシュを抽出
		ProcessNode(scene->mRootNode, scene, model, directory, m_device);

		Logger::Log("Loaded Model: " + path + " (" + std::to_string(model->meshes.size()) + " meshes)");
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

	// -------------------------------------------------------------------------
	// Animation 取得
	// -------------------------------------------------------------------------
	std::shared_ptr<AnimationClip> ResourceManager::GetAnimation(const std::string& keyName, std::shared_ptr<Model> model)
	{
		// 既にロード済みなら返す
		if (m_animations.find(keyName) != m_animations.end()) return m_animations[keyName];

		// パス解決
		std::string path = ResolvePath(keyName, m_animDirs, m_animExts);
		if (path.empty())
		{
			// ファイルが見つからない場合、もしかしたらModelと同じファイル内にあるかも？
			// その場合はモデルのパスを試す
			if (model && !model->filepath.empty())
			{
				path = model->filepath;
			}
			else
			{
				Logger::LogError("Animation not found: " + keyName);
				return nullptr;
			}
		}

		auto anim = LoadAnimationInternal(path, model);
		if (anim) m_animations[keyName] = anim;
		return anim;
	}

	std::shared_ptr<AnimationClip> ResourceManager::LoadAnimationInternal(const std::string& path, std::shared_ptr<Model> model)
	{
		// AnimationClipのコンストラクタでAssimpを使ってロードする
		// (以前作成した Animation.cpp の実装がここで動きます)
		auto anim = std::make_shared<AnimationClip>(path, model);

		if (anim->GetDuration() > 0.0f)
		{
			Logger::Log("Loaded Animation: " + path);
			return anim;
		}

		Logger::LogError("Failed to load animation (or no animation found): " + path);
		return nullptr;
	}

}	// namespace Arche