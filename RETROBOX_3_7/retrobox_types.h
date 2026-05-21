#pragma once

enum Difficulty { DIFF_EASY = 0, DIFF_MEDIUM, DIFF_HARD };

struct Button {
  int  pin;
  bool lastState;
  bool pressed;
  bool held;
};

#define FRAME_MS 33