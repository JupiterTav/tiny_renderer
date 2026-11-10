#include "Frame.hpp"

#include <algorithm>

namespace {
constexpr double kMaxDeltaSeconds = 0.1;
constexpr double kFpsUpdateInterval = 0.5;
} // namespace

float Frame::update() {
  const Clock::time_point now = Clock::now();

  if (!started_) {
    startTime_ = now;
    lastTime_ = now;
    started_ = true;
  }

  const double rawDelta =
      std::chrono::duration<double>(now - lastTime_).count();
  lastTime_ = now;

  frameTimeMs_ = static_cast<float>(rawDelta * 1000.0);
  deltaTime_ = static_cast<float>(std::min(rawDelta, kMaxDeltaSeconds));
  elapsedTime_ = std::chrono::duration<double>(now - startTime_).count();
  ++frameCount_;

  fpsAccumulator_ += rawDelta;
  ++fpsFrameCount_;
  if (fpsAccumulator_ >= kFpsUpdateInterval) {
    fps_ = static_cast<float>(fpsFrameCount_ / fpsAccumulator_);
    fpsAccumulator_ = 0.0;
    fpsFrameCount_ = 0;
    }

  return deltaTime_;
}
