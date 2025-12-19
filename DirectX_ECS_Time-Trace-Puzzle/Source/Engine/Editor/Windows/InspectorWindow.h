/*****************************************************************//**
 * @file	InspectorWindow.h
 * @brief	インスペクターウィンドウ
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
 * @note	
 * コンポーネントを追加した際：
 * 1. Draw()内、コンポーネント一覧に追加
 * 2. AddComponentPopupに追加
 *********************************************************************/

#ifndef ___INSPECTOR_WINDOW_H___
#define ___INSPECTOR_WINDOW_H___

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Editor/Core/Editor.h"
#include "Engine/Components/Components.h"
#include "Engine/Audio/AudioManager.h"
#include "Engine/Resource/Serializer.h"

class InspectorWindow : public EditorWindow
{
public:
	void Draw(World& world, Entity& selected, Context& ctx) override
	{
		Registry& reg = world.getRegistry();

		if (selected == NullEntity || !reg.has<Tag>(selected)) return;

		ImGui::Begin("Inspector");

		// --------------------------------------------------------
		// ヘッダー (ID & Name)
		// --------------------------------------------------------
		Tag& tag = reg.get<Tag>(selected);
		ImGui::Text("ID: %d", selected);

		char buffer[256];
		strcpy_s(buffer, tag.name.c_str());

		if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
		{
			tag.name = buffer;
		}

		ImGui::Separator();

		// --------------------------------------------------------
		// コンポーネント一覧
		// --------------------------------------------------------

		// 1. Transform
		if (reg.has<Transform>(selected)) {
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
				Transform& t = reg.get<Transform>(selected);
				ImGui::DragFloat3("Position", &t.position.x, 0.1f);

				// 回転を度数法で表示・編集
				XMFLOAT3 rotDeg;
				rotDeg.x = XMConvertToDegrees(t.rotation.x);
				rotDeg.y = XMConvertToDegrees(t.rotation.y);
				rotDeg.z = XMConvertToDegrees(t.rotation.z);
				if (ImGui::DragFloat3("Rotation", &rotDeg.x, 0.1f)) {
					t.rotation.x = XMConvertToRadians(rotDeg.x);
					t.rotation.y = XMConvertToRadians(rotDeg.y);
					t.rotation.z = XMConvertToRadians(rotDeg.z);
				}

				ImGui::DragFloat3("Scale", &t.scale.x, 0.01f);
			}
		}

