// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <GameEngine/GameEngine.hpp>
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

namespace dotname {

  GameEngine::GameEngine () {
    LOG_D_STREAM << libName_ << " ...constructed" << std::endl;
  }
  GameEngine::GameEngine (const std::filesystem::path& assetsPath) : GameEngine () {
    assetsPath_ = assetsPath;
    if (!assetsPath_.empty ()) {
      LOG_D_STREAM << "Assets path: " << assetsPath_ << std::endl;
      // Initialization (Note windowTitle is unused on Android)
      //---------------------------------------------------------
      InitWindow (screenWidth, screenHeight, "classic game: pong");

      Vector2 mainMonitorPosition = GetMonitorPosition (0);
      SetWindowPosition (mainMonitorPosition.x + (GetMonitorWidth (0) - GetScreenWidth ()) / 2,
                         mainMonitorPosition.y + (GetMonitorHeight (0) - GetScreenHeight ()) / 2);

      // get current working directory modern c++
      std::filesystem::path path = std::filesystem::current_path ();
      std::cout << "Current path is : " << path << std::endl;

      InitAudioDevice ();

      AudioSample c1, cis1, d1, dis1, e1, f1, fis1, g1, gis1, a1, ais1, b1, c2, cis2, d2, dis2, e2,
          f2, fis2, g2, gis2, a2, ais2, b2, c3, cis3, d3, dis3, e3, f3, fis3, g3, gis3, a3, ais3,
          b3, c4, cis4, d4, dis4, e4, f4, fis4, g4, gis4, a4, ais4, b4;

      notes = { c1, cis1, d1, dis1, e1, f1, fis1, g1, gis1, a1, ais1, b1,
                c2, cis2, d2, dis2, e2, f2, fis2, g2, gis2, a2, ais2, b2,
                c3, cis3, d3, dis3, e3, f3, fis3, g3, gis3, a3, ais3, b3,
                c4, cis4, d4, dis4, e4, f4, fis4, g4, gis4, a4, ais4, b4 };

      InitNotes (notes);

#if defined(PLATFORM_WEB)
      emscripten_set_main_loop (UpdateDrawFrame, 60, 1);
#else
      SetTargetFPS (120);
      //--------------------------------------------------------------------------------------

      // Main game loop
      while (!WindowShouldClose ()) // Detect window close button or ESC key
      {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame ();
        //----------------------------------------------------------------------------------
      }
#endif
      // De-Initialization
      //--------------------------------------------------------------------------------------
      UnloadGame (); // Unload loaded data (textures, sounds, models...)

      CloseWindow (); // Close window and OpenGL context
                      //-----------------------------------------------------------------
    } else {
      LOG_D_STREAM << "Assets path is empty" << std::endl;
    }
  }
  GameEngine::~GameEngine () {
    LOG_D_STREAM << libName_ << " ...destructed" << std::endl;
  }

  //------------------------------------------------------------------------------------
  // Module Functions Definitions (local)
  //------------------------------------------------------------------------------------

  // Initialize game variables
  void GameEngine::InitGame (void) {
    PlayProgressionCDur ();

    // brickSize = (Vector2){(float)GetScreenWidth() / BRICKS_PER_LINE, 40.0f};

    // Initialize player
    player.position = Vector2{ 57, static_cast<float> (screenHeight / 2) };
    player.size = Vector2{ 14, static_cast<float> (screenHeight / 6) };
    player.life = PLAYER_MAX_LIFE;

    // Initialize ball
    ball.position = Vector2{ player.position.x + ball.radius,
                             player.position.y - (player.size.y / 2) - ball.radius };

    ball.speed = Vector2{ 0, 0 };
    ball.radius = 7;
    ball.active = false;
  }

