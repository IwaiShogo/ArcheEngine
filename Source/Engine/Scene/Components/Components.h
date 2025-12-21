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
#include "Engine/pch.h"
#include "Engine/Core/StringId.h"
#include "Engine/Config.h"
#include "Engine/Scene/ECS/EntityDef.h"

// ============================================================
// 基本コンポーネント
// ============================================================
/**
 * @struct	Tag
 * @brief	名前
 */
struct Tag
{
	StringId name;

	// コンストラクタ
	Tag() = default;
	Tag(const char* str) : name(str) {}
	Tag(const std::string& str) : name(str) {}
	Tag(const StringId& id) : name(id) {}

	// 比較演算子（StringIdの比較に委譲）
	bool operator==(const Tag& other) const { return name == other.name; }
	bool operator==(const StringId& strId) const { return name == strId; }
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

	XMFLOAT4X4 worldMatrix;

	// 行列取得ヘルパー
	XMMATRIX GetWorldMatrix() const
	{
		return XMLoadFloat4x4(&worldMatrix);
	}

	Transform(XMFLOAT3 p = { 0.0f, 0.0f, 0.0f }, XMFLOAT3 r = { 0.0f, 0.0f, 0.0f }, XMFLOAT3 s = { 1.0f, 1.0f, 1.0f })
		: position(p), rotation(r), scale(s)
	{
		XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
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
	BodyType type;			// 物理挙動の種類
	XMFLOAT3 velocity;		// 速度
	float mass;				// 質量
	float drag;				// 空気抵抗
	bool useGravity;		// 重力を使用するか
	bool freezeRotation;	// 回転を固定するか
	float restitution;		// 反発係数 (0.0: 非反発 ～ 1.0: 完全反発)
	float friction;			// 摩擦係数（0.0: ツルツル ～ 1.0: ザラザラ）
	bool isGrounded;		// 地面に接地しているか（ジャンプ制御用など）

	Rigidbody(BodyType t = BodyType::Dynamic, float m = 1.0f)
		: type(t), velocity({ 0,0,0 }), mass(m), drag(0.1f), useGravity(true), freezeRotation(true), restitution(0.5f), friction(0.5f), isGrounded(false)
	{
		// StaticやKinematicなら重力OFFにするなどの初期化
		if (type != BodyType::Dynamic) useGravity = false;
	}
};

// ============================================================
// レイヤーシステム
// ============================================================
/**
 * @enum	Layer
 * @brief	レイヤー定義（ビットマスク）
 */
enum class Layer : uint32_t
{
	None = 0,
	Default = 1 << 0,
	Player = 1 << 1,
	Enemy = 1 << 2,
	Wall = 1 << 3,
	Item = 1 << 4,
	Projectile = 1 << 5,
	// 必要に応じて追加（最大32レイヤー）

	All = 0xFFFFFFFF
};

// 演算子オーバーロード（Layer同士のビット演算用）
constexpr Layer operator|(Layer a, Layer b) { return static_cast<Layer>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
constexpr Layer operator&(Layer a, Layer b) { return static_cast<Layer>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); }
constexpr Layer operator~(Layer a) { return static_cast<Layer>(~static_cast<uint32_t>(a)); }
constexpr Layer& operator|=(Layer& a, Layer b) { a = a | b; return a; }
constexpr Layer& operator&=(Layer& a, Layer b) { a = a & b; return a; }
// bool判定用
constexpr bool operator&&(Layer a, Layer b) { return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0; }
constexpr bool operator||(Layer a, Layer b) { return (static_cast<uint32_t>(a) | static_cast<uint32_t>(b)) != 0; }
constexpr bool operator!(Layer a) { return static_cast<uint32_t>(a) == 0; }

// ============================================================
// 衝突マトリックス（グローバル設定）
// ============================================================
/**
 * @class	PhysicsConfig
 * @brief	物理設定（レイヤーごとの衝突設定）
 */
class PhysicsConfig
{
public:
	// 設定用ヘルパー構造体（メソッドチェーン用）
	struct RuleBuilder
	{
		Layer target;

		RuleBuilder(Layer l) : target(l) {}

		// @brief	衝突設定を追加
		RuleBuilder& collidesWith(Layer other)
		{
			PhysicsConfig::matrix[target] |= other;
			PhysicsConfig::matrix[other] |= target;	// 相手側にも自分を追加
			return *this;
		}