		// 2. MeshComponent
		if (reg.has<MeshComponent>(selected)) {
			if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
				MeshComponent& m = reg.get<MeshComponent>(selected);

				// ファイル選択 (Modelsフォルダ)
				FileSelector("Model", m.modelKey, "Resources/Models", ".fbx", ResourceManager::ResourceType::Model); // .objなども可

				ImGui::ColorEdit4("Color", &m.color.x);
				ImGui::DragFloat3("Scale Offset", &m.scaleOffset.x, 0.01f);

				if (ImGui::Button("Remove Mesh")) reg.remove<MeshComponent>(selected);
			}
		}

		// 3. SpriteComponent
		if (reg.has<SpriteComponent>(selected)) {
			if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
				SpriteComponent& s = reg.get<SpriteComponent>(selected);

				FileSelector("Texture", s.textureKey, "Resources/Textures", ".png", ResourceManager::ResourceType::Texture);

				ImGui::ColorEdit4("Color", &s.color.x);

				if (ImGui::Button("Remove Sprite")) reg.remove<SpriteComponent>(selected);
			}
		}

		// 4. BillboardComponent
		if (reg.has<BillboardComponent>(selected)) {
			if (ImGui::CollapsingHeader("Billboard", ImGuiTreeNodeFlags_DefaultOpen)) {
				BillboardComponent& b = reg.get<BillboardComponent>(selected);

				FileSelector("Texture", b.textureKey, "Resources/Textures", ".png", ResourceManager::ResourceType::Texture);

				ImGui::DragFloat2("Size", &b.size.x);
				ImGui::ColorEdit4("Color", &b.color.x);

				if (ImGui::Button("Remove Billboard")) reg.remove<BillboardComponent>(selected);
			}
		}

		if (reg.has<TextComponent>(selected)) {
			if (ImGui::CollapsingHeader("Text", ImGuiTreeNodeFlags_DefaultOpen)) {
				TextComponent& t = reg.get<TextComponent>(selected);

				char buf[256]; strcpy_s(buf, t.text.c_str());
				if (ImGui::InputTextMultiline("Content", buf, sizeof(buf))) t.text = buf;

				// フォント名入力（簡易版）
				char fontBuf[64]; strcpy_s(fontBuf, t.fontKey.c_str());
				if (ImGui::InputText("Font", fontBuf, sizeof(fontBuf))) t.fontKey = fontBuf;

				ImGui::DragFloat("Size", &t.fontSize, 0.5f, 1.0f, 300.0f);
				ImGui::ColorEdit4("Color", &t.color.x);
				ImGui::DragFloat2("Offset", &t.offset.x);

				// スタイル設定
				ImGui::Checkbox("Bold", &t.isBold);
				ImGui::SameLine();
				ImGui::Checkbox("Italic", &t.isItalic);

				ImGui::Checkbox("Center Align", &t.centerAlign);

				if (ImGui::Button("Remove Text")) reg.remove<TextComponent>(selected);
			}
		}

		// 5. AudioSource
		if (reg.has<AudioSource>(selected)) {
			if (ImGui::CollapsingHeader("Audio Source", ImGuiTreeNodeFlags_DefaultOpen)) {
				AudioSource& a = reg.get<AudioSource>(selected);

				FileSelector("Sound", a.soundKey, "Resources/Sounds", ".wav", ResourceManager::ResourceType::Sound);

				ImGui::SliderFloat("Volume", &a.volume, 0.0f, 1.0f);
				ImGui::DragFloat("Range", &a.range, 0.1f);
				ImGui::Checkbox("Loop", &a.isLoop);
				ImGui::Checkbox("Play On Awake", &a.playOnAwake);

				if (ImGui::Button("Test Play")) {
					AudioManager::Instance().PlaySE(a.soundKey, a.volume);
				}
				if (ImGui::Button("Remove Audio")) reg.remove<AudioSource>(selected);
			}
		}

		// 6. Camera
		if (reg.has<Camera>(selected)) {
			if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
				Camera& c = reg.get<Camera>(selected);

				float fovDeg = XMConvertToDegrees(c.fov);
				if (ImGui::DragFloat("FOV", &fovDeg, 1.0f, 1.0f, 179.0f)) {
					c.fov = XMConvertToRadians(fovDeg);
				}
				ImGui::DragFloat("Near Z", &c.nearZ, 0.01f);
				ImGui::DragFloat("Far Z", &c.farZ, 1.0f);
				ImGui::Text("Aspect: %.2f", c.aspect);

				if (ImGui::Button("Remove Camera")) reg.remove<Camera>(selected);
			}
		}

		// 7. Collider (高度な当たり判定)
		if (reg.has<Collider>(selected)) {
			if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
				Collider& c = reg.get<Collider>(selected);

				// タイプの切り替え
				const char* types[] = { "Box", "Sphere", "Capsule", "Cylinder" };
				int currentType = (int)c.type;
				if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types))) {
					c.type = (ColliderType)currentType;
				}
				ImGui::Checkbox("Is Trigger", &c.isTrigger);

				ImGui::DragFloat3("Offset", &c.offset.x, 0.01f);

				// タイプごとのパラメータ
				if (c.type == ColliderType::Box)
				{
					ImGui::DragFloat3("Size", &c.boxSize.x, 0.01f);
				}
				else if (c.type == ColliderType::Sphere)
				{
					ImGui::DragFloat("Radius", &c.sphere.radius, 0.01f);
				}
				else if (c.type == ColliderType::Capsule)
				{
					ImGui::DragFloat("Radius", &c.capsule.radius, 0.01f);
					ImGui::DragFloat("Height", &c.capsule.height, 0.01f);
				}
				else if (c.type == ColliderType::Cylinder)
				{
					ImGui::DragFloat("Radius", &c.cylinder.radius, 0.01f);
					ImGui::DragFloat("Height", &c.cylinder.height, 0.01f);
				}

				if (ImGui::Button("Remove Collider")) reg.remove<Collider>(selected);
			}
		}

		// 8. Physics / Logic
		if (reg.has<Rigidbody>(selected)) {
			if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen)) {
				Rigidbody& rb = reg.get<Rigidbody>(selected);

				// BodyType コンボボックス
				const char* types[] = { "Static", "Dynamic", "Kinematic" };
				int current = (int)rb.type;
				if (ImGui::Combo("Body Type", &current, types, IM_ARRAYSIZE(types))) {
					rb.type = (BodyType)current;
				}

				ImGui::DragFloat("Mass", &rb.mass, 0.1f);
				ImGui::DragFloat("Drag", &rb.drag, 0.01f);
				ImGui::Checkbox("Use Gravity", &rb.useGravity);
				if (ImGui::Button("Stop")) rb.velocity = { 0,0,0 };
				if (ImGui::Button("Remove Rigidbody")) reg.remove<Rigidbody>(selected);
			}
		}
		if (reg.has<PlayerInput>(selected)) {
			if (ImGui::CollapsingHeader("Player Input", ImGuiTreeNodeFlags_DefaultOpen)) {
				PlayerInput& p = reg.get<PlayerInput>(selected);
				ImGui::DragFloat("Speed", &p.speed, 0.1f);
				ImGui::DragFloat("Jump", &p.jumpPower, 0.1f);
				if (ImGui::Button("Remove PlayerInput")) reg.remove<PlayerInput>(selected);
			}
		}
		if (reg.has<Lifetime>(selected)) {
			if (ImGui::CollapsingHeader("Lifetime", ImGuiTreeNodeFlags_DefaultOpen)) {
				Lifetime& l = reg.get<Lifetime>(selected);
				ImGui::DragFloat("Time Left", &l.time);
				if (ImGui::Button("Remove Lifetime")) reg.remove<Lifetime>(selected);
			}
		}

		ImGui::Separator();

		// --------------------------------------------------------
		// Add Component
		// --------------------------------------------------------
		if (ImGui::Button("Add Component", ImVec2(-1, 30))) {
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup")) {
			// ここに全てのコンポーネントを追加
			if (!reg.has<MeshComponent>(selected) && ImGui::Selectable("Mesh")) reg.emplace<MeshComponent>(selected, "hero");
			if (!reg.has<SpriteComponent>(selected) && ImGui::Selectable("Sprite")) reg.emplace<SpriteComponent>(selected, "player");
			if (!reg.has<BillboardComponent>(selected) && ImGui::Selectable("Billboard")) reg.emplace<BillboardComponent>(selected, "star");
			if (!reg.has<Collider>(selected) && ImGui::Selectable("Collider")) reg.emplace<Collider>(selected);
			if (!reg.has<AudioSource>(selected) && ImGui::Selectable("Audio Source")) reg.emplace<AudioSource>(selected, "jump");
			if (!reg.has<Camera>(selected) && ImGui::Selectable("Camera")) reg.emplace<Camera>(selected);

			if (!reg.has<Rigidbody>(selected) && ImGui::Selectable("Rigidbody")) reg.emplace<Rigidbody>(selected);
			if (!reg.has<PlayerInput>(selected) && ImGui::Selectable("Player Input")) reg.emplace<PlayerInput>(selected);
			if (!reg.has<Lifetime>(selected) && ImGui::Selectable("Lifetime")) reg.emplace<Lifetime>(selected, 10.0f);

			ImGui::EndPopup();
		}

		ImGui::Separator();

		// Save Prefab
		if (ImGui::Button("Save as Prefab")) {
			std::string path = std::string("Resources/Prefabs/") + reg.get<Tag>(selected).name.c_str() + ".json";
			Serializer::SaveEntity(reg, selected, path);
			Logger::Log("Saved Prefab: " + path);
		}

		// Destroy
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
		if (ImGui::Button("Destroy Entity", ImVec2(-1, 30))) {
			reg.destroy(selected);
			selected = NullEntity;
		}
		ImGui::PopStyleColor();

		ImGui::End();
	}

