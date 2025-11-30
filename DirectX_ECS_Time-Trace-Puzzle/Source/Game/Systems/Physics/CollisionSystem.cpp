/*****************************************************************//**
 * @file	CollisionSystem.cpp
 * @brief	衝突検出
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/24	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#define NOMINMAX
#include "Game/Systems/Physics/CollisionSystem.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <vector>

using namespace Physics;

// =================================================================
// 数学ヘルパー関数
// =================================================================

// ベクトルの長さの二乗
float LengthSq(FXMVECTOR v) {
	return XMVectorGetX(XMVector3LengthSq(v));
}

// 点Pの、線分AB上での最近接点を求める
XMVECTOR ClosestPointOnSegment(XMVECTOR P, XMVECTOR A, XMVECTOR B)
{
	XMVECTOR AP = P - A;
	XMVECTOR AB = B - A;
	float ab2 = LengthSq(AB);

	if (ab2 < 1e-6f) return A;

	float t = XMVectorGetX(XMVector3Dot(AP, AB)) / ab2;
	t = std::max(0.0f, std::min(1.0f, t));
	return A + AB * t;
}

// 2つの線分間の最短距離の二乗と、それぞれの線分上の最近接点(c1, c2)を求める
float SegmentSegmentDistanceSq(XMVECTOR p1, XMVECTOR q1, XMVECTOR p2, XMVECTOR q2,
	XMVECTOR& outC1, XMVECTOR& outC2)
{
	XMVECTOR d1 = q1 - p1;
	XMVECTOR d2 = q2 - p2;
	XMVECTOR r = p1 - p2;
	float a = LengthSq(d1);
	float e = LengthSq(d2);
	float f = XMVectorGetX(XMVector3Dot(d2, r));

	if (a <= 1e-6f && e <= 1e-6f) {
		outC1 = p1; outC2 = p2;
		return LengthSq(r);
	}

	float s, t;
	if (a <= 1e-6f) {
		s = 0.0f;
		t = std::max(0.0f, std::min(1.0f, f / e));
	}
	else {
		float c = XMVectorGetX(XMVector3Dot(d1, r));
		if (e <= 1e-6f) {
			t = 0.0f;
			s = std::max(0.0f, std::min(1.0f, -c / a));
		}
		else {
			float b = XMVectorGetX(XMVector3Dot(d1, d2));
			float denom = a * e - b * b;
			if (denom != 0.0f) {
				s = std::max(0.0f, std::min(1.0f, (b * f - c * e) / denom));
			}
			else {
				s = 0.0f;
			}
			t = (b * s + f) / e;
			if (t < 0.0f) {
				t = 0.0f;
				s = std::max(0.0f, std::min(1.0f, -c / a));
			}
			else if (t > 1.0f) {
				t = 1.0f;
				s = std::max(0.0f, std::min(1.0f, (b - c) / a));
			}
		}
	}

	outC1 = p1 + d1 * s;
	outC2 = p2 + d2 * t;
	XMVECTOR diff = outC1 - outC2;
	return LengthSq(diff);
}

// 点PからOBBへの最近接点を求める
XMVECTOR ClosestPointOnOBB(XMVECTOR P, const OBB& box) {
	XMVECTOR d = P - XMLoadFloat3(&box.center);
	XMVECTOR q = XMLoadFloat3(&box.center);

	for (int i = 0; i < 3; ++i) {
		XMVECTOR axis = XMLoadFloat3(&box.axes[i]);
		float dist = XMVectorGetX(XMVector3Dot(d, axis));
		float extent = (&box.extents.x)[i];

		if (dist > extent) dist = extent;
		if (dist < -extent) dist = -extent;

		q += axis * dist;
	}
	return q;
}

// レイ vs 球
bool IntersectRaySphere(XMVECTOR origin, XMVECTOR dir, XMVECTOR center, float radius, float& t)
{
	XMVECTOR m = origin - center;
	float b = XMVectorGetX(XMVector3Dot(m, dir));
	float c = XMVectorGetX(XMVector3Dot(m, m)) - radius * radius;

	// レイの始点が球の外にあり、かつ球から遠ざかっている場合
	if (c > 0.0f && b > 0.0f) return false;

	float discr = b * b - c;
	if (discr < 0.0f) return false;

	t = -b - sqrt(discr);
	if (t < 0.0f) t = 0.0f; // 始点が中にある場合は0
	return true;
}

// レイ vs OBB (回転した箱)
bool IntersectRayOBB(XMVECTOR origin, XMVECTOR dir, const OBB& obb, float& t)
{
	XMVECTOR boxCenter = XMLoadFloat3(&obb.center);
	XMVECTOR delta = boxCenter - origin; // レイの始点から箱の中心へのベクトル

	// OBBの各軸
	XMVECTOR axes[3] = {
		XMLoadFloat3(&obb.axes[0]),
		XMLoadFloat3(&obb.axes[1]),
		XMLoadFloat3(&obb.axes[2])
	};
	float extents[3] = { obb.extents.x, obb.extents.y, obb.extents.z };

	float tMin = 0.0f;
	float tMax = 100000.0f;

	for (int i = 0; i < 3; ++i)
	{
		// レイの方向と、箱の中心への方向を、OBBの各軸に投影
		// e = (BoxCenter - RayOrigin) . Axis
		// f = RayDir . Axis
		float e = XMVectorGetX(XMVector3Dot(axes[i], delta));
		float f = XMVectorGetX(XMVector3Dot(axes[i], dir));

		// レイが軸と平行でない場合
		if (std::abs(f) > 1e-6f)
		{
			float t1 = (e + extents[i]) / f;
			float t2 = (e - extents[i]) / f;

			if (t1 > t2) std::swap(t1, t2);

			tMin = std::max(tMin, t1);
			tMax = std::min(tMax, t2);

			if (tMin > tMax) return false;
			if (tMax < 0.0f) return false;
		}
		else
		{
			// 平行な場合、レイがスラブの外にあれば交差しない
			if (-e - extents[i] > 0.0f || -e + extents[i] < 0.0f) return false;
		}
	}

	t = tMin;
	return true;
}

// レイ vs カプセル
bool IntersectRayCapsule(XMVECTOR origin, XMVECTOR dir, const Physics::Capsule& cap, float& t)
{
	XMVECTOR pa = XMLoadFloat3(&cap.start);
	XMVECTOR pb = XMLoadFloat3(&cap.end);
	float r = cap.radius;

	XMVECTOR ba = pb - pa;
	XMVECTOR oa = origin - pa;

	float baba = XMVectorGetX(XMVector3Dot(ba, ba));
	float bard = XMVectorGetX(XMVector3Dot(ba, dir));
	float baoa = XMVectorGetX(XMVector3Dot(ba, oa));
	float rdoa = XMVectorGetX(XMVector3Dot(dir, oa));
	float oaoa = XMVectorGetX(XMVector3Dot(oa, oa));

	float a = baba - bard * bard;
	float b = baba * rdoa - baoa * bard;
	float c = baba * oaoa - baoa * baoa - r * r * baba;
	float h = b * b - a * c;

	if (h >= 0.0f)
	{
		float t_hit = (-b - sqrt(h)) / a;
		float y = baoa + t_hit * bard;

		// 胴体部分 (円柱) との交差
		if (y > 0.0f && y < baba)
		{
			t = t_hit;
			return true;
		}

		// キャップ (半球) との交差
		XMVECTOR oc = (y <= 0.0f) ? oa : origin - pb;
		b = XMVectorGetX(XMVector3Dot(dir, oc));
		c = XMVectorGetX(XMVector3Dot(oc, oc)) - r * r;
		h = b * b - c;
		if (h > 0.0f)
		{
			t = -b - sqrt(h);
			return true;
		}
	}
	return false;
}


// =================================================================
// Raycast 関数の修正版
// =================================================================

Entity CollisionSystem::Raycast(Registry& registry, const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir, float& outDist)
{
	Entity closestEntity = NullEntity;
	float closestDist = FLT_MAX;

	XMVECTOR originV = XMLoadFloat3(&rayOrigin);
	XMVECTOR dirV = XMLoadFloat3(&rayDir);

	registry.view<Transform, Collider>([&](Entity e, Transform& t, Collider& c)
		{
			// ワールド行列の分解
			XMVECTOR scale, rotQuat, pos;
			XMMatrixDecompose(&scale, &rotQuat, &pos, t.worldMatrix);
			XMFLOAT3 gScale; XMStoreFloat3(&gScale, scale);
			XMMATRIX rotMat = XMMatrixRotationQuaternion(rotQuat);

			// 中心座標の計算
			XMVECTOR offsetVec = XMLoadFloat3(&c.offset);
			XMVECTOR centerVec = XMVector3Transform(offsetVec, t.worldMatrix);
			XMFLOAT3 center; XMStoreFloat3(&center, centerVec);

			bool hit = false;
			float dist = 0.0f;

			// タイプごとの分岐
			if (c.type == ColliderType::Box)
			{
				OBB obb;
				obb.center = center;
				obb.extents = {
					c.boxSize.x * gScale.x * 0.5f,
					c.boxSize.y * gScale.y * 0.5f,
					c.boxSize.z * gScale.z * 0.5f
				};
				XMFLOAT4X4 rotM; XMStoreFloat4x4(&rotM, rotMat);
				obb.axes[0] = { rotM._11, rotM._12, rotM._13 };
				obb.axes[1] = { rotM._21, rotM._22, rotM._23 };
				obb.axes[2] = { rotM._31, rotM._32, rotM._33 };

				hit = IntersectRayOBB(originV, dirV, obb, dist);
			}
			else if (c.type == ColliderType::Sphere)
			{
				float maxS = std::max({ gScale.x, gScale.y, gScale.z });
				float r = c.sphere.radius * maxS;
				hit = IntersectRaySphere(originV, dirV, centerVec, r, dist);
			}
			else if (c.type == ColliderType::Capsule)
			{
				XMVECTOR axisY = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);
				float h = c.capsule.height * gScale.y;
				float r = c.capsule.radius * std::max(gScale.x, gScale.z);
				float halfLen = std::max(0.0f, h * 0.5f - r);

				Physics::Capsule cap;
				XMStoreFloat3(&cap.start, centerVec - axisY * halfLen);
				XMStoreFloat3(&cap.end, centerVec + axisY * halfLen);
				cap.radius = r;

				hit = IntersectRayCapsule(originV, dirV, cap, dist);
			}
			else if (c.type == ColliderType::Cylinder)
			{
				// 円柱はカプセル近似で判定
				XMVECTOR axisY = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);
				float h = c.cylinder.height * gScale.y;
				float r = c.cylinder.radius * std::max(gScale.x, gScale.z);
				float halfLen = h * 0.5f;

				Physics::Capsule cap;
				XMStoreFloat3(&cap.start, centerVec - axisY * halfLen);
				XMStoreFloat3(&cap.end, centerVec + axisY * halfLen);
				cap.radius = r;

				hit = IntersectRayCapsule(originV, dirV, cap, dist);
			}

			// 最近接の更新
			if (hit)
			{
				if (dist < closestDist && dist >= 0.0f)
				{
					closestDist = dist;
					closestEntity = e;
				}
			}
		});

	outDist = closestDist;
	return closestEntity;
}

// =================================================================
// 判定関数群
// =================================================================

// Sphere vs Sphere [cite: 2]
bool CollisionSystem::CheckSphereSphere(const Physics::Sphere& a, const Physics::Sphere& b, Physics::Contact& outContact) {
	XMVECTOR vA = XMLoadFloat3(&a.center);
	XMVECTOR vB = XMLoadFloat3(&b.center);
	XMVECTOR distVec = vB - vA;
	float distSq = LengthSq(distVec);
	float rSum = a.radius + b.radius;

	if (distSq >= rSum * rSum) return false;

	float dist = std::sqrt(distSq);
	if (dist < 1e-4f) { distVec = XMVectorSet(0, 1, 0, 0); dist = 0.0f; }

	XMVECTOR normal = distVec / dist; // A -> B
	XMStoreFloat3(&outContact.normal, normal);
	outContact.depth = rSum - dist;
	return true;
}

// Sphere vs OBB [cite: 2]
bool CollisionSystem::CheckSphereOBB(const Physics::Sphere& s, const Physics::OBB& b, Physics::Contact& outContact) {
	XMVECTOR sphereCenter = XMLoadFloat3(&s.center);
	XMVECTOR closest = ClosestPointOnOBB(sphereCenter, b);
	XMVECTOR distVec = closest - sphereCenter;
	float distSq = LengthSq(distVec);

	if (distSq > s.radius * s.radius) return false;

	float dist = std::sqrt(distSq);
	XMVECTOR normal;

	if (dist < 1e-4f) {
		// 中心がOBB内部にある場合：最も浅い面へ押し出す
		// (簡易的に球中心からOBB中心へのベクトル等を使うか、各面との距離を測る)
		// ここでは簡易的に、OBBの中心から球の中心へのベクトルを採用
		XMVECTOR obbCenter = XMLoadFloat3(&b.center);
		normal = sphereCenter - obbCenter;
		if (LengthSq(normal) < 1e-4f) normal = XMVectorSet(0, 1, 0, 0);
		normal = XMVector3Normalize(normal);
		// 深度は正確には「浸透距離 + 半径」だが、簡易計算
		outContact.depth = s.radius;
	}
	else {
		normal = -distVec / dist; // A(Sphere) -> B(OBB)
		outContact.depth = s.radius - dist;
	}

	XMStoreFloat3(&outContact.normal, normal);
	return true;
}

// Sphere vs Capsule [cite: 2]
bool CollisionSystem::CheckSphereCapsule(const Physics::Sphere& s, const Physics::Capsule& c, Physics::Contact& outContact) {
	XMVECTOR sphP = XMLoadFloat3(&s.center);
	XMVECTOR capA = XMLoadFloat3(&c.start);
	XMVECTOR capB = XMLoadFloat3(&c.end);

	XMVECTOR closest = ClosestPointOnSegment(sphP, capA, capB);
	XMVECTOR distVec = closest - sphP; // 球 -> カプセル
	float distSq = LengthSq(distVec);
	float rSum = s.radius + c.radius;

	if (distSq >= rSum * rSum) return false;

	float dist = std::sqrt(distSq);
	XMVECTOR normal;

	if (dist < 1e-4f) {
		normal = XMVectorSet(0, 1, 0, 0);
		dist = 0.0f;
	}
	else {
		normal = distVec / dist;
	}

	XMStoreFloat3(&outContact.normal, normal);
	outContact.depth = rSum - dist;
	return true;
}

// OBB vs OBB (SAT: 分離軸定理) [cite: 2]
bool CollisionSystem::CheckOBBOBB(const Physics::OBB& a, const Physics::OBB& b, Physics::Contact& outContact) {
	XMVECTOR centerA = XMLoadFloat3(&a.center);
	XMVECTOR centerB = XMLoadFloat3(&b.center);
	XMVECTOR translation = centerB - centerA;

	XMVECTOR axesA[3] = { XMLoadFloat3(&a.axes[0]), XMLoadFloat3(&a.axes[1]), XMLoadFloat3(&a.axes[2]) };
	XMVECTOR axesB[3] = { XMLoadFloat3(&b.axes[0]), XMLoadFloat3(&b.axes[1]), XMLoadFloat3(&b.axes[2]) };
	float extA[3] = { a.extents.x, a.extents.y, a.extents.z };
	float extB[3] = { b.extents.x, b.extents.y, b.extents.z };

	float minOverlap = FLT_MAX;
	XMVECTOR mtvAxis = XMVectorSet(0, 1, 0, 0); // 最小移動ベクトル

	// 15軸すべてで分離判定
	auto TestAxis = [&](XMVECTOR axis) -> bool {
		if (LengthSq(axis) < 1e-6f) return true; // 平行などで軸が潰れた場合
		axis = XMVector3Normalize(axis);

		float rA = extA[0] * std::abs(XMVectorGetX(XMVector3Dot(axesA[0], axis))) +
			extA[1] * std::abs(XMVectorGetX(XMVector3Dot(axesA[1], axis))) +
			extA[2] * std::abs(XMVectorGetX(XMVector3Dot(axesA[2], axis)));

		float rB = extB[0] * std::abs(XMVectorGetX(XMVector3Dot(axesB[0], axis))) +
			extB[1] * std::abs(XMVectorGetX(XMVector3Dot(axesB[1], axis))) +
			extB[2] * std::abs(XMVectorGetX(XMVector3Dot(axesB[2], axis)));

		float dist = std::abs(XMVectorGetX(XMVector3Dot(translation, axis)));
		float overlap = (rA + rB) - dist;

		if (overlap < 0) return false; // 分離している

		// 最小の押し出し量を記録
		if (overlap < minOverlap) {
			minOverlap = overlap;
			mtvAxis = axis;
			// 軸の向きを A -> B に揃える
			if (XMVectorGetX(XMVector3Dot(translation, axis)) < 0) {
				mtvAxis = -axis;
			}
		}
		return true;
		};

	// 1. Aの面法線 (3)
	for (int i = 0; i < 3; ++i) if (!TestAxis(axesA[i])) return false;
	// 2. Bの面法線 (3)
	for (int i = 0; i < 3; ++i) if (!TestAxis(axesB[i])) return false;
	// 3. エッジの外積 (9)
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			if (!TestAxis(XMVector3Cross(axesA[i], axesB[j]))) return false;
		}
	}

	// 衝突確定
	XMStoreFloat3(&outContact.normal, mtvAxis);
	outContact.depth = minOverlap;
	return true;
}

// OBB vs Capsule (SATベース)
bool CollisionSystem::CheckOBBCapsule(const Physics::OBB& box, const Physics::Capsule& cap, Physics::Contact& outContact) {
	// カプセルを「半径付き線分」としてSATで判定
	XMVECTOR centerA = XMLoadFloat3(&box.center);
	// カプセル中心
	XMVECTOR capStart = XMLoadFloat3(&cap.start);
	XMVECTOR capEnd = XMLoadFloat3(&cap.end);
	XMVECTOR capAxis = capEnd - capStart;
	XMVECTOR centerB = (capStart + capEnd) * 0.5f;
	XMVECTOR translation = centerB - centerA;

	XMVECTOR axesA[3] = { XMLoadFloat3(&box.axes[0]), XMLoadFloat3(&box.axes[1]), XMLoadFloat3(&box.axes[2]) };
	float extA[3] = { box.extents.x, box.extents.y, box.extents.z };

	float minOverlap = FLT_MAX;
	XMVECTOR mtvAxis = XMVectorSet(0, 1, 0, 0);

	auto TestAxis = [&](XMVECTOR axis) -> bool {
		if (LengthSq(axis) < 1e-6f) return true;
		axis = XMVector3Normalize(axis);

		// Boxの投影
		float rA = extA[0] * std::abs(XMVectorGetX(XMVector3Dot(axesA[0], axis))) +
			extA[1] * std::abs(XMVectorGetX(XMVector3Dot(axesA[1], axis))) +
			extA[2] * std::abs(XMVectorGetX(XMVector3Dot(axesA[2], axis)));

		// Capsuleの投影: (線分長/2 * cosθ) + 半径
		float rB = (XMVectorGetX(XMVector3Length(capAxis)) * 0.5f) * std::abs(XMVectorGetX(XMVector3Dot(XMVector3Normalize(capAxis), axis))) +
			cap.radius;

		float dist = std::abs(XMVectorGetX(XMVector3Dot(translation, axis)));
		float overlap = (rA + rB) - dist;

		if (overlap < 0) return false;
		if (overlap < minOverlap) {
			minOverlap = overlap;
			mtvAxis = axis;
			if (XMVectorGetX(XMVector3Dot(translation, axis)) < 0) mtvAxis = -axis;
		}
		return true;
		};

	// Boxの面法線 (3)
	for (int i = 0; i < 3; ++i) if (!TestAxis(axesA[i])) return false;

	// Capsuleの軸との外積 (3)
	for (int i = 0; i < 3; ++i) if (!TestAxis(XMVector3Cross(axesA[i], capAxis))) return false;

	// 線分端点とBoxの最近接点方向 (Cross積だけではエッジvsエッジしか見れないため)
	// 簡易的に「球 vs OBB」のロジックを混ぜて補完する
	// （今回はSAT軸のみで判定し、最近接点ベースの軸は省略）

	// Box(A) -> Capsule(B)
	XMStoreFloat3(&outContact.normal, mtvAxis);
	outContact.depth = minOverlap;
	return true;
}

bool CollisionSystem::CheckOBBCylinder(const Physics::OBB& box, const Physics::Cylinder& cyl, Physics::Contact& outContact)
{
	XMVECTOR boxCenter = XMLoadFloat3(&box.center);
	XMVECTOR boxAxes[3] = { XMLoadFloat3(&box.axes[0]), XMLoadFloat3(&box.axes[1]), XMLoadFloat3(&box.axes[2]) };
	float boxExtents[3] = { box.extents.x, box.extents.y, box.extents.z };

	XMVECTOR cylCenter = XMLoadFloat3(&cyl.center);
	XMVECTOR cylAxis = XMLoadFloat3(&cyl.axis);
	XMVECTOR translation = cylCenter - boxCenter;

	float minOverlap = FLT_MAX;
	XMVECTOR mtvAxis = XMVectorSet(0, 1, 0, 0);

	auto TestAxis = [&](XMVECTOR axis) -> bool {
		if (LengthSq(axis) < 1e-6f) return true;
		axis = XMVector3Normalize(axis);

		// Box Projection
		float rBox = boxExtents[0] * std::abs(XMVectorGetX(XMVector3Dot(boxAxes[0], axis))) +
			boxExtents[1] * std::abs(XMVectorGetX(XMVector3Dot(boxAxes[1], axis))) +
			boxExtents[2] * std::abs(XMVectorGetX(XMVector3Dot(boxAxes[2], axis)));

		// Cylinder Projection
		// 円柱の射影半径 = (高さ/2 * |cosθ|) + (半径 * |sinθ|)
		// |cosθ| = |dot(axis, cylAxis)|
		// |sinθ| = |cross(axis, cylAxis)| の長さ
		float rCylH = (cyl.height * 0.5f) * std::abs(XMVectorGetX(XMVector3Dot(cylAxis, axis)));
		float rCylR = cyl.radius * std::sqrt(XMVectorGetX(XMVector3LengthSq(XMVector3Cross(axis, cylAxis))));
		float rCyl = rCylH + rCylR;

		float dist = std::abs(XMVectorGetX(XMVector3Dot(translation, axis)));
		float overlap = (rBox + rCyl) - dist;

		if (overlap < 0) return false;
		if (overlap < minOverlap) {
			minOverlap = overlap;
			mtvAxis = axis;
			if (XMVectorGetX(XMVector3Dot(translation, axis)) < 0) mtvAxis = -axis;
		}
		return true;
		};

	// Box Axes
	for (int i = 0; i < 3; ++i) if (!TestAxis(boxAxes[i])) return false;

	// Cylinder Axis
	if (!TestAxis(cylAxis)) return false;

	// Cross Products (Box axes x Cylinder axis)
	for (int i = 0; i < 3; ++i) if (!TestAxis(XMVector3Cross(boxAxes[i], cylAxis))) return false;

	XMStoreFloat3(&outContact.normal, mtvAxis);
	outContact.depth = minOverlap;
	return true;
}

// Capsule vs Capsule
bool CollisionSystem::CheckCapsuleCapsule(const Physics::Capsule& a, const Physics::Capsule& b, Physics::Contact& outContact) {
	XMVECTOR a1 = XMLoadFloat3(&a.start);
	XMVECTOR a2 = XMLoadFloat3(&a.end);
	XMVECTOR b1 = XMLoadFloat3(&b.start);
	XMVECTOR b2 = XMLoadFloat3(&b.end);

	XMVECTOR c1, c2; // 線分上の最近接点
	float distSq = SegmentSegmentDistanceSq(a1, a2, b1, b2, c1, c2);
	float rSum = a.radius + b.radius;

	if (distSq >= rSum * rSum) return false;

	float dist = std::sqrt(distSq);
	XMVECTOR normal;

	if (dist < 1e-4f) {
		normal = XMVectorSet(0, 1, 0, 0);
		dist = 0.0f;
	}
	else {
		normal = (c2 - c1) / dist; // A -> B
	}

	XMStoreFloat3(&outContact.normal, normal);
	outContact.depth = rSum - dist;
	return true;
}

// Cylinder vs Cylinder (カプセル近似)
bool CollisionSystem::CheckCylinderCylinder(const Physics::Cylinder& a, const Physics::Cylinder& b, Physics::Contact& outContact) {
	Physics::Capsule capA, capB;
	XMVECTOR axisA = XMLoadFloat3(&a.axis);
	XMVECTOR centerA = XMLoadFloat3(&a.center);
	float halfHA = a.height * 0.5f;
	XMStoreFloat3(&capA.start, centerA - axisA * halfHA);
	XMStoreFloat3(&capA.end, centerA + axisA * halfHA);
	capA.radius = a.radius;

	XMVECTOR axisB = XMLoadFloat3(&b.axis);
	XMVECTOR centerB = XMLoadFloat3(&b.center);
	float halfHB = b.height * 0.5f;
	XMStoreFloat3(&capB.start, centerB - axisB * halfHB);
	XMStoreFloat3(&capB.end, centerB + axisB * halfHB);
	capB.radius = b.radius;

	return CheckCapsuleCapsule(capA, capB, outContact);
}

// Sphere vs Cylinder (カプセル近似)
bool CollisionSystem::CheckSphereCylinder(const Physics::Sphere& s, const Physics::Cylinder& c, Physics::Contact& outContact) {
	Physics::Capsule cap;
	XMVECTOR axis = XMLoadFloat3(&c.axis);
	XMVECTOR center = XMLoadFloat3(&c.center);
	float halfH = c.height * 0.5f;
	XMStoreFloat3(&cap.start, center - axis * halfH);
	XMStoreFloat3(&cap.end, center + axis * halfH);
	cap.radius = c.radius;

	return CheckSphereCapsule(s, cap, outContact);
}

// =================================================================
// メイン更新ループ
// =================================================================
void CollisionSystem::Update(Registry& registry)
{
	// データ収集
	struct CollisionProxy {
		Entity entity;
		ColliderType type;
		bool isTrigger;
		BodyType bodyType;
		Physics::Sphere sphere;
		Physics::OBB obb;
		Physics::Capsule capsule;
		Physics::Cylinder cylinder;
	};
	std::vector<CollisionProxy> proxies;

	registry.view<Transform, Collider>([&](Entity e, Transform& t, Collider& c) {
		CollisionProxy proxy;
		proxy.entity = e;
		proxy.type = c.type;
		proxy.isTrigger = c.isTrigger;
		if (registry.has<Rigidbody>(e)) proxy.bodyType = registry.get<Rigidbody>(e).type;
		else proxy.bodyType = BodyType::Static;

		XMVECTOR scale, rotQuat, pos;
		XMMatrixDecompose(&scale, &rotQuat, &pos, t.worldMatrix);
		XMFLOAT3 gScale; XMStoreFloat3(&gScale, scale);
		XMMATRIX rotMat = XMMatrixRotationQuaternion(rotQuat);

		XMVECTOR offsetVec = XMLoadFloat3(&c.offset);
		XMVECTOR centerVec = XMVector3Transform(offsetVec, t.worldMatrix);
		XMFLOAT3 center; XMStoreFloat3(&center, centerVec);

		if (c.type == ColliderType::Box) {
			proxy.obb.center = center;
			proxy.obb.extents = { c.boxSize.x * gScale.x * 0.5f, c.boxSize.y * gScale.y * 0.5f, c.boxSize.z * gScale.z * 0.5f };
			XMFLOAT4X4 rotM; XMStoreFloat4x4(&rotM, rotMat);
			proxy.obb.axes[0] = { rotM._11, rotM._12, rotM._13 };
			proxy.obb.axes[1] = { rotM._21, rotM._22, rotM._23 };
			proxy.obb.axes[2] = { rotM._31, rotM._32, rotM._33 };
		}
		else if (c.type == ColliderType::Sphere) {
			proxy.sphere.center = center;
			proxy.sphere.radius = c.sphere.radius * std::max({ gScale.x, gScale.y, gScale.z });
		}
		else if (c.type == ColliderType::Capsule) {
			XMVECTOR axisY = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);
			float h = c.capsule.height * gScale.y;
			float r = c.capsule.radius * std::max(gScale.x, gScale.z);
			float segLen = std::max(0.0f, h * 0.5f - r);
			XMStoreFloat3(&proxy.capsule.start, centerVec - axisY * segLen);
			XMStoreFloat3(&proxy.capsule.end, centerVec + axisY * segLen);
			proxy.capsule.radius = r;
		}
		else if (c.type == ColliderType::Cylinder) {
			XMStoreFloat3(&proxy.cylinder.center, centerVec);
			XMVECTOR axisY = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);
			XMStoreFloat3(&proxy.cylinder.axis, axisY);
			proxy.cylinder.height = c.cylinder.height * gScale.y;
			proxy.cylinder.radius = c.cylinder.radius * std::max(gScale.x, gScale.z);
		}
		proxies.push_back(proxy);
		});


	// 総当たり判定
	std::vector<Physics::Contact> contacts;

	for (size_t i = 0; i < proxies.size(); ++i) {
		for (size_t j = i + 1; j < proxies.size(); ++j) {
			auto& A = proxies[i];
			auto& B = proxies[j];

			if (A.bodyType == BodyType::Static && B.bodyType == BodyType::Static) continue;

			Physics::Contact contact;
			contact.a = A.entity;
			contact.b = B.entity;
			bool hit = false;

			// Sphere vs ...
			if (A.type == ColliderType::Sphere && B.type == ColliderType::Sphere)
				hit = CheckSphereSphere(A.sphere, B.sphere, contact);
			else if (A.type == ColliderType::Sphere && B.type == ColliderType::Box)
				hit = CheckSphereOBB(A.sphere, B.obb, contact);
			else if (A.type == ColliderType::Box && B.type == ColliderType::Sphere) {
				hit = CheckSphereOBB(B.sphere, A.obb, contact);
				if (hit) { contact.normal.x *= -1; contact.normal.y *= -1; contact.normal.z *= -1; }
			}
			else if (A.type == ColliderType::Sphere && B.type == ColliderType::Capsule)
				hit = CheckSphereCapsule(A.sphere, B.capsule, contact);
			else if (A.type == ColliderType::Capsule && B.type == ColliderType::Sphere) {
				hit = CheckSphereCapsule(B.sphere, A.capsule, contact);
				if (hit) { contact.normal.x *= -1; contact.normal.y *= -1; contact.normal.z *= -1; }
			}
			else if (A.type == ColliderType::Sphere && B.type == ColliderType::Cylinder)
				hit = CheckSphereCylinder(A.sphere, B.cylinder, contact);
			else if (A.type == ColliderType::Cylinder && B.type == ColliderType::Sphere) {
				hit = CheckSphereCylinder(B.sphere, A.cylinder, contact);
				if (hit) { contact.normal.x *= -1; contact.normal.y *= -1; contact.normal.z *= -1; }
			}

			// Box vs ...
			else if (A.type == ColliderType::Box && B.type == ColliderType::Box)
				hit = CheckOBBOBB(A.obb, B.obb, contact);
			else if (A.type == ColliderType::Box && B.type == ColliderType::Capsule)
				hit = CheckOBBCapsule(A.obb, B.capsule, contact);
			else if (A.type == ColliderType::Capsule && B.type == ColliderType::Box) {
				hit = CheckOBBCapsule(B.obb, A.capsule, contact);
				if (hit) { contact.normal.x *= -1; contact.normal.y *= -1; contact.normal.z *= -1; }
			}
			else if (A.type == ColliderType::Box && B.type == ColliderType::Cylinder)
				hit = CheckOBBCylinder(A.obb, B.cylinder, contact);
			else if (A.type == ColliderType::Cylinder && B.type == ColliderType::Box) {
				hit = CheckOBBCylinder(B.obb, A.cylinder, contact);
				if (hit) { contact.normal.x *= -1; contact.normal.y *= -1; contact.normal.z *= -1; }
			}

			// Capsule vs ...
			else if (A.type == ColliderType::Capsule && B.type == ColliderType::Capsule)
				hit = CheckCapsuleCapsule(A.capsule, B.capsule, contact);
			else if (A.type == ColliderType::Capsule && B.type == ColliderType::Cylinder) {
				// 円柱をカプセル近似して判定
				Physics::Capsule cylCap;
				XMVECTOR cAx = XMLoadFloat3(&B.cylinder.axis);
				XMVECTOR cC = XMLoadFloat3(&B.cylinder.center);
				float hH = B.cylinder.height * 0.5f;
				XMStoreFloat3(&cylCap.start, cC - cAx * hH);
				XMStoreFloat3(&cylCap.end, cC + cAx * hH);
				cylCap.radius = B.cylinder.radius;

				// CheckCylinderCylinder ではなく CheckCapsuleCapsule を呼ぶ！
				hit = CheckCapsuleCapsule(A.capsule, cylCap, contact);
			}
			else if (A.type == ColliderType::Cylinder && B.type == ColliderType::Capsule) {
				Physics::Capsule cylCap;
				XMVECTOR cAx = XMLoadFloat3(&A.cylinder.axis);
				XMVECTOR cC = XMLoadFloat3(&A.cylinder.center);
				float hH = A.cylinder.height * 0.5f;
				XMStoreFloat3(&cylCap.start, cC - cAx * hH);
				XMStoreFloat3(&cylCap.end, cC + cAx * hH);
				cylCap.radius = A.cylinder.radius;

				// ここも CheckCapsuleCapsule を呼ぶ！
				hit = CheckCapsuleCapsule(cylCap, B.capsule, contact);
			}

			// Cylinder vs Cylinder
			else if (A.type == ColliderType::Cylinder && B.type == ColliderType::Cylinder)
				hit = CheckCylinderCylinder(A.cylinder, B.cylinder, contact);


			if (hit) {
				if (A.isTrigger || B.isTrigger) {
					// Logger::Log("Trigger Hit!");
				}
				else {
					contacts.push_back(contact);
				}
			}
		}
	}

	// 解決
	PhysicsSystem::Solve(registry, contacts);
}