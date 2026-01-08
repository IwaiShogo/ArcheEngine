/*****************************************************************//**
 * @file	InspectorGui.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
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
#include "Engine/Renderer/Text/FontManager.h"

namespace Arche
{
	ARCHE_API void Inspector_Snapshot(ImGuiID id, std::function<void(json&)> saveFn);
	ARCHE_API void Inspector_Commit(ImGuiID id, std::function<void(json&)> saveFn, std::function<void(const json&, const json&)> cmdFn);
	ARCHE_API bool Inspector_HasState(ImGuiID id);

	inline std::unordered_map<std::string, ImFont*> g_InspectorFontMap;

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
					namespace fs = std::filesystem;

					// ヘルパー: ディレクトリ内のファイルを走査してコンボボックスを表示
					auto ShowResourceCombo = [&](const std::string& dir, const std::vector<std::string>& extensions, bool useStem)
						{
							if (ImGui::BeginCombo(name, val.c_str()))
							{
								if (fs::exists(dir))
								{
									for (const auto& entry : fs::directory_iterator(dir))
									{
										if (!entry.is_regular_file()) continue;

										std::string ext = entry.path().extension().string();

										// 拡張子チェック
										bool isMatch = false;
										for (const auto& targetExt : extensions) {
											if (ext == targetExt) { isMatch = true; break; }
										}

										if (isMatch)
										{
											// useStemがtrueなら拡張子なし、falseならファイル名そのまま
											std::string itemName = useStem ? entry.path().stem().string() : entry.path().filename().string();

											bool isSelected = (val == itemName);
											if (ImGui::Selectable(itemName.c_str(), isSelected))
											{
												val = itemName;
											}
											if (isSelected) ImGui::SetItemDefaultFocus();
										}
									}
								}
								ImGui::EndCombo();
							}
						};

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
					// 2. フォント選択 (.ttf / .otf)
					else if (strcmp(name, "fontKey") == 0)
					{
						// FontManagerが認識している「正式なフォント名リスト」を取得して表示する
						const auto& loadedFonts = FontManager::Instance().GetLoadedFontNames();

						if (ImGui::BeginCombo(name, val.c_str()))
						{
							// カスタムフォント一覧
							for (const auto& fontName : loadedFonts)
							{
								bool isSelected = (val == fontName);
								bool pushed = false;
								if (g_InspectorFontMap.find(fontName) != g_InspectorFontMap.end())
								{
									ImGui::PushFont(g_InspectorFontMap[fontName]);
									pushed = true;
								}

								if (ImGui::Selectable(fontName.c_str(), isSelected))
								{
									val = fontName;
								}

								if (pushed) ImGui::PopFont();

								if (isSelected) ImGui::SetItemDefaultFocus();
							}

							// システムフォントの代表例も追加しておく（必要であれば）
							const char* systemFonts[] = { "Meiryo", "Yu Gothic", "MS Gothic", "Arial" };
							for (const char* sysFont : systemFonts)
							{
								bool isSelected = (val == sysFont);
								if (ImGui::Selectable(sysFont, isSelected))
								{
									val = sysFont;
								}
								if (isSelected) ImGui::SetItemDefaultFocus();
							}

							ImGui::EndCombo();
						}
					}
					// 3. テクスチャ/画像選択 (.png, .jpg, etc)
					// 変数名に "texture", "image", "sprite", "path" などが含まれる場合
					else if (strstr(name, "texture") || strstr(name, "Texture") ||
						strstr(name, "image") || strstr(name, "Image") ||
						strstr(name, "sprite") || strstr(name, "Sprite"))
					{
						// useStem = false (拡張子あり) で表示。ResourceManagerの仕様に合わせて調整してください。
						// 画像ファイルは拡張子で区別することが多いため、通常は拡張子ありが望ましいです。
						ShowResourceCombo("Resources/Game/Textures", { ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds" }, false);
					}
					// 4. モデル選択 (.fbx, .obj, etc)
					else if (strstr(name, "model") || strstr(name, "Model") ||
						strstr(name, "mesh") || strstr(name, "Mesh"))
					{
						ShowResourceCombo("Resources/Game/Models", { ".fbx", ".obj", ".gltf", ".glb" }, false);
					}
					// 5. アニメーション選択
					else if (strstr(name, "animation") || strstr(name, "Animation"))
					{
						ShowResourceCombo("Resources/Game/Animations", { ".fbx" }, false);
					}
					// 6. その他 (標準入力 + D&D)
					else
					{
						char buf[256];
						strcpy_s(buf, sizeof(buf), val.c_str());
						if (ImGui::InputText(name, buf, sizeof(buf)))
						{
							val = buf;
						}

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
							{
								const wchar_t* droppedPathW = (const wchar_t*)payload->Data;
								std::filesystem::path p(droppedPathW);
								std::string fullPath = p.string();
								std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

								size_t found = fullPath.find("Resources");
								if (found != std::string::npos) val = fullPath.substr(found);
								else val = fullPath;
							}
							ImGui::EndDragDropTarget();
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
					std::string s = val.c_str();
					char buf[256];
					strcpy_s(buf, sizeof(buf), s.c_str());

					ImGui::InputText(name, buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
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
		Entity entity,
		Registry& registry,
		bool& removed,
		std::function<void(json&)> serializeFunc,
		std::function<void(const json&, const json&)> commandFunc,
		int index = -1,
		std::function<void(int, int)> onReorder = nullptr
	)
	{
		// D&D用のID
		ImGui::PushID(label);

		// チェックボックスの状態を取得
		bool isEnabled = registry.isComponentEnabled<T>(entity);

		float headerX = ImGui::GetCursorPosX();

		ImGui::Checkbox("##Enabled", &isEnabled);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			registry.setComponentEnabled<T>(entity, isEnabled);
		}
		ImGui::SameLine();

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