private:
	// --------------------------------------------------------
	// リソース選択用ヘルパー (StringId & ResourceType対応)
	// --------------------------------------------------------
	void FileSelector(const char* label, StringId& currentId, const std::string& dir, const std::string& filterExt, ResourceManager::ResourceType type)
	{
		// 1. 表示名の取得
		// StringIdからパスを取得を試みる
		std::string preview = ResourceManager::Instance().GetPathByKey(currentId, type);

		// 見つからない場合（未登録 or ID=0）
		if (preview.empty()) {
			if (currentId.GetHash() == 0) preview = "None";
			else preview = currentId.c_str(); // Debug時のみハッシュ値または元文字列
		}

		// 2. コンボボックス描画
		if (ImGui::BeginCombo(label, preview.c_str()))
		{
			namespace fs = std::filesystem;
			if (fs::exists(dir)) {
				for (const auto& entry : fs::recursive_directory_iterator(dir)) {
					if (!entry.is_regular_file()) continue;

					// 拡張子フィルタ
					std::string ext = entry.path().extension().string();
					if (ext == filterExt) { // 大文字小文字無視は省略
						std::string path = entry.path().string();
						std::replace(path.begin(), path.end(), '\\', '/');

						StringId pathId(path);
						bool isSelected = (currentId == pathId);

						if (ImGui::Selectable(path.c_str(), isSelected)) {
							// ★重要: 選択されたらResourceManagerに「パスをキーとして」登録する
							ResourceManager::Instance().RegisterResource(pathId, path, type);
							currentId = pathId;
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
				}
			}
			ImGui::EndCombo();
		}

		// 3. ドラッグ＆ドロップ受け入れ
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
			{
				const char* droppedPath = (const char*)payload->Data;
				StringId newId(droppedPath);

				// ドロップされたらResourceManagerに登録する
				ResourceManager::Instance().RegisterResource(newId, droppedPath, type);

				currentId = newId;
			}
			ImGui::EndDragDropTarget();
		}
	}
};

#endif	//!___INSPECTOR_WINDOW_H___
