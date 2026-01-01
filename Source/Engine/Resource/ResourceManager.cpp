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
#include "Engine/Audio/AudioManager.h"
#include "Engine/Core/Base/Logger.h"
#include "Engine/Renderer/Text/FontManager.h"

namespace Arche
{

	// ファイルヘッダ構造体
	struct RIFF_HEADER
	{
		char chunkId[4];	// "RIFF"
		unsigned int chunkSize;
		char format[4];		// "WAVE"
	};
	struct CHUNK_HEADER
	{
		char chunkId[4];	// "fmt" or "data"
		unsigned int chunkSize;
	};

	// ワイド文字列変換用ヘルパー
	std::wstring ToWString(const std::string& str)
	{
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

	void ResourceManager::Initialize(ID3D11Device* device)
	{
		m_device = device;
	}

	void ResourceManager::LoadManifest(const std::string& jsonPath) {
		std::ifstream file(jsonPath);
		if (!file.is_open()) {
			OutputDebugStringA(("Failed to open manifest: " + jsonPath + "\n").c_str());
			return;
		}

		try {
			nlohmann::json j;
			file >> j;

			// "textures" セクションを読み込む
			if (j.contains("textures")) {
				for (auto& element : j["textures"].items()) {
					std::string key = element.key();
					std::string path = element.value()["path"];
					// パスを登録 (std::string -> StringId は暗黙変換される)
					m_texturePaths[key] = path;
				}
			}
			// "models" セクションを読み込む
			if (j.contains("models")) {
				for (auto& element : j["models"].items()) {
					std::string key = element.key();
					std::string path = element.value()["path"];
					m_modelPaths[key] = path;
				}
			}
			// "sounds" セクションを読み込む
			if (j.contains("sounds")) {
				for (auto& element : j["sounds"].items()) {
					std::string key = element.key();
					std::string path = element.value()["path"];
					m_soundPaths[key] = path;
				}
			}
		}
		catch (const std::exception& e) {
			OutputDebugStringA(("JSON Parse Error: " + std::string(e.what()) + "\n").c_str());
		}
	}

	void ResourceManager::LoadAll()
	{
		// テクスチャ全ロード
		for (auto& pair : m_texturePaths) {
			LoadTextureFromFile(pair.second);
		}
		// モデル全ロード
		for (auto& pair : m_modelPaths) {
			LoadModelFromFile(pair.second);
		}
		// サウンド全ロード
		for (auto& pair : m_soundPaths) {
			LoadWav(pair.second);
		}
	}

	// キー名から取得 (引数を StringId に変更)
	std::shared_ptr<Texture> ResourceManager::GetTexture(StringId key) {
		// 1. キャッシュチェック (StringIdで高速検索)
		auto itCache = m_textures.find(key);
		if (itCache != m_textures.end()) {
			return itCache->second;
		}

		// 2. パスリストチェック
		auto itPath = m_texturePaths.find(key);
		if (itPath == m_texturePaths.end()) {
			// 未登録の場合はnullptr
			// OutputDebugStringA("Texture Key Not Found\n"); 
			return nullptr;
		}

		// 3. ロードしてキャッシュ
		std::string filepath = itPath->second;
		auto texture = LoadTextureFromFile(filepath);
		if (texture) {
			m_textures[key] = texture;
		}
		return texture;
	}

	std::shared_ptr<Model> ResourceManager::GetModel(StringId key)
	{
		auto itCache = m_models.find(key);
		if (itCache != m_models.end()) return itCache->second;

		auto itPath = m_modelPaths.find(key);
		if (itPath == m_modelPaths.end()) return nullptr;

		std::string filepath = itPath->second;
		auto model = LoadModelFromFile(filepath);
		if (model) {
			m_models[key] = model;
		}
		return model;
	}

	std::shared_ptr<Sound> ResourceManager::GetSound(StringId key)
	{
		auto itCache = m_sounds.find(key);
		if (itCache != m_sounds.end()) return itCache->second;

		auto itPath = m_soundPaths.find(key);
		if (itPath == m_soundPaths.end()) return nullptr;

		std::string filepath = itPath->second;
		auto sound = LoadWav(filepath);
		if (sound) {
			m_sounds[key] = sound;
		}
		return sound;
	}

	std::string ResourceManager::GetPathByKey(StringId key, ResourceType type)
	{
		if (type == ResourceType::Texture) {
			auto it = m_texturePaths.find(key);
			if (it != m_texturePaths.end()) return it->second;
		}
		else if (type == ResourceType::Model) {
			auto it = m_modelPaths.find(key);
			if (it != m_modelPaths.end()) return it->second;
		}
		else if (type == ResourceType::Sound) {
			auto it = m_soundPaths.find(key);
			if (it != m_soundPaths.end()) return it->second;
		}
		return ""; // 見つからない
	}

	void ResourceManager::RegisterResource(StringId key, const std::string& path, ResourceType type)
	{
		// 既に登録済みなら何もしない（上書き防止）
		if (type == ResourceType::Texture) {
			if (m_texturePaths.find(key) == m_texturePaths.end()) m_texturePaths[key] = path;
		}
		else if (type == ResourceType::Model) {
			if (m_modelPaths.find(key) == m_modelPaths.end()) m_modelPaths[key] = path;
		}
		else if (type == ResourceType::Sound) {
			if (m_soundPaths.find(key) == m_soundPaths.end()) m_soundPaths[key] = path;
		}
	}

	std::vector<std::string> ResourceManager::GetResourceList(ResourceType type)
	{
		std::vector<std::string> list;

		if (type == ResourceType::Texture)
		{
			for (const auto& pair : m_texturePaths)
			{
				list.push_back(pair.first.c_str());
			}
		}
		else if (type == ResourceType::Model)
		{
			for (const auto& pair : m_modelPaths)
			{
				list.push_back(pair.first.c_str());
			}
		}
		else if (type == ResourceType::Sound)
		{
			for (const auto& pair : m_soundPaths)
			{
				list.push_back(pair.first.c_str());
			}
		}

		// 見やすいようにソートしておく
		std::sort(list.begin(), list.end());
		return list;
	}

	// 内部ロード関数
	std::shared_ptr<Texture> ResourceManager::LoadTextureFromFile(const std::string& filepath) {
		if (!std::filesystem::exists(filepath)) {
			OutputDebugStringA(("Texture Not Found: " + filepath + "\n").c_str());
			return nullptr;
		}

		DirectX::ScratchImage image;
		HRESULT hr = E_FAIL;
		std::wstring wpath = ToWString(filepath);
		std::string ext = std::filesystem::path(filepath).extension().string();

		if (ext == ".dds" || ext == ".DDS")
			hr = DirectX::LoadFromDDSFile(wpath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
		else
			hr = DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);

		if (FAILED(hr)) return nullptr;

		auto texture = std::make_shared<Texture>();
		texture->filepath = filepath; // ここで保存されたパスを使います
		texture->width = static_cast<int>(image.GetMetadata().width);
		texture->height = static_cast<int>(image.GetMetadata().height);

		hr = DirectX::CreateShaderResourceView(m_device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &texture->srv);
		if (FAILED(hr)) return nullptr;

		return texture;
	}

	std::shared_ptr<Model> ResourceManager::LoadModelFromFile(const std::string& filepath)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(
			filepath,
			aiProcess_Triangulate |
			aiProcess_ConvertToLeftHanded
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			OutputDebugStringA(("Assimp Error: " + std::string(importer.GetErrorString()) + "\n").c_str());
			return nullptr;
		}

		auto model = std::make_shared<Model>();
		model->filepath = filepath; // ここで保存されたパスを使います

		std::string directory = std::filesystem::path(filepath).parent_path().string();
		ProcessNode(scene->mRootNode, scene, model, directory);

		return model;
	}

