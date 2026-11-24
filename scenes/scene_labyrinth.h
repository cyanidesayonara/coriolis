// Labyrinth — a first-person raycaster maze, the display's most ambitious
// scene. A maze is carved fresh each time (recursive backtracker), rendered
// column by column in the Wolfenstein style: cast a ray per screen column,
// march the grid until it hits a wall, draw a vertical strip whose height is
// the inverse of the distance with subtle stone panel seams and trim. Walls
// take the theme palette and fade with range; the exit is a pulsing gold
// pillar. Arrows walk and turn; OK opens a pause/exit menu; Back returns to
// the setup card; left idle, the view wanders the maze on its own.
#ifndef CORIOLIS_SCENE_LABYRINTH_H
#define CORIOLIS_SCENE_LABYRINTH_H

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/font5x7.h"
#include "../core/math8.h"
#include "intro.h"

namespace coriolis {

class LabyrinthScene : public Scene {
 public:
  const char* name() const { return "Labyrinth"; }
  bool autoplayEligible() const { return false; }

  void start(Context&) {
    started_ = false;
    paused_ = false;
    depth_ = 1;
    generate();
  }

  bool input(Context& ctx, Key k) {
    if (!started_) {
      if (k == Key::Select) {
        started_ = true;
        paused_ = false;
        lastInputMs_ = ctx.nowMs;
        return true;
      }
      if (k == Key::Up || k == Key::Down) {
        sizeIdx_ = (sizeIdx_ + (k == Key::Up ? 1 : 2)) % 3;
        generate();
        return true;
      }
      return false;
    }

    if (paused_) {
      switch (k) {
        case Key::Up:
          pauseRow_ = (pauseRow_ + 2) % 3;
          return true;
        case Key::Down:
          pauseRow_ = (pauseRow_ + 1) % 3;
          return true;
        case Key::Left:
        case Key::Right:
          return true;
        case Key::Back:
          paused_ = false;
          lastInputMs_ = ctx.nowMs;
          return true;
        case Key::Select:
          if (pauseRow_ == 0) {
            paused_ = false;
            lastInputMs_ = ctx.nowMs;
          } else if (pauseRow_ == 1) {
            paused_ = false;
            depth_ = 1;
            generate();
            lastInputMs_ = ctx.nowMs;
          } else {
            paused_ = false;
            started_ = false;
          }
          return true;
        default:
          return false;
      }
    }

    if (k == Key::Back) {
      started_ = false;
      return true;
    }
    if (k == Key::Select) {
      paused_ = true;
      pauseRow_ = 0;
      return true;
    }
    // consume directional keys while exploring, so steering into a wall
    // never leaks out to switch scenes
    if (k == Key::Up || k == Key::Down || k == Key::Left || k == Key::Right) {
      lastInputMs_ = ctx.nowMs;
      return true;
    }
    return false;
  }

  uint32_t draw(Context& ctx) {
    if (!started_) {
      static const char* sizes[3] = {"SMALL 13X13", "NORMAL 19X19",
                                     "LARGE 25X25"};
      const char* lines[] = {sizes[sizeIdx_], "OK MENU  BACK EXIT"};
      intro::draw(ctx, "LABYRINTH", lines, 2, RGB(175, 150, 245));
      return 60;
    }

    if (!paused_) stepMovement(ctx);
    renderView(ctx);
    drawMinimap(ctx);
    drawDepth(ctx);
    if (paused_) drawPauseMenu(ctx);
    return 24;
  }

 private:
  static const int MAX_N = 25;  // odd square maze grid up to 25x25
  uint8_t map_[MAX_N][MAX_N];   // 0 empty, 1 wall, 2 exit pillar
  bool visited_[MAX_N][MAX_N];
  int sizeIdx_ = 1;
  int n_ = 19;
  float posX_ = 1.5f, posY_ = 1.5f, ang_ = 0.0f;
  float turnTarget_ = 0.0f;  // idle auto-walk heading
  int exitX_ = 17, exitY_ = 17;
  int depth_ = 1;
  bool started_ = false;
  bool paused_ = false;
  int pauseRow_ = 0;
  uint32_t lastInputMs_ = 0;

  bool wall(int x, int y) const {
    if (x < 0 || y < 0 || x >= n_ || y >= n_) return true;
    return map_[y][x] == 1;
  }

