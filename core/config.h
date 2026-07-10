// Display dimensions. Coriolis targets two shapes with identical panels:
// Stage A: 128x64 widescreen (default), Stage B: 128x128 square.
// All code must work at either size — override at build time, never hardcode.
#ifndef CORIOLIS_CONFIG_H
#define CORIOLIS_CONFIG_H

#ifndef CORIOLIS_WIDTH
#define CORIOLIS_WIDTH 128
#endif

#ifndef CORIOLIS_HEIGHT
#define CORIOLIS_HEIGHT 64
#endif

namespace coriolis {

static const int WIDTH = CORIOLIS_WIDTH;
static const int HEIGHT = CORIOLIS_HEIGHT;
static const int PIXELS = WIDTH * HEIGHT;
static const int CENTER_X = WIDTH / 2;
static const int CENTER_Y = HEIGHT / 2;

}

#endif
