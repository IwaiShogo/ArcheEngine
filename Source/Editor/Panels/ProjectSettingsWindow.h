/*****************************************************************//**
 * @file	ProjectSettingsWindow.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#ifndef ___PROJECT_SETTINGS_WINDOW_H___
#define ___PROJECT_SETTINGS_WINDOW_H___

// ===== インクルード =====
#include "Editor/Core/Editor.h"
#include "Engine/Scene/Components/Components.h"

namespace Arche
{
	class ProjectSettingsWindow
		: public EditorWindow
	{
	public:
		void Draw(World& world, Entity& selected, Context& ctx) override
		{
			ImGui::Begin("Project Settings");

			if (ImGui::CollapsingHeader("Physics Collision Matrix", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// 定義されているレイヤー一覧（名前は管理クラスを作るのが理想ですが、一旦ハードコードで例示）
				const char* layerNames[] = { "Default", "Player", "Enemy", "Wall", "Item" };
				Layer layers[] = { Layer::Default, Layer::Player, Layer::Enemy, Layer::Wall, Layer::Item };
				int count = 5;

				// テーブルヘッダー
				ImGui::Columns(count + 1, "LayerMatrix", false);
				ImGui::NextColumn();
				for (int i = 0; i < count; i++)
				{
					// 縦書き風に表示、あるいは斜めにするのはImGuiでは難しいので番号か名前で
					ImGui::Text("%s", layerNames[i]);
					ImGui::NextColumn();
				}
				ImGui::Separator();

				// マトリックス描画
				for (int i = 0; i < count; i++)
				{
					ImGui::Text("%s", layerNames[i]); // 行見出し
					ImGui::NextColumn();

					for (int j = 0; j < count; j++)
					{
						// 対角線より上だけ表示（重複設定を防ぐため）
						if (j < i)
						{
							ImGui::NextColumn();
							continue;
						}

						std::string id = std::string("##") + std::to_string(i) + "_" + std::to_string(j);

						Layer maskA = PhysicsConfig::GetMask(layers[i]);
						bool check = (maskA & layers[j]) == layers[j];

						if (ImGui::Checkbox(id.c_str(), &check))
						{
							if (check)
								PhysicsConfig::Configure(layers[i]).collidesWith(layers[j]);
							else
								PhysicsConfig::Configure(layers[i]).ignore(layers[j]);
						}
						ImGui::NextColumn();
					}
				}
				ImGui::Columns(1);
			}

			ImGui::End();
		}
	};
}

#endif // !___PROJECT_SETTINGS_WINDOW_H___