  void revealAround(int px, int py) {
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        int wx = px + dx, wy = py + dy;
        if (wx >= 0 && wy >= 0 && wx < n_ && wy < n_) visited_[wy][wx] = true;
      }
    }
  }

  void generate() {
    n_ = 13 + sizeIdx_ * 6;
    if (n_ > MAX_N) n_ = MAX_N;

    for (int y = 0; y < n_; y++) {
      for (int x = 0; x < n_; x++) {
        map_[y][x] = 1;
        visited_[y][x] = false;
      }
    }

    // recursive backtracker over the odd cells, walls carved between them
    int stackX[MAX_N * MAX_N], stackY[MAX_N * MAX_N], sp = 0;
    map_[1][1] = 0;
    stackX[sp] = 1; stackY[sp] = 1; sp++;
    while (sp > 0) {
      int cx = stackX[sp - 1], cy = stackY[sp - 1];
      int dirs[4] = {0, 1, 2, 3};
      for (int i = 3; i > 0; i--) {  // shuffle directions
        int j = randomInt(i + 1);
        int t = dirs[i]; dirs[i] = dirs[j]; dirs[j] = t;
      }
      bool carved = false;
      for (int d = 0; d < 4; d++) {
        int nx = cx, ny = cy;
        if (dirs[d] == 0) ny -= 2;
        else if (dirs[d] == 1) nx += 2;
        else if (dirs[d] == 2) ny += 2;
        else nx -= 2;
        if (nx <= 0 || ny <= 0 || nx >= n_ - 1 || ny >= n_ - 1) continue;
        if (map_[ny][nx] == 0) continue;
        map_[ny][nx] = 0;
        map_[(cy + ny) / 2][(cx + nx) / 2] = 0;  // knock out the shared wall
        stackX[sp] = nx; stackY[sp] = ny; sp++;
        carved = true;
        break;
      }
      if (!carved) sp--;
    }

    exitX_ = n_ - 2; exitY_ = n_ - 2;
    map_[exitY_][exitX_] = 2;

    posX_ = 1.5f; posY_ = 1.5f;
    // face the first open neighbor so the scene opens onto a corridor
    if (!wall(2, 1)) ang_ = 0.0f;
    else if (!wall(1, 2)) ang_ = 1.5708f;
    else ang_ = 0.0f;
    turnTarget_ = ang_;
    revealAround(int(posX_), int(posY_));
  }

  void moveWithCollision(float dx, float dy) {
    const float m = 0.2f;  // keep a body-width off the walls
    float nx = posX_ + dx;
    if (!wall(int(nx + (dx > 0 ? m : -m)), int(posY_))) posX_ = nx;
    float ny = posY_ + dy;
    if (!wall(int(posX_), int(ny + (dy > 0 ? m : -m)))) posY_ = ny;
  }

  void stepMovement(Context& ctx) {
    const float moveSpd = 0.06f, turnSpd = 0.05f;
    float dirX = cosf(ang_), dirY = sinf(ang_);
    bool idle = ctx.nowMs - lastInputMs_ > 6000;

    if (!idle) {
      if (ctx.held.isDown(Key::Up)) moveWithCollision(dirX * moveSpd, dirY * moveSpd);
      if (ctx.held.isDown(Key::Down)) moveWithCollision(-dirX * moveSpd, -dirY * moveSpd);
      if (ctx.held.isDown(Key::Left)) ang_ -= turnSpd;
      if (ctx.held.isDown(Key::Right)) ang_ += turnSpd;
    } else {
      autoWalk(dirX, dirY, moveSpd, turnSpd);
    }
    revealAround(int(posX_), int(posY_));

    // reached the exit pillar: deeper we go, with a fresh maze
    float ex = posX_ - (exitX_ + 0.5f), ey = posY_ - (exitY_ + 0.5f);
    if (ex * ex + ey * ey < 1.4f) {
      depth_++;
      ctx.audio.play(Cue::Score);
      generate();
    }
  }

  // idle behaviour: roll forward, turn toward an opening at dead ends, so the
  // maze is never a static wall on the wall
  void autoWalk(float dirX, float dirY, float moveSpd, float turnSpd) {
    float diff = turnTarget_ - ang_;
    while (diff > 3.14159f) diff -= TWO_PI_F;
    while (diff < -3.14159f) diff += TWO_PI_F;
    if (diff > 0.03f || diff < -0.03f) {
      ang_ += (diff > 0 ? turnSpd : -turnSpd);  // finish the turn first
      return;
    }
    float ax = posX_ + dirX * 0.6f, ay = posY_ + dirY * 0.6f;
    if (wall(int(ax), int(ay))) {
      float lx = posX_ + cosf(ang_ - 1.5708f) * 0.6f;
      float ly = posY_ + sinf(ang_ - 1.5708f) * 0.6f;
      bool leftOpen = !wall(int(lx), int(ly));
      turnTarget_ = ang_ + (leftOpen ? -1.5708f : 1.5708f);
    } else {
      moveWithCollision(dirX * moveSpd * 0.7f, dirY * moveSpd * 0.7f);
    }
  }

  void renderView(Context& ctx) {
    const int W = ctx.fb.width(), H = ctx.fb.height();
    const int horizon = H / 2;
    float dirX = cosf(ang_), dirY = sinf(ang_);
    const float fov = 0.66f;
    float planeX = -dirY * fov, planeY = dirX * fov;

    // ceiling and floor: deep indigo sky gradient above, warm stone floor below
    for (int y = 0; y < H; y++) {
      RGB c;
      if (y < horizon) {
        uint8_t v = uint8_t(4 + (horizon - y) * 24 / (horizon + 1));
        c = RGB(v / 3, v / 3, uint8_t(v * 2 / 3));
      } else {
        uint8_t v = uint8_t(5 + (y - horizon) * 26 / (horizon + 1));
        c = RGB(uint8_t(v * 5 / 9), uint8_t(v / 2), uint8_t(v * 4 / 9));
      }
      ctx.fb.hLine(0, W - 1, y, c);
    }

    for (int x = 0; x < W; x++) {
      float cameraX = 2.0f * x / W - 1.0f;
      float rayX = dirX + planeX * cameraX;
      float rayY = dirY + planeY * cameraX;

      int mapX = int(posX_), mapY = int(posY_);
      float deltaX = (rayX == 0.0f) ? 1e30f : fabsf(1.0f / rayX);
      float deltaY = (rayY == 0.0f) ? 1e30f : fabsf(1.0f / rayY);
      int stepX, stepY;
      float sideDistX, sideDistY;
      if (rayX < 0) { stepX = -1; sideDistX = (posX_ - mapX) * deltaX; }
      else { stepX = 1; sideDistX = (mapX + 1.0f - posX_) * deltaX; }
      if (rayY < 0) { stepY = -1; sideDistY = (posY_ - mapY) * deltaY; }
      else { stepY = 1; sideDistY = (mapY + 1.0f - posY_) * deltaY; }

      int side = 0, hitVal = 1;
      for (int guard = 0; guard < 64; guard++) {
        if (sideDistX < sideDistY) { sideDistX += deltaX; mapX += stepX; side = 0; }
        else { sideDistY += deltaY; mapY += stepY; side = 1; }
        if (mapX < 0 || mapY < 0 || mapX >= n_ || mapY >= n_) break;
        if ((x & 3) == 0 && guard < 5) visited_[mapY][mapX] = true;
        if (map_[mapY][mapX] > 0) { hitVal = map_[mapY][mapX]; break; }
      }

      float perp = (side == 0) ? (sideDistX - deltaX) : (sideDistY - deltaY);
      if (perp < 0.05f) perp = 0.05f;
      float wallX = (side == 0) ? (posY_ + perp * rayY) : (posX_ + perp * rayX);
      wallX -= floorf(wallX);

      int lineH = int(H / perp);
      if (lineH < 1) lineH = 1;
      int topY = horizon - lineH / 2;
      int y0 = topY < 0 ? 0 : topY;
      int y1 = topY + lineH - 1;
      if (y1 >= H) y1 = H - 1;

      RGB base;
      if (hitVal == 2) {  // exit pillar, pulsing gold with luminous core
        uint8_t pulse = sin8(uint8_t(ctx.nowMs / 4));
        base = RGB(255, uint8_t(175 + pulse / 4), 45);
        uint8_t sh = uint8_t(255.0f / (1.0f + perp * 0.25f));
        base.dim(sh < 95 ? 95 : sh);
      } else {
        base = ctx.palette->lookupBright(uint8_t(90 + (side ? 24 : 0)), 150);
        uint8_t sh = uint8_t(255.0f / (1.0f + perp * perp * 0.15f));
        if (side == 1) sh = scale8(sh, 195);  // shade N/S faces darker
        base.dim(sh);
      }

      float invH = 1.0f / float(lineH);
      for (int y = y0; y <= y1; y++) {
        float texY = (y - topY) * invH;
        RGB col = base;
        if (hitVal == 2) {
          // glowing vertical core + warm energy bands
          float core = 1.0f - fabsf(wallX - 0.5f) * 1.4f;
          if (core < 0.0f) core = 0.0f;
          uint8_t glow = uint8_t(165 + core * 90);
          col.dim(glow);
          if (fabsf(texY - 0.5f) < 0.18f && core > 0.4f) {
            col = lerpColor(col, RGB(255, 250, 210), 110);
          }
        } else {
          // architectural top/bottom trim and subtle staggered ashlar seams
          bool trim = texY < 0.055f || texY > 0.945f;
          float row = texY * 2.0f;
          bool hSeam = fabsf(texY - 0.5f) < 0.025f;
          float shiftedX = row < 1.0f ? wallX : fmodf(wallX + 0.5f, 1.0f);
          bool vSeam = shiftedX < 0.032f || shiftedX > 0.968f;
          if (trim) col.dim(155);
          else if (hSeam || vSeam) col.dim(195);
        }
        ctx.fb.set(x, y, col);
      }
    }
  }

  // a windowed minimap centered on the player — explored corridors shine
  // brighter than unseen cells, with the exit and facing dot
  void drawMinimap(Context& ctx) {
    const int cells = ctx.fb.height() >= 100 ? 15 : 11;
    const int s = 2, mx0 = 3, my0 = 3, half = cells / 2;
    int px = int(posX_), py = int(posY_);
    int mw = cells * s, mh = cells * s;
    ctx.fb.rect(mx0 - 1, my0 - 1, mw + 2, mh + 2, RGB(34, 36, 50));
    for (int j = 0; j < cells; j++) {
      for (int i = 0; i < cells; i++) {
        int wx = px - half + i, wy = py - half + j;
        RGB c;
        if (wx < 0 || wy < 0 || wx >= n_ || wy >= n_) {
          c = RGB(4, 4, 8);
        } else if (map_[wy][wx] == 2) {
          c = RGB(240, 185, 45);
        } else if (visited_[wy][wx]) {
          c = map_[wy][wx] == 1 ? RGB(74, 78, 106) : RGB(18, 20, 30);
        } else {
          c = map_[wy][wx] == 1 ? RGB(28, 30, 44) : RGB(6, 6, 10);
        }
        ctx.fb.rect(mx0 + i * s, my0 + j * s, s, s, c);
      }
    }
    int pcx = mx0 + half * s, pcy = my0 + half * s;
    ctx.fb.rect(pcx, pcy, s, s, RGB(120, 240, 130));
    ctx.fb.set(pcx + int(cosf(ang_) * 3) + 1, pcy + int(sinf(ang_) * 3) + 1,
               RGB(215, 255, 215));
  }

  void drawDepth(Context& ctx) {
    char buf[8];
    snprintf(buf, sizeof(buf), "D%d", depth_);
    font3x5::drawText(ctx.fb, buf, ctx.fb.width() - font3x5::textWidth(buf, 1) - 3,
                      3, 1, RGB(185, 175, 218));
  }

  void drawPauseMenu(Context& ctx) {
    const int W = ctx.fb.width(), H = ctx.fb.height();
    int bw = W - 36;
    if (bw > 88) bw = 88;
    bool compact = H < 100;
    int bh = compact ? 42 : 48;
    int bx = (W - bw) / 2, by = (H - bh) / 2;

    for (int y = by; y < by + bh; y++)
      for (int x = bx; x < bx + bw; x++) ctx.fb.at(x, y).dim(45);

    RGB border(155, 135, 225);
    border.dim(150);
    ctx.fb.hLine(bx, bx + bw - 1, by, border);
    ctx.fb.hLine(bx, bx + bw - 1, by + bh - 1, border);
    ctx.fb.vLine(bx, by, by + bh - 1, border);
    ctx.fb.vLine(bx + bw - 1, by, by + bh - 1, border);

    font5x7::drawCentered(ctx.fb, "Paused", W / 2, by + 3, 1,
                          RGB(215, 200, 255));

    static const char* opts[3] = {"Resume", "New maze", "Exit"};
    int rowH = compact ? 9 : 11;
    int y0 = by + (compact ? 13 : 15);
    for (int i = 0; i < 3; i++) {
      int ry = y0 + i * rowH;
      if (i == pauseRow_) {
        RGB hi(140, 120, 220);
        hi.dim(110);
        ctx.fb.rect(bx + 4, ry - 1, bw - 8, rowH - 1, hi);
      }
      RGB tc = i == pauseRow_ ? RGB(255, 255, 255) : RGB(155, 155, 168);
      font5x7::drawCentered(ctx.fb, opts[i], W / 2, ry, 1, tc);
    }
  }
};

}

#endif
