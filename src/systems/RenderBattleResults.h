// Duplicate removed

#pragma once

#include "../components/battle_result.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include <afterhours/ah.h>

struct RenderBattleResults : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Results;
  }

  void once(float) const override {
    auto resultEntity = afterhours::EntityHelper::get_singleton<BattleResult>();
    auto &entity = resultEntity.get();

    if (!entity.has<BattleResult>()) {
      return;
    }

    auto &result = entity.get<BattleResult>();

    float screenWidth = raylib::GetScreenWidth();
    float screenHeight = raylib::GetScreenHeight();

    raylib::DrawRectangle(0, 0, (int)screenWidth, (int)screenHeight,
                          raylib::Color{20, 20, 20, 255});

    std::string title = "BATTLE RESULTS";
    float titleWidth = raylib::MeasureText(title.c_str(), 48);
    float titleX = (screenWidth - titleWidth) / 2.0f;
    float titleY = 100.0f;

    raylib::DrawText(title.c_str(), (int)titleX, (int)titleY, 48,
                     raylib::YELLOW);

    std::string outcomeText;
    raylib::Color outcomeColor;

    switch (result.outcome) {
    case BattleResult::Outcome::PlayerWin:
      outcomeText = "PLAYER WINS!";
      outcomeColor = raylib::GREEN;
      break;
    case BattleResult::Outcome::OpponentWin:
      outcomeText = "OPPONENT WINS!";
      outcomeColor = raylib::RED;
      break;
    case BattleResult::Outcome::Tie:
      outcomeText = "IT'S A TIE!";
      outcomeColor = raylib::YELLOW;
      break;
    }

    float outcomeWidth = raylib::MeasureText(outcomeText.c_str(), 36);
    float outcomeX = (screenWidth - outcomeWidth) / 2.0f;
    float outcomeY = titleY + 80.0f;

    raylib::DrawText(outcomeText.c_str(), (int)outcomeX, (int)outcomeY, 36,
                     outcomeColor);

    float judgeY = outcomeY + 100.0f;
    float judgeStartX = (screenWidth - 400.0f) / 2.0f;
    float judgeSpacing = 150.0f;

    for (size_t i = 0; i < result.judgeScores.size(); ++i) {
      const auto &judgeScore = result.judgeScores[i];

      float judgeX = judgeStartX + i * judgeSpacing;

      raylib::DrawText(judgeScore.judgeName.c_str(), (int)judgeX, (int)judgeY,
                       18, raylib::WHITE);

      std::string playerScoreText =
          "P: " + std::to_string(judgeScore.playerScore);
      raylib::DrawText(playerScoreText.c_str(), (int)judgeX, (int)(judgeY + 30),
                       16, raylib::GREEN);

      std::string opponentScoreText =
          "O: " + std::to_string(judgeScore.opponentScore);
      raylib::DrawText(opponentScoreText.c_str(), (int)judgeX,
                       (int)(judgeY + 55), 16, raylib::RED);
    }

    float totalY = judgeY + 120.0f;

    std::string totalPlayerText =
        "Player Total: " + std::to_string(result.totalPlayerScore);
    std::string totalOpponentText =
        "Opponent Total: " + std::to_string(result.totalOpponentScore);

    float totalPlayerWidth = raylib::MeasureText(totalPlayerText.c_str(), 24);
    float totalOpponentWidth =
        raylib::MeasureText(totalOpponentText.c_str(), 24);

    float totalPlayerX =
        (screenWidth - totalPlayerWidth - totalOpponentWidth - 50) / 2.0f;
    float totalOpponentX = totalPlayerX + totalPlayerWidth + 50;

    raylib::DrawText(totalPlayerText.c_str(), (int)totalPlayerX, (int)totalY,
                     24, raylib::GREEN);
    raylib::DrawText(totalOpponentText.c_str(), (int)totalOpponentX,
                     (int)totalY, 24, raylib::RED);
  }
};