	std::shared_ptr<Sound> ResourceManager::LoadWav(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			OutputDebugStringA(("Failed to open WAV: " + filepath + "\n").c_str());
			return nullptr;
		}

		RIFF_HEADER riff;
		file.read((char*)&riff, sizeof(RIFF_HEADER));

		if (strncmp(riff.chunkId, "RIFF", 4) != 0 || strncmp(riff.format, "WAVE", 4) != 0)
		{
			return nullptr; // Not a WAV file
		}

		auto sound = std::make_shared<Sound>();
		sound->filepath = filepath; // ここで保存されたパスを使います

		while (!file.eof())
		{
			CHUNK_HEADER chunk;
			file.read((char*)&chunk, sizeof(CHUNK_HEADER));
			if (file.eof()) break;

			if (strncmp(chunk.chunkId, "fmt ", 4) == 0)
			{
				file.read((char*)&sound->wfx, std::min((size_t)chunk.chunkSize, sizeof(WAVEFORMATEX)));
				if (chunk.chunkSize > sizeof(WAVEFORMATEX))
				{
					file.seekg(chunk.chunkSize - sizeof(WAVEFORMATEX), std::ios::cur);
				}
			}
			else if (strncmp(chunk.chunkId, "data", 4) == 0)
			{
				sound->buffer.resize(chunk.chunkSize);
				file.read((char*)sound->buffer.data(), chunk.chunkSize);

				sound->xBuffer.pAudioData = sound->buffer.data();
				sound->xBuffer.AudioBytes = chunk.chunkSize;
				sound->xBuffer.Flags = XAUDIO2_END_OF_STREAM;

				if (sound->wfx.nAvgBytesPerSec > 0)
				{
					sound->duration = static_cast<float>(chunk.chunkSize) / static_cast<float>(sound->wfx.nAvgBytesPerSec);
				}
				break;
			}
			else
			{
				file.seekg(chunk.chunkSize, std::ios::cur);
			}
		}

