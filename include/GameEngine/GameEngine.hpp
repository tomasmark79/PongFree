#ifndef __GAMEENGINE_H__
#define __GAMEENGINE_H__

// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

// Public API

#include <raylib.h>
#include <string>
#include <vector>

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define PLAYER_MAX_LIFE 5
#define LINES_OF_BRICKS 5
#define BRICKS_PER_LINE 20

namespace library {

class GameEngine {

  //----------------------------------------------------------------------------------
  // Types and Structures Definition
  //----------------------------------------------------------------------------------
  typedef struct Player {
    Vector2 position;
    Vector2 size;
    int life;
  } Player;

  typedef struct Ball {
    Vector2 position;
    Vector2 speed;
    int radius;
    bool active;
  } Ball;

  typedef struct Brick {
    Vector2 position;
    bool active;
  } Brick;

  typedef struct AudioSample {
    Sound sound;
    bool loaded;
  } AudioSample;

public:
  const int screenWidth = 800;
  const int screenHeight = 600;
  bool gameOver = false;
  bool pause = false;
  Player player = {{0, 0}, {0, 0}, 0};
  Ball ball = {{0, 0}, {0, 0}, 0, false};
  Brick brick[LINES_OF_BRICKS][BRICKS_PER_LINE] = {};
  Vector2 brickSize = {0, 0};
  int score = 0;
  std::vector<AudioSample> notes;
  std::string m_assetsPath;

  GameEngine(const std::string &assetsPath);
  ~GameEngine();

  void InitGame(void);
  void UpdateGame(void);
  void DrawGame(void);
  void UnloadGame(void);
  void UpdateDrawFrame(void);
  void PlayRandomNote();
  void PlayCDur();
  void PlayProgressionCDur();
  void PlayProgressionCMinor();
  void PlayProgressionCMinorReversed();
  void PlayRandomNoteInCMinorProgression();
  void InitNotes(std::vector<AudioSample> &notes);

private:
};

} // namespace library

#endif // __GAMEENGINE_H__
