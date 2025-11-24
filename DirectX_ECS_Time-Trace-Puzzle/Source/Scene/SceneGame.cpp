/*****************************************************************//**
 * @file	SceneGame.cpp
 * @brief	ゲームシーン
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

// ===== インクルード =====
#include "Scene/SceneGame.h"
#include "Scene/SceneManager.h"
#include <string>

void SceneGame::Initialize()
{
	// 親クラスの初期化
	IScene::Initialize();

	// システム追加
	m_world.registerSystem<MovementSystem>();
	m_world.registerSystem<CollisionSystem>();
	auto inputSys = m_world.registerSystem<InputSystem>();
	inputSys->SetContext(m_context);

	// Entityの生成
	// Camera
	m_world.create_entity()
		.add<Tag>("MainCamera")
		.add<Transform>(XMFLOAT3(2.0f, 10.0f, -10.0f), XMFLOAT3(0.78f, 0.0f, 0.0f))
		.add<Camera>();

	// Player
	m_world.create_entity()
		.add<Tag>("Player")
		.add<Transform>(XMFLOAT3(0.0f, 0.0f, 0.0f))
		.add<Velocity>(XMFLOAT3(0.0f, 0.0f, 0.0f))
		.add<BoxCollider>(XMFLOAT3(1.0f, 1.0f, 1.0f))
		.add<PlayerInput>();

	// Enemy
	m_world.create_entity()
		.add<Tag>("Enemy")
		.add<Transform>(XMFLOAT3(5.0f, 0.0f, 0.0f))
		.add<BoxCollider>(XMFLOAT3(1.0f, 1.0f, 1.0f));
}

void SceneGame::Finalize()
{
}

void SceneGame::Update()
{
	IScene::Update();

	if (m_context && m_context->debug.useDebugCamera) {

		Registry& reg = m_world.getRegistry();

		// MainCameraを探す
		reg.view<Tag, Transform, Camera>([&](Entity e, Tag& tag, Transform& t, Camera& c) {
			if (strcmp(tag.name, "MainCamera") == 0) {

				// ------------------------------------------------
				// 1. マウス回転 (右クリック中のみ)
				// ------------------------------------------------
				if (Input::GetMouseRightButton()) {
					float rotSpeed = 0.005f; // 感度

					// X移動 -> Y軸回転(Yaw)
					t.rotation.y += Input::GetMouseDeltaX() * rotSpeed;
					// Y移動 -> X軸回転(Pitch)
					t.rotation.x += Input::GetMouseDeltaY() * rotSpeed;
				}

				// ------------------------------------------------
				// 2. カメラ視点での移動 (WASD)
				// ------------------------------------------------
				float moveSpeed = 10.0f * Time::DeltaTime();

				// 左Shiftで倍速
				if (Input::GetKey(VK_SHIFT)) moveSpeed *= 3.0f;

				// 現在の回転から、前方・右方ベクトルを計算
				XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, 0.0f);

				XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotationMatrix);
				XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotationMatrix);
				// 上昇下降用に上ベクトル
				XMVECTOR up = XMVectorSet(0, 1, 0, 0); // ワールドY軸

				XMVECTOR pos = XMLoadFloat3(&t.position);

				// W/S (前後)
				if (Input::GetKey('W')) pos += forward * moveSpeed;
				if (Input::GetKey('S')) pos -= forward * moveSpeed;

				// D/A (右左)
				if (Input::GetKey('D')) pos += right * moveSpeed;
				if (Input::GetKey('A')) pos -= right * moveSpeed;

				// E/Q (上昇下降: Unityスタイル)
				if (Input::GetKey('E')) pos += up * moveSpeed;
				if (Input::GetKey('Q')) pos -= up * moveSpeed;

				// 結果を保存
				XMStoreFloat3(&t.position, pos);
			}
			});
	}
}

void SceneGame::Render()
{
	IScene::Render();
}

void SceneGame::OnInspector()
{
	// メインのInspectorウィンドウ
	ImGui::Begin("Scene Inspector");

	Registry& reg = m_world.getRegistry();

	// --------------------------------------------------------
	// 1. システム稼働状況 (System Monitor)
	// --------------------------------------------------------
	if (ImGui::CollapsingHeader("System Monitor", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Columns(2, "systems"); // 2列表示
		ImGui::Text("System Name"); ImGui::NextColumn();
		ImGui::Text("Time (ms)"); ImGui::NextColumn();
		ImGui::Separator();

		for (const auto& sys : m_world.getSystems()) {
			ImGui::Text("%s", sys->m_systemName.c_str());
			ImGui::NextColumn();

			// 処理時間によって色を変える (重いと赤)
			if (sys->m_lastExecutionTime > 1.0) ImGui::TextColored(ImVec4(1, 0, 0, 1), "%.3f ms", sys->m_lastExecutionTime);
			else ImGui::Text("%.3f ms", sys->m_lastExecutionTime);

			ImGui::NextColumn();
		}
		ImGui::Columns(1); // 列解除
	}

	// --------------------------------------------------------
	// 2. エンティティリスト (Entity Inspector)
	// --------------------------------------------------------
	if (ImGui::CollapsingHeader("Entity List", ImGuiTreeNodeFlags_DefaultOpen)) {
		// 全Entityを列挙するために Tag コンポーネントで回す
		// (Tagを持たないEntityは表示されない制限がありますが、今は全て持っているのでOK)
		int count = 0;
		reg.view<Tag>([&](Entity e, Tag& tag) {
			count++;

			// ツリーノードで表示
			// IDと名前を表示
			std::string label = std::to_string(e) + ": " + tag.name;
			if (ImGui::TreeNode(label.c_str())) {

				// --- コンポーネント情報の詳細表示 ---
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "[Components]");

				// Transform
				if (reg.has<Transform>(e)) {
					Transform& t = reg.get<Transform>(e);
					if (ImGui::TreeNode("Transform")) {
						ImGui::DragFloat3("Position", &t.position.x, 0.1f);
						ImGui::DragFloat3("Rotation", &t.rotation.x, 1.0f);
						ImGui::DragFloat3("Scale", &t.scale.x, 0.1f);
						ImGui::TreePop();
					}
				}

				// Velocity
				if (reg.has<Velocity>(e)) {
					Velocity& v = reg.get<Velocity>(e);
					ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", v.velocity.x, v.velocity.y, v.velocity.z);
				}

				// BoxCollider
				if (reg.has<BoxCollider>(e)) {
					BoxCollider& b = reg.get<BoxCollider>(e);
					if (ImGui::TreeNode("BoxCollider")) {
						ImGui::DragFloat3("Size", &b.size.x, 0.1f);
						ImGui::DragFloat3("Offset", &b.offset.x, 0.1f);
						ImGui::TreePop();
					}
				}

				// PlayerInput
				if (reg.has<PlayerInput>(e)) {
					PlayerInput& p = reg.get<PlayerInput>(e);
					ImGui::Text("Input: Speed=%.1f, Jump=%.1f", p.speed, p.jumpPower);
				}

				// 削除ボタン
				if (ImGui::Button("Destroy")) {
					// 削除処理（簡易実装：Tagを消すことでリストから見えなくする）
					// 本来は m_world.destroy(e) のようなメソッドが必要
					reg.remove<Tag>(e);
				}

				ImGui::TreePop();
			}
			});

		ImGui::Separator();
		ImGui::Text("Total Entities: %d", count);
	}

	// --------------------------------------------------------
	// 3. プレイヤー詳細情報 (Player Watcher)
	// --------------------------------------------------------
	// Tagが"Player"であるものを探す
	bool playerFound = false;
	reg.view<Tag>([&](Entity e, Tag& tag) {
		if (!playerFound && strcmp(tag.name, "Player") == 0) {
			playerFound = true;

			if (ImGui::CollapsingHeader("Player Watcher", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Text("ID: %d", e);

				// 座標
				Transform& t = reg.get<Transform>(e);
				ImGui::Text("Pos: (%.2f, %.2f, %.2f)", t.position.x, t.position.y, t.position.z);

				// 速度
				Velocity& v = reg.get<Velocity>(e);
				ImGui::Text("Vel: (%.2f, %.2f, %.2f)", v.velocity.x, v.velocity.y, v.velocity.z);
				// スピードメーター（長さ）
				float speed = std::sqrt(v.velocity.x * v.velocity.x + v.velocity.z * v.velocity.z);
				ImGui::ProgressBar(speed / 10.0f, ImVec2(0, 0), "Speed");

				// 接地判定 (簡易)
				bool isGrounded = (t.position.y <= 0.001f);
				ImGui::Text("Grounded: %s", isGrounded ? "YES" : "NO");
			}
		}
		});

	ImGui::End();
}