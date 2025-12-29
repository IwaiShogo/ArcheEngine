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

#ifdef _DEBUG

// ===== インクルード =====
#include "Engine/pch.h"
#include "Editor/Core/Editor.h"
#include "Engine/Scene/Components/Components.h"
#include "Engine/Scene/Serializer/ComponentRegistry.h"
#include "Editor/Core/CommandHistory.h"
#include "Editor/Core/EditorCommands.h"

namespace Arche
{

	class InspectorWindow : public EditorWindow
	{
	public:
		void Draw(World& world, Entity& selected, Context& ctx) override
		{
			ImGui::Begin("Inspector");

			if (selected != NullEntity)
			{
				Registry& reg = world.getRegistry();

				// 1. Tag編集（特別扱い）
				// ------------------------------------------------------------
				if (reg.has<Tag>(selected))
				{
					auto& tag = reg.get<Tag>(selected);
					char buf[256];
					strcpy_s(buf, tag.name.c_str());

					ImGui::SetNextItemWidth(-1);
					if (ImGui::InputText("##Tag", buf, sizeof(buf)))
					{
						tag.name = buf;
					}
				}
				else
				{
					ImGui::Text("Entity ID: %d", (uint32_t)selected);
				}

				ImGui::Separator();

				// 2. コンポーネント自動描画
				// ------------------------------------------------------------
				// レジストリに登録されているすべてのコンポーネントを確認
				for (auto& [name, iface] : ComponentRegistry::Instance().GetInterfaces())
				{
					// エンティティが持っている場合のみ表示
					if (iface.has(reg, selected))
					{
						// 折り畳みヘッダーを表示
						bool isOpen = ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

						// 右クリックで削除メニュー
						if (ImGui::BeginPopupContextItem(name.c_str()))
						{
							if (ImGui::MenuItem("Remove Component"))
							{
								CommandHistory::Execute(std::make_shared<RemoveComponentCommand>(world, selected, name));
							}
							ImGui::EndPopup();
						}

						if (iface.has(reg, selected) && isOpen)
						{
							ImGui::PushID(name.c_str());
							ImGui::Indent();

							// コマンド発行用のコールバックを作成
							auto onCommand = [&](json oldVal, json newVal)
							{
								CommandHistory::Execute(std::make_shared<ChangeComponentValueCommand>(
									world, selected, name, oldVal, newVal
								));
							};

							// Inspector描画実行
							iface.drawInspector(reg, selected, onCommand);

							ImGui::Unindent();
							ImGui::PopID();
							ImGui::Spacing();
						}
					}
				}

				ImGui::Separator();

				// 3. コンポーネント追加ボタン
				// ------------------------------------------------------------
				// ボタンを中央揃えにする
				float width = ImGui::GetWindowWidth();
				float btnW = 200.0f;
				ImGui::SetCursorPosX((width - btnW) * 0.5f);
				if (ImGui::Button("Add Component", ImVec2(btnW, 0)))
				{
					ImGui::OpenPopup("AddComponentPopup");
				}

				if (ImGui::BeginPopup("AddComponentPopup"))
				{
					// 検索ボックス
					static char searchBuf[64] = "";
					ImGui::InputTextWithHint("##Search", "Search...", searchBuf, sizeof(searchBuf));

					ImGui::Separator();

					for (auto& [name, iface] : ComponentRegistry::Instance().GetInterfaces())
					{
						// まだ持っていない、かつ検索ワードにヒットするものだけ表示
						if (!iface.has(reg, selected))
						{
							// 検索フィルター
							if (searchBuf[0] != '\0')
							{
								// 部分一致検索
								std::string n = name;
								std::string s = searchBuf;
								if (n.find(s) == std::string::npos) continue;
							}

							if (ImGui::MenuItem(name.c_str()))
							{
								CommandHistory::Execute(std::make_shared<AddComponentCommand>(world, selected, name));
								ImGui::CloseCurrentPopup();
							}
						}
					}
					ImGui::EndPopup();
				}
			}
			ImGui::End();
		}
	};

}	// namespace Arche

#endif // _DEBUG

#endif	//!___INSPECTOR_WINDOW_H___