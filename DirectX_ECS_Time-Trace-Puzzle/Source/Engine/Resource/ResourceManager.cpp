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

				// パスを登録
				m_texturePaths[key] = path;

				// ログ (任意)
				// OutputDebugStringA(("Registered Texture: " + key + " -> " + path + "\n").c_str());
			}
		}
		// "models" セクションを読み込む
		if (j.contains("models")) {
			for (auto& element : j["models"].items()) {
				std::string key = element.key();
				std::string path = element.value()["path"];

				// パスを登録
				m_modelPaths[key] = path;
			}
		}
		// "sounds" セクションを読み込む
		if (j.contains("sounds")) {
			for (auto& element : j["sounds"].items()) {
				std::string key = element.key();
				std::string path = element.value()["path"];

				// パスを登録
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

// キー名から取得
std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& key) {
	// 1. キーが登録されているか確認
	auto itPath = m_texturePaths.find(key);
	if (itPath == m_texturePaths.end()) {
		OutputDebugStringA(("Texture Key Not Found: " + key + "\n").c_str());

		// 未登録の場合、キー自体をパスとみなしてトライする（開発中の利便性のため）
		// return LoadTextureFromFile(key); 
		return nullptr;
	}

	std::string filepath = itPath->second;

	// 2. キャッシュチェック
	auto itCache = m_textures.find(filepath);
	if (itCache != m_textures.end()) {
		return itCache->second;
	}

	// 3. ロードしてキャッシュ
	return LoadTextureFromFile(filepath);
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::string& key)
{
	auto itPath = m_modelPaths.find(key);
	if (itPath == m_modelPaths.end()) return nullptr;

	std::string filepath = itPath->second;
	auto itCache = m_models.find(filepath);
	if (itCache != m_models.end()) return itCache->second;

	return LoadModelFromFile(filepath);
}

std::shared_ptr<Sound> ResourceManager::GetSound(const std::string& key)
{
	// 1. キー登録チェック
	auto itPath = m_soundPaths.find(key);
	std::string filepath;

	if (itPath != m_soundPaths.end())
	{
		filepath = itPath->second;
	}
	else
	{
		filepath = key;
	}

	// 2. キャッシュをチェック
	auto itCache = m_sounds.find(filepath);
	if (itCache != m_sounds.end()) return itCache->second;

	// 3. ロード
	return LoadWav(filepath);
}

// 内部ロード関数 (前回のLoadTextureの中身を移動)
std::shared_ptr<Texture> ResourceManager::LoadTextureFromFile(const std::string& filepath) {
	// ※ 前回の LoadTexture の中身をそのままここにコピペしてください ※
	// (キャッシュチェック部分は削除してOKですが、残っていても二重チェックになるだけなので問題ありません)

	// --- 以下コピペ用再掲 ---
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
	else if (ext == ".tga" || ext == ".TGA") {}
		//hr = DirectX::LoadFromTGAFile(wpath.c_str(), nullptr, image);
	else
		hr = DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);

	if (FAILED(hr)) return nullptr;

	auto texture = std::make_shared<Texture>();
	texture->filepath = filepath;
	texture->width = static_cast<int>(image.GetMetadata().width);
	texture->height = static_cast<int>(image.GetMetadata().height);

	hr = DirectX::CreateShaderResourceView(m_device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &texture->srv);
	if (FAILED(hr)) return nullptr;

	m_textures[filepath] = texture; // パスでキャッシュ
	return texture;
}

std::shared_ptr<Model> ResourceManager::LoadModelFromFile(const std::string& filepath)
{
	Assimp::Importer importer;
	// 読み込みフラグ: 三角形化 | UV反転（DirectX用） | 法線計算
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
	model->filepath = filepath;

	// ディレクトリパスの取得（テクスチャ読み込み用）
	std::string directory = std::filesystem::path(filepath).parent_path().string();

	// ノード解析開始
	ProcessNode(scene->mRootNode, scene, model, directory);

	m_models[filepath] = model;
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
	sound->filepath = filepath;

	// チャンク解析
	while (!file.eof())
	{
		CHUNK_HEADER chunk;
		file.read((char*)&chunk, sizeof(CHUNK_HEADER));
		if (file.eof()) break;

		if (strncmp(chunk.chunkId, "fmt ", 4) == 0)
		{
			// フォーマット情報
			file.read((char*)&sound->wfx, std::min((size_t)chunk.chunkSize, sizeof(WAVEFORMATEX)));

			// 余分なデータがあればスキップ
			if (chunk.chunkSize > sizeof(WAVEFORMATEX))
			{
				file.seekg(chunk.chunkSize - sizeof(WAVEFORMATEX), std::ios::cur);
			}
		}
		else if (strncmp(chunk.chunkId, "data", 4) == 0)
		{
			// 波形データ
			sound->buffer.resize(chunk.chunkSize);
			file.read((char*)sound->buffer.data(), chunk.chunkSize);

			// XAudio2用バッファ設定
			sound->xBuffer.pAudioData = sound->buffer.data();
			sound->xBuffer.AudioBytes = chunk.chunkSize;
			sound->xBuffer.Flags = XAUDIO2_END_OF_STREAM;

			// 再生時間の計算
			if (sound->wfx.nAvgBytesPerSec > 0)
			{
				sound->duration = static_cast<float>(chunk.chunkSize) / static_cast<float>(sound->wfx.nAvgBytesPerSec);
			}

			// 読み込み完了
			break;
		}
		else
		{
			// 知らないチャンクはスキップ
			file.seekg(chunk.chunkSize, std::ios::cur);
		}
	}

	m_sounds[filepath] = sound;
	return sound;
}

