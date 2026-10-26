// The Shell: everything between "a key was pressed" and "here is the frame
// to show" that isn't a scene — the boot splash, scene switching with
// transitions, the launcher, the settings menu, the clock overlay, toasts,
// autoplay and night dimming. Both backends (simulator and firmware) run
// this same code; they only translate their inputs into keys/commands and
// copy frame() to their output, so the desktop exercises exactly what the
// device runs.
//
// Frame pacing: a scene is redrawn when its requested delay has passed,
// while the composed output (transition, HUD, toast, launcher) refreshes at
// ~60 fps whenever something on top of the scene is animating.
#ifndef CORIOLIS_SHELL_H
#define CORIOLIS_SHELL_H

#include <stdio.h>
#include <string.h>

#include "../core/scene.h"
#include "../core/settings.h"
#include "../core/transition.h"
#include "../core/palette.h"
#include "scene_settings.h"
#include "scene_launcher.h"
#include "clock_overlay.h"
#include "toast.h"
#include "splash.h"

namespace coriolis {

// buttons beyond the six navigation keys
enum class Command : uint8_t {
  None,
  Menu,      // launcher; closes launcher/settings when one is open
  Settings,  // settings menu (remote: hold Menu)
  Overlay,   // cycle the clock overlay
  Next,      // next scene regardless of what the scene wants
  Home,      // straight back to the clock
};

class Shell {
 public:
  Shell(Scene* const* scenes, int count, Settings& settings,
        SettingsScene& settingsScene)
      : scenes_(scenes),
        count_(count),
        settings_(settings),
        settingsScene_(settingsScene) {}

  void begin(Context& ctx, bool splash = true) {
    launcher_.attach(scenes_, count_);
    settings_.sanitize();
    ctx.palette = &palettes::byIndex(settings_.paletteIndex);
    ctx.audio.setVolume(settings_.volume);
    lastVolume_ = settings_.volume;
    if (splash) {
      splashOn_ = true;
      splash_.start(ctx);
    } else {
      scenes_[current_]->start(ctx);
    }
    lastSwitchMs_ = ctx.nowMs;
    nextFrameMs_ = 0;
  }

  // a navigation key press. The launcher and settings take keys while open;
  // otherwise the scene gets first refusal and the shell handles the rest
  void key(Context& ctx, Key k) {
    if (k == Key::None) return;
    nextFrameMs_ = 0;
    if (splashOn_) {
      endSplash(ctx);
      return;
    }
    if (launcher_.isActive()) {
      int chosen = launcher_.handleInput(k);
      // choosing the scene already showing just closes the menu — it must
      // not restart a game or workout in progress
      if (chosen >= 0 && chosen != current_)
        switchTo(ctx, chosen, TransitionType::CrossFade, true);
      return;
    }
    if (inSettings_) {
      if (!settingsScene_.input(ctx, k) && k == Key::Back) closeSettings(ctx);
      return;
    }
    if (scenes_[current_]->input(ctx, k)) return;
    switch (k) {
      case Key::Right:
        switchTo(ctx, (current_ + 1) % count_, TransitionType::SlideLeft);
        break;
      case Key::Left:
        switchTo(ctx, (current_ + count_ - 1) % count_, TransitionType::SlideRight);
        break;
      case Key::Up:
      case Key::Down: {
        int d = k == Key::Up ? 1 : palettes::COUNT - 1;
        settings_.paletteIndex = (settings_.paletteIndex + d) % palettes::COUNT;
        toast_.show(palettes::byIndex(settings_.paletteIndex).name, ctx.nowMs);
        break;
      }
      case Key::Back:
        if (current_ != 0) switchTo(ctx, 0, TransitionType::CrossFade);
        break;
      default:
        break;
    }
  }

  void command(Context& ctx, Command c) {
    if (c == Command::None) return;
    nextFrameMs_ = 0;
    if (splashOn_) endSplash(ctx);
    switch (c) {
      case Command::Menu:
        if (launcher_.isActive()) launcher_.close();
        else if (inSettings_) closeSettings(ctx);
        else openLauncher(ctx);
        break;
      case Command::Settings:
        if (inSettings_) {
          closeSettings(ctx);
        } else {
          launcher_.close();
          settingsScene_.openSection(scenes_[current_]->name());
          inSettings_ = true;
        }
        break;
      case Command::Overlay: {
        settings_.overlayType = uint8_t((settings_.overlayType + 1) % 4);
        char m[24];
        snprintf(m, sizeof(m), "Clock: %s", overlay::typeName(settings_.overlayType));
        toast_.show(m, ctx.nowMs);
        break;
      }
      case Command::Next:
        if (!launcher_.isActive() && !inSettings_)
          switchTo(ctx, (current_ + 1) % count_, TransitionType::SlideLeft);
        break;
      case Command::Home:
        launcher_.close();
        if (inSettings_) closeSettings(ctx);
        if (current_ != 0) switchTo(ctx, 0, TransitionType::CrossFade);
        break;
      default:
        break;
    }
  }

  // jump straight to a scene (number keys on a full remote)
  void show(Context& ctx, int index) {
    if (index < 0 || index >= count_) return;
    if (splashOn_) endSplash(ctx);
    launcher_.close();
    if (inSettings_) closeSettings(ctx);
    if (index != current_) switchTo(ctx, index, TransitionType::CrossFade);
  }

