/*****************************************************************//**
 * @file	RenderTarget.h
 * @brief	シーンビュー、ゲームビューの実体
 * 
 * @details	
 * 「画面に表示せずに、メモリ上の画像に描画する」ためのクラス作成。 
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/29	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___RENDER_TARGET_H___
#define ___RENDER_TARGET_H___

// ===== インクルード =====
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class RenderTarget
{
public:
	RenderTarget(ID3D11Device* device, int width, int height);
	~RenderTarget() = default;

	// 描画先としてセットする
	void Activate(ID3D11DeviceContext* context, ID3D11DepthStencilView* depthStencil);

	// 描画結果をクリアする
	void Clear(ID3D11DeviceContext* context, float r, float g, float b, float a);

	// ImGuiで表示するためのテクスチャIDを取得
	void* GetID() const { return m_srv.Get(); }

	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }

	// リサイズ対応（ウィンドウサイズが変わった時用）
	void Resize(ID3D11Device* device, int width, int height);

	ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }

private:
	int m_width, m_height;
	ComPtr<ID3D11Texture2D> m_texture;
	ComPtr<ID3D11RenderTargetView> m_rtv;
	ComPtr<ID3D11ShaderResourceView> m_srv;	// ImGui表示用
};

#endif // !___RENDER_TARGET_H___