		// @brief	衝突設定を除外
		RuleBuilder& ignore(Layer other)
		{
			PhysicsConfig::matrix[target] &= ~other;
			PhysicsConfig::matrix[other] &= ~target;	// 相手側からも自分を除外
			return *this;
		}
	};

	// @brief	設定開始（チェーンの起点）
	static RuleBuilder Configure(Layer layer)
	{
		return RuleBuilder(layer);
	}

	// @brief	全レイヤーの設定をクリア
	static void Reset()
	{
		matrix.clear();
		// DefaultはAllと当たるのが基本
		Configure(Layer::Default).collidesWith(Layer::All);
	}

	// @brief	指定レイヤーのマスクを取得（設定が無ければAllを返す）
	static Layer GetMask(Layer layer)
	{
		if (matrix.find(layer) != matrix.end())
		{
			// 設定されていないレイヤーはデフォルトで「全て」と当たるようにするか、
			// あるいは「Default」と同じにするか。ここでは安全のためDefault設定を返す。
			if (matrix.find(Layer::Default) != matrix.end()) return matrix[Layer::Default];
			else return Layer::All;
		}
		return matrix[layer];
	}

private:
	static inline std::map<Layer, Layer> matrix;	// Layer -> Mask
};

// ============================================================
// コライダー定義（メソッドチェーン対応）
// ============================================================
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

struct ColliderInfo
{
	ColliderType type = ColliderType::Box;
	Layer layer = Layer::Default;
	bool isTrigger = false;
	XMFLOAT3 size = { 1.0f, 1.0f, 1.0f }; // box: x,y,z | sphere: x=radius | capsule/cylinder: x=radius,y=height
	XMFLOAT3 offset = { 0.0f, 0.0f, 0.0f };
};

/**
 * @struct	Collider
 * @brief	当たり判定（メソッドチェーン対応）
 */
struct Collider
{
	ColliderType type;	// タイプの種類
	bool isTrigger;		// 物理衝突を無視してイベントのみ発生させるか	
	Layer layer;		// 所属
	Layer mask;			// 衝突判定を行うレイヤーのマスク

	XMFLOAT3 boxSize = { 1.0f, 1.0f, 1.0f };
	float radius = 0.5f;
	float height = 1.0f;

	XMFLOAT3 offset;	// オフセット

	// デフォルトコンストラクタ
	Collider() :type(ColliderType::Box), isTrigger(false), layer(Layer::Default), mask(Layer::All), boxSize{ 1.0f, 1.0f, 1.0f }, offset{ 0.0f, 0.0f, 0.0f } {}

	// 一括設定コンストラクタ
	Collider(const ColliderInfo& info)
		: type(info.type), isTrigger(info.isTrigger), layer(info.layer), offset(info.offset)
	{
		// 衝突マスクをグローバル設定から取得
		mask = PhysicsConfig::GetMask(layer);
		// サイズ設定
		if (type == ColliderType::Box) { boxSize = { info.size.x, info.size.y, info.size.z }; }
		else if (type == ColliderType::Sphere) { radius = info.size.x; }
		else if (type == ColliderType::Capsule) { radius = info.size.x; height = info.size.y; }
		else if (type == ColliderType::Cylinder) { radius = info.size.x; height = info.size.y; }
	}

	// ------------------------------------------------------------
	// Fluent Interface用セッター
	// ------------------------------------------------------------

	// @brief	自分の所属を設定
	Collider& setGroup(Layer l)
	{
		this->layer = l;
		this->mask = PhysicsConfig::GetMask(l); // マスクも更新
		return *this;
	}

	// @brief	衝突対象を上書き設定
	Collider& setMask(Layer l)
	{
		this->mask = l;
		return *this;
	}

	// @brief	衝突対象を追加（例：collidesWith(Layer::Enemy)）
	Collider& collidesWith(Layer l)
	{
		this->mask |= l;
		return *this;
	}

	// @brief	衝突対象から除外（例：ignore(Layer::Player)）
	Collider& ignore(Layer l)
	{
		this->mask = this->mask & (~l);
		return *this;
	}

	// @brief	トリガー設定
	Collider& setTrigger(bool trigger)
	{
		this->isTrigger = trigger;
		return *this;
	}

