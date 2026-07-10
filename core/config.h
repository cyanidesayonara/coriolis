// Display dimensions. The final form is decided: 128x128 square (four 64x64
// panels). A widescreen 128x64 build remains available via -DWIDE=ON for
// experiments — code must still work at any size, never hardcode.
#ifndef CORIOLIS_CONFIG_H
#define CORIOLIS_CONFIG_H

#ifndef CORIOLIS_WIDTH
#define CORIOLIS_WIDTH 128
#endif

#ifndef CORIOLIS_HEIGHT
#define CORIOLIS_HEIGHT 128
#endif

namespace coriolis {

static const int WIDTH = CORIOLIS_WIDTH;
static const int HEIGHT = CORIOLIS_HEIGHT;
static const int PIXELS = WIDTH * HEIGHT;
static const int CENTER_X = WIDTH / 2;
static const int CENTER_Y = HEIGHT / 2;

}

#endif
