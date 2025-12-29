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
#include "Sandbox/Scenes/SceneGame.h"
#include "Engine/Core/Window/Input.h"
#include "Engine/Scene/Components/Components.h"

// エンジン側
#include "Engine/Scene/Systems/Physics/CollisionSystem.h"
#include "Engine/Scene/Systems/Physics/PhysicsSystem.h"

// ゲーム側（未だ移動出来ていない）
#include "Engine/Scene/Systems/Graphics/RenderSystem.h"
#include "Engine/Scene/Systems/Logic/InputSystem.h"
#include "Engine/Scene/Systems/Graphics/SpriteRenderSystem.h"
#include "Engine/Scene/Systems/Graphics/ModelRenderSystem.h"
#include "Engine/Scene/Systems/Audio/AudioSystem.h"
#include "Engine/Scene/Systems/Logic/LifetimeSystem.h"
#include "Engine/Scene/Systems/Logic/HierarchySystem.h"
#include "Engine/Scene/Systems/Graphics/BillboardSystem.h"
#include "Engine/Scene/Systems/Graphics/TextRenderSystem.h"
#include "Engine/Scene/Systems/Logic/UISystem.h"

#include "Engine/Scene/Core/SceneManager.h"
#include "Editor/Core/Editor.h"
#include "Editor/Core/GameCommands.h"

using namespace Arche;

void SceneGame::Initialize()
{
	// 親クラスの初期化
	//IScene::Initialize();
	m_world.getRegistry().clear();

	// ------------------------------------------------------------
	// システムの登録
	// ------------------------------------------------------------
	// 1. ロジック・物理系
	// 入力
	auto inputSys = m_world.registerSystem<InputSystem>(SystemGroup::Always);
	inputSys->SetContext(m_context);
	// 移動
	m_world.registerSystem<PhysicsSystem>();
	// 衝突判定
	m_world.registerSystem<CollisionSystem>();
	// UI
	m_world.registerSystem<UISystem>(SystemGroup::Always);
	// 寿命管理
	m_world.registerSystem<LifetimeSystem>();
	// 行列計算
	m_world.registerSystem<HierarchySystem>(SystemGroup::Always);

	// 2. 3D描画系（背景・モデル）
	// プリミティブ
	m_world.registerSystem<RenderSystem>(SystemGroup::Always);
	// 3Dモデル
	m_world.registerSystem<ModelRenderSystem>(SystemGroup::Always);

	// 3. 2D・透過描画系
	// ビルボード（3D空間内のアイコン等）
	m_world.registerSystem<BillboardSystem>(SystemGroup::Always);
	// 2D画像
	m_world.registerSystem<SpriteRenderSystem>(SystemGroup::Always);
	// 2Dテキスト
	m_world.registerSystem<TextRenderSystem>(SystemGroup::Always);

	// 4. オーディオ関連
	// オーディオ
	m_world.registerSystem<AudioSystem>(SystemGroup::Always);

	// 5. コマンドの登録
#ifdef _DEBUG
	if (m_context)
	{
		GameCommands::RegisterAll(m_world, *m_context);
	}
	Arche::Editor::Instance().Initialize();
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
		.add<AudioListener>()
		.add<Collider>(Collider::CreateSphere(0.5f, Layer::Default));

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

	m_world.create_entity()
		.add<Tag>("Floor")
		.add<Transform>(XMFLOAT3(1.0f, 0.0f, 0.0f))
		.add<Collider>(Collider::CreateBox(10.0f, 1.0f, 10.0f))
		.add<Rigidbody>(BodyType::Static);

	// UIルート（Canvas）
	Entity canvas = m_world.create_entity()
		.add<Tag>("Canvas")
		.add<Transform2D>(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT)
		.add<Canvas>(true, XMFLOAT2(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT)).id();

	m_world.create_entity()
		.add<Tag>("Text")
		.setParent(canvas)
		.add<Transform2D>(XMFLOAT2(0.0f, 0.0f), XMFLOAT2(1000.0f, 80.0f))
		.add<TextComponent>(
			(const char*)u8"Text (テキスト)",		// テキスト
			"Oradano-mincho-GSRR",		// フォントキー
			80.0f,						// フォントサイズ
			XMFLOAT4(1, 0.5f, 0.5f, 1)	// 色
		);

	m_world.create_entity()
		.add<Tag>("Text")
		.setParent(canvas)
		.add<Transform2D>(XMFLOAT2(0.0f, 0.0f), XMFLOAT2(100.0f, 100.0f))
		.add<SpriteComponent>("test");
}

void SceneGame::Finalize()
{
#ifdef _DEBUG
	Logger::ClearCommands();
#endif // _DEBUG
}

void SceneGame::Update()
{
	m_world.Tick(m_context->editorState);
}

void SceneGame::Render()
{
	m_world.Render(*m_context);
}
