/*****************************************************************//**
 * @file	ECS.h
 * @brief	ECSの中核となるライブラリ
 *
 * @details
 * 機能：
 * - SparseSet: データの密な管理
 * - Signal: イベント通知（追加/削除/更新）
 * - Observer: 変更検知（リアクティブシステム用）
 * - Dispatcher: グローバルイベントバス
 * - View Exclude: 除外フィルタリング
 * - Patch: 更新通知の手動発火
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
#include "Engine/Core/Time.h"
#include "Engine/Core/Context.h"
#include "Engine/Core/Logger.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <cassert>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <set>
#include <chrono>

// ------------------------------------------------------------
// 基本定義
// ------------------------------------------------------------
using Entity = uint32_t;
const Entity NullEntity = 0xFFFFFFFF;

class ComponentFamily
{
	static std::size_t identifier()
	{
		static std::size_t value = 0;
		return value++;
	}

public:
	template<typename T>
	static std::size_t type()
	{
		static const std::size_t value = identifier();
		return value;
	}
};

// ------------------------------------------------------------
// Signal（イベント通知）
// ------------------------------------------------------------
template<typename... Args>
class Signal
{
public:
	using Callback = std::function<void(Args...)>;

	void connect(Callback cb)
	{
		callbacks.push_back(cb);
	}

	void publish(Args... args)
	{
		for (auto& cb : callbacks)
		{
			cb(args...);
		}
	}

	void clear()
	{
		callbacks.clear();
	}

private:
	std::vector<Callback> callbacks;
};

// ------------------------------------------------------------
// Pool（インターフェース / 基底クラス）
// ------------------------------------------------------------
class IPool
{
public:
	virtual ~IPool() = default;
	virtual void remove(Entity entity) = 0;
	virtual bool has(Entity entity) const = 0;
	virtual std::size_t size() const = 0;	// 最適化用

	// Observer接続用インターフェース
	Signal<Entity> onConstruct;	// 追加時
	Signal<Entity> onDestroy;	// 削除時
	Signal<Entity> onUpdate;	// 更新時
};

// ------------------------------------------------------------
// SparseSet（コンポーネントデータ管理 / Signal対応）
// ------------------------------------------------------------
template<typename T>
class SparseSet
	: public IPool
{
public:
	// コンポーネントが存在するか
	bool has(Entity entity) const override
	{
		return	entity < sparse.size() &&
				sparse[entity] < dense.size() &&
				dense[sparse[entity]] == entity;
	}

	std::size_t size() const override
	{
		return dense.size();
	}

	// コンポーネントの構築（Emplace）
	template<typename... Args>
	T& emplace(Entity entity, Args&&... args)
	{
		if (has(entity))
		{
			// 既に存在する場合は上書き＆更新通知
			T& ref = data[sparse[entity]];
			ref = T(std::forward<Args>(args)...);
			onUpdate.publish(entity);
			return data[sparse[entity]];
		}

		if (sparse.size() <= entity)
		{
			sparse.resize(entity + 1);
		}

		sparse[entity] = (Entity)dense.size();
		dense.push_back(entity);
		data.emplace_back(std::forward<Args>(args)...);

		// 追加通知
		onConstruct.publish(entity);
		return data.back();
	}

	// コンポーネントの取得
	T& get(Entity entity)
	{
		assert(has(entity));
		return data[sparse[entity]];
	}

	// 値を書き換えた後に呼び出す（Observerへの通知用）
	void patch(Entity entity)
	{
		if (has(entity))
		{
			onUpdate.publish(entity);
		}
	}

	// 削除
	void remove(Entity entity) override
	{
		if (!has(entity)) return;

		// 削除通知
		onDestroy.publish(entity);

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

private:
	std::vector<Entity> sparse;	// Entity ID -> Dense Index
	std::vector<Entity> dense;	// Dense Index -> Entity ID
	std::vector<T> data;		// Component Data（Dense配列と同期）
};

// ------------------------------------------------------------
// 3. Registry
// ------------------------------------------------------------
class Registry
{
	Entity nextEntity = 1;
	// 再利用可能なIDのリスト
	std::vector<Entity> freeIds;
	std::vector<std::unique_ptr<IPool>> pools;

public:
	// -----------------------------------------------------------
	// 自動検知用ラッパー
	// -----------------------------------------------------------
	template<typename T>
	struct ScopedComponent
	{
		Registry* registry;
		Entity entity;
		T* component;

		ScopedComponent(Registry* r, Entity e, T* c) : registry(r), entity(e), component(c) {}

		~ScopedComponent()
		{
			if(registry && component)
			{
				registry->patch<T>(entity);
			}
		}

		T* operator->() { return component; }
		T& operator*() { return *component; }
	};

	// -----------------------------------------------------------
	// 基本機能
	// -----------------------------------------------------------
	// 型Tに対応するプールを取得（無ければ作成）
	template<typename T>
	SparseSet<T>& getPool()
	{
		std::size_t componentId = ComponentFamily::type<T>();
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

	// プールが存在するか確認（安全なアクセスの為）
	template<typename T>
	bool hasPool() const
	{
		std::size_t componentId = ComponentFamily::type<T>();
		return componentId < pools.size() && pools[componentId] != nullptr;
	}

	// 汎用プール取得
	IPool* getPoolBase(std::size_t typeId)
	{
		if (typeId < pools.size())
		{
			return pools[typeId].get();
		}
		return nullptr;
	}

	// Entity作成
	Entity create()
	{
		// 再利用できるIDがあればそれを使う
		if (!freeIds.empty())
		{
			Entity id = freeIds.back();
			freeIds.pop_back();
			return id;
		}
		// 無ければ新規発行
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

	// 読み取り用
	template<typename T>
	const T& get(Entity entity) const
	{
		return const_cast<Registry*>(this)->getPool<T>().get(entity);
	}

	// 書き込み用
	template<typename T>
	T& get(Entity entity)
	{
		return getPool<T>().get(entity);
	}

	// 変更検知付き書き込み用
	template<typename T>
	ScopedComponent<T> modify(Entity entity)
	{
		return ScopedComponent<T>(this, entity, &getPool<T>().get(entity));
	}

	// 変更通知を手動で送る
	template<typename T>
	void patch(Entity entity)
	{
		getPool<T>().patch(entity);
	}

	// コンポーネント削除
	template<typename T>
	void remove(Entity entity)
	{
		getPool<T>().remove(entity);
	}

	void destroy(Entity entity)
	{
		for (auto& pool : pools)
		{
			if (pool)
			{
				pool->remove(entity);
			}
		}

		freeIds.push_back(entity);
	}

	void clear()
	{
		for (auto& pool : pools)
		{
			if (pool) pool.reset();
		}
		pools.clear();
		freeIds.clear();
		nextEntity = 1;
	}

	// ============================================================
	// Multi-View Class (Chainable)
	// ============================================================ 
	template<typename... Components>
	class View
	{
		Registry* registry;
		// 必要なプールへのポインタ（ダブルで保持）
		std::tuple<SparseSet<Components>*...> pools;
		// 除外するコンポーネントIDのリスト
		std::vector<std::size_t> excludeTypes;
		// ループ駆動に使うプールのインデックス（最小サイズのプール）
		std::size_t bestIndex = 0;

	public:
		View(Registry* r)
			: registry(r)
		{
			// 全てのプールを取得
			pools = std::make_tuple(&registry->getPool<Components>()...);

			// 最も要素数が少ないプールを探して駆動用にする（最適化）
			std::size_t minsize = SIZE_MAX;
			findSmallestPool(std::index_sequence_for<Components...>{}, minsize);
		}

		// 除外設定（.exclude<Static>()）
		template<typename TExclude>
		View& exclude()
		{
			excludeTypes.push_back(ComponentFamily::type<TExclude>());
			return *this;
		}

		// ヘルパー：最小プールを探す
		template<std::size_t... Is>
		void findSmallestPool(std::index_sequence<Is...>, std::size_t& minsize)
		{
			// fold expressionで各プールのサイズをチェック
			((checkSize<Is>(minsize)), ...);
		}

		template<std::size_t I>
		void checkSize(std::size_t& minsize)
		{
			auto* pool = std::get<I>(pools);
			if (pool->size() < minsize)
			{
				minsize = pool->size();
				bestIndex = I;
			}
		}

		// -----------------------------------------------------------
		// イテレータ（範囲for文用）
		// -----------------------------------------------------------
		struct Iterator
		{
			View* view;
			// 駆動用プールのイテレータ
			typename std::vector<Entity>::const_iterator current;
			typename std::vector<Entity>::const_iterator end;

			Iterator(View* v, typename std::vector<Entity>::const_iterator c, typename std::vector<Entity>::const_iterator e)
				: view(v), current(c), end(e)
			{
				// 最初が有効化チェック、無効なら進める
				if(current != end && !isValid(*current))
				{
					++(*this);
				}
			}

			Entity operator*() const
			{
				return *current;
			}

			Iterator& operator++()
			{
				do
				{
					++current;
				} while (current != end && !isValid(*current));
				return *this;
			}

			bool operator!=(const Iterator& other) const
			{
				return current != other.current;
			}
		};

		Iterator begin()
		{
			return createIterator(bestIndex);
		}

		Iterator end()
		{
			return createEndIterator(bestIndex);
		}

		// ヘルパー：インデックスからイテレータ生成
		Iterator createIterator(std::size_t index)
		{
			const std::vector<Entity>* entities = nullptr;
			std::size_t i = 0;
			std::apply([&](auto*... p) {
				((i++ == index ? entities = &p->getEntities() : nullptr), ...);
				}, pools);

			return Iterator(this, entities->begin(), entities->end());
		}

		Iterator createEndIterator(std::size_t index)
		{
			const std::vector<Entity>* entities = nullptr;
			std::size_t i = 0;
			std::apply([&](auto*... p) {
				((i++ == index ? entities = &p->getEntities() : nullptr), ...);
				}, pools);
			return Iterator(this, entities->end(), entities->end());
		}

		// エンティティが条件を満たすかチェック
		bool isValid(Entity entity)
		{
			// 1. Excludeチェック
			for (auto id : excludeTypes)
			{
				if(id < registry->pools.size() && registry->pools[id] && registry->pools[id]->has(entity))
				{
					return false;
				}
			}

			// 2. Othersチェック
			bool allHas = std::apply([&](auto*... p) {
				return (p->has(entity) && ...);
				}, pools);

			return allHas;
		}

		// -----------------------------------------------------------
		// each関数（ラムダ実行用）
		// -----------------------------------------------------------
		template<typename Func>
		void each(Func func)
		{
			// 最適化されたループ
			const std::vector<Entity>* entities = nullptr;
			std::size_t i = 0;
			std::apply([&](auto... p) {
				((i++ == bestIndex ? entities = &p->getEntities() : nullptr), ...);
				}, pools);

			for(const auto& entity : *entities)
			{
				if (isValid(entity))
				{
					// 全て持っているので関数実行
					std::apply([&](auto... p) {
						func(
							entity,
							p->get(entity)...
						);
						}, pools);

					// 自動通知
					(auto_patch<Components>(entity), ...);
				}
			}
		}

		// 特定コンポーネント取得ヘルパー
		template<typename T>
		T& get(Entity entity)
		{
			return std::get<SparseSet<T>*>(pools)->get(entity);
		}

	private:
		// 自動パッチ通知（each内で使用）
		template<typename T>
		void auto_patch(Entity e)
		{
			// const修飾されていない型のみ通知
			if constexpr (!std::is_const_v<T>)
			{
				registry->getPool<T>().patch(e);
			}
		}
	};

	// ビューの作成
	template<typename... Components>
	View<Components...> view()
	{
		return View<Components...>(this);
	}
};

// ------------------------------------------------------------
// Observer（変更検知）
// ------------------------------------------------------------
/**
 * @class	Observer
 * @brief	特定のコンポーネントの変更を監視し、エンティティをリスト化する
 * @usage
 * Observer observer;
 * observer.connect<Transform>(registry);
 * for (auto e : observer) { ... }
 * observer.clear();
 */
