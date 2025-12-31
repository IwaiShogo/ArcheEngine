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

					// 順序リストの同期
					SyncComponentOrder(reg, selected, tag);
				}
				else
				{
					ImGui::Text("Entity ID: %d", (uint32_t)selected);
				}

				ImGui::Separator();

				// 2. コンポーネント自動描画
				// ------------------------------------------------------------
				// レジストリに登録されているすべてのコンポーネントを確認
				if (reg.has<Tag>(selected))
				{
					auto& tag = reg.get<Tag>(selected);
					auto& interfaces = ComponentRegistry::Instance().GetInterfaces();

					// 並び替え用コールバック
					auto onReorder = [&](int srcIdx, int dstIdx)
					{
						std::vector<std::string> newOrder = tag.componentOrder;
						std::string item = newOrder[srcIdx];
						newOrder.erase(newOrder.begin() + srcIdx);
						newOrder.insert(newOrder.begin() + dstIdx, item);

						CommandHistory::Execute(std::make_shared<ReorderComponentCommand>(
							world, selected, tag.componentOrder, newOrder
						));
					};

					// 順序リストに基づいて描画
					for (int i = 0; i < tag.componentOrder.size(); ++i)
					{
						std::string name = tag.componentOrder[i];

						if (name == "Tag") continue;

						if (interfaces.count(name))
						{
							auto& iface = interfaces.at(name);

							// 削除時のコールバック
							auto onRemove = [&, name]() {
								CommandHistory::Execute(std::make_shared<RemoveComponentCommand>(world, selected, name));
								};

							// 値変更用コールバック
							auto onCommand = [&](json oldVal, json newVal)
							{
								CommandHistory::Execute(std::make_shared<ChangeComponentValueCommand>(
									world, selected, name, oldVal, newVal));
							};

							// if文の羅列を廃止し、統一されたインターフェースを呼ぶ
							if (iface.drawInspectorDnD)
							{
								iface.drawInspectorDnD(reg, selected, i, onReorder, onRemove, onCommand);
							}
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

					// A. 通常コンポーネント（Data Components）
					for (auto& [name, iface] : ComponentRegistry::Instance().GetInterfaces())
					{
						if (name == "NativeScriptComponent") continue;

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

	private:
		// コンポーネント順序リストを実体と同期させる
		void SyncComponentOrder(Registry& reg, Entity e, Tag& tag)
		{
			auto& interfaces = ComponentRegistry::Instance().GetInterfaces();
			std::vector<std::string> actualComponents;

			// 1. 実際に持っているコンポーネントをリストアップ
			for (auto& [name, iface] : interfaces)
			{
				if (iface.has(reg, e))
				{
					actualComponents.push_back(name);
				}
			}

			// 2. componentOrder に足りないものを追加
			for (const auto& name : actualComponents)
			{
				if (std::find(tag.componentOrder.begin(), tag.componentOrder.end(), name) == tag.componentOrder.end())
				{
					tag.componentOrder.push_back(name);
				}
			}

			// 3. componentOrder にあるが実際にはないものを削除
			for (auto it = tag.componentOrder.begin(); it != tag.componentOrder.end();)
			{
				if (std::find(actualComponents.begin(), actualComponents.end(), *it) == actualComponents.end())
				{
					it = tag.componentOrder.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	};

}	// namespace Arche

#endif // _DEBUG

#endif	//!___INSPECTOR_WINDOW_H___