/*****************************************************************//**
 * @file	UISystem.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/18	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___UI_SYSTEM_H___
#define ___UI_SYSTEM_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Scene/Core/ECS/ECS.h"
#include "Engine/Scene/Components/Components.h"

class UISystem : public ISystem
{
public:
	UISystem() { m_systemName = "UI System"; }

	void Update(Registry& registry) override
	{
		// 1. 画面サイズ（Canvas）の取得
		// Canvasコンポーネントを持つエンティティを探す
		float screenW = 1920.0f;
		float screenH = 1080.0f;

		// Canvasが見つかれば、そのサイズを画面サイズとする
		auto canvasView = registry.view<Canvas>();
		for (auto e : canvasView) {
			const auto& canvas = canvasView.get<Canvas>(e);
			screenW = canvas.referenceSize.x;
			screenH = canvas.referenceSize.y;
			break; // ルートは1つと仮定
		}

		// 2. 再帰計算用の関数
		std::function<void(Entity, const D2D1_RECT_F&, const D2D1_MATRIX_3X2_F&)> updateNode =
			[&](Entity entity, const D2D1_RECT_F& parentRect, const D2D1_MATRIX_3X2_F& parentMat)
			{
				if (!registry.has<Transform2D>(entity)) return;
				auto& t = registry.get<Transform2D>(entity);

				float parentW = parentRect.right - parentRect.left;
				float parentH = parentRect.bottom - parentRect.top;

				// アンカー基準位置
				float anchorX = parentRect.left + (parentW * (t.anchorMin.x + t.anchorMax.x) * 0.5f);
				float anchorY = parentRect.top + (parentH * (t.anchorMin.y + t.anchorMax.y) * 0.5f);

				// 中心座標
				float posX = anchorX + t.position.x;
				float posY = anchorY + t.position.y;

				// サイズ (簡易版)
				float width = t.size.x;
				float height = t.size.y;

				// 矩形計算 (左上)
				float left = posX - (width * t.pivot.x);
				float top = posY - (height * t.pivot.y);

				// 計算結果を保存
				t.calculatedRect = { left, top, left + width, top + height };

				// 行列計算 (回転・スケール)
				D2D1_POINT_2F centerPoint = { left + (width * t.pivot.x), top + (height * t.pivot.y) };

				D2D1_MATRIX_3X2_F localMat =
					D2D1::Matrix3x2F::Scale(t.scale.x, t.scale.y, centerPoint) *
					D2D1::Matrix3x2F::Rotation(t.rotation.z, centerPoint); // Z軸回転

				// 親行列と合成
				t.worldMatrix = localMat * parentMat;

				// 子へ再帰
				if (registry.has<Relationship>(entity)) {
					for (Entity child : registry.get<Relationship>(entity).children) {
						
						D2D1_RECT_F childParentRect = D2D1::RectF(
							t.calculatedRect.x,
							t.calculatedRect.y,
							t.calculatedRect.z,
							t.calculatedRect.w
						);

						updateNode(child, childParentRect, t.worldMatrix);
					}
				}
			};

		// 3. ルート要素（Canvas または 親なしTransform2D）から計算開始
		D2D1_RECT_F rootRect = D2D1::RectF(0, 0, screenW, screenH);
		D2D1_MATRIX_3X2_F identity = D2D1::IdentityMatrix();

		// Canvasを持つエンティティがあればそれを起点にする
		bool canvasFound = false;
		for (auto e : canvasView) {
			updateNode(e, rootRect, identity);
			canvasFound = true;
		}

		// Canvasがない、またはCanvas外のルート要素もケアする場合
		if (!canvasFound) {
			registry.view<Transform2D>().each([&](Entity e, Transform2D& t) {
				// 親がいない、かつCanvasも持っていないものをルートとみなす
				bool isRoot = true;
				if (registry.has<Relationship>(e)) {
					Entity p = registry.get<Relationship>(e).parent;
					if (p != NullEntity && registry.has<Transform2D>(p)) isRoot = false;
				}
				if (isRoot && !registry.has<Canvas>(e)) {
					updateNode(e, rootRect, identity);
				}
				});
		}
	}
};

#endif // !___UI_SYSTEM_H___