class Observer
{
public:
	Observer() = default;

	// チェーン開始
	Observer& connect(Registry& r)
	{
		registry = &r;
		clear();
		return *this;
	}

	// 更新検知
	template<typename T>
	Observer& update()
	{
		assert(registry);
		registry->getPool<T>().onUpdate.connect(
			[this](Entity e) { this->on_trigger(e); }
		);
		return *this;
	}

	// 生成/追加検知
	template<typename T>
	Observer& group()
	{
		assert(registry);
		registry->getPool<T>().onConstruct.connect(
			[this](Entity e) { this->on_trigger(e); }
		);
		return *this;
	}

	// 条件フィルタ（.where）
	template<typename... Us>
	Observer& where()
	{
		filters.push_back(
			[](Registry& r, Entity e) {
				return (r.has<Us>(e) && ...);
			}
		);
		return *this;
	}

	// イテレータ
	auto begin() { return dense.begin(); }
	auto end() { return dense.end(); }

	// ラムダ式でループ処理（.each）
	// 引数: [](Entity e, Component&... c)
	template<typename Func>
	void each(Func func)
	{
		for (auto e : dense) func(e);
	}

	// リセット
	void clear()
	{
		for(auto e : dense)
		{
			if(e < sparse.size())
			{
				sparse[e] = NullEntity;
			}
		}
		dense.clear();
	}

