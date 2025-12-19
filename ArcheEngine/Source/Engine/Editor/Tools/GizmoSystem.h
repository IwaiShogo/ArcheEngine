/*****************************************************************//**
 * @file	GizmoSystem.h
 * @brief	ギズモの描画ロジック
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/27	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___GIZMO_SYSTEM_H___
#define ___GIZMO_SYSTEM_H___

#include "Engine/pch.h"
#include "Engine/ECS/ECS.h"
#include "Engine/Components/Components.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/Context.h" // Config::SCREEN_WIDTH等用
#include "Engine/Config.h"

class GizmoSystem
{
public:
	static void Draw(Registry& reg, Entity& selected, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj, float x, float y, float width, float height)
	{
		// --- ギズモ処理 ---
		// 選択中のEntityがあり、Transformを持っている場合
		if (selected != NullEntity && reg.has<Transform>(selected))
		{
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(x, y, width, height);

			// 行列をfloat配列に変換 (Transpose含む)
			float viewM[16], projM[16];
			MatrixToFloat16(view, viewM);
			MatrixToFloat16(proj, projM);

			// 対象のTransformを取得
			Transform& t = reg.get<Transform>(selected);

			// Transform -> Matrix
			XMMATRIX worldMat = 
				XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z) *
				XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, t.rotation.z) *
				XMMatrixTranslation(t.position.x, t.position.y, t.position.z);

			float worldM[16];
			MatrixToFloat16(t.GetWorldMatrix(), worldM);

			if (Input::GetKeyDown('L'))
			{
				auto LogMatrix = [](const char* name, float* m) {
					Logger::Log("--- " + std::string(name) + " ---");
					for (int i = 0; i < 4; ++i) {
						Logger::Log(
							std::to_string(m[i + 0]) + ", " + std::to_string(m[i + 4]) + ", " +
							std::to_string(m[i + 8]) + ", " + std::to_string(m[i + 12])
						);
					}
					};

				LogMatrix("View Matrix", viewM);
				LogMatrix("Proj Matrix", projM);
				LogMatrix("World Matrix", worldM);

				// ギズモの有効状態も確認
				Logger::Log("Gizmo Enabled: " + std::string(ImGuizmo::IsOver() ? "Over" : "Not Over"));
				Logger::Log("Gizmo Using: " + std::string(ImGuizmo::IsUsing() ? "Using" : "Not Using"));
			}

			// 操作モード
			static ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
			if (Input::GetKeyDown('1')) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
			if (Input::GetKeyDown('2')) mCurrentGizmoOperation = ImGuizmo::ROTATE;
			if (Input::GetKeyDown('3')) mCurrentGizmoOperation = ImGuizmo::SCALE;

			// ギズモ描画と操作判定
			if (ImGuizmo::Manipulate(viewM, projM, mCurrentGizmoOperation, ImGuizmo::WORLD, worldM)) {
				Float16ToTransform(worldM, t);
			}
		}

	}

private:
	// ヘルパー: XMMATRIX -> float[16]
	static void MatrixToFloat16(const DirectX::XMMATRIX& m, float* out) {
		DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)out, m);
	}

	// ヘルパー: float[16] -> XMMATRIX, Decompose
	static void Float16ToTransform(const float* in, Transform& t) {
		DirectX::XMFLOAT4X4 m;
		memcpy(&m, in, sizeof(float) * 16);

		DirectX::XMMATRIX mat = DirectX::XMLoadFloat4x4(&m);

		DirectX::XMVECTOR scale, rotQuat, trans;
		DirectX::XMMatrixDecompose(&scale, &rotQuat, &trans, mat);

		DirectX::XMStoreFloat3(&t.position, trans);
		DirectX::XMStoreFloat3(&t.scale, scale);

		// クォータニオンからオイラー角への変換は少し複雑ですが、
		// 簡易的にImGuizmoの機能を使って分解します
		float translation[3], rotation[3], scale_f[3];
		ImGuizmo::DecomposeMatrixToComponents(in, translation, rotation, scale_f);

		// ImGuizmoは度数法(Degrees)で返すので、ラジアンに戻す
		t.rotation.x = DirectX::XMConvertToRadians(rotation[0]);
		t.rotation.y = DirectX::XMConvertToRadians(rotation[1]);
		t.rotation.z = DirectX::XMConvertToRadians(rotation[2]);
	}
};

#endif