  // run whatever is due; true when frame() holds a new image to show
  bool tick(Context& ctx) {
    ctx.palette = &palettes::byIndex(settings_.paletteIndex);
    if (settings_.volume != lastVolume_) {
      ctx.audio.setVolume(settings_.volume);
      lastVolume_ = settings_.volume;
    }
    ctx.audio.update(ctx.nowMs);

    if (splashOn_ && splash_.done(ctx.nowMs)) endSplash(ctx);

    // autoplay cycles eligible scenes; never while a menu is open
    if (!splashOn_ && !inSettings_ && !launcher_.isActive() &&
        settings_.autoplay && scenes_[current_]->autoplayEligible() &&
        ctx.nowMs - lastSwitchMs_ >= uint32_t(settings_.autoplaySeconds) * 1000) {
      int to = (current_ + 1) % count_;
      while (to != current_ && !scenes_[to]->autoplayEligible())
        to = (to + 1) % count_;
      switchTo(ctx, to, TransitionType::SlideLeft);
    }

    bool transitioning = transition_.isActive(ctx.nowMs);
    bool animating = transitioning || toast_.visible(ctx.nowMs) ||
                     launcher_.isActive();
    // signed differences survive millis() wrapping (every ~49.7 days)
    bool sceneDue = int32_t(ctx.nowMs - nextFrameMs_) >= 0;
    bool composeDue = (animating || hudWasAnimating_) &&
                      ctx.nowMs - lastComposeMs_ >= 16;
    if (!sceneDue && !composeDue) return false;

    // the scene canvas: paused (not drawn) under the launcher
    if (sceneDue && !launcher_.isActive()) {
      Scene* s = splashOn_ ? static_cast<Scene*>(&splash_)
                           : (inSettings_ ? static_cast<Scene*>(&settingsScene_)
                                          : scenes_[current_]);
      uint32_t delay = s->draw(ctx);
      nextFrameMs_ = ctx.nowMs + delay;
    } else if (sceneDue) {
      nextFrameMs_ = ctx.nowMs + 33;
    }

    // compose the output: scene (+ transition) + overlay + toast
    Context out = {out_, ctx.time, ctx.held, ctx.audio, ctx.palette, ctx.nowMs};
    if (launcher_.isActive()) {
      memcpy(out_.px, transition_.scratch().px, sizeof(out_.px));
      launcher_.draw(out);
    } else if (transitioning) {
      transition_.composite(out_, ctx.fb, ctx.nowMs);
    } else {
      memcpy(out_.px, ctx.fb.px, sizeof(out_.px));
    }
    if (!splashOn_ && !inSettings_ && !launcher_.isActive())
      overlay::draw(out, settings_);
    toast_.draw(out);

    lastComposeMs_ = ctx.nowMs;
    hudWasAnimating_ = animating;  // one more compose clears a finished toast
    return true;
  }

  const FrameBuffer& frame() const { return out_; }

  // output brightness right now, honoring the night schedule
  uint8_t brightness(Context& ctx) const {
    return settings_.effectiveBrightness(ctx.time.now().hour);
  }

  const char* activeName() const {
    if (splashOn_) return splash_.name();
    if (launcher_.isActive()) return "Launcher";
    if (inSettings_) return settingsScene_.name();
    return scenes_[current_]->name();
  }

  int current() const { return current_; }
  bool menuOpen() const { return inSettings_ || launcher_.isActive(); }
  Toast& toast() { return toast_; }

 private:
  Scene* const* scenes_;
  int count_;
  Settings& settings_;
  SettingsScene& settingsScene_;

  SceneLauncher launcher_;
  SplashScene splash_;
  TransitionEngine transition_;
  Toast toast_;
  FrameBuffer out_;

  int current_ = 0;
  bool inSettings_ = false;
  bool splashOn_ = false;
  bool hudWasAnimating_ = false;
  uint8_t lastVolume_ = 255;
  uint32_t nextFrameMs_ = 0;
  uint32_t lastSwitchMs_ = 0;
  uint32_t lastComposeMs_ = 0;

  static const uint32_t SWITCH_MS = 260;

  void switchTo(Context& ctx, int to, TransitionType type,
                bool fromLauncher = false) {
    TransitionType resolved = resolveTransition(settings_.transitionMode, type);
    // snapshot what the viewer sees: the launcher if it was up, otherwise
    // the bare scene (the overlay is redrawn on top, pinned, as it slides)
    const FrameBuffer& from = fromLauncher ? out_ : ctx.fb;
    if (resolved != TransitionType::Cut)
      transition_.start(from, resolved, ctx.nowMs, SWITCH_MS);
    else
      transition_.cancel();
    scenes_[current_]->stop(ctx);
    current_ = to;
    ctx.fb.clear();  // trail-style scenes start from black, not the old scene
    scenes_[current_]->start(ctx);
    nextFrameMs_ = 0;
    lastSwitchMs_ = ctx.nowMs;
    toast_.show(scenes_[current_]->name(), ctx.nowMs);
  }

  void openLauncher(Context& ctx) {
    // freeze a dimmed still of the scene as the launcher's backdrop
    FrameBuffer& backdrop = transition_.scratch();
    memcpy(backdrop.px, ctx.fb.px, sizeof(backdrop.px));
    backdrop.dimAll(55);
    launcher_.open(current_);
  }

  void closeSettings(Context& ctx) {
    settingsScene_.stop(ctx);  // persists any changes
    inSettings_ = false;
    ctx.fb.clear();  // trail scenes would otherwise fade the menu out slowly
  }

  void endSplash(Context& ctx) {
    if (!splashOn_) return;
    splashOn_ = false;
    if (settings_.transitionMode != 3)
      transition_.start(ctx.fb, TransitionType::CrossFade, ctx.nowMs, 600);
    ctx.fb.clear();
    scenes_[current_]->start(ctx);
    lastSwitchMs_ = ctx.nowMs;
    nextFrameMs_ = 0;
  }
};

}

#endif
