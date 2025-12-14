/*****************************************************************//**
 * @file	Components.h
 * @brief	基本的なコンポーネント
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___COMPONENTS_H___
#define ___COMPONENTS_H___

// ===== インクルード =====
#define NOMINMAX
#include <DirectXMath.h>
#include <vector>
#include <string>
#include "main.h"

using namespace DirectX;

// ============================================================
// 基本コンポーネント
// ============================================================
/**
 * @struct	Tag
 * @brief	名前
 */
struct Tag
{
	std::string name;

	Tag(const std::string& n = "Entity")
		: name(n) {}
};

/**
 * @struct	Transform
 * @brief	位置・回転・スケール
 */
struct Transform
{
	XMFLOAT3 position;	// x, y, z
	XMFLOAT3 rotation;	// pitch, yaw, roll (Euler angles in degrees or radians)
	XMFLOAT3 scale;		// x, y, z

	// ワールド行列
	DirectX::XMMATRIX worldMatrix;

	Transform(XMFLOAT3 p = { 0.0f, 0.0f, 0.0f }, XMFLOAT3 r = { 0.0f, 0.0f, 0.0f }, XMFLOAT3 s = { 1.0f, 1.0f, 1.0f })
		: position(p), rotation(r), scale(s)
	{
		worldMatrix = DirectX::XMMatrixIdentity();
	}
};

/**
 * @struct	Relationship
 * @brief	親子関係
 */
struct Relationship
{
	Entity parent = NullEntity;
	std::vector<Entity> children;

	Relationship() : parent(NullEntity) {}
	Relationship(Entity p) : parent(p) {}
};

/**
 * @struct	Lifetime
 * @brief	寿命（秒）
 */
struct Lifetime
{
	float time;	// 残り時間

	Lifetime(float t = 0.0f)
		: time(t) {}
};

// ============================================================
// 物理コンポーネント
// ============================================================
/**
 * @enum	BodyType
 * @brief	物理挙動の種類
 */
enum class BodyType
{
	Static,		// 動かない（質量無限大）。壁、地面など。
	Dynamic,	// 物理演算で動く。重力や衝突の影響を受ける。プレイヤー、敵、箱など。
	Kinematic,	// 物理演算を無視し、プログラムで動かす。移動床、エレベーターなど。
};

/**
 * @struct	Rigidbody
 * @brief	物理挙動
 */
struct Rigidbody
{
	BodyType type;
	XMFLOAT3 velocity;
	float mass;
	float drag;
	bool useGravity;
	bool freezeRotation;	// 回転を固定するか

	Rigidbody(BodyType t = BodyType::Dynamic, float m = 1.0f)
		: type(t), velocity({ 0,0,0 }), mass(m), drag(0.1f), useGravity(true), freezeRotation(true)
	{
		// StaticやKinematicなら重力OFFにするなどの初期化
		if (type != BodyType::Dynamic) useGravity = false;
	}
};

/**
 * @enum	ColliderType
 * @brief	当たり判定の形状
 */
enum class ColliderType
{
	Box,		// ボックス
	Sphere,		// 球体
	Capsule,	// カプセル
	Cylinder,	// 円柱
};

/**
 * @struct	Collider
 * @brief	当たり判定
 */
struct Collider
{
	ColliderType type;
	bool isTrigger;

	// 共用体でメモリ節約
	union
	{
		struct { float x, y, z; } boxSize;
		struct { float radius; } sphere;
		struct { float radius, height; } capsule;
		struct { float radius, height; } cylinder;
	};

	XMFLOAT3 offset;

	// コンストラクタ
	Collider() : type(ColliderType::Box), isTrigger(false), offset({ 0,0,0 }) { boxSize = { 1,1,1 }; }

	// ヘルパー関数
	static Collider CreateSphere(float r)
	{
		Collider c; c.type = ColliderType::Sphere; c.sphere.radius = r; return c;
	}
	struct Collider CreateBox(float x, float y, float z)
	{
		Collider c; c.type = ColliderType::Box; c.boxSize = { x, y, z }; return c;
	}
	static Collider CreateCapsule(float r, float h)
	{
		Collider c; c.type = ColliderType::Capsule; c.capsule.radius = r; c.capsule.height = h; return c;
	}
	static Collider CreateCylinder(float r, float h)
	{
		Collider c; c.type = ColliderType::Cylinder; c.cylinder.radius = r; c.cylinder.height = h; return c;
	}
};

// 物理エンジンが計算して使う「キャッシュデータ」
// これをObserverで更新し、マイフレームの計算をスキップする
struct AABB
{
	XMFLOAT3 min;
	XMFLOAT3 max;

