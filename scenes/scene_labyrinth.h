// Labyrinth — a first-person raycaster maze, the display's most ambitious
// scene. A maze is carved fresh each time (recursive backtracker), rendered
// column by column in the Wolfenstein style: cast a ray per screen column,
// march the grid until it hits a wall, draw a vertical strip whose height is
// the inverse of the distance. Walls take the theme palette and fade with
// range; the exit is a pulsing gold pillar. Arrows walk and turn; left idle,
// the view wanders the maze on its own so it still lives on the wall.
#ifndef CORIOLIS_SCENE_LABYRINTH_H
#define CORIOLIS_SCENE_LABYRINTH_H

#include <math.h>
#include <stdio.h>

#include "../core/scene.h"
#include "../core/font.h"
#include "../core/math8.h"
#include "intro.h"

namespace coriolis {

class LabyrinthScene : public Scene {
 public:
  const char* name() const { return "Labyrinth"; }
  bool autoplayEligible() const { return false; }

  void start(Context& ctx) {
    started_ = false;
    depth_ = 1;
    generate();
  }

  bool input(Context& ctx, Key k) {
    if (!started_) {
      if (k == Key::Select) {
        started_ = true;
        lastInputMs_ = ctx.nowMs;
        return true;
      }
      return false;
    }
    // consume every control key while exploring, so steering into a wall
    // never leaks out to switch scenes
    if (k == Key::Up || k == Key::Down || k == Key::Left || k == Key::Right ||
        k == Key::Select) {
      lastInputMs_ = ctx.nowMs;
      return true;
    }
    return false;  // Back still leaves
  }

  uint32_t draw(Context& ctx) {
    if (!started_) {
      const char* lines[] = {"FIND THE EXIT", "ARROWS TO MOVE"};
      intro::draw(ctx, "LABYRINTH", lines, 2, RGB(175, 150, 245));
      return 60;
    }

    stepMovement(ctx);
    renderView(ctx);
    drawMinimap(ctx);
    drawDepth(ctx);
    return 24;
  }

 private:
  static const int N = 21;  // odd square maze grid
  uint8_t map_[N][N];       // 0 empty, 1 wall, 2 exit pillar
  float posX_ = 1.5f, posY_ = 1.5f, ang_ = 0.0f;
  float turnTarget_ = 0.0f;  // idle auto-walk heading
  int exitX_ = N - 2, exitY_ = N - 2;
  int depth_ = 1;
  bool started_ = false;
  uint32_t lastInputMs_ = 0;

  bool wall(int x, int y) const {
    if (x < 0 || y < 0 || x >= N || y >= N) return true;
    return map_[y][x] == 1;
  }

  void generate() {
    for (int y = 0; y < N; y++)
      for (int x = 0; x < N; x++) map_[y][x] = 1;

    // recursive backtracker over the odd cells, walls carved between them
    int stackX[N * N], stackY[N * N], sp = 0;
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
        if (nx <= 0 || ny <= 0 || nx >= N - 1 || ny >= N - 1) continue;
        if (map_[ny][nx] == 0) continue;
        map_[ny][nx] = 0;
        map_[(cy + ny) / 2][(cx + nx) / 2] = 0;  // knock out the shared wall
        stackX[sp] = nx; stackY[sp] = ny; sp++;
        carved = true;
        break;
      }
      if (!carved) sp--;
    }

    exitX_ = N - 2; exitY_ = N - 2;
    map_[exitY_][exitX_] = 2;

    posX_ = 1.5f; posY_ = 1.5f;
    // face the first open neighbor so the scene opens onto a corridor
    if (!wall(2, 1)) ang_ = 0.0f;
    else if (!wall(1, 2)) ang_ = 1.5708f;
    else ang_ = 0.0f;
    turnTarget_ = ang_;
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

    // ceiling and floor: a cool gradient above the horizon, gray below
    for (int y = 0; y < H; y++) {
      RGB c;
      if (y < horizon) {
        uint8_t v = uint8_t(6 + (horizon - y) * 22 / (horizon + 1));
        c = RGB(v / 3, v / 3, v / 2);
      } else {
        uint8_t v = uint8_t(6 + (y - horizon) * 22 / (horizon + 1));
        c = RGB(v / 2, v / 2, v / 2);
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
        if (mapX < 0 || mapY < 0 || mapX >= N || mapY >= N) break;
        if (map_[mapY][mapX] > 0) { hitVal = map_[mapY][mapX]; break; }
      }

      float perp = (side == 0) ? (sideDistX - deltaX) : (sideDistY - deltaY);
      if (perp < 0.05f) perp = 0.05f;
      int lineH = int(H / perp);
      int y0 = horizon - lineH / 2, y1 = horizon + lineH / 2;
      if (y0 < 0) y0 = 0;
      if (y1 >= H) y1 = H - 1;

      RGB col;
      if (hitVal == 2) {  // exit pillar, pulsing gold, ignores distance shade
        uint8_t pulse = sin8(uint8_t(ctx.nowMs / 4));
        col = RGB(255, uint8_t(170 + pulse / 4), 40);
        uint8_t sh = uint8_t(255.0f / (1.0f + perp * 0.25f));
        col.dim(sh < 90 ? 90 : sh);
      } else {
        col = ctx.palette->lookupBright(uint8_t(90 + (side ? 24 : 0)), 150);
        uint8_t sh = uint8_t(255.0f / (1.0f + perp * perp * 0.15f));
        if (side == 1) sh = scale8(sh, 200);  // shade the N/S faces darker
        col.dim(sh);
      }
      ctx.fb.vLine(x, y0, y1, col);
    }
  }

  // a windowed minimap centered on the player — walls, the exit, a facing dot
  void drawMinimap(Context& ctx) {
    const int cells = 15, s = 2, mx0 = 3, my0 = 3, half = cells / 2;
    int px = int(posX_), py = int(posY_);
    ctx.fb.rect(mx0 - 1, my0 - 1, cells * s + 2, cells * s + 2, RGB(0, 0, 0));
    for (int j = 0; j < cells; j++) {
      for (int i = 0; i < cells; i++) {
        int wx = px - half + i, wy = py - half + j;
        RGB c;
        if (wx < 0 || wy < 0 || wx >= N || wy >= N) c = RGB(0, 0, 0);
        else if (map_[wy][wx] == 1) c = RGB(55, 55, 75);
        else if (map_[wy][wx] == 2) c = RGB(230, 175, 40);
        else c = RGB(12, 12, 18);
        ctx.fb.rect(mx0 + i * s, my0 + j * s, s, s, c);
      }
    }
    int pcx = mx0 + half * s, pcy = my0 + half * s;
    ctx.fb.rect(pcx, pcy, s, s, RGB(120, 235, 120));
    ctx.fb.set(pcx + int(cosf(ang_) * 3) + 1, pcy + int(sinf(ang_) * 3) + 1,
               RGB(210, 255, 210));
  }

  void drawDepth(Context& ctx) {
    char buf[8];
    snprintf(buf, sizeof(buf), "D%d", depth_);
    font3x5::drawText(ctx.fb, buf, ctx.fb.width() - 14, 3, 1, RGB(180, 170, 210));
  }
};

}

#endif
