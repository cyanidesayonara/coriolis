(function () {
  "use strict";

  var MATRIX_W = 128;
  var MATRIX_H = 128;
  var PIXEL_BYTES = MATRIX_W * MATRIX_H * 4;

  var KEY_UP = 1;
  var KEY_DOWN = 2;
  var KEY_LEFT = 3;
  var KEY_RIGHT = 4;
  var KEY_SELECT = 5;
  var KEY_BACK = 6;

  var CMD_MENU = 1;
  var CMD_SETTINGS = 2;
  var CMD_OVERLAY = 3;
  var CMD_NEXT = 4;
  var CMD_HOME = 5;

  var bezelEl = document.getElementById("bezel");
  var canvas = document.getElementById("matrix");
  var gridCanvas = document.getElementById("led-grid");
  var loadingEl = document.getElementById("emu-loading");
  var cockpitEl = document.getElementById("cockpit");
  var remoteDotEl = document.getElementById("remote-dot");
  var activeNameEl = document.getElementById("hud-scene");
  var paletteNameEl = document.getElementById("hud-palette");
  var nightBadgeEl = document.getElementById("hud-night");
  var nightBtnEl = document.getElementById("btn-night");
  var audioBtnEl = document.getElementById("btn-audio");
  var gridBtnEl = document.getElementById("btn-grid");
  var weatherBtnEl = document.getElementById("btn-weather");
  var quickPillsEl = document.getElementById("quick-pills");

  if (!canvas || !cockpitEl) return;

  var ctx2d = canvas.getContext("2d", { alpha: false });
  var offscreen = document.createElement("canvas");
  offscreen.width = MATRIX_W;
  offscreen.height = MATRIX_H;
  var offCtx = offscreen.getContext("2d", { alpha: false });
  var imgData = offCtx.createImageData(MATRIX_W, MATRIX_H);

  // Subtle HUB75 LED matrix pitch mask drawn once onto #led-grid
  function initGridMask() {
    if (!gridCanvas) return;
    var g = gridCanvas.getContext("2d");
    var w = gridCanvas.width;
    var h = gridCanvas.height;
    var cell = w / MATRIX_W; // 4px per LED at 512x512
    g.clearRect(0, 0, w, h);
    g.fillStyle = "rgba(0, 0, 0, 0.72)";
    for (var i = 0; i < w; i += cell) {
      g.fillRect(i + cell - 1, 0, 1, h);
    }
    for (var j = 0; j < h; j += cell) {
      g.fillRect(0, j + cell - 1, w, 1);
    }
  }
  initGridMask();

  // Web Audio synthesizer matching SimAudio in sim/main.cpp
  var audioEnabled = false;
  var audioVolume = 20;
  var audioCtx = null;

  var CUE_MAP = {
    1: [880, 90, false],    // Chime
    2: [660, 170, false],   // StartBell
    3: [523, 240, false],   // FinishBell
    4: [988, 110, false],   // Halfway
    5: [1175, 35, true],    // Tick
    6: [523, 220, false],   // BreatheIn
    7: [440, 90, false],    // BreatheHold
    8: [392, 280, false],   // BreatheOut
    9: [190, 55, true],     // Pop
    11: [1046, 45, false],  // Eat
    12: [320, 340, true],   // Die
    13: [640, 25, false],   // Bounce
    14: [784, 140, false]   // Score
  };

  function ensureAudioCtx() {
    if (!audioCtx && (window.AudioContext || window.webkitAudioContext)) {
      var Ctor = window.AudioContext || window.webkitAudioContext;
      audioCtx = new Ctor();
    }
    if (audioCtx && audioCtx.state === "suspended") {
      audioCtx.resume().catch(function () {});
    }
  }

  function playTone(freq, ms, descend) {
    if (!audioEnabled || audioVolume <= 0) return;
    ensureAudioCtx();
    if (!audioCtx) return;

    var now = audioCtx.currentTime;
    var dur = ms / 1000;
    var osc = audioCtx.createOscillator();
    var gain = audioCtx.createGain();
    var peak = Math.min(0.25, (audioVolume / 30) * 0.22);

    osc.type = "sine";
    osc.frequency.setValueAtTime(freq, now);
    if (descend) {
      osc.frequency.linearRampToValueAtTime(freq * 0.4, now + dur);
    }

    gain.gain.setValueAtTime(peak, now);
    gain.gain.linearRampToValueAtTime(0.0001, now + dur);

    osc.connect(gain);
    gain.connect(audioCtx.destination);
    osc.start(now);
    osc.stop(now + dur + 0.01);
  }

  window._coriolisAudio = {
    play: function (cueId) {
      var spec = CUE_MAP[cueId];
      if (spec) playTone(spec[0], spec[1], spec[2]);
    },
    voice: function () {
      playTone(700, 70, false);
    },
    loop: function () {},
    setVolume: function (v) {
      audioVolume = v;
    }
  };

  var mod = null;
  var rgbaPtr = 0;
  var startPerf = performance.now();
  var lastHudScene = "";
  var lastHudPalette = "";
  var lastNight = -1;
  var lastNightEnabled = -1;
  var dotTimer = 0;

  function blinkIrLed() {
    if (!remoteDotEl) return;
    remoteDotEl.classList.add("lit");
    clearTimeout(dotTimer);
    dotTimer = setTimeout(function () {
      remoteDotEl.classList.remove("lit");
    }, 140);
  }

  function syncHud() {
    if (!mod) return;
    var active = mod.UTF8ToString(mod._coriolis_active_name());
    var palette = mod.UTF8ToString(mod._coriolis_palette_name());
    var isNight = mod._coriolis_is_night();
    var nightEnabled = mod._coriolis_night_enabled();

    if (active !== lastHudScene) {
      lastHudScene = active;
      if (activeNameEl) activeNameEl.textContent = active;
      if (weatherBtnEl) weatherBtnEl.hidden = active !== "Weather";
      updateQuickPills(active);
    }
    if (palette !== lastHudPalette) {
      lastHudPalette = palette;
      if (paletteNameEl) paletteNameEl.textContent = palette;
    }
    if (isNight !== lastNight) {
      lastNight = isNight;
      if (nightBadgeEl) nightBadgeEl.hidden = !isNight;
    }
    if (nightEnabled !== lastNightEnabled) {
      lastNightEnabled = nightEnabled;
      if (nightBtnEl) nightBtnEl.setAttribute("aria-pressed", nightEnabled ? "true" : "false");
    }
  }

  function paintFrame() {
    if (!mod || !rgbaPtr) return;
    var slice = mod.HEAPU8.subarray(rgbaPtr, rgbaPtr + PIXEL_BYTES);
    imgData.data.set(slice);
    offCtx.putImageData(imgData, 0, 0);
    ctx2d.imageSmoothingEnabled = false;
    ctx2d.drawImage(offscreen, 0, 0, canvas.width, canvas.height);
  }

  function renderQuickPills() {
    if (!quickPillsEl || !mod) return;
    var featured = [
      [0, "Clock"],
      [1, "Analog"],
      [2, "Word Clock"],
      [3, "Weather"],
      [4, "Calendar"],
      [9, "Yoga"],
      [10, "Exercise"],
      [11, "Breathe"],
      [12, "Focus"],
      [8, "Labyrinth"],
      [6, "Snake"],
      [7, "Tetris"],
      [5, "Pong"],
      [24, "Aurora"],
      [17, "Lava"],
      [23, "Fireplace"],
      [22, "Aquarium"],
      [15, "Mandala"],
      [14, "Spiro"],
      [16, "Coriolis"],
      [21, "Life"],
      [20, "Starfield"]
    ];
    quickPillsEl.innerHTML = "";
    featured.forEach(function (pair) {
      var btn = document.createElement("button");
      btn.type = "button";
      btn.className = "scene-pill";
      btn.dataset.sceneIndex = String(pair[0]);
      btn.dataset.sceneName = pair[1];
      btn.textContent = pair[1];
      btn.addEventListener("click", function () {
        blinkIrLed();
        mod._coriolis_show(pair[0]);
        syncHud();
      });
      quickPillsEl.appendChild(btn);
    });
  }

  function updateQuickPills(activeName) {
    if (!quickPillsEl) return;
    var buttons = quickPillsEl.querySelectorAll(".scene-pill");
    buttons.forEach(function (btn) {
      var match = btn.dataset.sceneName === activeName;
      btn.setAttribute("aria-current", match ? "true" : "false");
    });
  }

  function toggleAudio() {
    audioEnabled = !audioEnabled;
    if (audioEnabled) ensureAudioCtx();
    if (audioBtnEl) {
      audioBtnEl.setAttribute("aria-pressed", audioEnabled ? "true" : "false");
      audioBtnEl.textContent = audioEnabled ? "Sound: On" : "Sound: Off";
    }
  }

  function toggleNight() {
    if (!mod) return;
    blinkIrLed();
    mod._coriolis_toggle_night();
    syncHud();
  }

  function toggleGrid() {
    if (!bezelEl || !gridBtnEl) return;
    var hidden = bezelEl.classList.toggle("no-grid");
    gridBtnEl.setAttribute("aria-pressed", hidden ? "false" : "true");
  }

  function sendKey(keyId) {
    if (!mod) return 0;
    blinkIrLed();
    return mod._coriolis_key(keyId);
  }

  function sendCmd(cmdId) {
    if (!mod) return;
    blinkIrLed();
    mod._coriolis_cmd(cmdId);
  }

  // Wire virtual IR remote buttons (tap + hold for Pong/Tetris/Labyrinth)
  var remoteButtons = cockpitEl.querySelectorAll("[data-key], [data-cmd], [data-action]");
  remoteButtons.forEach(function (btn) {
    if (btn.dataset.key) {
      var kId = parseInt(btn.dataset.key, 10);
      var release = function () {
        btn.classList.remove("is-down");
        if (mod) mod._coriolis_set_held(kId, 0);
      };
      btn.addEventListener("pointerdown", function (e) {
        e.preventDefault();
        btn.classList.add("is-down");
        if (audioEnabled) ensureAudioCtx();
        if (mod) {
          mod._coriolis_set_held(kId, 1);
          sendKey(kId);
          syncHud();
        }
      });
      btn.addEventListener("pointerup", release);
      btn.addEventListener("pointerleave", release);
      btn.addEventListener("pointercancel", release);
      // keyboard and assistive tech activate with a click and no pointer
      btn.addEventListener("click", function (e) {
        if (e.detail !== 0) return;
        if (audioEnabled) ensureAudioCtx();
        sendKey(kId);
        syncHud();
      });
    } else if (btn.dataset.cmd) {
      var cId = parseInt(btn.dataset.cmd, 10);
      btn.addEventListener("click", function () {
        if (audioEnabled) ensureAudioCtx();
        sendCmd(cId);
        syncHud();
      });
    } else if (btn.dataset.action) {
      var act = btn.dataset.action;
      btn.addEventListener("click", function () {
        if (audioEnabled) ensureAudioCtx();
        if (act === "rotate" && mod) {
          blinkIrLed();
          mod._coriolis_rotate();
        } else if (act === "night") {
          toggleNight();
        } else if (act === "audio") {
          toggleAudio();
        } else if (act === "grid") {
          toggleGrid();
        } else if (act === "weather" && mod) {
          blinkIrLed();
          mod._coriolis_cycle_weather();
        }
        syncHud();
      });
    }
  });

  if (bezelEl) {
    bezelEl.addEventListener("click", function () {
      bezelEl.focus({ preventScroll: true });
    });
  }

  // Keyboard bindings operate cleanly only while the cockpit has focus,
  // avoiding hijacking arrow/space scrolling on the rest of the page.
  var KEY_TO_ID = {
    ArrowUp: KEY_UP,
    ArrowDown: KEY_DOWN,
    ArrowLeft: KEY_LEFT,
    ArrowRight: KEY_RIGHT,
    Enter: KEY_SELECT,
    Backspace: KEY_BACK,
    Escape: KEY_BACK
  };

  // After a mouse or touch click, hand focus back to the display so the
  // keyboard keeps driving it; keyboard users keep their place.
  cockpitEl.addEventListener("click", function (e) {
    if (e.detail !== 0 && bezelEl && e.target.closest("button")) {
      bezelEl.focus({ preventScroll: true });
    }
  });

  // A keyup or pointerup can be lost while the page is in the background.
  function releaseAllHeld() {
    remoteButtons.forEach(function (btn) {
      btn.classList.remove("is-down");
    });
    if (!mod) return;
    for (var k = KEY_UP; k <= KEY_BACK; k++) mod._coriolis_set_held(k, 0);
  }
  window.addEventListener("blur", releaseAllHeld);
  document.addEventListener("visibilitychange", function () {
    if (document.hidden) releaseAllHeld();
  });

  document.addEventListener("keydown", function (e) {
    if (!mod || !cockpitEl.contains(document.activeElement)) return;
    if (e.metaKey || e.ctrlKey || e.altKey) return;

    // Enter and Space belong to a focused button
    if ((e.code === "Enter" || e.code === "Space") &&
        document.activeElement.closest("button")) return;

    var keyId = KEY_TO_ID[e.code];
    if (keyId) {
      e.preventDefault();
      if (audioEnabled) ensureAudioCtx();
      mod._coriolis_set_held(keyId, 1);
      if (!e.repeat) {
        sendKey(keyId);
        syncHud();
      }
      return;
    }

    if (e.repeat) return;
    switch (e.code) {
      case "KeyM":
        e.preventDefault();
        sendCmd(CMD_MENU);
        break;
      case "KeyS":
        e.preventDefault();
        sendCmd(CMD_SETTINGS);
        break;
      case "KeyC":
        e.preventDefault();
        sendCmd(CMD_OVERLAY);
        break;
      case "Space":
        e.preventDefault();
        sendCmd(CMD_NEXT);
        break;
      case "KeyH":
        e.preventDefault();
        sendCmd(CMD_HOME);
        break;
      case "KeyR":
        e.preventDefault();
        blinkIrLed();
        mod._coriolis_rotate();
        break;
      case "KeyN":
        e.preventDefault();
        toggleNight();
        break;
      case "KeyA":
        e.preventDefault();
        toggleAudio();
        break;
      default:
        return;
    }
    syncHud();
  });

  document.addEventListener("keyup", function (e) {
    if (!mod) return;
    var keyId = KEY_TO_ID[e.code];
    if (keyId) mod._coriolis_set_held(keyId, 0);
  });

  window.coriolisJumpTo = function (sceneIndex) {
    if (!mod) return;
    if (audioEnabled) ensureAudioCtx();
    blinkIrLed();
    mod._coriolis_show(sceneIndex);
    syncHud();
    if (bezelEl) bezelEl.focus({ preventScroll: true });
  };

  function loop(nowPerf) {
    if (mod) {
      var d = new Date();
      mod._coriolis_set_time(
        d.getFullYear(),
        d.getMonth() + 1,
        d.getDate(),
        d.getHours(),
        d.getMinutes(),
        d.getSeconds()
      );
      var elapsed = Math.max(0, Math.floor(nowPerf - startPerf));
      if (mod._coriolis_tick(elapsed)) {
        paintFrame();
      }
      syncHud();
    }
    requestAnimationFrame(loop);
  }

  function boot() {
    if (typeof window.CoriolisModule !== "function") {
      if (loadingEl) loadingEl.textContent = "Failed to load emu/coriolis.js";
      return;
    }
    window
      .CoriolisModule({
        locateFile: function (path) {
          return "./emu/" + path;
        }
      })
      .then(function (instance) {
        mod = instance;
        startPerf = performance.now();
        var d = new Date();
        mod._coriolis_set_time(
          d.getFullYear(),
          d.getMonth() + 1,
          d.getDate(),
          d.getHours(),
          d.getMinutes(),
          d.getSeconds()
        );
        mod._coriolis_init(1);
        rgbaPtr = mod._coriolis_rgba_ptr();
        paintFrame();
        renderQuickPills();
        syncHud();
        if (loadingEl) loadingEl.hidden = true;
        requestAnimationFrame(loop);
      })
      .catch(function (err) {
        console.error("Coriolis WASM boot error:", err);
        if (loadingEl) loadingEl.textContent = "WASM failed to initialize.";
      });
  }

  boot();
})();