	std::size_t size() const { return dense.size(); }

private:
	// トリガー時共通処理
	void on_trigger(Entity e)
	{
		if (!registry) return;

		// フィルタチェック（.where）
		for (auto& f : filters)
		{
			if(!f(*registry, e)) return;
		}

		// 重複登録防止
		if (sparse.size() <= e) sparse.resize(e + 1, NullEntity);
		if (sparse[e] != NullEntity) return;

		sparse[e] = static_cast<Entity>(dense.size());
		dense.push_back(e);
	}

	Registry* registry = nullptr;
	std::vector<Entity> dense;
	std::vector<Entity> sparse;
	std::vector<std::function<bool(Registry&, Entity)>> filters;
};

// ------------------------------------------------------------
// Dispatcher（グローバルイベントバス）
// ------------------------------------------------------------
class Dispatcher
{
public:
	template<typename EventType>
	static Signal<const EventType&>& sink()
	{
		static Signal<const EventType&> signal;
		return signal;
	}

	template<typename EventType>
	static void trigger(const EventType& e)
	{
		sink<EventType>().publish(e);
	}
};


// ------------------------------------------------------------
// EntityHandle（チェーンメソッド用）
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

	template<typename T>
	void remove()
	{
		registry->remove<T>(entity);
	}

	template<typename T>
	T& get()
	{
		return registry->get<T>(entity);
	}

	template<typename T>
	bool has()
	{
		return registry->has<T>(entity);
	}

	// コンポーネントを更新したことを通知
	template<typename T>
	void patch()
	{
		registry->patch<T>(entity);
	}

	// IDを取得して終了
	Entity id() const { return entity; }
};

// ------------------------------------------------------------
// System Interface & World
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
	const Registry& getRegistry() const { return registry; }
};

#endif // !___ECS_H___