		return sound;
	}

	void ResourceManager::ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<Model> model, const std::string& directory)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			model->meshes.push_back(ProcessMesh(mesh, scene, directory));
		}
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, model, directory);
		}
	}

	Mesh ResourceManager::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory)
	{
		std::vector<ModelVertex> vertices;
		std::vector<unsigned int> indices;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			ModelVertex vertex;
			vertex.position.x = mesh->mVertices[i].x;
			vertex.position.y = mesh->mVertices[i].y;
			vertex.position.z = mesh->mVertices[i].z;

			if (mesh->HasNormals()) {
				vertex.normal.x = mesh->mNormals[i].x;
				vertex.normal.y = mesh->mNormals[i].y;
				vertex.normal.z = mesh->mNormals[i].z;
			}
			else {
				vertex.normal = { 0, 1, 0 };
			}

			if (mesh->mTextureCoords[0]) {
				vertex.uv.x = mesh->mTextureCoords[0][i].x;
				vertex.uv.y = mesh->mTextureCoords[0][i].y;
			}
			else {
				vertex.uv = { 0, 0 };
			}
			vertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}

		std::shared_ptr<Texture> texture = nullptr;
		if (mesh->mMaterialIndex >= 0) {
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
				aiString str;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
				std::string texPath = directory + "/" + str.C_Str();
				texture = LoadTextureFromFile(texPath);
			}
		}

		Mesh retMesh;
		retMesh.indexCount = static_cast<unsigned int>(indices.size());
		retMesh.texture = texture;

		D3D11_BUFFER_DESC vbd = {};
		vbd.Usage = D3D11_USAGE_DEFAULT;
		vbd.ByteWidth = sizeof(ModelVertex) * vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		D3D11_SUBRESOURCE_DATA vInit = {};
		vInit.pSysMem = vertices.data();
		m_device->CreateBuffer(&vbd, &vInit, &retMesh.vertexBuffer);

		D3D11_BUFFER_DESC ibd = {};
		ibd.Usage = D3D11_USAGE_DEFAULT;
		ibd.ByteWidth = sizeof(unsigned int) * indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		D3D11_SUBRESOURCE_DATA iInit = {};
		iInit.pSysMem = indices.data();
		m_device->CreateBuffer(&ibd, &iInit, &retMesh.indexBuffer);

		return retMesh;
	}

	// ------------------------------------------------------------
	// デバッグ表示 (StringId対応版)
	// ------------------------------------------------------------
	void ResourceManager::OnInspector() {
		ImGui::Begin("Resource Monitor");

		// ドラッグ用ヘルパー
		auto SetDragDropSource = [](const std::string& path) {
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::SetDragDropPayload("ASSET_PATH", path.c_str(), path.length() + 1);
				ImGui::Text("%s", path.c_str());
				ImGui::EndDragDropSource();
			}
			};

		// ------------------------------------------------------------
		// 1. Textures
		// ------------------------------------------------------------
		if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
			// パスごとに集約して重複を排除
			std::map<std::string, std::vector<StringId>> uniquePaths;
			for (const auto& pair : m_texturePaths) {
				uniquePaths[pair.second].push_back(pair.first);
			}

			ImGui::Text("Unique: %d (Total Keys: %d) / Loaded: %d",
				uniquePaths.size(), m_texturePaths.size(), m_textures.size());

			if (ImGui::BeginTable("TexTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("Path");
				ImGui::TableSetupColumn("Status");
				ImGui::TableSetupColumn("Preview");
				ImGui::TableHeadersRow();

				for (const auto& pair : uniquePaths) {
					const std::string& path = pair.first;
					// このパスに対応するリソースがロードされているか確認
					// LoadTextureFromFileは StringId(path) をキーにキャッシュする仕様なので、それをチェック
					bool isLoaded = m_textures.find(StringId(path)) != m_textures.end();
					// 念のため、エイリアスキーでのロードもチェック
					if (!isLoaded) {
						for (auto k : pair.second) {
							if (m_textures.find(k) != m_textures.end()) { isLoaded = true; break; }
						}
					}

					ImGui::TableNextRow();

					// Path
					ImGui::TableSetColumnIndex(0);
					ImGui::Selectable(path.c_str());
					SetDragDropSource(path);

					// Status
					ImGui::TableSetColumnIndex(1);
					if (isLoaded) ImGui::TextColored(ImVec4(0, 1, 0, 1), "Loaded");
					else		  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "Unloaded");

					// Preview
					ImGui::TableSetColumnIndex(2);
					if (isLoaded) {
						// ロード済みならキャッシュから取得（キーは何でも良いが、StringId(path)が確実）
						auto it = m_textures.find(StringId(path));
						if (it == m_textures.end() && !pair.second.empty()) it = m_textures.find(pair.second[0]);

						if (it != m_textures.end()) {
							auto& tex = it->second;
							if (tex && tex->srv) {
								ImGui::Image((void*)tex->srv.Get(), ImVec2(32, 32));
								SetDragDropSource(path); // 画像もドラッグ可能に
								if (ImGui::IsItemHovered()) {
									ImGui::BeginTooltip();
									ImGui::Image((void*)tex->srv.Get(), ImVec2(256, 256));
									ImGui::EndTooltip();
								}
							}
						}
					}
				}
				ImGui::EndTable();
			}
		}

		// ------------------------------------------------------------
		// 2. Models
		// ------------------------------------------------------------
		if (ImGui::CollapsingHeader("Models")) {
			std::map<std::string, std::vector<StringId>> uniquePaths;
			for (const auto& pair : m_modelPaths) uniquePaths[pair.second].push_back(pair.first);

			ImGui::Text("Unique: %d / Loaded: %d", uniquePaths.size(), m_models.size());

			for (const auto& pair : uniquePaths) {
				const std::string& path = pair.first;
				bool isLoaded = m_models.find(StringId(path)) != m_models.end();
				if (!isLoaded) {
					for (auto k : pair.second) { if (m_models.find(k) != m_models.end()) { isLoaded = true; break; } }
				}

				// ツリーノード表示
				bool nodeOpen = ImGui::TreeNode(path.c_str());
				SetDragDropSource(path);

				if (nodeOpen) {
					if (isLoaded) {
						auto it = m_models.find(StringId(path));
						if (it == m_models.end() && !pair.second.empty()) it = m_models.find(pair.second[0]);

						if (it != m_models.end()) {
							auto& model = it->second;
							ImGui::TextColored(ImVec4(0, 1, 0, 1), "[Loaded]");
							ImGui::Text("Mesh Count: %d", model->meshes.size());
						}
					}
					else {
						ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "[Unloaded]");
						ImGui::SameLine();
						if (ImGui::SmallButton("Load")) {
							GetModel(StringId(path)); // パスIDでロード
						}
					}
					ImGui::TreePop();
				}
			}
		}

		// ------------------------------------------------------------
		// 3. Sounds
		// ------------------------------------------------------------
		if (ImGui::CollapsingHeader("Sounds")) {
			std::map<std::string, std::vector<StringId>> uniquePaths;
			for (const auto& pair : m_soundPaths) uniquePaths[pair.second].push_back(pair.first);

			ImGui::Text("Unique: %d / Loaded: %d", uniquePaths.size(), m_sounds.size());

			for (const auto& pair : uniquePaths) {
				const std::string& path = pair.first;
				bool isLoaded = m_sounds.find(StringId(path)) != m_sounds.end();
				if (!isLoaded) {
					for (auto k : pair.second) { if (m_sounds.find(k) != m_sounds.end()) { isLoaded = true; break; } }
				}

				ImGui::Selectable(path.c_str());
				SetDragDropSource(path);
				ImGui::SameLine();

				if (isLoaded) {
					auto it = m_sounds.find(StringId(path));
					if (it == m_sounds.end() && !pair.second.empty()) it = m_sounds.find(pair.second[0]);

					if (it != m_sounds.end()) {
						auto& sound = it->second;
						ImGui::TextColored(ImVec4(0, 1, 0, 1), "[Loaded]");
						ImGui::SameLine();
						if (ImGui::Button(("Play##" + path).c_str())) {
							// 再生時は StringId(path) を使う（AudioManager側でこれをキーに検索するため）
							AudioManager::Instance().PlaySE(StringId(path));
						}
					}
				}
				else {
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "[Unloaded]");
					ImGui::SameLine();
					if (ImGui::SmallButton(("Load##" + path).c_str())) {
						GetSound(StringId(path));
					}
				}
			}
		}

		// ------------------------------------------------------------
		// 4. Fonts (Debug Info)
		// ------------------------------------------------------------
		if (ImGui::CollapsingHeader("Fonts (Loaded)")) {
			const auto& fontNames = FontManager::Instance().GetLoadedFontNames();

			ImGui::Text("Loaded Custom Fonts: %d", fontNames.size());

			if (ImGui::BeginTable("FontsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("Family Name (Use this key!)");
				ImGui::TableSetupColumn("Action");
				ImGui::TableHeadersRow();

				for (const auto& name : fontNames) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", name.c_str());

					// クリップボードにコピー機能
					ImGui::TableSetColumnIndex(1);
					std::string btnLabel = "Copy##" + name;
					if (ImGui::Button(btnLabel.c_str())) {
						ImGui::SetClipboardText(name.c_str());
					}
				}
				ImGui::EndTable();
			}
		}

		ImGui::End();
	}

	void ResourceManager::Clear()
	{
		m_textures.clear();
		m_models.clear();
		m_sounds.clear();
	}

}	// namespace Arche