	//AABB() : min({ 0,0,0 }), max({ 0,0,0 }) {}
};

/**
 * @struct	WorldCollider
 * @brief	ワールド空間での形状データキャッシュ
 */
struct WorldCollider
{
	// 形状データ
	union
	{
		struct { XMFLOAT3 center; XMFLOAT3 extents; XMFLOAT3 axes[3]; } obb;
		struct { XMFLOAT3 center; float radius; } sphere;
		struct { XMFLOAT3 start; XMFLOAT3 end; float radius; } capsule;
		struct { XMFLOAT3 center; XMFLOAT3 axis; float height; float radius; } cylinder;
	};

	// ブロードフェーズ用AABB
	AABB aabb;

	// 更新が必要かどうか（システム側で管理）
	bool isDirty = true;

	WorldCollider() {
		std::memset(this, 0, sizeof(WorldCollider));
		isDirty = true;
	}
};

// ============================================================
// ゲームロジック・入力
// ============================================================
/**
 * @struct	PlayerInput
 * @brief	操作可能フラグ
 */
struct PlayerInput
{
	float speed;
	float jumpPower;

	PlayerInput(float s = 3.0f, float j = 5.0f)
		: speed(s), jumpPower(j) {
	}
};

// ============================================================
// レンダリング関連
// ============================================================
/**
 * @struct	MeshComponent
 * @brief	モデル描画
 */
struct MeshComponent
{
	std::string modelKey;	// ResourceManagerのキー
	XMFLOAT3 scaleOffset;	// モデル固有のスケール補正（アセットが巨大/極小な場合用）
	XMFLOAT4 color;			// マテリアルカラー乗算用

	MeshComponent(const std::string& key = "", const XMFLOAT3& scale = {1,1,1}, const XMFLOAT4& c = {1, 1, 1, 1})
		: modelKey(key), scaleOffset(scale), color(c) {}
};

/**
 * @struct	SpriteComponent
 * @brief	2D描画
 */
struct SpriteComponent
{
	std::string textureKey;	// ResourceManagerで使うキー
	float width, height;	// 描画サイズ（ピクセル）
	XMFLOAT4 color;			// 色と透明度
	XMFLOAT2 pivot;			// 中心点（0.0 ~ 1.0）デフォルトは左上（0, 0）

	SpriteComponent(const std::string& key = "", float w = 0.0f, float h = 0.0f, const XMFLOAT4& c = { 1, 1, 1, 1 }, const XMFLOAT2& p = { 0, 0 })
		: textureKey(key), width(w), height(h), color(c), pivot(p) {}
};

/**
 * @struct	BillboardComponent
 * @brief	ビルボード
 */
struct BillboardComponent
{
	std::string textureKey;
	XMFLOAT2 size;	// 幅、高さ
	XMFLOAT4 color;

	BillboardComponent(const std::string& key = "", float w = 1.0f, float h = 1.0f, const XMFLOAT4& c = { 1,1,1,1 })
		: textureKey(key), size({ w, h }), color(c) {
	}
};

// ============================================================
// オーディオ関連
// ============================================================

/**
 * @struct	Audiosource
 * @brief	音源（鳴らす側）
 */
struct AudioSource
{
	std::string soundKey;	// ResourceManagerのキー
	float volume;			// 基本音量
	float range;			// 音が聞こえる最大距離（3Dサウンド用）
	bool isLoop;			// ループするか
	bool playOnAwake;		// 生成時に即再生するか

	// 内部状態管理用
	bool isPlaying = false;

	AudioSource(const std::string& key = "", float vol = 1.0f, float r = 20.0f, bool loop = false, bool awake = false)
		: soundKey(key), volume(vol), range(r), isLoop(loop), playOnAwake(awake) {}
};

/**
 * @struct	AudioListener
 * @brief	聞く側（通常はカメラかプレイヤーに1つだけつける）
 */
struct AudioListener
{
	// データは不要、タグとして機能する。
};

/**
 * @struct	Camera
 * @brief	カメラ
 */
struct Camera
{
	float fov;			// 視野角（Radian）
	float nearZ, farZ;	// 視錐台の範囲
	float aspect;		// アスペクト比

	// コンストラクタ
	Camera(float f = XM_PIDIV4, float n = 0.1f, float r = 1000.0f, float a = static_cast<float>(Config::SCREEN_WIDTH) / static_cast<float>(Config::SCREEN_HEIGHT))
		: fov(f), nearZ(n), farZ(r), aspect(a) {
	}
};

#endif // !___COMPONENTS_H___