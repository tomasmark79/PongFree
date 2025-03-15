// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <GameEngine/GameEngine.hpp>
#include <GameEngine/version.h>
#include <Logger/Logger.hpp>
#include <Utils/Utils.hpp>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#if defined(PLATFORM_WEB)
  #include <emscripten/emscripten.h>
#endif

namespace library {

GameEngine::GameEngine(const std::string &assetsPath)
    : m_assetsPath(assetsPath) {
  LOG_IMPORTANT("GameEngine v." + std::string(GAMEENGINE_VERSION) +
                " constructed.");
  LOG_DEBUG("Assets Path: " + assetsPath);

  // Initialization (Note windowTitle is unused on Android)
  //---------------------------------------------------------
  InitWindow(screenWidth, screenHeight, "classic game: pong");

  Vector2 mainMonitorPosition = GetMonitorPosition(0);
  SetWindowPosition(
      mainMonitorPosition.x + (GetMonitorWidth(0) - GetScreenWidth()) / 2,
      mainMonitorPosition.y + (GetMonitorHeight(0) - GetScreenHeight()) / 2);

  // get current working directory modern c++
  std::filesystem::path path = std::filesystem::current_path();
  std::cout << "Current path is : " << path << std::endl;

  InitAudioDevice();

  AudioSample c1, cis1, d1, dis1, e1, f1, fis1, g1, gis1, a1, ais1, b1, c2,
      cis2, d2, dis2, e2, f2, fis2, g2, gis2, a2, ais2, b2, c3, cis3, d3, dis3,
      e3, f3, fis3, g3, gis3, a3, ais3, b3, c4, cis4, d4, dis4, e4, f4, fis4,
      g4, gis4, a4, ais4, b4;

  notes = {c1, cis1, d1, dis1, e1, f1, fis1, g1, gis1, a1, ais1, b1,
           c2, cis2, d2, dis2, e2, f2, fis2, g2, gis2, a2, ais2, b2,
           c3, cis3, d3, dis3, e3, f3, fis3, g3, gis3, a3, ais3, b3,
           c4, cis4, d4, dis4, e4, f4, fis4, g4, gis4, a4, ais4, b4};

  InitNotes(notes);

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
  SetTargetFPS(120);
  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    // Update and Draw
    //----------------------------------------------------------------------------------
    UpdateDrawFrame();
    //----------------------------------------------------------------------------------
  }
#endif
  // De-Initialization
  //--------------------------------------------------------------------------------------
  UnloadGame(); // Unload loaded data (textures, sounds, models...)

  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------
}

GameEngine::~GameEngine() { LOG_DEBUG("GameEngine deconstructed."); }

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

// Initialize game variables
void GameEngine::InitGame(void) {
  PlayProgressionCDur();

  // brickSize = (Vector2){(float)GetScreenWidth() / BRICKS_PER_LINE, 40.0f};

  // Initialize player
  player.position = Vector2{57, static_cast<float>(screenHeight / 2)};
  player.size = Vector2{14, static_cast<float>(screenHeight / 6)};
  player.life = PLAYER_MAX_LIFE;

  // Initialize ball
  ball.position =
      Vector2{player.position.x + ball.radius,
              player.position.y - (player.size.y / 2) - ball.radius};

  ball.speed = Vector2{0, 0};
  ball.radius = 7;
  ball.active = false;
}

// Update game (one frame)
void GameEngine::UpdateGame(void) {
  if (!gameOver) {
    if (IsKeyPressed('P'))
      pause = !pause;

    if (!pause) {
      // Player movement logic
      if (IsKeyDown(KEY_UP))
        player.position.y -= 5;
      if ((player.position.y - (player.size.y / 2)) <= 0)
        player.position.y = (player.size.y / 2);

      if (IsKeyDown(KEY_DOWN))
        player.position.y += 5;
      if ((player.position.y + player.size.y / 2) >= screenHeight)
        player.position.y = screenHeight - (player.size.y / 2);

      // Ball launching logic
      if (!ball.active) {
        if (IsKeyPressed(KEY_SPACE)) {
          ball.active = true;
          ball.speed = Vector2{5, 0};
        }
      }

      // Ball movement logic
      if (ball.active) {
        ball.position.x += ball.speed.x;
        ball.position.y += ball.speed.y;
      } else {
        ball.position =
            Vector2{player.position.x + (ball.radius * 2), player.position.y};
      }

      // Collision logic: ball vs walls
      if ((ball.position.x + ball.radius) >= screenWidth) {
        ball.speed.x *= -1;
        PlayRandomNoteInCMinorProgression();
      }

      if (ball.active && (ball.position.x - ball.radius) <= 0) {
        ball.speed = Vector2{0, 0};
        PlayRandomNoteInCMinorProgression();
        ball.active = false;
        player.life--;
        // PlayProgressionCMinor();
        PlayProgressionCMinorReversed();
      }

      if (((ball.position.y - ball.radius) <= 0 ||
           ((ball.position.y + ball.radius) >= screenHeight))) {
        ball.speed.y *= -1;
        PlayRandomNoteInCMinorProgression();
      }

      // Collision logic: ball vs player
      if (CheckCollisionCircleRec(
              ball.position, ball.radius,
              Rectangle{player.position.x - (player.size.x / 2),
                        player.position.y - (player.size.y / 2), player.size.x,
                        player.size.y})) {
        if (ball.speed.x < 0) {
          ball.speed.x *= -1;
          ball.speed.y =
              (ball.position.y - player.position.y) / (player.size.y / 2) * 5;
          PlayRandomNoteInCMinorProgression();
          score++;
        }
      }

      // Game over logic
      if (player.life <= 0)
        gameOver = true;

      else {
        // gameOver = true;

        // for (int i = 0; i < LINES_OF_BRICKS; i++)
        // {
        //     for (int j = 0; j < BRICKS_PER_LINE; j++)
        //     {
        //         if (brick[i][j].active)
        //             // gameOver = false;
        //     }
        // }
      }
    }
  } else {
    if (IsKeyPressed(KEY_ENTER)) {
      InitGame();
      gameOver = false;
    }
  }
}

