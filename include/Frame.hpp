#pragma once

#include <chrono>
#include <cstdint>

// Tracks frame timing and exposes a frame delta suitable for per-frame updates.
class Frame {
public:
  // Advances the frame clock. Returns the clamped delta time in seconds.
  float update();

  float deltaTime() const { return deltaTime_; }
  float frameTimeMs() const { return frameTimeMs_; }
  float fps() const { return fps_; }
  double elapsedTime() const { return elapsedTime_; }
  std::uint64_t frameCount() const { return frameCount_; }

private:
  using Clock = std::chrono::steady_clock;

  Clock::time_point startTime_{};
  Clock::time_point lastTime_{};
  bool started_{false};

  float deltaTime_{0.0f};
  float frameTimeMs_{0.0f};
  float fps_{0.0f};
  double elapsedTime_{0.0};
  double fpsAccumulator_{0.0};
  std::uint32_t fpsFrameCount_{0};
  std::uint64_t frameCount_{0};
};
