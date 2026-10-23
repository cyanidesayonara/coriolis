// Scene transition engine — hardware-agnostic crossfades, slides, and wipes
// between scenes. Snapshots the outgoing scene's last frame and composites it
// with the incoming scene's live render loop using smoothstep easing.
// Runs with zero per-frame heap allocations across desktop simulator and Teensy 4.1.
#ifndef CORIOLIS_TRANSITION_H
#define CORIOLIS_TRANSITION_H

#include <stdint.h>
#include <string.h>
#include <math.h>

#include "config.h"
#include "color.h"
#include "display.h"

namespace coriolis {

enum class TransitionType : uint8_t {
  Cut = 0,
  CrossFade,
  SlideLeft,
  SlideRight,
  SlideUp,
  SlideDown,
  WipeLeft,
  WipeRight,
};

inline TransitionType resolveTransition(uint8_t mode, TransitionType requested) {
  switch (mode) {
    case 1:  // Always slide
      if (requested == TransitionType::CrossFade || requested == TransitionType::Cut)
        return TransitionType::SlideLeft;
      return requested;
    case 2:  // Always crossfade
      return TransitionType::CrossFade;
    case 3:  // Always cut (instant)
      return TransitionType::Cut;
    case 0:  // Auto
    default:
      return requested;
  }
}

class TransitionEngine {
 public:
  static const uint32_t DEFAULT_DURATION_MS = 240;

  TransitionEngine() : active_(false), startMs_(0), durationMs_(DEFAULT_DURATION_MS),
                       type_(TransitionType::Cut) {}

  void start(const FrameBuffer& currentFb, TransitionType type, uint32_t nowMs,
             uint32_t durationMs = DEFAULT_DURATION_MS) {
    if (type == TransitionType::Cut || durationMs == 0) {
      active_ = false;
      return;
    }
    memcpy(snapshot_.px, currentFb.px, sizeof(snapshot_.px));
    type_ = type;
    startMs_ = nowMs;
    durationMs_ = durationMs;
    active_ = true;
  }

  void cancel() { active_ = false; }

  bool isActive(uint32_t nowMs) const {
    if (!active_) return false;
    if (nowMs - startMs_ >= durationMs_) {
      return false;
    }
    return true;
  }

  float progress(uint32_t nowMs) const {
    if (!active_ || durationMs_ == 0) return 1.0f;
    if (nowMs <= startMs_) return 0.0f;
    uint32_t elapsed = nowMs - startMs_;
    if (elapsed >= durationMs_) return 1.0f;
    return float(elapsed) / float(durationMs_);
  }

  TransitionType type() const { return type_; }

