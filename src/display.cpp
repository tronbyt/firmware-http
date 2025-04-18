#include "display.h"

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#ifdef TIDBYT_GEN2
  #define R1 5
  #define G1 23
  #define BL1  4
  #define R2 2
  #define G2 22
  #define BL2 32

  #define CH_A 25
  #define CH_B 21
  #define CH_C 26
  #define CH_D 19
  #define CH_E -1  // assign to pin 14 if using more than two panels

  #define LAT 18
  #define OE 27
  #define CLK 15
#elif defined(TRONBYT_S3)
  #define R1 4
  #define G1 6
  #define BL1 5
  #define R2 7
  #define G2 16
  #define BL2 15

  #define CH_A 17
  #define CH_B 18
  #define CH_C 8
  #define CH_D 3
  #define CH_E -1  // assign to pin 14 if using more than two panels

  #define LAT 9
  #define OE 10
  #define CLK 11
#elif defined(PIXOTICKER)
  #define R1 2
  #define G1 4
  #define BL1 15
  #define R2 16
  #define G2 17
  #define BL2 27
  #define CH_A 5
  #define CH_B 18
  #define CH_C 19
  #define CH_D 21
  #define CH_E 12
  #define CLK 22
  #define LAT 26
  #define OE 25
  #elif defined(MATRIXPORTALS3)
//                     R1, G1, B1, R2, G2, B2
// uint8_t rgbPins[] = {42, 41, 40, 38, 39, 37};
// uint8_t addrPins[] = {45, 36, 48, 35, 21};
// uint8_t clockPin = 2;
// uint8_t latchPin = 47;
// uint8_t oePin = 14;
  #define R1 42
  #define G1 41
  #define BL1 40
  #define R2 38
  #define G2 39
  #define BL2 37
  #define CH_A 45
  #define CH_B 36
  #define CH_C 48
  #define CH_D 35
  #define CH_E 21
  #define CLK 2
  #define LAT 47
  #define OE 14

#else // GEN1 from here down.
  #ifdef SWAP_COLORS
    #define R1 21
    #define G1 2
    #define BL1 22
    #define R2 23
    #define G2 4
    #define BL2 27
  #else
    #define R1 2
    #define G1 22
    #define BL1 21
    #define R2 4
    #define G2 27
    #define BL2 23
  #endif

#define CH_A 26
#define CH_B 5
#define CH_C 25
#define CH_D 18
#define CH_E -1  // assign to pin 14 if using more than two panels

#define LAT 19
#define OE 32
#define CLK 33
#endif

static MatrixPanel_I2S_DMA *_matrix;
static int _brightness = -1;

int display_initialize() {
  // Initialize the panel.
  HUB75_I2S_CFG::i2s_pins pins = {R1,   G1,   BL1,  R2,   G2,  BL2, CH_A,
                                  CH_B, CH_C, CH_D, CH_E, LAT, OE,  CLK};

  #ifdef NO_INVERT_CLOCK_PHASE
  bool invert_clock_phase = false;
  #else
  bool invert_clock_phase = true;
  #endif

  // HUB75_I2S_CFG mxconfig(64,                      // width
  //                        32,                      // height
  //                        1,                       // chain length
  //                        pins,                    // pin mapping
  //                        HUB75_I2S_CFG::FM6126A,  // driver chip
  //                        true,                    // double-buffering
  //                        HUB75_I2S_CFG::HZ_10M);

  HUB75_I2S_CFG mxconfig(64,                      // width
                         32,                      // height
                         1,                       // chain length
                         pins,                    // pin mapping
                         HUB75_I2S_CFG::FM6126A,  // driver chip
                         true,                    // double-buffering
                         HUB75_I2S_CFG::HZ_10M,   // clock speed
                         1,                       // latch blanking
                         invert_clock_phase       // invert clock phase
  );

  _matrix = new MatrixPanel_I2S_DMA(mxconfig);

  if (!_matrix->begin()) {
    return 1;
  }
  display_set_brightness(TIDBYT_DEFAULT_BRIGHTNESS);

  return 0;
}

void display_set_brightness(int b) {
  if (b != _brightness) {
    _brightness = b;
    _matrix->setBrightness8(b);
    _matrix->clearScreen();
  }
}

void display_shutdown() {
  _matrix->clearScreen();
  _matrix->stopDMAoutput();
}

void display_draw(const uint8_t *pix, int width, int height,
		  int channels, int ixR, int ixG, int ixB) {
  for (unsigned int i = 0; i < height; i++) {
    for (unsigned int j = 0; j < width; j++) {
      const uint8_t *p = &pix[(i * width + j) * channels];
      uint8_t r = p[ixR];
      uint8_t g = p[ixG];
      uint8_t b = p[ixB];

      _matrix->drawPixelRGB888(j, i, r, g, b);
    }
  }
  _matrix->flipDMABuffer();
}

void display_clear() { _matrix->clearScreen(); }
