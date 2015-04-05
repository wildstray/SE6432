/*
 * SE6432.h
 * defintions for Holtek SE6432 LED driver.
 */

#ifndef SE6432_h
#define SE6432_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

/* Arduino specific definitions */

#if defined(__AVR__)
#ifndef prog_uint8_t
#define prog_uint8_t const uint8_t
#endif
#ifndef prog_uint16_t
#define prog_uint16_t const uint16_t
#endif
#endif

/* Chipkit specific definitions */

#if defined(__PIC32MX__)
#endif

/* Leaflab Maple specific definitions */

#if defined (__ARMEL__)
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned char prog_uint8_t;
typedef unsigned int prog_uint16_t;
uint8_t inline pgm_read_byte_near(uint8_t *ptr) { return (uint8_t)*ptr; }
uint16_t inline pgm_read_word_near(uint16_t *ptr) { return (uint16_t)*ptr; }
#define PROGMEM __FLASH__
#define inline inline __attribute__((always_inline))
#endif

#define BLACK        0
#define GREEN        1
#define RED          2
#define ORANGE       3
#define RANDOMCOLOR  4
#define MULTICOLOR   8
#define BLINK       16

#define BOLD         1
#define UNDERLINE    2
#define ITALIC       4
#define PROPORTIONAL 8

#define LEFT         0
#define RIGHT        1
#define UP           0
#define DOWN         1

#define FONT_4x6     1
#define FONT_5x7     2
#define FONT_5x8     3
#define FONT_5x7W    4
//#define FONT_6x10    5
//#define FONT_6x12    6
//#define FONT_6x13    7
//#define FONT_6x13B   8
//#define FONT_6x13O   9
//#define FONT_6x9    10
//#define FONT_7x13   11
//#define FONT_7x13B  12
//#define FONT_7x13O  13
//#define FONT_7x14   14
//#define FONT_7x14B  15
#define FONT_8x8    16
#define FONT_8x13   17
#define FONT_8x13B  18
//#define FONT_8x13O  19
//#define FONT_9x15   20
//#define FONT_9x15B  21
#define FONT_8x16   22
#define FONT_8x16B  23

//#define FONT_8x13BK 118

/* USE_ASM experimental feature: use assembler for low level function, faster then C for bitbanging */
#define USE_ASM

/* assembler low level functions only for AVR */
#if defined (__ARMEL__) || defined (__PIC32MX__)
#undef USE_ASM
#endif

/* from Arduino >= 1.0 Print functions returns size_t instead of void */
#if defined(ARDUINO) && ARDUINO >= 100
#define PRINT_NEW
#endif

/* Chipkit and Leaflab Maple structure for defining a port (port register address and bit mask for pin) */
struct _port_t {
#if defined (__ARMEL__)
  gpio_dev *dev;
  uint8_t mask;
#elif defined (__PIC32MX__)
  volatile p32_ioport *regs;
  uint16_t mask;
#endif
};

class SE6432 : public Print {

#ifdef putchar
#undef putchar
#endif

public:
    SE6432(const uint8_t g1, const uint8_t g2, const uint8_t r1, const uint8_t r2, const uint8_t latch, const uint8_t clock, const uint8_t enable, const uint8_t abcd);
    SE6432(volatile uint8_t *port, const uint8_t g1, const uint8_t g2, const uint8_t r1, const uint8_t r2, const uint8_t latch, const uint8_t clock, const uint8_t enable, volatile uint8_t *prow, const uint8_t abcd);

    void pwm(uint8_t value);
    void sendframe();
    void clear();
    void plot(uint8_t x, uint8_t y, uint8_t color);
    uint8_t getpixel(uint8_t x, uint8_t y);
    uint8_t putchar(int x, int y, char c, uint8_t color = GREEN, uint8_t attr = 0, uint8_t bgcolor = BLACK);
    void putbitmap(int x, int y, prog_uint16_t *bitmap, uint8_t w, uint8_t h, uint8_t color);
    void hscrolltext(int y, char *text, uint8_t color, int delaytime, int times = 1, uint8_t dir = LEFT, uint8_t attr = 0, uint8_t bgcolor = BLACK);
    void vscrolltext(int x, char *text, uint8_t color, int delaytime, int times = 1, uint8_t dir = UP, uint8_t attr = 0, uint8_t bgcolor = BLACK);
    void setfont(uint8_t userfont);
    void line(int x0, int y0, int x1, int y1, uint8_t color);
    void rect(int x0, int y0, int x1, int y1, uint8_t color);
    void circle(int xm, int ym, int r, uint8_t color);
    void ellipse(int x0, int y0, int x1, int y1, uint8_t color);
    void fill(uint8_t x, uint8_t y, uint8_t color);
    void bezier(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color);
    void profile();
#ifdef PRINT_NEW
    virtual size_t write(uint8_t chr);
    virtual size_t write(const char *str);
#else
    virtual void write(uint8_t chr);
    virtual void write(const char *str);
#endif

    uint8_t x_max;
    uint8_t y_max;
    int fps;

private:
    void _setrow(uint8_t row);
    void _sendcmd(uint8_t cs, uint8_t command);
    void _setup();
    void _update_fb(uint8_t *ptr, uint8_t target, uint8_t pixel);

    inline void _set(uint8_t val);
    inline void _toggle(uint8_t val);
    inline void _reset(uint8_t val);
    inline void _pulse(uint8_t num, uint8_t val);

    inline void _set(_port_t port);
    inline void _toggle(_port_t port);
    inline void _reset(_port_t port);
    inline void _pulse(uint8_t num, _port_t port);

    void _writebits (uint8_t bits, uint8_t msb);
    void _chipselect(uint8_t cs);

    void _fill_r(uint8_t x, uint8_t y, uint8_t color);
    void _fill_l(uint8_t x, uint8_t y, uint8_t color);

    static uint8_t *g_fb;
    static uint8_t *r_fb;
    uint16_t fb_size;
    uint8_t cs_max;
    prog_uint8_t *font;
    prog_uint16_t *wfont;
    uint8_t font_width;
    uint8_t font_height;
    uint8_t x_cur;
    uint8_t y_cur;

    static volatile uint8_t *_port;
    static volatile uint8_t *_prow;
#if defined (__AVR__)
    static uint8_t _abcd;
    static uint8_t _a;
    static uint8_t _g1;
    static uint8_t _g2;
    static uint8_t _r1;
    static uint8_t _r2;
    static uint8_t _lt;
    static uint8_t _ck;
    static uint8_t _en;
#elif defined (__ARMEL__) || defined (__PIC32MX__)
    static _port_t _abcd;
    static _port_t _g1;
    static _port_t _g2;
    static _port_t _r1;
    static _port_t _r2;
    static _port_t _lt;
    static _port_t _ck;
    static _port_t _en;
#endif
};

#endif