  void composite(FrameBuffer& target, const FrameBuffer& incomingFb, uint32_t nowMs) const {
    if (!active_) {
      memcpy(target.px, incomingFb.px, sizeof(target.px));
      return;
    }

    float t = progress(nowMs);
    if (t >= 1.0f) {
      memcpy(target.px, incomingFb.px, sizeof(target.px));
      return;
    }

    // Smoothstep easing: 3t^2 - 2t^3
    float e = t * t * (3.0f - 2.0f * t);

    switch (type_) {
      case TransitionType::CrossFade: {
        uint8_t frac = uint8_t(e * 255.0f);
        for (int i = 0; i < PIXELS; i++) {
          target.px[i] = lerpColor(snapshot_.px[i], incomingFb.px[i], frac);
        }
        break;
      }

      case TransitionType::SlideLeft: {
        int shift = int(roundf(e * WIDTH));
        if (shift <= 0) {
          memcpy(target.px, snapshot_.px, sizeof(target.px));
        } else if (shift >= WIDTH) {
          memcpy(target.px, incomingFb.px, sizeof(target.px));
        } else {
          int outgoingWidth = WIDTH - shift;
          for (int y = 0; y < HEIGHT; y++) {
            int rowOffset = y * WIDTH;
            memcpy(&target.px[rowOffset],
                   &snapshot_.px[rowOffset + shift],
                   outgoingWidth * sizeof(RGB));
            memcpy(&target.px[rowOffset + outgoingWidth],
                   &incomingFb.px[rowOffset],
                   shift * sizeof(RGB));
          }
        }
        break;
      }

      case TransitionType::SlideRight: {
        int shift = int(roundf(e * WIDTH));
        if (shift <= 0) {
          memcpy(target.px, snapshot_.px, sizeof(target.px));
        } else if (shift >= WIDTH) {
          memcpy(target.px, incomingFb.px, sizeof(target.px));
        } else {
          int outgoingWidth = WIDTH - shift;
          for (int y = 0; y < HEIGHT; y++) {
            int rowOffset = y * WIDTH;
            memcpy(&target.px[rowOffset],
                   &incomingFb.px[rowOffset + outgoingWidth],
                   shift * sizeof(RGB));
            memcpy(&target.px[rowOffset + shift],
                   &snapshot_.px[rowOffset],
                   outgoingWidth * sizeof(RGB));
          }
        }
        break;
      }

      case TransitionType::SlideUp: {
        int shift = int(roundf(e * HEIGHT));
        if (shift <= 0) {
          memcpy(target.px, snapshot_.px, sizeof(target.px));
        } else if (shift >= HEIGHT) {
          memcpy(target.px, incomingFb.px, sizeof(target.px));
        } else {
          int outgoingRows = HEIGHT - shift;
          memcpy(&target.px[0],
                 &snapshot_.px[shift * WIDTH],
                 outgoingRows * WIDTH * sizeof(RGB));
          memcpy(&target.px[outgoingRows * WIDTH],
                 &incomingFb.px[0],
                 shift * WIDTH * sizeof(RGB));
        }
        break;
      }

      case TransitionType::SlideDown: {
        int shift = int(roundf(e * HEIGHT));
        if (shift <= 0) {
          memcpy(target.px, snapshot_.px, sizeof(target.px));
        } else if (shift >= HEIGHT) {
          memcpy(target.px, incomingFb.px, sizeof(target.px));
        } else {
          int outgoingRows = HEIGHT - shift;
          memcpy(&target.px[0],
                 &incomingFb.px[(HEIGHT - shift) * WIDTH],
                 shift * WIDTH * sizeof(RGB));
          memcpy(&target.px[shift * WIDTH],
                 &snapshot_.px[0],
                 outgoingRows * WIDTH * sizeof(RGB));
        }
        break;
      }

      case TransitionType::WipeLeft: {
        int split = int(roundf(e * WIDTH));
        if (split <= 0) {
          memcpy(target.px, snapshot_.px, sizeof(target.px));
        } else if (split >= WIDTH) {
          memcpy(target.px, incomingFb.px, sizeof(target.px));
        } else {
          for (int y = 0; y < HEIGHT; y++) {
            int rowOffset = y * WIDTH;
            memcpy(&target.px[rowOffset],
                   &incomingFb.px[rowOffset],
                   split * sizeof(RGB));
            memcpy(&target.px[rowOffset + split],
                   &snapshot_.px[rowOffset + split],
                   (WIDTH - split) * sizeof(RGB));
          }
        }
        break;
      }

      case TransitionType::WipeRight: {
        int split = WIDTH - int(roundf(e * WIDTH));
        if (split <= 0) {
          memcpy(target.px, incomingFb.px, sizeof(target.px));
        } else if (split >= WIDTH) {
          memcpy(target.px, snapshot_.px, sizeof(target.px));
        } else {
          for (int y = 0; y < HEIGHT; y++) {
            int rowOffset = y * WIDTH;
            memcpy(&target.px[rowOffset],
                   &snapshot_.px[rowOffset],
                   split * sizeof(RGB));
            memcpy(&target.px[rowOffset + split],
                   &incomingFb.px[rowOffset + split],
                   (WIDTH - split) * sizeof(RGB));
          }
        }
        break;
      }

      case TransitionType::Cut:
      default:
        memcpy(target.px, incomingFb.px, sizeof(target.px));
        break;
    }
  }

 private:
  bool active_;
  uint32_t startMs_;
  uint32_t durationMs_;
  TransitionType type_;
  FrameBuffer snapshot_;
};

}  // namespace coriolis

#endif  // CORIOLIS_TRANSITION_H
