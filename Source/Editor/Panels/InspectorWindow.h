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
#include "Editor/Core/Editor.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Audio/AudioManager.h"
#include "Editor/Core/InspectorGui.h"
#include "Engine/Scene/Serializer/SceneSerializer.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Renderer/Text/FontManager.h"

class InspectorWindow : public EditorWindow
{
public:
	void Draw(World& world, Entity& selected, Context& ctx) override
	{
		Registry& reg = world.getRegistry();

		if (selected == NullEntity || !reg.has<Tag>(selected)) return;

		ImGui::Begin("Inspector");

		// ★重要: エンティティごとにIDをプッシュして、入力フォーカス外れを防ぐ
		ImGui::PushID((int)selected);

		// --------------------------------------------------------
		// ヘッダー (ID & Name)
		// --------------------------------------------------------
		ImGui::Text("ID: %d", selected);

		// Tagコンポーネントは手動で描画（名前変更のため）
		auto& tag = reg.get<Tag>(selected);
		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		strcpy_s(buffer, sizeof(buffer), tag.name.c_str());
		if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
			tag.name = buffer;
		}

		ImGui::Separator();

		// --------------------------------------------------------
		// コンポーネント一覧 (テンプレート関数呼び出し)
		// --------------------------------------------------------
		DrawAllComponents<ComponentList>(reg, selected);

		ImGui::Separator();

		// --------------------------------------------------------
		// Add Component
		// --------------------------------------------------------
		if (ImGui::Button("Add Component", ImVec2(-1, 30))) {
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup")) {
			// (既存のリストと同じ)
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

		// Save / Destroy
		if (ImGui::Button("Save as Prefab")) {
			std::string path = std::string("Resources/Game/Prefabs/") + reg.get<Tag>(selected).name.c_str() + ".json";
			SceneSerializer::SaveEntity(reg, selected, path);
			Logger::Log("Saved Prefab: " + path);
		}

		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
		if (ImGui::Button("Destroy Entity", ImVec2(-1, 30))) {
			reg.destroy(selected);
			selected = NullEntity;
		}
		ImGui::PopStyleColor();

		// ★IDポップ
		ImGui::PopID();

		ImGui::End();
	}

private:
	// --------------------------------------------------------
	// リソース選択用プルダウン (ResourceManager連携版)
	// --------------------------------------------------------
	// label: ラベル
	// currentKey: 現在のStringId (入出力)
	// type: リソースの種類
	void DrawResourceCombo(const char* label, StringId& currentKey, ResourceManager::ResourceType type)
	{
		// 現在設定されているキー名を文字列で取得
		std::string currentKeyName = currentKey.c_str();

		// 現在設定されているパス（ツールチップ用）
		std::string currentPath = ResourceManager::Instance().GetPathByKey(currentKey, type);

		// プレビュー表示は「キー名」にする (空ならNone)
		const char* previewValue = currentKeyName.empty() ? "(None)" : currentKeyName.c_str();

		if (ImGui::BeginCombo(label, previewValue))
		{
			// (None) を選べるようにする
			if (ImGui::Selectable("(None)", currentKey.GetHash() == 0)) {
				currentKey = StringId("");
			}

			// ResourceManagerから「キー名」のリストを取得
			auto list = ResourceManager::Instance().GetResourceList(type);

			for (const auto& keyName : list)
			{
				// キー名同士で比較する
				bool isSelected = (currentKeyName == keyName);

				if (ImGui::Selectable(keyName.c_str(), isSelected))
				{
					currentKey = StringId(keyName); // キー名をセット
				}

				// マウスオーバーで実際のパスを表示（便利機能）
				if (ImGui::IsItemHovered()) {
					std::string path = ResourceManager::Instance().GetPathByKey(StringId(keyName), type);
					ImGui::SetTooltip("%s", path.c_str());
				}

				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// ドラッグ＆ドロップ対応
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const char* droppedRelativePath = (const char*)payload->Data;

				// エンジンが読み込めるようにルートパスを補完
				std::string rootPath = "Resources/Game/";
				std::string fullPath = rootPath + droppedRelativePath;

				// ドロップされたパスをキーとして登録してしまう
				// (例: キー="Resources/Texture/a.png", パス="Resources/Texture/a.png")
				StringId newKey(fullPath);
				ResourceManager::Instance().RegisterResource(newKey, fullPath, type);
				currentKey = newKey;
			}
			ImGui::EndDragDropTarget();
		}
	}



