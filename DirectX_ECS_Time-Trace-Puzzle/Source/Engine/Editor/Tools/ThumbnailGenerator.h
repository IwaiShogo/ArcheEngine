/*****************************************************************//**
 * @file	ThumbnailGenerator.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/30	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___THUMBNAIL_GENERATOR_H___
#define ___THUMBNAIL_GENERATOR_H___

#include "Engine/pch.h"
#include "Engine/Graphics/Core/RenderTarget.h"
#include "Engine/Graphics/Renderers/ModelRenderer.h"
#include "Engine/Resource/ResourceManager.h"

class ThumbnailGenerator {
public:
	static ThumbnailGenerator& Instance() {
		static ThumbnailGenerator instance;
		return instance;
	}

	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, ModelRenderer* renderer) {
		m_device = device;
		m_context = context;
		m_renderer = renderer;
	}

	// 指定ディレクトリの全プレファブのサムネイルを生成する
	void GenerateAll(const std::string& prefabDir) {
		namespace fs = std::filesystem;
		if (!fs::exists(prefabDir)) return;

		// 一時的なレンダーターゲット (128x128)
		RenderTarget rt(m_device, 128, 128);

		// カメラ設定 (サムネイル用)
		// 斜め前から原点を見る
		DirectX::XMVECTOR eye = DirectX::XMVectorSet(2.0f, 2.0f, -3.0f, 0.0f);
		DirectX::XMVECTOR target = DirectX::XMVectorSet(0.0f, 0.5f, 0.0f, 0.0f); // 少し上を見る
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, target, up);
		DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1.0f, 0.1f, 100.0f);

		// ライティング
		DirectX::XMFLOAT3 lightDir = { 0.5f, -1.0f, 0.5f };
		DirectX::XMFLOAT3 lightColor = { 1.0f, 1.0f, 1.0f };

		for (const auto& entry : fs::directory_iterator(prefabDir)) {
			if (entry.path().extension() == ".json") {
				std::string filename = entry.path().filename().string();
				std::string path = entry.path().string();

				// JSONを読み込んでモデルキーを探す
				std::string modelKey = "";
				try {
					std::ifstream i(path);
					nlohmann::json j;
					i >> j;
					if (j.contains("MeshComponent")) {
						modelKey = j["MeshComponent"]["key"];
					}
				}
				catch (...) { continue; }

				if (!modelKey.empty()) {
					// モデル取得
					auto model = ResourceManager::Instance().GetModel(modelKey);
					if (model) {
						// 描画
						rt.Clear(m_context, 0.2f, 0.2f, 0.2f, 1.0f); // 背景グレー
						rt.Activate(m_context, nullptr); // 深度なし(簡易) または別途用意

						m_renderer->Begin(view, proj, lightDir, lightColor);
						m_renderer->Draw(model, { 0,0,0 }, { 1,1,1 }, { 0,0,0 });

						// テクスチャのコピーを作成して保存
						CreateTextureFromRT(&rt, filename);
					}
				}
			}
		}
	}

	// ファイル名に対応するテクスチャIDを取得
	void* GetThumbnailID(const std::string& filename) {
		if (m_thumbnails.count(filename)) {
			return m_thumbnails[filename].Get();
		}
		return nullptr; // なければnull
	}

private:
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
	ModelRenderer* m_renderer;

	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_thumbnails;

	// RenderTargetの内容を新しいテクスチャとして保存する
	void CreateTextureFromRT(RenderTarget* rt, const std::string& key) {
		// RTのテクスチャリソースを取得
		Microsoft::WRL::ComPtr<ID3D11Resource> res;
		rt->GetSRV()->GetResource(&res); // RenderTargetクラスにGetSRV()を追加する必要あり

		// コピー用テクスチャ作成
		D3D11_TEXTURE2D_DESC desc;
		((ID3D11Texture2D*)res.Get())->GetDesc(&desc);

		Microsoft::WRL::ComPtr<ID3D11Texture2D> newTex;
		m_device->CreateTexture2D(&desc, nullptr, &newTex);

		// コピー実行
		m_context->CopyResource(newTex.Get(), res.Get());

		// SRV作成
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newSrv;
		m_device->CreateShaderResourceView(newTex.Get(), nullptr, &newSrv);

		m_thumbnails[key] = newSrv;
	}
};

#endif // !___THUMBNAIL_GENERATOR_H___