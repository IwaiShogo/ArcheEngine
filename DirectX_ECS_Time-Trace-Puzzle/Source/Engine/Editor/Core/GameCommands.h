/*****************************************************************//**
 * @file	ヘッダー.h
 * @brief	
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
 * @note	（省略可）
 *********************************************************************/

#ifndef ___GAME_COMMANDS_H___
#define ___GAME_COMMANDS_H___

// ===== インクルード =====
#include "Game/Scenes/SceneManager.h"
#include "Engine/Core/Logger.h"
#include "Engine/ECS/ECS.h"
#include "Game/Utils/Prefab.h"

namespace GameCommands
{
	void RegisterAll(World& world, Context& ctx)
	{
		// --- コマンド登録 ---
		// fps [value]: FPS制限変更
		Logger::RegisterCommand("fps", [](auto args) {
			if (args.empty()) return;
			int fps = std::stoi(args[0]);
			Time::SetFrameRate(fps);
			Logger::Log("FPS limit set to " + std::to_string(fps));
			});

		// debug [grid/axis/collider] [0/1]: 表示切替
		// ※ m_appContext へのアクセスが必要なので、ラムダ式でキャプチャするか、
		//	 Applicationインスタンス経由でアクセスする必要があります。
		//	 ここでは簡易的に static なポインタを用意するか、m_sceneManager経由で取得します。
		Logger::RegisterCommand("debug", [&](auto args) {
			if (args.size() < 2) { Logger::LogWarning("Usage: debug [grid/axis/col] [0/1]"); return; }

			bool enable = (args[1] == "1" || args[1] == "on");

			if (args[0] == "grid") ctx.debug.showGrid = enable;
			else if (args[0] == "axis") ctx.debug.showAxis = enable;
			else if (args[0] == "col") ctx.debug.showColliders = enable;

			Logger::Log("Debug setting updated.");
			});

		// scene [title/game]: シーン遷移
		Logger::RegisterCommand("scene", [&](auto args) {
			if (args.empty()) return;
			if (args[0] == "title") SceneManager::ChangeScene(SceneType::Title);
			else if (args[0] == "game") SceneManager::ChangeScene(SceneType::Game);
			Logger::Log("Switching scene...");
			});

		// wireframe [on/off]: ワイヤーフレーム表示の切り替え
		Logger::RegisterCommand("wireframe", [&](auto args) {
			if (args.empty()) {
				// 引数なしならトグル
				ctx.debug.wireframeMode = !ctx.debug.wireframeMode;
			}
			else {
				ctx.debug.wireframeMode = (args[0] == "on" || args[0] == "1");
			}
			Logger::Log("Wireframe: " + std::string(ctx.debug.wireframeMode ? "ON" : "OFF"));
			});

		// quit: ゲーム終了
		Logger::RegisterCommand("quit", [](auto args) {
			PostQuitMessage(0);
			});

		// play [sound_key]: サウンド再生テスト
		Logger::RegisterCommand("play", [](auto args) {
			if (args.empty()) { Logger::LogWarning("Usage: play [sound_key]"); return; }
			AudioManager::Instance().PlaySE(args[0]);
			Logger::Log("Playing sound: " + args[0]);
			});

		// bgm [sound_key]: BGM変更
		Logger::RegisterCommand("bgm", [](auto args) {
			if (args.empty()) { Logger::LogWarning("Usage: bgm [sound_key]"); return; }
			AudioManager::Instance().PlayBGM(args[0]);
			Logger::Log("Changed BGM: " + args[0]);
			});

		// tp [x] [y] [z]: プレイヤーをテレポート
		Logger::RegisterCommand("tp", [&world](std::vector<std::string> args) {
			if (args.size() < 3) { Logger::LogWarning("Usage: tp [x] [y] [z]"); return; }

			float x = std::stof(args[0]);
			float y = std::stof(args[1]);
			float z = std::stof(args[2]);

			// Playerタグを持つEntityを探して移動
			bool found = false;
			world.getRegistry().view<Tag, Transform>([&](Entity e, Tag& tag, Transform& t) {
				if (tag.name == "Player") {
					t.position = { x, y, z };
					// 物理挙動をリセット（落下速度などを0にする）
					if (world.getRegistry().has<Rigidbody>(e)) {
						world.getRegistry().get<Rigidbody>(e).velocity = { 0, 0, 0 };
					}
					found = true;
				}
				});

			if (found) Logger::Log("Teleported Player to (" + args[0] + ", " + args[1] + ", " + args[2] + ")");
			else Logger::LogWarning("Player not found.");
			});

		// list: 現在の全エンティティを表示
		Logger::RegisterCommand("list", [&world](auto args) {
			Logger::Log("--- Entity List ---");
			world.getRegistry().view<Tag>([&](Entity e, Tag& tag) {
				std::string msg = "ID:" + std::to_string(e) + " [" + tag.name + "]";
				// 座標も表示してみる
				if (world.getRegistry().has<Transform>(e)) {
					auto& t = world.getRegistry().get<Transform>(e);
					msg += " Pos(" + std::to_string((int)t.position.x) + "," +
						std::to_string((int)t.position.y) + "," +
						std::to_string((int)t.position.z) + ")";
				}
				Logger::Log(msg);
				});
			Logger::Log("-------------------");
			});

		// kill [id] / kill all: エンティティを削除
		Logger::RegisterCommand("kill", [&world](auto args) {
			if (args.empty()) return;

			if (args[0] == "all") {
				// 全削除（危険ですがデバッグ用として）
				// ※ループ中の削除は危険なのでIDリストを作ってから消す
				std::vector<Entity> ids;
				world.getRegistry().view<Tag>([&](Entity e, Tag& t) { ids.push_back(e); });
				for (auto id : ids) world.getRegistry().destroy(id);
				Logger::Log("Killed all entities.");
			}
			else {
				// ID指定削除
				Entity id = (Entity)std::stoi(args[0]);
				if (world.getRegistry().has<Tag>(id)) {
					world.getRegistry().destroy(id);
					Logger::Log("Killed Entity ID: " + args[0]);
				}
				else {
					Logger::LogWarning("Entity not found.");
				}
			}
			});

		// spawn [enemy/cube]: デバッグ生成
		Logger::RegisterCommand("spawn", [&world](auto args) {
			if (args.empty()) return;

			XMFLOAT3 pos = { 0, 5, 0 }; // 頭上にスポーン
			// プレイヤーがいればその近くに
			world.getRegistry().view<Tag, Transform>([&](auto, Tag& t, Transform& tr) {
				if (t.name == "Player") pos = tr.position;
				});
			pos.y += 3.0f;

			if (args[0] == "enemy") {
				world.create_entity()
					.add<Tag>("Enemy")
					.add<Transform>(pos)
					.add<Collider>()
					.add<MeshComponent>("hero"); // 仮モデル
				Logger::Log("Spawned Enemy");
			}
			else if (args[0] == "sound") {
				Prefab::CreateSoundEffect(world, "jump", pos);
				Logger::Log("Spawned Sound");
			}
			});
	}
}

#endif // !___GAME_COMMANDS_H___