void ResourceManager::ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<Model> model, const std::string& directory)
{
	// ノード内の全メッシュを処理
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		model->meshes.push_back(ProcessMesh(mesh, scene, directory));
	}
	// 子ノードへ
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, model, directory);
	}
}

Mesh ResourceManager::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory)
{
	std::vector<ModelVertex> vertices;
	std::vector<unsigned int> indices;

	// 1. 頂点情報の抽出
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		ModelVertex vertex;

		// 位置
		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;

		// 法線
		if (mesh->HasNormals()) {
			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;
		}
		else {
			vertex.normal = { 0, 1, 0 };
		}

		// UV (0番目のUVチャンネルのみ使用)
		if (mesh->mTextureCoords[0]) {
			vertex.uv.x = mesh->mTextureCoords[0][i].x;
			vertex.uv.y = mesh->mTextureCoords[0][i].y;
		}
		else {
			vertex.uv = { 0, 0 };
		}

		vertices.push_back(vertex);
	}

	// 2. インデックス情報の抽出
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	// 3. マテリアル（テクスチャ）の処理
	std::shared_ptr<Texture> texture = nullptr;
	if (mesh->mMaterialIndex >= 0) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		// Diffuseテクスチャがあるか確認
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString str;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &str);

			// フルパスを作成してロード
			std::string texPath = directory + "/" + str.C_Str();

			// 内部ロード関数を再利用（LoadTextureFromFileはpublicにしておくと便利かも）
			texture = LoadTextureFromFile(texPath);
		}
	}

	// 4. DirectXバッファの作成
	Mesh retMesh;
	retMesh.indexCount = static_cast<unsigned int>(indices.size());
	retMesh.texture = texture;

	// Vertex Buffer
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(ModelVertex) * vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vInit = {};
	vInit.pSysMem = vertices.data();
	m_device->CreateBuffer(&vbd, &vInit, &retMesh.vertexBuffer);

	// Index Buffer
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(unsigned int) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA iInit = {};
	iInit.pSysMem = indices.data();
	m_device->CreateBuffer(&ibd, &iInit, &retMesh.indexBuffer);

	return retMesh;
}

void ResourceManager::OnInspector() {
	ImGui::Begin("Resource Monitor");

	// ------------------------------------------------------------
	// 1. Textures (画像)
	// ------------------------------------------------------------
	if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Loaded: %d", m_textures.size());

		// テーブル表示
		if (ImGui::BeginTable("TexTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Key");
			ImGui::TableSetupColumn("Size");
			ImGui::TableSetupColumn("Preview");
			ImGui::TableHeadersRow();

			for (const auto& pair : m_textures) {
				const std::string& path = pair.first; // mapのキー(ファイルパス)
				auto& tex = pair.second;

				ImGui::TableNextRow();

				// Key (Path)
				ImGui::TableSetColumnIndex(0);
				ImGui::TextWrapped("%s", path.c_str());

				// Size
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%d x %d", tex->width, tex->height);

				// Preview (マウスホバーで拡大表示)
				ImGui::TableSetColumnIndex(2);
				if (tex->srv) {
					// サムネイル表示 (32x32)
					ImGui::Image((void*)tex->srv.Get(), ImVec2(32, 32));

					// ホバー時のツールチップ拡大表示
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Image((void*)tex->srv.Get(), ImVec2(256, 256));
						ImGui::EndTooltip();
					}
				}
			}
			ImGui::EndTable();
		}
	}

	// ------------------------------------------------------------
	// 2. Models (3Dモデル)
	// ------------------------------------------------------------
	if (ImGui::CollapsingHeader("Models")) {
		ImGui::Text("Loaded: %d", m_models.size());

		for (const auto& pair : m_models) {
			const std::string& path = pair.first;
			auto& model = pair.second;

			if (ImGui::TreeNode(path.c_str())) {
				ImGui::Text("Mesh Count: %d", model->meshes.size());
				// メッシュごとの詳細
				for (size_t i = 0; i < model->meshes.size(); ++i) {
					ImGui::Text("  Mesh %d: %d indices", i, model->meshes[i].indexCount);
				}
				ImGui::TreePop();
			}
		}
	}

	// ------------------------------------------------------------
	// 3. Sounds (音声)
	// ------------------------------------------------------------
	if (ImGui::CollapsingHeader("Sounds")) {
		ImGui::Text("Loaded: %d", m_sounds.size());

		for (const auto& pair : m_sounds) {
			const std::string& path = pair.first;
			auto& sound = pair.second;

			ImGui::Text("%s", path.c_str());
			ImGui::SameLine();
			ImGui::TextDisabled("(%.2f sec)", sound->duration);

			// 再生テストボタン
			ImGui::SameLine();
			std::string btnLabel = "Play##" + path;
			if (ImGui::Button(btnLabel.c_str()))
			{
				AudioManager::Instance().PlaySE(path);
			}
		}
	}

	ImGui::End();
}