	// @brief	オフセット設定
	Collider& setOffset(const XMFLOAT3& off)
	{
		this->offset = off;
		return *this;
	}

	// ------------------------------------------------------------
	// Static Creator Methods
	// ------------------------------------------------------------
	static Collider CreateBox(float x, float y, float z, Layer l = Layer::Default)
	{
		return Collider(ColliderInfo{ ColliderType::Box, l, false, { x, y, z }, { 0,0,0 } });
	}
	static Collider CreateSphere(float radius, Layer l = Layer::Default)
	{
		return Collider(ColliderInfo{ ColliderType::Sphere, l, false, { radius, 0, 0 }, { 0,0,0 } });
	}
	static Collider CreateCapsule(float radius, float height, Layer l = Layer::Default)
	{
		return Collider(ColliderInfo{ ColliderType::Capsule, l, false, { radius, height, 0 }, { 0,0,0 } });
	}
	static Collider CreateCylinder(float radius, float height, Layer l = Layer::Default)
	{
		return Collider(ColliderInfo{ ColliderType::Cylinder, l, false, { radius, height, 0 }, { 0,0,0 } });
	}
	// トリガー作成用ショートカット
	static Collider CreateTriggerBox(float x, float y, float z, Layer l = Layer::Default)
	{
		return Collider(ColliderInfo{ ColliderType::Box, l, true, { x, y, z }, { 0,0,0 } });
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
	// --- 共通 / OBB用 ---
	XMFLOAT3 center = { 0,0,0 };
	XMFLOAT3 extents = { 0,0,0 };		// OBBのハーフサイズ
	XMFLOAT3 axes[3] = {				// OBBの回転軸
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
	};

	// --- Sphere / Capsule / Cylinder用 ---
	float radius = 0.0f;
	float height = 0.0f;

	// --- Capsule用 ---
	XMFLOAT3 start = { 0,0,0 };
	XMFLOAT3 end = { 0,0,0 };

	// --- Cylinder用 ---
	XMFLOAT3 axis = { 0,1,0 };			// 円柱の軸ベクトル

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
	StringId modelKey;	// ResourceManagerのキー
	XMFLOAT3 scaleOffset;	// モデル固有のスケール補正（アセットが巨大/極小な場合用）
	XMFLOAT4 color;			// マテリアルカラー乗算用

	MeshComponent(StringId key = "", const XMFLOAT3& scale = {1,1,1}, const XMFLOAT4& c = {1, 1, 1, 1})
		: modelKey(key), scaleOffset(scale), color(c) {}
};

/**
 * @struct	SpriteComponent
 * @brief	2D描画
 */
struct SpriteComponent
{
	StringId textureKey;	// ResourceManagerで使うキー
	XMFLOAT4 color;			// 色と透明度

	SpriteComponent(StringId key = "", const XMFLOAT4& c = { 1, 1, 1, 1 })
		: textureKey(key), color(c) {}
};

/**
 * @struct	BillboardComponent
 * @brief	ビルボード
 */
struct BillboardComponent
{
	StringId textureKey;
	XMFLOAT2 size;	// 幅、高さ
	XMFLOAT4 color;

	BillboardComponent(StringId key = "", float w = 1.0f, float h = 1.0f, const XMFLOAT4& c = { 1,1,1,1 })
		: textureKey(key), size({ w, h }), color(c) {
	}
};

/**
 * @struct	TextComponent
 * @brief	テキスト描画（DirectWrite）
 */
struct TextComponent
{
	std::string text;
	std::string fontKey;	// フォント名（例: "Meiryo", "CustomFont"）
	float fontSize;		// 基本サイズ（1080p基準など）
	XMFLOAT4 color;		// カラー
	XMFLOAT2 offset;	// 親Transformからのオフセット

	// レイアウト設定
	float maxWidth = 0.0f;		// 0なら制限なし
	bool centerAlign = false;

	// スタイルオプション
	bool isBold = false;
	bool isItalic = false;

	TextComponent(const std::string& t = "Text", std::string font = "Meiryo", float size = 24.0f, const XMFLOAT4& c = { 1,1,1,1 })
		: text(t), fontKey(font), fontSize(size), color(c), offset({ 0,0 }) {}
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
	StringId soundKey;	// ResourceManagerのキー
	float volume;			// 基本音量
	float range;			// 音が聞こえる最大距離（3Dサウンド用）
	bool isLoop;			// ループするか
	bool playOnAwake;		// 生成時に即再生するか

