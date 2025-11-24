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
#include "Core/Time.h"
#include "Core/Context.h"
#include "Core/Logger.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <cassert>
#include <chrono>

// ------------------------------------------------------------
// 1. 基本定義 & ComponentFamiliy
// ------------------------------------------------------------
using Entity = uint32_t;
const Entity NullEntity = 0;

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

// ------------------------------------------------------------
// 2. Pool & SparseSet
// ------------------------------------------------------------
class IPool
{
public:
	virtual ~IPool() = default;
	virtual void remove(Entity entity) = 0;
	virtual bool has(Entity entity) const = 0;
};

template<typename T>
class SparseSet
	: public IPool
{
	std::vector<Entity> sparse;	// Entity ID -> Dense Index
	std::vector<Entity> dense;	// Dense Index -> Entity ID
	std::vector<T> data;		// Component Data（Dense配列と同期）

public:
	// コンポーネントが存在するか
	bool has(Entity entity) const override
	{
		return	entity < sparse.size() &&
			sparse[entity] < dense.size() &&
			dense[sparse[entity]] == entity;
	}

	// コンポーネントの構築（Emplace）
	template<typename... Args>
	T& emplace(Entity entity, Args&&... args)
	{
		if (has(entity))
		{
			return data[sparse[entity]];
		}

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

// ------------------------------------------------------------
// 3. Registry
// ------------------------------------------------------------
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

	// コンポーネントを持っているか確認
	template<typename T>
	bool has(Entity entity)
	{
		return getPool<T>().has(entity);
	}

	// コンポーネント取得
	template<typename T>
	T& get(Entity entity)
	{
		return getPool<T>().get(entity);
	}

	// コンポーネント削除
	template<typename T>
	void remove(Entity entity)
	{
		getPool<T>().remove(entity);
	}

	// ============================================================
	// Multi-View Implementation (C++17)
	// ============================================================
	/**
	 * @brief	特定Componentを持つEntityのループ
	 * @details	
	 * 使い方：registry.view<Transform, Velocity>([](Entity e, Transform& t, Velocity& v) { ... });
	 * @warning
	 * 一番最初の型（TFirst）を基準にループします。
	 * Entity数が「最も少ない」コンポーネントを最初に指定すると高速です。
	 */
	template<typename TFirst, typename... TOthers, typename Func>
	void view(Func func)
	{
		auto& poolFirst = getPool<TFirst>();	// ループ駆動用プール

		// 他のコンポーネントのプールへの参照をタプルで取得
		auto poolTuple = std::make_tuple(&getPool<TOthers>()...);

		auto& entities = poolFirst.getEntities();
		auto& dataFirst = poolFirst.getData();

		// 基準プールの全Entityをループ
		for (size_t i = 0; i < entities.size(); ++i)
		{
			Entity entity = entities[i];
			
			// 他の全てのプールがこのEntityを持っているかチェック
			// 1つでも持っていなければ false になり、ifに入らない
			if ((std::get<SparseSet<TOthers>*>(poolTuple)->has(entity) && ...))
			{
				// 全て持っているので関数実行
				func(
					entity,
					dataFirst[i],
					std::get<SparseSet<TOthers>*>(poolTuple)->get(entity)...
				);
			}
		}
	}
};

// ------------------------------------------------------------
// 4. EntityHandle（チェーンメソッド用）
// ------------------------------------------------------------
/**
 * @class	EntityHandle
 * @brief	一気にComponentを追加するためのヘルパー
 */
class EntityHandle
{
	Registry* registry;
	Entity entity;

public:
	EntityHandle(Registry* r, Entity e)
		: registry(r), entity(e) {}

	// .add<Transform>(...) のように繋げて書ける
	template<typename T, typename... Args>
	EntityHandle& add(Args&&... args)
	{
		registry->emplace<T>(entity, std::forward<Args>(args)...);
		return *this;
	}

	// IDを取得して終了
	Entity id() const { return entity; }
};

// ------------------------------------------------------------
// 5. System Interface & World
// ------------------------------------------------------------
class ISystem
{
public:
	virtual ~ISystem() = default;
	virtual void Update(Registry& registry) {}
	virtual void Render(Registry& registry, const Context& context) {}

	// システム名（デバッグ用）
	std::string m_systemName = "System";
	// 処理時間（デバッグ, ms）
	double m_lastExecutionTime = 0.0;
};

class World
{
	Registry registry;
	std::vector<std::unique_ptr<ISystem>> systems;

public:
	// Entity作成を開始する（ビルダーを返す）
	EntityHandle create_entity()
	{
		return EntityHandle(&registry, registry.create());
	}

	// システムの登録
	template<typename T, typename... Args>
	T* registerSystem(Args&&... args)
	{
		auto sys = std::make_unique<T>(std::forward<Args>(args)...);
		auto ptr = sys.get();
		systems.push_back(std::move(sys));
		return ptr;
	}

	// 全システムのUpdateを実行
	void Tick()
	{
		for (auto& sys : systems)
		{
			// 計測開始
			auto start = std::chrono::high_resolution_clock::now();

			sys->Update(registry);

			// 計測終了
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> ms = end - start;
			sys->m_lastExecutionTime = ms.count();
		}
	}

	// 全システムのRenderを実行
	void Render(const Context& context)
	{
		for (auto& sys : systems)
		{
			sys->Render(registry, context);
		}
	}

	// デバッグ用にシステムリストを取得
	const std::vector<std::unique_ptr<ISystem>>& getSystems() const { return systems; }

	// Registryへの直接アクセスが必要な場合
	Registry& getRegistry() { return registry; }
};

#endif // !___ECS_H___