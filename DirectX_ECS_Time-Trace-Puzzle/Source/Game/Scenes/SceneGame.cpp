/*****************************************************************//**
 * @file	SceneGame.cpp
 * @brief	ゲームシーン
 * 
 * @details	
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

// ===== インクルード =====
#include "Engine/pch.h"
#include "Game/Scenes/SceneGame.h"
#include "Engine/Core/Input.h"
#include "Engine/Components/Components.h"

// エンジン側
#include "Engine/Physics/CollisionSystem.h"
#include "Engine/Physics/PhysicsSystem.h"

// ゲーム側（未だ移動出来ていない）
#include "Game/Systems/Graphics/RenderSystem.h"
#include "Game/Systems/Logic/InputSystem.h"
#include "Game/Systems/Graphics/SpriteRenderSystem.h"
#include "Game/Systems/Graphics/ModelRenderSystem.h"
#include "Game/Systems/Audio/AudioSystem.h"
#include "Game/Systems/Logic/LifetimeSystem.h"
#include "Game/Systems/Logic/HierarchySystem.h"
#include "Game/Systems/Graphics/BillboardSystem.h"

#include "Engine/Scene/SceneManager.h"
#include "Engine/Editor/Core/Editor.h"
#include "Engine/Editor/Core/GameCommands.h"

void SceneGame::Initialize()
{
	// 親クラスの初期化
	//IScene::Initialize();
	m_world.getRegistry().clear();

	// ------------------------------------------------------------
	// システムの登録
	// ------------------------------------------------------------
	// 1. 入力
	auto inputSys = m_world.registerSystem<InputSystem>();
	inputSys->SetContext(m_context);
	// 2. 移動
	m_world.registerSystem<PhysicsSystem>();
	// 3. 寿命管理
	m_world.registerSystem<LifetimeSystem>();
	// 4. 行列計算
	m_world.registerSystem<HierarchySystem>();
	// 5. 衝突判定
	m_world.registerSystem<CollisionSystem>();
	// 6. 描画
	if (m_context->spriteRenderer)
	{
		m_world.registerSystem<SpriteRenderSystem>(m_context->spriteRenderer);
	}
	if (m_context->billboardRenderer)
	{
		m_world.registerSystem<BillboardSystem>(m_context->billboardRenderer);
	}
	if (m_context->modelRenderer)
	{
		m_world.registerSystem<ModelRenderSystem>(m_context->modelRenderer);
	}
	if (m_context->renderer)
	{
		m_world.registerSystem<RenderSystem>(m_context->renderer);
	}
	// 7. オーディオ
	m_world.registerSystem<AudioSystem>();
#ifdef _DEBUG
	if (m_context)
	{
		GameCommands::RegisterAll(m_world, *m_context);
	}
	Editor::Instance().Initialize();
#endif // _DEBUG

	// ------------------------------------------------------------
	// レイヤーの設定
	// ------------------------------------------------------------
	PhysicsConfig::Reset();

	// "Player" レイヤー設定
	PhysicsConfig::Configure(Layer::Player)
		.collidesWith(Layer::Enemy | Layer::Wall | Layer::Item);

	// "Enemy" レイヤー設定
	PhysicsConfig::Configure(Layer::Enemy)
		.collidesWith(Layer::Player | Layer::Wall);


	// ------------------------------------------------------------
	// Entityの生成
	// ------------------------------------------------------------
	// Camera
	m_world.create_entity()
		.add<Tag>("MainCamera")
		.add<Transform>(XMFLOAT3(2.0f, 10.0f, -10.0f), XMFLOAT3(0.78f, 0.0f, 0.0f))
		.add<Camera>()
		.add<AudioListener>();

	// Player
	m_world.create_entity()
		.add<Tag>("Player")
		.add<Transform>(XMFLOAT3(0.0f, 0.0f, 0.0f))
		.add<Rigidbody>(BodyType::Dynamic)
		.add<Collider>(Collider::CreateCapsule(0.5f, 2.0f, Layer::Player))
		.add<PlayerInput>()
		.add<MeshComponent>("hero", XMFLOAT3(0.1f, 0.1f, 0.1f));

	// Enemy
	m_world.create_entity()
		.add<Tag>("Enemy")
		.add<Transform>(XMFLOAT3(5.0f, 0.0f, 0.0f))
		.add<Collider>()
		.add<Rigidbody>();

	// UI
	m_world.create_entity()
		.add<Tag>("UI")
		.add<Transform>(XMFLOAT3(50.0f, 50.0f, 0.0f))
		.add<SpriteComponent>("test", 64.0f, 64.0f);

	m_world.create_entity()
		.add<Tag>("Floor")
		.add<Transform>(XMFLOAT3(1.0f, 0.0f, 0.0f))
		.add<Collider>(Collider::CreateBox(10.0f, 1.0f, 10.0f))
		.add<Rigidbody>(BodyType::Static);

	m_world.create_entity()
		.add<Tag>("Text")
		.add<Transform>(XMFLOAT3(10.0f, 10.0f, 0.0f))
		.add<TextComponent>("Draw Text");
}

void SceneGame::Finalize()
{
#ifdef _DEBUG
	Logger::ClearCommands();
#endif // _DEBUG
}

void SceneGame::Update()
{
	m_world.Tick();
}

void SceneGame::Render()
{
	m_world.Render(*m_context);
}