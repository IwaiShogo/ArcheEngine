/*****************************************************************//**
 * @file	InspectorGui.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/19	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___INSPECTOR_GUI_H___
#define ___INSPECTOR_GUI_H___

#ifdef _DEBUG

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Core/Base/Reflection.h"
#include "Engine/Core/Base/StringId.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Scene/Components/ComponentDefines.h"

namespace Arche
{
	ARCHE_API void Inspector_Snapshot(ImGuiID id, std::function<void(json&)> saveFn);
	ARCHE_API void Inspector_Commit(ImGuiID id, std::function<void(json&)> saveFn, std::function<void(const json&, const json&)> cmdFn);
	ARCHE_API bool Inspector_HasState(ImGuiID id);

	// ------------------------------------------------------------
	// InspectorGuiVisitor
	// 型に応じたImGuiウィジェットを描画する
	// ------------------------------------------------------------
	struct InspectorGuiVisitor
	{
		std::function<void(json&)> serializeFunc;
		std::function<void(const json&, const json&)> commandFunc;

		// 編集開始時の状態を保持する
		//static inline std::map<ImGuiID, json> s_startStates;

		InspectorGuiVisitor(std::function<void(json&)> sFunc, std::function<void(json, json)> cFunc)
			: serializeFunc(sFunc), commandFunc(cFunc) {}

		// 比較用ヘルパー
		// ------------------------------------------------------------
		template<typename T> bool IsChanged(const T& a, const T& b) { return a != b; }
		bool IsChanged(const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b) { return a.x != b.x || a.y != b.y; }
		bool IsChanged(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) { return a.x != b.x || a.y != b.y || a.z != b.z; }
		bool IsChanged(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b) { return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w; }

		// 共通処理ラッパー
		// ImGuiのウィジェットを描画し、編集開始・終了を検知する
		// ------------------------------------------------------------
		template<typename T, typename Func>
		void DrawWidget(const char* label, T& currentVal, Func func)
		{
			// 1. 描画前の値をバックアップ
			T oldVal = currentVal;

			ImGui::PushID(label);
			ImGuiID id = ImGui::GetID(label);

			// 2. ウィジェット描画実行
			func();

			// 3. 編集開始検知
			if (ImGui::IsItemActivated() || (ImGui::IsItemActive() && !Inspector_HasState(id)))
			{
				if (!Inspector_HasState(id))
				{
					if (IsChanged(oldVal, currentVal))
					{
						T temp = currentVal;
						currentVal = oldVal;

						// Engine側でスナップショット作成
						Inspector_Snapshot(id, serializeFunc);

						currentVal = temp;
					}
					else
					{
						Inspector_Snapshot(id, serializeFunc);
					}
				}
			}

			// 4. 編集終了検知
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (Inspector_HasState(id))
				{
					Inspector_Commit(id, serializeFunc, commandFunc);
				}
			}

			ImGui::PopID();
		}

		// 各型の描画実装
		// ------------------------------------------------------------

		// 基本型
		// ------------------------------------------------------------
		// bool
		void operator()(bool& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				ImGui::Checkbox(name, &val);
			});
		}

		// int
		void operator()(int& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				ImGui::DragInt(name, &val);
			});
		}

		// float
		void operator()(float& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				float speed = 0.1f;
				float min = 0.0f;
				float max = 0.0f;

				// 名前による挙動の変化
				if (strstr(name, "volume") || strstr(name, "Volume"))
				{
					speed = 0.01f;
					max = 1.0f;
				}
				else if (strstr(name, "size") || strstr(name, "Size"))
				{
					speed = 1.0f;
				}

				ImGui::DragFloat(name, &val, speed, min, max);
			});
		}

		// 文字列（std::string）
		// ------------------------------------------------------------
		void operator()(std::string& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				// 1. テキスト本文
				if (strcmp(name, "text") == 0 || strcmp(name, "Content") == 0)
				{
					ImGui::LabelText(name, "Text Content");
					char buf[1024];
					strcpy_s(buf, sizeof(buf), val.c_str());
					if (ImGui::InputTextMultiline(name, buf, sizeof(buf), ImVec2(-1, 60)))
					{
						val = buf;
					}
				}
				// 2. フォント名 (プルダウン表示)
				else if (strcmp(name, "fontKey") == 0)
				{
					if (ImGui::BeginCombo(name, val.c_str())) {
						std::string fontDir = "Resources/Game/Fonts";
						namespace fs = std::filesystem;

						if (fs::exists(fontDir)) {
							for (const auto& entry : fs::directory_iterator(fontDir)) {
								if (!entry.is_regular_file()) continue;

								// 拡張子チェック (.ttf, .otf)
								std::string ext = entry.path().extension().string();
								if (ext == ".ttf" || ext == ".otf") {
									// ファイル名（拡張子なし）をキーとする場合
									// std::string fontName = entry.path().stem().string();

									// ファイル名（拡張子あり）をキーとする場合 ← FontManagerの実装に合わせてください
									std::string fontName = entry.path().filename().string();

									bool isSelected = (val == fontName);
									if (ImGui::Selectable(fontName.c_str(), isSelected)) {
										val = fontName;
									}
									if (isSelected) ImGui::SetItemDefaultFocus();
								}
							}
						}
						ImGui::EndCombo();
					}
				}
				else
				{
					char buf[256];
					strcpy_s(buf, sizeof(buf), val.c_str());
					if (ImGui::InputText(name, buf, sizeof(buf)))
					{
						val = buf;
					}
				}
			});
		}

		// リソースキー（StringId） -> ドラッグ&ドロップ対応
		// ------------------------------------------------------------
		void operator()(StringId& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				std::string currentStr = val.c_str();
				char buf[256];
				strcpy_s(buf, currentStr.c_str());

				// 入力欄
				if (ImGui::InputText(name, buf, sizeof(buf)))
				{
					val = StringId(buf);
				}

				// マウスオーバーでパスを表示
				if (ImGui::IsItemHovered())
				{
					// リソースタイプを推測してパスを取得
					ResourceManager::ResourceType type = ResourceManager::ResourceType::Texture;
					if (strstr(name, "model") || strstr(name, "Model")) type = ResourceManager::ResourceType::Model;
					else if (strstr(name, "sound") || strstr(name, "Sound")) type = ResourceManager::ResourceType::Sound;

					std::string path = ResourceManager::Instance().GetPathByKey(val, type);
					if (!path.empty()) ImGui::SetTooltip("Path: %s", path.c_str());
				}

				// ドラッグ&ドロップ受け入れ
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const char* droppedPath = (const char*)payload->Data;
						std::string fullPath = "Resources/Game/" + std::string(droppedPath);

						// リソースタイプを推測
						ResourceManager::ResourceType type = ResourceManager::ResourceType::Texture;
						std::string n = name;
						if (n.find("model") != std::string::npos || n.find("Model") != std::string::npos) type = ResourceManager::ResourceType::Model;
						else if (n.find("sound") != std::string::npos || n.find("Sound") != std::string::npos) type = ResourceManager::ResourceType::Sound;

						// キーとして登録 & 設定
						StringId newKey(fullPath);
						ResourceManager::Instance().RegisterResource(newKey, fullPath, type);
						val = newKey;
					}
					ImGui::EndDragDropTarget();
				}
			});
		}

		// DirectX Math型
		// ------------------------------------------------------------
		// XMFLOAT2
		void operator()(DirectX::XMFLOAT2& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				float v[2] = { val.x, val.y };
				if (ImGui::DragFloat2(name, v, 0.1f))
				{
					val = { v[0], v[1] };
				}
			});
		}

		// XMFLOAT3
		void operator()(DirectX::XMFLOAT3& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				float v[3] = { val.x, val.y, val.z };
				if (strstr(name, "color") || strstr(name, "Color"))
				{
					if (ImGui::ColorEdit3(name, v)) val = { v[0], v[1], v[2] };
				}
				else
				{
					if (ImGui::DragFloat3(name, v, 0.1f)) val = { v[0], v[1], v[2] };
				}
			});
		}

		// XMFLOAT4型（空ピッカー）
		void operator()(DirectX::XMFLOAT4& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				// 変数名に Color が含まれていなくても、XMFLOAT4は基本的に色として扱う
				float v[4] = { val.x, val.y, val.z, val.w };
				if (ImGui::ColorEdit4(name, v))
				{
					val = { v[0], v[1], v[2], v[3] };
				}
			});
		}

		// Enum型
		// ------------------------------------------------------------
		// ColliderType
		void operator()(ColliderType& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				// プルダウンの中身
				const char* items[] = { "Box", "Sphere", "Capsule", "Cylinder" };
				int item = (int)val;

				if (ImGui::Combo(name, &item, items, IM_ARRAYSIZE(items))) val = (ColliderType)item;
			});
		}

		// BodyType
		void operator()(BodyType& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				const char* items[] = { "Static", "Dynamic", "Kinematic" };
				int item = (int)val;
				if (ImGui::Combo(name, &item, items, IM_ARRAYSIZE(items))) val = (BodyType)item;
			});
		}

		// Layer
		void operator()(Layer& val, const char* name)
		{
			DrawWidget(name, val, [&]()
			{
				int layerVal = (int)val;
				if (ImGui::InputInt(name, &layerVal)) val = (Layer)layerVal;
			});
		}

		// その他（Entity等）
		// ------------------------------------------------------------
		// Entity
		void operator()(Entity& val, const char* name)
		{
			if (val == NullEntity) ImGui::Text("%s: (None)", name);
			else ImGui::Text("%s: Entity(%d)", name, (uint32_t)val);
		}

		// Vector (Sprcialized for Entity)
		void operator()(std::vector<Entity>& val, const char* name)
		{
			// 折り畳み式のツリーで子要素を表示
			if (ImGui::TreeNode(name))
			{
				if (val.empty())
				{
					ImGui::TextDisabled("Empty");
				}
				else
				{
					for (size_t i = 0; i < val.size(); ++i)
					{
						ImGui::Text("[%d] Entity(ID: %d)", i, (uint32_t)val[i]);
					}
				}
				ImGui::TreePop();
			}
			else
			{
				// 閉じているときは要素数だけ表示
				ImGui::SameLine();
				ImGui::TextDisabled("(%d items)", val.size());
			}
		}

		// Fallback
		template<typename T>
		void operator()(T& val, const char* name)
		{
			ImGui::TextDisabled("%s: (Not Supported)", name);
		}
	};

	// ------------------------------------------------------------
	// ヘルパー関数：コンポーネントを描画する
	// ------------------------------------------------------------
	template<typename T>
	void DrawComponent(
		const char* label,
		T& component,
		bool& removed,
		std::function<void(json&)> serializeFunc,			// ★追加: 保存用関数
		std::function<void(const json&, const json&)> commandFunc,		// ★追加: コマンド発行用関数
		int index = -1,
		std::function<void(int, int)> onReorder = nullptr
	)
	{
		// D&D用のID
		ImGui::PushID(label);

		// ヘッダー描画
		bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);

		// ドラッグ&ドロップ処理
		// ------------------------------------------------------------
		if (onReorder && index >= 0)
		{
			// 1. ドラッグ元（Source）
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				// 並び変え元のインデックスを渡す
				ImGui::SetDragDropPayload("COMPONENT_ORDER", &index, sizeof(int));
				ImGui::Text("Move %s", label);
				ImGui::EndDragDropSource();
			}

			// 2. ドロップ先（Target）
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT_ORDER"))
				{
					int srcIndex = *(const int*)payload->Data;
					// 自分自身へのドロップでなければ並び替え実行
					if (srcIndex != index)
					{
						onReorder(srcIndex, index);
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		if (open)
		{
			// 受け取った関数を使ってVisitorを正しく構築する
			InspectorGuiVisitor visitor(serializeFunc, commandFunc);
			Reflection::VisitMembers(component, visitor);

			ImGui::Spacing();
			if (ImGui::Button("Remove Component"))
			{
				removed = true;
			}
		}

		ImGui::PopID();
	}

}	// namespace Arche

#endif // _DEBUG

#endif // !___INSPECTOR_GUI_H___