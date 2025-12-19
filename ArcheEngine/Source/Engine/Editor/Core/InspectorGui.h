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

// ===== インクルード =====
#include "Engine/pch.h"
#include "Engine/Core/Reflection.h"
#include "Engine/Core/StringId.h"

// ------------------------------------------------------------
// InspectorGuiVisitor
// リフレクション経由で変数を渡され、型に応じたImGuiを描画する
// ------------------------------------------------------------
struct InspectorGuiVisitor
{
	// flaot型
	void operator()(float& val, const char* name)
	{
		// 変数名によってスライダーの感度や範囲を変える工夫も可能
		if (strstr(name, "Size") || strstr(name, "size"))
		{
			ImGui::DragFloat(name, &val, 1.0f, 0.0f, 1000.0f);
		}
		else
		{
			ImGui::DragFloat(name, &val, 0.1f);
		}
	}

	// int型
	void operator()(int& val, const char* name)
	{
		ImGui::DragInt(name, &val);
	}

	// bool型
	void operator()(bool& val, const char* name)
	{
		ImGui::Checkbox(name, &val);
	}

	// XMFLOAT2型
	void operator()(DirectX::XMFLOAT2& val, const char* name)
	{
		float v[2] = { val.x, val.y };
		if (ImGui::DragFloat2(name, v, 0.1f))
		{
			val = { v[0], v[1] };
		}
	}

	// XMFLOAT3型
	void operator()(DirectX::XMFLOAT3& val, const char* name)
	{
		float v[3] = { val.x, val.y, val.z };
		if (ImGui::DragFloat3(name, v, 0.1f))
		{
			val = { v[0], v[1], v[2] };
		}
	}

	// XMFLOAT4型（空ピッカー）
	void operator()(DirectX::XMFLOAT4& val, const char* name)
	{
		// 変数名に Color が含まれていなくても、XMFLOAT4は基本的に色として扱う
		float v[4] = { val.x, val.y, val.z, val.w };
		if (ImGui::ColorEdit4(name, v))
		{
			val = { v[0], v[1], v[2], v[3] };
		}
	}

	// std::string型（日本語入力対応）
	void operator()(std::string& val, const char* name)
	{
		// 1. テキスト本文 (大きく表示)
		if (strcmp(name, "text") == 0) {
			char buf[1024];
			memset(buf, 0, sizeof(buf));

			// コピーと切り詰め
			if (val.size() < sizeof(buf)) strcpy_s(buf, val.c_str());
			else memcpy(buf, val.data(), sizeof(buf) - 1);

			ImGui::LabelText(name, "Content"); // ラベル
			if (ImGui::InputTextMultiline(
				"##text", // ID衝突防止のため##を使う
				buf, sizeof(buf),
				ImVec2(-1.0f, 100.0f) // 横幅いっぱい、高さ100px
			)) {
				val = buf;
			}
			return;
		}

		// 2. フォント名 (プルダウン表示)
		if (strcmp(name, "fontKey") == 0) {
			if (ImGui::BeginCombo(name, val.c_str())) {
				// Resources/Fonts フォルダを走査して表示
				// ※本来はResourceManagerかFontManagerから取得するのがベストですが、簡易実装として
				std::string fontDir = "Resources/Fonts";
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
			return;
		}

		// 3. その他 (通常の1行入力)
		char buf[256];
		memset(buf, 0, sizeof(buf));
		if (val.size() < sizeof(buf)) strcpy_s(buf, val.c_str());
		else memcpy(buf, val.data(), sizeof(buf) - 1);

		if (ImGui::InputText(name, buf, sizeof(buf))) {
			val = buf;
		}
	}

	// StringId
	void operator()(StringId& val, const char* name)
	{
		// 現在の文字列を取得
		// 注意：Releaseビルドで StringId が文字列を保持していない場合、ここは空文字になります
		std::string currentStr = val.c_str();

		char buf[256];
		memset(buf, 0, sizeof(buf));
		strcpy_s(buf, currentStr.c_str());

		// テキストボックスで編集（IDなのでラベルの横にハッシュ値も出す）
		std::string label = std::string(name) + " (Hash: " + std::to_string(val.GetHash()) + ")";

		if (ImGui::InputText(label.c_str(), buf, sizeof(buf)))
		{
			val = StringId(buf);
		}
	}

	// その他の型が来てもエラーにならないようにするテンプレート
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
void DrawComponent(const char* label, T& component, bool& removed)
{
	// ツリーノードを開く（デフォルトで開いた状態）
	// ImGuiTreeNodeFlags_Framed などで見た目を整えるのもGood
	if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushID(label);	// 名前衝突防止

		// メンバ変数を１つずつ Visitor に渡して描画させる
		Reflection::VisitMembers(component, InspectorGuiVisitor{});

		ImGui::Spacing();
		if (ImGui::Button("Remove Component"))
		{
			removed = true;
		}

		ImGui::PopID();
	}
}

#endif // !___INSPECTOR_GUI_H___