// Draw game (one frame)
void GameEngine::DrawGame(void) {
  BeginDrawing();

  ClearBackground(RAYWHITE);

  if (!gameOver) {
    // Draw player bar with unified coordinates
    DrawRectangle(player.position.x - player.size.x / 2,
                  player.position.y - player.size.y / 2, player.size.x,
                  player.size.y, BLACK);

    std::string scoreStr = "Score:\t" + std::to_string(score);
    DrawText(scoreStr.c_str(), 10, 10, 20, MAROON);

    // Draw player lives
    for (int i = 0; i < player.life; i++)
      DrawText("*", screenWidth - 100 - (40 * i), screenHeight - 40, 40,
               MAROON);

    // Draw ball
    DrawCircleV(ball.position, ball.radius, MAROON);

    if (pause)
      DrawText("GAME PAUSED",
               screenWidth / 2 - MeasureText("GAME PAUSED", 40) / 2,
               screenHeight / 2 - 40, 40, GRAY);
  } else
    DrawText("PRESS [ENTER] TO PLAY AGAIN",
             GetScreenWidth() / 2 -
                 MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20) / 2,
             GetScreenHeight() / 2 - 50, 20, GRAY);

  EndDrawing();
}

// Unload game variables
void GameEngine::UnloadGame(void) {
  // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
void GameEngine::UpdateDrawFrame(void) {
  UpdateGame();
  DrawGame();
}

void GameEngine::InitNotes(std::vector<AudioSample> &notes) {
  LOG.debug(m_assetsPath);
  notes[0].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "C1.wav").c_str());
  notes[1].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "C#1.wav").c_str());
  notes[2].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "D1.wav").c_str());
  notes[3].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "D#1.wav").c_str());
  notes[4].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "E1.wav").c_str());
  notes[5].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "F1.wav").c_str());
  notes[6].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "F#1.wav").c_str());
  notes[7].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "G1.wav").c_str());
  notes[8].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "G#1.wav").c_str());
  notes[9].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "A1.wav").c_str());
  notes[10].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "A#1.wav").c_str());
  notes[11].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "B1.wav").c_str());

  notes[12].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "C2.wav").c_str());
  notes[13].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "C#2.wav").c_str());
  notes[14].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "D2.wav").c_str());
  notes[15].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "D#2.wav").c_str());
  notes[16].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "E2.wav").c_str());
  notes[17].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "F2.wav").c_str());
  notes[18].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "F#2.wav").c_str());
  notes[19].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "G2.wav").c_str());
  notes[20].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "G#2.wav").c_str());
  notes[21].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "A2.wav").c_str());
  notes[22].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "A#2.wav").c_str());
  notes[23].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "B2.wav").c_str());

  notes[24].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "C3.wav").c_str());
  notes[25].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "C#3.wav").c_str());
  notes[26].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "D3.wav").c_str());
  notes[27].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "D#3.wav").c_str());
  notes[28].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "E3.wav").c_str());
  notes[29].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "F3.wav").c_str());
  notes[30].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "F#3.wav").c_str());
  notes[31].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "G3.wav").c_str());
  notes[32].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "G#3.wav").c_str());
  notes[33].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "A3.wav").c_str());
  notes[34].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "A#3.wav").c_str());
  notes[35].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "B3.wav").c_str());

  notes[36].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "C4.wav").c_str());
  notes[37].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "C#4.wav").c_str());
  notes[38].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "D4.wav").c_str());
  notes[39].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "D#4.wav").c_str());
  notes[40].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "E4.wav").c_str());
  notes[41].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "F4.wav").c_str());
  notes[42].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "F#4.wav").c_str());
  notes[43].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "G4.wav").c_str());
  notes[44].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "G#4.wav").c_str());
  notes[45].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "A4.wav").c_str());
  notes[46].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "A#4.wav").c_str());
  notes[47].sound =
      LoadSound((std::string(m_assetsPath) + "/" + "B4.wav").c_str());
}

void GameEngine::PlayRandomNote() {
  // modern c++ random number generation
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 47); // 48 notes
  int randomNote = dis(gen);
  PlaySound(notes[randomNote].sound);
}

void GameEngine::PlayRandomNoteInCMinorProgression() {
  std::vector<int> cMinorProgression = {0,  3,  7,  10, 12, 15, 19, 22,
                                        24, 27, 31, 34, 36, 39, 43, 46};
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15); // 16 notes
  int randomNote = dis(gen);
  PlaySound(notes[cMinorProgression[randomNote]].sound);
}

void GameEngine::PlayCDur() {
  PlaySound(notes[0].sound);
  PlaySound(notes[4].sound);
  PlaySound(notes[7].sound);
}

void GameEngine::PlayProgressionCDur() {
  PlaySound(notes[0].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[4].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[7].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[12].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[16].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[19].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[24].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[28].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[31].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[36].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[40].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[43].sound);
}

void GameEngine::PlayProgressionCMinor() {
  PlaySound(notes[0].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[3].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[7].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[10].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[12].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[15].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[19].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[22].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[24].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[27].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[31].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[34].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[36].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[39].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[43].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[46].sound);
}

void GameEngine::PlayProgressionCMinorReversed() {
  PlaySound(notes[46].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[43].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[39].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[36].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[34].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[31].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[27].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[24].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[22].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[19].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[15].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[12].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[10].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[7].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[3].sound);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  PlaySound(notes[0].sound);
}

} // namespace library