	// 内部状態管理用
	bool isPlaying = false;

	AudioSource(StringId key = "", float vol = 1.0f, float r = 20.0f, bool loop = false, bool awake = false)
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

// ============================================================
// 2D / UI関連
// ============================================================

/**
 * @struct	Transform2D
 * @brief	UIや2Dオブジェクト専用の座標・サイズ管理
 */
struct Transform2D
{
	// --- 基本パラメータ ---
	XMFLOAT2 position = { 0.0f, 0.0f };			// アンカー中心からのオフセット座標
	XMFLOAT2 size = { 100.0f, 100.0f };			// 幅と高さ
	XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };	// 回転（X, Y, Z）
	XMFLOAT2 scale = { 1.0f, 1.0f };			// スケール

	// --- UIレイアウト用 ---
	XMFLOAT2 pivot = { 0.5f, 0.5f };			// 回転・拡大縮小の中心（0.0 = 左上, 0.5 = 中央, 1.0 = 右下）

	// アンカー（親のどの位置を基準にするか 0.0 ~ 1.0）
	// 例: min{0.5, 0.5}, max{0.5, 0.5} = 親の中央に固定
	//     min{0, 0}, max{1, 1} = 親全体にストレッチ（全画面など）
	XMFLOAT2 anchorMin = { 0.5f, 0.5f };
	XMFLOAT2 anchorMax = { 0.5f, 0.5f };

	// 描画順（手前に表示したいものは大きくする）
	int zIndex = 0;

	// --- 内部計算用（Systemが書き込む）---
	// 描画用の変換行列（Direct2D用）
	D2D1_MATRIX_3X2_F worldMatrix = D2D1::IdentityMatrix();

	// 計算済みのスクリーン矩形 { left, up, right, bottom }
	// マウス判定（当たり判定）などで使用
	XMFLOAT4 calculatedRect = { 0, 0, 0, 0 };

	// コンストラクタ
	// ------------------------------------------------------------
	// 1. デフォルト
	Transform2D() = default;

	// 2. 位置のみ指定 (サイズは100x100)
	Transform2D(const XMFLOAT2& pos)
		: position(pos)
	{
		calculatedRect = { pos.x, pos.y, pos.x + size.x, pos.y + size.y };
	}

	// 3. 位置とサイズ指定
	Transform2D(const XMFLOAT2& pos, const XMFLOAT2& sz)
		: position(pos), size(sz)
	{
		calculatedRect = { pos.x, pos.y, pos.x + sz.x, pos.y + sz.y };
	}

	// 4. floatで直接指定 (x, y, w, h) - これが一番楽かもしれません
	Transform2D(float x, float y, float w, float h)
		: position({ x, y }), size({ w, h })
	{
		calculatedRect = { x, y, x + w, y + h };
	}

	// 5. 位置・サイズ・アンカー指定 (UI配置用)
	// anchorを1つ渡すと、min/max両方にセットします（固定配置）
	Transform2D(const XMFLOAT2& pos, const XMFLOAT2& sz, const XMFLOAT2& anchor)
		: position(pos), size(sz), anchorMin(anchor), anchorMax(anchor)
	{
		calculatedRect = { pos.x, pos.y, pos.x + sz.x, pos.y + sz.y };
	}
};

/**
 * @struct	Canvas
 * @brief	UIのルート要素。画面サイズ情報を提供するタグ的なコンポーネント
 */
struct Canvas
{
	bool isScreenSpace = true;	// trueなら画面解像度に合わせる
	XMFLOAT2 referenceSize = { 1920.0f, 1080.0f };	// 基準解像度

	// デフォルトコンストラクタ
	Canvas() = default;

	// 引数付きコンストラクタ
	Canvas(bool isScreen, const XMFLOAT2& size)
		: isScreenSpace(isScreen), referenceSize(size) {}

