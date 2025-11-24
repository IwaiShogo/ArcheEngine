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
#include <DirectXMath.h>
#include "main.h"

using namespace DirectX;

/**
 * @struct	Transform
 * @brief	位置・回転・スケール
 */
struct Transform
{
	XMFLOAT3 position;	// x, y, z
	XMFLOAT3 rotation;	// pitch, yaw, roll (Euler angles in degrees or radians)
	XMFLOAT3 scale;		// x, y, z

	Transform(XMFLOAT3 p = { 0.0f, 0.0f, 0.0f }, XMFLOAT3 r = { 0.0f, 0.0f, 0.0f }, XMFLOAT3 s = { 1.0f, 1.0f, 1.0f })
		: position(p), rotation(r), scale(s) {}
};

/**
 * @struct	Velocity
 * @brief	速度（物理挙動用）
 */
struct Velocity
{
	XMFLOAT3 velocity;

	Velocity(XMFLOAT3 v = { 0.0f, 0.0f, 0.0f })
		: velocity(v) {}
};

/**
 * @struct	Tag
 * @brief	名前
 */
struct Tag
{
	const char* name;

	Tag(const char* n = "Entity")
		: name(n) {}
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
		: fov(f), nearZ(n), farZ(r), aspect(a) {}
};

/**
 * @struct	BoxCollider
 * @brief	ボックスの当たり判定
 */
struct BoxCollider
{
	XMFLOAT3 size;		// 幅・高さ・奥行
	XMFLOAT3 offset;	// 中心からのオフセット

	BoxCollider(XMFLOAT3 s = { 1.0f, 1.0f,1.0f }, XMFLOAT3 o = { 0.0f, 0.0f, 0.0f })
		: size(s), offset(o) {}
};

/**
 * @struct	PlayerInput
 * @brief	操作可能フラグ
 */
struct PlayerInput
{
	float speed;
	float jumpPower;

	PlayerInput(float s = 3.0f, float j = 5.0f)
		: speed(s), jumpPower(j) {}
};

/**
 * @struct	GridPosition
 * @brief	論理的なグリッド描画（整数）
 */
struct GridPosition
{
	int x, z;

	GridPosition(int _x = 0, int _z = 0)
		: x(_x), z(_z) {}
};

struct GridMoveState
{
	bool isMoving = false;	// 移動中か？
	XMFLOAT3 startPos;		// 移動開始時のワールド座標
	XMFLOAT3 targetPos;		// 目的地のワールド座標
	float progress = 0.0f;	// 進捗（0.0 ~ 1.0）
	float moveSpeed = 4.0f;	// 移動アニメーション速度

	GridMoveState(float speed = 4.0f)
		: isMoving(false), startPos(0, 0, 0), targetPos(0, 0, 0), progress(0.0f), moveSpeed(speed) {}
};

#endif // !___COMPONENTS_H___