  // Update game (one frame)
  void GameEngine::UpdateGame (void) {
    if (!gameOver) {
      if (IsKeyPressed ('P'))
        pause = !pause;

      if (!pause) {
        // Player movement logic
        if (IsKeyDown (KEY_UP))
          player.position.y -= 5;
        if ((player.position.y - (player.size.y / 2)) <= 0)
          player.position.y = (player.size.y / 2);

        if (IsKeyDown (KEY_DOWN))
          player.position.y += 5;
        if ((player.position.y + player.size.y / 2) >= screenHeight)
          player.position.y = screenHeight - (player.size.y / 2);

        // Ball launching logic
        if (!ball.active) {
          if (IsKeyPressed (KEY_SPACE)) {
            ball.active = true;
            ball.speed = Vector2{ 5, 0 };
          }
        }

        // Ball movement logic
        if (ball.active) {
          ball.position.x += ball.speed.x;
          ball.position.y += ball.speed.y;
        } else {
          ball.position = Vector2{ player.position.x + (ball.radius * 2), player.position.y };
        }

        // Collision logic: ball vs walls
        if ((ball.position.x + ball.radius) >= screenWidth) {
          ball.speed.x *= -1;
          PlayRandomNoteInCMinorProgression ();
        }

        if (ball.active && (ball.position.x - ball.radius) <= 0) {
          ball.speed = Vector2{ 0, 0 };
          PlayRandomNoteInCMinorProgression ();
          ball.active = false;
          player.life--;
          // PlayProgressionCMinor();
          PlayProgressionCMinorReversed ();
        }

        if (((ball.position.y - ball.radius) <= 0
             || ((ball.position.y + ball.radius) >= screenHeight))) {
          ball.speed.y *= -1;
          PlayRandomNoteInCMinorProgression ();
        }

        // Collision logic: ball vs player
        if (CheckCollisionCircleRec (ball.position, ball.radius,
                                     Rectangle{ player.position.x - (player.size.x / 2),
                                                player.position.y - (player.size.y / 2),
                                                player.size.x, player.size.y })) {
          if (ball.speed.x < 0) {
            ball.speed.x *= -1;
            ball.speed.y = (ball.position.y - player.position.y) / (player.size.y / 2) * 5;
            PlayRandomNoteInCMinorProgression ();
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
      if (IsKeyPressed (KEY_ENTER)) {
        InitGame ();
        gameOver = false;
      }
    }
  }

  // Draw game (one frame)
  void GameEngine::DrawGame (void) {
    BeginDrawing ();

    ClearBackground (RAYWHITE);

    if (!gameOver) {
      // Draw player bar with unified coordinates
      DrawRectangle (player.position.x - player.size.x / 2, player.position.y - player.size.y / 2,
                     player.size.x, player.size.y, BLACK);

      std::string scoreStr = "Score:\t" + std::to_string (score);
      DrawText (scoreStr.c_str (), 10, 10, 20, MAROON);

      // Draw player lives
      for (int i = 0; i < player.life; i++)
        DrawText ("*", screenWidth - 100 - (40 * i), screenHeight - 40, 40, MAROON);

      // Draw ball
      DrawCircleV (ball.position, ball.radius, MAROON);

      if (pause)
        DrawText ("GAME PAUSED", screenWidth / 2 - MeasureText ("GAME PAUSED", 40) / 2,
                  screenHeight / 2 - 40, 40, GRAY);
    } else
      DrawText ("PRESS [ENTER] TO PLAY AGAIN",
                GetScreenWidth () / 2 - MeasureText ("PRESS [ENTER] TO PLAY AGAIN", 20) / 2,
                GetScreenHeight () / 2 - 50, 20, GRAY);

    EndDrawing ();
  }

  // Unload game variables
  void GameEngine::UnloadGame (void) {
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
  }

  // Update and Draw (one frame)
  void GameEngine::UpdateDrawFrame (void) {
    UpdateGame ();
    DrawGame ();
  }

  void GameEngine::InitNotes (std::vector<AudioSample>& notes) {
    notes[0].sound = LoadSound ((assetsPath_ / "C1.wav").string ().c_str ());

    notes[1].sound = LoadSound ((assetsPath_ / "C#1.wav").string ().c_str ());
    notes[2].sound = LoadSound ((assetsPath_ / "D1.wav").string ().c_str ());
    notes[3].sound = LoadSound ((assetsPath_ / "D#1.wav").string ().c_str ());
    notes[4].sound = LoadSound ((assetsPath_ / "E1.wav").string ().c_str ());
    notes[5].sound = LoadSound ((assetsPath_ / "F1.wav").string ().c_str ());
    notes[6].sound = LoadSound ((assetsPath_ / "F#1.wav").string ().c_str ());
    notes[7].sound = LoadSound ((assetsPath_ / "G1.wav").string ().c_str ());
    notes[8].sound = LoadSound ((assetsPath_ / "G#1.wav").string ().c_str ());
    notes[9].sound = LoadSound ((assetsPath_ / "A1.wav").string ().c_str ());
    notes[10].sound = LoadSound ((assetsPath_ / "A#1.wav").string ().c_str ());
    notes[11].sound = LoadSound ((assetsPath_ / "B1.wav").string ().c_str ());

    notes[12].sound = LoadSound ((assetsPath_ / "C2.wav").string ().c_str ());
    notes[13].sound = LoadSound ((assetsPath_ / "C#2.wav").string ().c_str ());
    notes[14].sound = LoadSound ((assetsPath_ / "D2.wav").string ().c_str ());
    notes[15].sound = LoadSound ((assetsPath_ / "D#2.wav").string ().c_str ());
    notes[16].sound = LoadSound ((assetsPath_ / "E2.wav").string ().c_str ());
    notes[17].sound = LoadSound ((assetsPath_ / "F2.wav").string ().c_str ());
    notes[18].sound = LoadSound ((assetsPath_ / "F#2.wav").string ().c_str ());
    notes[19].sound = LoadSound ((assetsPath_ / "G2.wav").string ().c_str ());
    notes[20].sound = LoadSound ((assetsPath_ / "G#2.wav").string ().c_str ());
    notes[21].sound = LoadSound ((assetsPath_ / "A2.wav").string ().c_str ());
    notes[22].sound = LoadSound ((assetsPath_ / "A#2.wav").string ().c_str ());
    notes[23].sound = LoadSound ((assetsPath_ / "B2.wav").string ().c_str ());

    notes[24].sound = LoadSound ((assetsPath_ / "C3.wav").string ().c_str ());
    notes[25].sound = LoadSound ((assetsPath_ / "C#3.wav").string ().c_str ());
    notes[26].sound = LoadSound ((assetsPath_ / "D3.wav").string ().c_str ());
    notes[27].sound = LoadSound ((assetsPath_ / "D#3.wav").string ().c_str ());
    notes[28].sound = LoadSound ((assetsPath_ / "E3.wav").string ().c_str ());
    notes[29].sound = LoadSound ((assetsPath_ / "F3.wav").string ().c_str ());
    notes[30].sound = LoadSound ((assetsPath_ / "F#3.wav").string ().c_str ());
    notes[31].sound = LoadSound ((assetsPath_ / "G3.wav").string ().c_str ());
    notes[32].sound = LoadSound ((assetsPath_ / "G#3.wav").string ().c_str ());
    notes[33].sound = LoadSound ((assetsPath_ / "A3.wav").string ().c_str ());
    notes[34].sound = LoadSound ((assetsPath_ / "A#3.wav").string ().c_str ());
    notes[35].sound = LoadSound ((assetsPath_ / "B3.wav").string ().c_str ());

    notes[36].sound = LoadSound ((assetsPath_ / "C4.wav").string ().c_str ());
    notes[37].sound = LoadSound ((assetsPath_ / "C#4.wav").string ().c_str ());
    notes[38].sound = LoadSound ((assetsPath_ / "D4.wav").string ().c_str ());
    notes[39].sound = LoadSound ((assetsPath_ / "D#4.wav").string ().c_str ());
    notes[40].sound = LoadSound ((assetsPath_ / "E4.wav").string ().c_str ());
    notes[41].sound = LoadSound ((assetsPath_ / "F4.wav").string ().c_str ());
    notes[42].sound = LoadSound ((assetsPath_ / "F#4.wav").string ().c_str ());
    notes[43].sound = LoadSound ((assetsPath_ / "G4.wav").string ().c_str ());
    notes[44].sound = LoadSound ((assetsPath_ / "G#4.wav").string ().c_str ());
    notes[45].sound = LoadSound ((assetsPath_ / "A4.wav").string ().c_str ());
    notes[46].sound = LoadSound ((assetsPath_ / "A#4.wav").string ().c_str ());
    notes[47].sound = LoadSound ((assetsPath_ / "B4.wav").string ().c_str ());
  }

  void GameEngine::PlayRandomNote () {
    // modern c++ random number generation
    std::random_device rd;
    std::mt19937 gen (rd ());
    std::uniform_int_distribution<> dis (0, 47); // 48 notes
    int randomNote = dis (gen);
    PlaySound (notes[randomNote].sound);
  }

  void GameEngine::PlayRandomNoteInCMinorProgression () {
    std::vector<int> cMinorProgression
        = { 0, 3, 7, 10, 12, 15, 19, 22, 24, 27, 31, 34, 36, 39, 43, 46 };
    std::random_device rd;
    std::mt19937 gen (rd ());
    std::uniform_int_distribution<> dis (0, 15); // 16 notes
    int randomNote = dis (gen);
    PlaySound (notes[cMinorProgression[randomNote]].sound);
  }

  void GameEngine::PlayCDur () {
    PlaySound (notes[0].sound);
    PlaySound (notes[4].sound);
    PlaySound (notes[7].sound);
  }

  void GameEngine::PlayProgressionCDur () {
    PlaySound (notes[0].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[4].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[7].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[12].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[16].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[19].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[24].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[28].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[31].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[36].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[40].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[43].sound);
  }

  void GameEngine::PlayProgressionCMinor () {
    PlaySound (notes[0].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[3].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[7].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[10].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[12].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[15].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[19].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[22].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[24].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[27].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[31].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[34].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[36].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[39].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[43].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[46].sound);
  }

  void GameEngine::PlayProgressionCMinorReversed () {
    PlaySound (notes[46].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[43].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[39].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[36].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[34].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[31].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[27].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[24].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[22].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[19].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[15].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[12].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[10].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[7].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[3].sound);
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    PlaySound (notes[0].sound);
  }

} // namespace dotname