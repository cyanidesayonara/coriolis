// GIF slideshow — Borealis's Animations mode reborn. The scene only talks
// to the GifSource interface; the sim implements it with raylib's animated
// GIF loader, the device will implement it with pixelmatix/GifDecoder
// reading from SD.
#ifndef CORIOLIS_SCENE_GIFS_H
#define CORIOLIS_SCENE_GIFS_H

#include "../core/scene.h"
#include "../core/font.h"

namespace coriolis {

class GifSource {
 public:
  virtual ~GifSource() {}
  virtual int count() = 0;
  virtual bool open(int index) = 0;
  // render the next frame into the framebuffer (fit, centered);
  // returns the delay in ms until the following frame
  virtual uint32_t drawNextFrame(FrameBuffer& fb) = 0;
};

class GifScene : public Scene {
 public:
  explicit GifScene(GifSource& source) : source_(source) {}

  const char* name() const { return "Gifs"; }

  void start(Context& ctx) {
    current_ = 0;
    opened_ = source_.count() > 0 && source_.open(current_);
    nextFrameMs_ = ctx.nowMs;
  }

  bool input(Context& ctx, Key k) {
    if (source_.count() < 1) return false;
    // up/down browses gifs; left/right stay free for scene switching
    if (k == Key::Up || k == Key::Down) {
      int n = source_.count();
      current_ = (current_ + (k == Key::Down ? 1 : n - 1)) % n;
      opened_ = source_.open(current_);
      nextFrameMs_ = ctx.nowMs;
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    if (!opened_) {
      ctx.fb.clear();
      const char* msg = "NO GIFS";
      int w = font3x5::textWidth(msg, 2);
      font3x5::drawText(ctx.fb, msg, (ctx.fb.width() - w) / 2,
                        ctx.fb.height() / 2 - 5, 2, RGB(150, 150, 150));
      return 250;
    }

    if (ctx.nowMs >= nextFrameMs_) {
      uint32_t delay = source_.drawNextFrame(ctx.fb);
      if (delay == 0) delay = 80;
      nextFrameMs_ = ctx.nowMs + delay;
    }
    return 15;
  }

 private:
  GifSource& source_;
  int current_ = 0;
  bool opened_ = false;
  uint32_t nextFrameMs_ = 0;
};

}

#endif
