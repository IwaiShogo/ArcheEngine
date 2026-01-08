/*****************************************************************//**
 * @file	Model.h
 * @brief	3Dモデルのデータを保持するクラス
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/11/26	初回作成日
 * 			作業内容：	- 追加：
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#ifndef ___MODEL_H___
#define ___MODEL_H___

#include "Engine/pch.h"
#include "Engine/Renderer/RHI/Texture.h"
#include "Engine/Renderer/RHI/MeshBuffer.h"

struct aiScene;
struct aiNode;

namespace Arche
{
	class Model
	{
	public:
		using AnimeNo = int;
		static const AnimeNo ANIME_NONE = -1;

		enum Flip { None, XFlip, ZFlip, ZFlipUseAnime };

		struct Bone {
			int index;
			XMMATRIX invOffset;
		};

		struct Mesh {
			std::vector<MeshBuffer::Vertex> vertices;
			std::vector<uint32_t> indices;
			unsigned int materialID;
			std::vector<Bone> bones;
			MeshBuffer* pMesh = nullptr;
		};

		struct Material {
			XMFLOAT4 diffuse;
			XMFLOAT4 ambient;
			XMFLOAT4 specular;
			std::shared_ptr<Texture> pTexture = nullptr; // shared_ptrに変更（ResourceManagerとの親和性のため）
		};

		struct Node {
			std::string name;
			int parent;
			std::vector<int> children;
			XMMATRIX mat;
			XMMATRIX localMat;
		};

		struct TransformData {
			XMFLOAT3 translate;
			XMFLOAT4 quaternion;
			XMFLOAT3 scale;
		};

		struct Animation {
			std::string name; // 名前を追加
			float totalTime = 0.0f;
			float nowTime = 0.0f;
			float speed = 1.0f;
			bool isLoop = false;

			struct Channel {
				int nodeIndex;
				std::vector<std::pair<float, XMFLOAT3>> positionKeys;
				std::vector<std::pair<float, XMFLOAT4>> rotationKeys;
				std::vector<std::pair<float, XMFLOAT3>> scalingKeys;
			};
			std::vector<Channel> channels;
		};

	public:
		Model();
		~Model();

		bool Load(const std::string& filename, float scale = 1.0f, Flip flip = None);
		void Reset();

		// 描画（レンダラーから呼ばれる）
		const std::vector<Mesh>& GetMeshes() const { return m_meshes; }
		const std::vector<Node>& GetNodes() const { return m_nodes; }
		const Material* GetMaterial(size_t index) const { return &m_materials[index]; }

		// アニメーション操作
		void Step(float deltaTime);

		// ファイルからアニメーションを追加してIDを返す
		AnimeNo AddAnimation(const std::string& filename);

		// ID指定再生
		void Play(AnimeNo no, bool loop = true, float speed = 1.0f);

		// ★追加: 名前指定再生（Animatorコンポーネント用）
		void Play(const std::string& animName, bool loop = true, float speed = 1.0f);

		// 現在再生中のアニメIDを取得
		AnimeNo GetCurrentAnimation() const { return m_playNo; }

	private:
		void MakeMesh(const aiScene* pScene, float scale, Flip flip);
		void MakeMaterial(const aiScene* pScene, const std::string& directory);
		void MakeBoneNodes(const aiScene* pScene);
		void MakeWeight(const aiScene* pScene, int meshIdx);

		void UpdateNodeTransforms();
		void CalcAnimation(AnimeNo no);

		XMFLOAT3 Lerp3(const XMFLOAT3& a, const XMFLOAT3& b, float t);
		XMFLOAT4 Slerp4(const XMFLOAT4& a, const XMFLOAT4& b, float t);

	private:
		std::vector<Mesh> m_meshes;
		std::vector<Material> m_materials;
		std::vector<Node> m_nodes;
		std::vector<Animation> m_animes;

		AnimeNo m_playNo = ANIME_NONE;
		std::vector<TransformData> m_currentTransforms;

		float m_loadScale = 1.0f;
		Flip m_loadFlip = None;
	};
}
#endif