	// 引数付きコンストラクタ
	Canvas(bool isScreen, float w, float h)
		: isScreenSpace(isScreen), referenceSize({ w, h }) {}
};

// ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼
// 全コンポーネントリスト
// ここに新しいコンポーネントを追加するだけで、Inspectorに自動表示されるようになります。
// ------------------------------------------------------------
#include "Engine/Core/Reflection.h"

// ▼ ここに登録したいコンポーネントと、Inspectorに表示したい変数を列挙 ▼
// 書式: X (構造体名, REFLECT_VAR(変数名)... )

#define COMPONENT_LIST(X) \
	X(Tag, \
		REFLECT_VAR(name) \
	) \
	X(Transform, \
		REFLECT_VAR(position) \
		REFLECT_VAR(rotation) \
		REFLECT_VAR(scale) \
	) \
	X(Relationship, \
		REFLECT_VAR(parent) \
		REFLECT_VAR(children) \
	) \
	X(Transform2D, \
		REFLECT_VAR(position) \
		REFLECT_VAR(size) \
		REFLECT_VAR(rotation) \
		REFLECT_VAR(scale) \
		REFLECT_VAR(anchorMin) \
		REFLECT_VAR(anchorMax) \
		REFLECT_VAR(pivot) \
	) \
	X(Canvas, \
		REFLECT_VAR(isScreenSpace) \
		REFLECT_VAR(referenceSize) \
	) \
	X(TextComponent, \
		REFLECT_VAR(text) \
		REFLECT_VAR(fontKey) \
		REFLECT_VAR(fontSize) \
		REFLECT_VAR(color) \
		REFLECT_VAR(offset) \
		REFLECT_VAR(isBold) \
		REFLECT_VAR(isItalic) \
		REFLECT_VAR(centerAlign) \
	) \
	X(MeshComponent, \
		REFLECT_VAR(modelKey) \
		REFLECT_VAR(scaleOffset) \
		REFLECT_VAR(color) \
	) \
	X(SpriteComponent, \
		REFLECT_VAR(textureKey) \
		REFLECT_VAR(color) \
	) \
	X(BillboardComponent, \
		REFLECT_VAR(textureKey) \
		REFLECT_VAR(size) \
		REFLECT_VAR(color) \
	) \
	X(AudioSource, \
		REFLECT_VAR(soundKey) \
		REFLECT_VAR(volume) \
		REFLECT_VAR(range) \
		REFLECT_VAR(isLoop) \
		REFLECT_VAR(playOnAwake) \
	) \
	X(AudioListener, \
		/* データなしタグコンポーネント */ \
	) \
	X(Camera, \
		REFLECT_VAR(fov) \
		REFLECT_VAR(nearZ) \
		REFLECT_VAR(farZ) \
		REFLECT_VAR(aspect) \
	) \
	X(Collider, \
		REFLECT_VAR(type) \
		REFLECT_VAR(isTrigger) \
		REFLECT_VAR(offset) \
		REFLECT_VAR(boxSize) \
		REFLECT_VAR(radius) \
		REFLECT_VAR(height) \
	) \
	X(Rigidbody, \
		REFLECT_VAR(type) \
		REFLECT_VAR(mass) \
		REFLECT_VAR(drag) \
		REFLECT_VAR(useGravity) \
	) \
	X(PlayerInput, \
		REFLECT_VAR(speed) \
		REFLECT_VAR(jumpPower) \
	) \
	X(Lifetime, \
		REFLECT_VAR(time) \
	) \
/* 新しいコンポーネントを作ったらここに行を足すだけ！ */

// ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲

// ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼
// マクロ展開による自動コード生成（ここはいじらなくてOK）
// ------------------------------------------------------------

// 1. 各コンポーネントの REFLECT_STRUCT_BEGIN ... END を自動生成
#define GENERATE_REFLECTION(Type, ...) \
	REFLECT_STRUCT_BEGIN(Type) \
		__VA_ARGS__ \
	REFLECT_STRUCT_END()

// マクロを実行して展開
COMPONENT_LIST(GENERATE_REFLECTION)

// 2. ComponentList タプルを自動生成
#define GENERATE_TUPLE_ENTRY(Type, ...) Type,

struct DummyComponent {};

using ComponentList = std::tuple<
	COMPONENT_LIST(GENERATE_TUPLE_ENTRY)
	DummyComponent // ダミーの末尾 (コンマ対策)
>;

// マクロ掃除
#undef COMPONENT_LIST
#undef GENERATE_REFLECTION
#undef GENERATE_TUPLE_ENTRY

// ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲
#endif // !___COMPONENTS_H___