	// --------------------------------------------------------
	// 専用インスペクター関数
	// --------------------------------------------------------
	// --------------------------------------------------------
	// MeshComponent Inspector
	// --------------------------------------------------------
	void DrawMeshInspector(MeshComponent& c)
	{
		if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 1. モデル選択
			DrawResourceCombo("Model File", c.modelKey, ResourceManager::ResourceType::Model);

			ImGui::Spacing();

			// 2. スケール補正 (DragFloat3)
			// アセット自体のサイズが大きすぎたり小さすぎたりする場合の補正値
			if (ImGui::DragFloat3("Scale Offset", &c.scaleOffset.x, 0.01f, 0.001f, 1000.0f))
			{
				// 0以下にならないようにガード
				if (c.scaleOffset.x < 0.001f) c.scaleOffset.x = 0.001f;
				if (c.scaleOffset.y < 0.001f) c.scaleOffset.y = 0.001f;
				if (c.scaleOffset.z < 0.001f) c.scaleOffset.z = 0.001f;
			}
			// マウスオーバー時に説明を表示
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adjusts the intrinsic scale of the model asset.");

			// 3. カラー (ColorEdit4)
			ImGui::ColorEdit4("Color", &c.color.x);
		}
	}

	// --------------------------------------------------------
	// SpriteComponent Inspector
	// --------------------------------------------------------
	void DrawSpriteInspector(SpriteComponent& c)
	{
		if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 1. テクスチャ選択
			DrawResourceCombo("Texture", c.textureKey, ResourceManager::ResourceType::Texture);

			// 2. カラー & 透明度 (ColorEdit4)
			ImGui::ColorEdit4("Color", &c.color.x);

			// 3. プレビュー表示
			// ResourceManagerからテクスチャの実体を取得して表示する
			auto texture = ResourceManager::Instance().GetTexture(c.textureKey);
			if (texture && texture->srv)
			{
				ImGui::Separator();
				ImGui::Text("Preview:");

				// アスペクト比を維持して表示幅を決める
				float width = (float)texture->width;
				float height = (float)texture->height;
				float aspect = width / height;

				float displayWidth = 128.0f;			// プレビューの最大幅
				float displayHeight = displayWidth / aspect;

				// 画像を表示 (ImTextureID に SRV をキャストして渡す)
				ImGui::Image((void*)texture->srv.Get(), ImVec2(displayWidth, displayHeight), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f));

				// 画像情報の表示
				ImGui::SameLine();
				ImGui::BeginGroup();
				ImGui::Text("Size: %dx%d", texture->width, texture->height);
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Path: %s", c.textureKey.c_str());
				ImGui::EndGroup();
			}
			else if (c.textureKey.GetHash() != 0)
			{
				// 設定されているがロードできていない場合
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Texture not loaded or found.");
			}
		}
	}

	void DrawAudioInspector(AudioSource& c)
	{
		if (ImGui::CollapsingHeader("Audio Source", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// リソース選択プルダウン
			DrawResourceCombo("Audio Clip", c.soundKey, ResourceManager::ResourceType::Sound);
			
			// ボリュームと範囲
			ImGui::DragFloat("Volume", &c.volume, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Range", &c.range, 0.1f, 0.0f, 100.0f);

			// ループ設定
			ImGui::Checkbox("Loop", &c.isLoop);
			ImGui::Checkbox("Play On Awake", &c.playOnAwake);

			// プレビュー再生ボタン
			if (ImGui::Button("Preview Play")) AudioManager::Instance().PlaySE(c.soundKey);
			ImGui::SameLine();
		}
	}

	// --------------------------------------------------------
	// TextComponent Inspector
	// --------------------------------------------------------
	void DrawTextInspector(TextComponent& c)
	{
		if (ImGui::CollapsingHeader("Text", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 1. テキスト本文 (複数行入力対応)
			ImGui::Text("Content");
			char buffer[1024];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, sizeof(buffer), c.text.c_str());
			// Ctrl+Enterで改行できるようにする
			if (ImGui::InputTextMultiline("##Content", buffer, sizeof(buffer), ImVec2(-1, 60))) {
				c.text = buffer;
			}

			ImGui::Spacing();

			// 2. フォント選択 (FontManagerからリスト取得)
			// 現在のフォント名
			std::string currentFont = c.fontKey;
			if (currentFont.empty()) currentFont = "Default";

			if (ImGui::BeginCombo("Font Family", currentFont.c_str()))
			{
				// デフォルト選択肢
				if (ImGui::Selectable("Default", c.fontKey == "Default" || c.fontKey.empty())) {
					c.fontKey = "Default";
				}

				// ロード済みフォント一覧を表示
				// FontManager::Initialize() が呼ばれている前提
				const auto& fontList = FontManager::Instance().GetLoadedFontNames();
				for (const auto& fontName : fontList)
				{
					bool isSelected = (c.fontKey == fontName);
					if (ImGui::Selectable(fontName.c_str(), isSelected))
					{
						c.fontKey = fontName;
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// 3. 基本設定
			ImGui::DragFloat("Size", &c.fontSize, 0.5f, 1.0f, 300.0f);
			ImGui::ColorEdit4("Color", &c.color.x);

			// 4. レイアウト・スタイル
			ImGui::Separator();
			ImGui::Text("Style & Layout");

			// 中央揃え
			ImGui::Checkbox("Center Align", &c.centerAlign);

			// 太字・斜体 (同じ行に並べる)
			ImGui::SameLine();
			ImGui::Checkbox("Bold", &c.isBold);
			ImGui::SameLine();
			ImGui::Checkbox("Italic", &c.isItalic);

			// オフセット調整
			ImGui::DragFloat2("Offset", &c.offset.x);
		}
	}

	// --------------------------------------------------------
	// コンポーネント全描画ループ
	// --------------------------------------------------------
	template<typename TupleT>
	void DrawAllComponents(Registry& registry, Entity entity)
	{
		auto view = [&]<typename... Ts>(std::tuple<Ts...>*)
		{
			(..., [&](void)
				{
					using ComponentType = Ts;
					if constexpr (std::is_same_v<ComponentType, DummyComponent>) return; // Dummyスキップ
					if constexpr (std::is_same_v<ComponentType, Tag>) return; // Tagは上で描画したのでスキップ

					if (registry.has<ComponentType>(entity))
					{
						// ★ここで分岐！ 特定の型なら専用関数、それ以外は汎用関数
						if constexpr (std::is_same_v<ComponentType, MeshComponent>) {
							DrawMeshInspector(registry.get<ComponentType>(entity));
						}
						else if constexpr (std::is_same_v<ComponentType, SpriteComponent>) {
							DrawSpriteInspector(registry.get<ComponentType>(entity));
						}
						else if constexpr (std::is_same_v<ComponentType, AudioSource>) {
							DrawAudioInspector(registry.get<ComponentType>(entity));
						}
						else if constexpr (std::is_same_v<ComponentType, TextComponent>)
						{
							DrawTextInspector(registry.get<ComponentType>(entity));
						}
						else {
							// それ以外（Transform, Colliderなど）は既存の汎用描画を使う
							bool removed = false;
							// コンポーネントごとにIDをプッシュ（念のため）
							ImGui::PushID(Reflection::Meta<ComponentType>::Name);

							DrawComponent(
								Reflection::Meta<ComponentType>::Name,
								registry.get<ComponentType>(entity),
								removed
							);

							ImGui::PopID();

							if (removed) registry.remove<ComponentType>(entity);
						}
					}
				}());
		};
		view(static_cast<TupleT*>(nullptr));
	}
};

#endif	//!___INSPECTOR_WINDOW_H___