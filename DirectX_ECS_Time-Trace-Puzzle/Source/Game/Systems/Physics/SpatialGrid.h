/*****************************************************************//**
 * @file	SpatialGrid.h
 * @brief	空間をセルに分割して管理するクラス
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/12/01	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___SPATIAL_GRID_H___
#define ___SPATIAL_GRID_H___

// ===== インクルード =====
#include "Engine/ECS/ECS.h"
#include <unordered_map>
#include <vector>
#include <DirectXMath.h>
#include <cmath>

namespace Physics
{
	// ハッシュキー生成用
	struct GridKey {
		int x, y, z;
		bool operator==(const GridKey& other) const {
			return x == other.x && y == other.y && z == other.z;
		}
	};

	// ハッシュ関数
	struct GridKeyHash {
		std::size_t operator()(const GridKey& k) const {
			// シンプルなハッシュ合成 (X,Y,Zをビットシフトして混ぜる)
			return ((std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1)) >> 1) ^ (std::hash<int>()(k.z) << 1);
		}
	};

	class SpatialGrid {
	public:
		SpatialGrid(float cellSize = 2.0f) : m_cellSize(cellSize) {}

		void Clear() {
			m_grid.clear();
		}

		// オブジェクトをグリッドに登録tttt
		void Insert(Entity entity, const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max) {
			int minX = (int)std::floor(min.x / m_cellSize);
			int maxX = (int)std::floor(max.x / m_cellSize);
			int minY = (int)std::floor(min.y / m_cellSize);
			int maxY = (int)std::floor(max.y / m_cellSize);
			int minZ = (int)std::floor(min.z / m_cellSize);
			int maxZ = (int)std::floor(max.z / m_cellSize);

			for (int x = minX; x <= maxX; ++x) {
				for (int y = minY; y <= maxY; ++y) {
					for (int z = minZ; z <= maxZ; ++z) {
						m_grid[{x, y, z}].push_back(entity);
					}
				}
			}
		}

		// 候補リストの取得
		// ※重複を除くために、Entityペアの管理が必要になりますが、
		//	 簡易実装として「自分のセルにいる相手」を全て返すイテレータを提供します。
		const std::vector<Entity>& GetCell(const DirectX::XMFLOAT3& position) {
			int x = (int)std::floor(position.x / m_cellSize);
			int y = (int)std::floor(position.y / m_cellSize);
			int z = (int)std::floor(position.z / m_cellSize);
			return m_grid[{x, y, z}];
		}

		// マップ全体へのアクセス
		const std::unordered_map<GridKey, std::vector<Entity>, GridKeyHash>& GetMap() const {
			return m_grid;
		}

	private:
		float m_cellSize;
		std::unordered_map<GridKey, std::vector<Entity>, GridKeyHash> m_grid;
	};
}

#endif // !___SPATIAL_GRID_H___