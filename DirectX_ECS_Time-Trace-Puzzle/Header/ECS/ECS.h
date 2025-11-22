/*****************************************************************//**
 * @file	ECS.h
 * @brief	ECSの中核となるライブラリ
 *
 * @details
 * 設計コンセプト：SparseSet (スパースセット)
 * ・Sparse配列(疎): Entity IDをインデックスとし、実際のデータがある配列（Dense）
 * 　へのインデックスを保持します。サイズはEntityの最大数です。
 * ・Dense配列(密)：コンポーネントデータそのものが隙間なく詰まっています。
 * 　これにより、インテレーション（ループ処理）時のキャッシュヒット率が劇的に向上し、
 * 　物理演算やレンダリングの高速化に寄与します。
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ECS_H___
#define ___ECS_H___

 // ===== インクルード =====
#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <cassert>

// 1. Entity定義
using Entity = uint32_t;
const Entity NullEntity = 0;

// 2. Type ID Generator
class ComponentFamily
{
	static size_t identifier()
	{
		static size_t value = 0;
		return value++;
	}

public:
	template<typename T>
	static size_t type()
	{
		static const size_t value = identifier();
		return value;
	}
};

// 3. Base Pool Interface（型消去用）
class IPool
{
public:
	virtual ~IPool() = default;
	virtual void remove(Entity entity) = 0;
};

// 4. Sparse Set実装
template<typename T>
class SparseSet
	: public IPool
{
	std::vector<Entity> sparse;	// Entity ID -> Dense Index
	std::vector<Entity> dense;	// Dense Index -> Entity ID
	std::vector<T> data;		// Component Data（Dense配列と同期）

public:
	// コンポーネントが存在するか
	bool has(Entity entity) const
	{
		return	entity < sparse.size() &&
			sparse[entity] < dense.size() &&
			dense[sparse[entity]] == entity;
	}

	// コンポーネントの構築（Emplace）
	template<typename... Args>
	T& emplace(Entity entity, Args&&... args)
	{
		assert(!has(entity));

		if (sparse.size() <= entity)
		{
			sparse.resize(entity + 1);
		}

		sparse[entity] = (Entity)dense.size();
		dense.push_back(entity);
		data.emplace_back(std::forward<Args>(args)...);

		return data.back();
	}

	// コンポーネントの取得
	T& get(Entity entity)
	{
		assert(has(entity));
		return data[sparse[entity]];
	}

	// 削除
	void remove(Entity entity) override
	{
		if (!has(entity)) return;

		Entity lastEntity = dense.back();
		Entity indexToRemove = sparse[entity];

		// データとEntityIDを末尾のものとスワップ
		std::swap(dense[indexToRemove], dense.back());
		std::swap(data[indexToRemove], data.back());

		// Sparse配列のリンクを更新
		sparse[lastEntity] = indexToRemove;

		// 削除
		dense.pop_back();
		data.pop_back();
	}

	// データへの直接アクセス（Systemでのループ用）
	std::vector<T>& getData() { return data; }
	const std::vector<Entity>& getEntities() const { return dense; }
};

// 5. Registry（ECSの管理者）
class Registry
{
	Entity nextEntity = 1;
	std::vector<std::unique_ptr<IPool>> pools;

	// 型Tに対応するプールを取得（無ければ作成）
	template<typename T>
	SparseSet<T>& getPool()
	{
		size_t componentId = ComponentFamily::type<T>();
		if (componentId >= pools.size())
		{
			pools.resize(componentId + 1);
		}
		if (!pools[componentId])
		{
			pools[componentId] = std::make_unique<SparseSet<T>>();
		}
		return *static_cast<SparseSet<T>*>(pools[componentId].get());
	}

public:
	// Entity作成
	Entity create()
	{
		return nextEntity++;
	}

	// コンポーネント追加
	template<typename T, typename... Args>
	T& emplace(Entity entity, Args&&... args)
	{
		return getPool<T>().emplace(entity, std::forward<Args>(args)...);
	}

	// コンポーネント取得
	template<typename T>
	T& get(Entity entity)
	{
		return getPool<T>().get(entity);
	}

	// View的な機能: 特定componentを持つEntityのループ
	template<typename T, typename Func>
	void view(Func func)
	{
		auto& pool = getPool<T>();
		auto& data = pool.getData();
		auto& entities = pool.getEntities();

		for (size_t i = 0; i < data.size(); ++i)
		{
			func(entities[i], data[i]);
		}
	}
};

#endif // !___ECS_H___