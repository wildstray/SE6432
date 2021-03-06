#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SE6432.h"
#include "Print.h"
#include "font.h"
#include "font_koi8.h"

/* fast integer (1 uint8_t) modulus - returns n % d */

uint8_t _mod(uint8_t n, uint8_t d)
{
  while(n >= d)
    n -= d;

  return n;
}

/* fast integer (1 uint8_t) division - returns n / d */

uint8_t _div(uint8_t n, uint8_t d)
{
  uint8_t q = 0;
  while(n >= d)
  {
    n -= d;
    q++;
  }
  return q;
}

/* fast integer (1 uint8_t) PRNG */

uint8_t _rnd(uint8_t min, uint8_t max)
{
  static uint8_t seed;
  seed = (21 * seed + 21);
  return min + _mod(seed, --max);
}

/* SE6432 class properties */

uint8_t *SE6432::g_fb;
uint8_t *SE6432::r_fb;
volatile uint8_t *SE6432::_port;
volatile uint8_t *SE6432::_prow;
#if defined (__AVR__)
uint8_t SE6432::_abcd;
uint8_t SE6432::_a;
uint8_t SE6432::_g1;
uint8_t SE6432::_g2;
uint8_t SE6432::_r1;
uint8_t SE6432::_r2;
uint8_t SE6432::_ck;
uint8_t SE6432::_lt;
uint8_t SE6432::_en;
#elif defined (__ARMEL__) || defined (__PIC32MX__)
_port_t SE6432::_abcd;
_port_t SE6432::_g1;
_port_t SE6432::_g2;
_port_t SE6432::_r1;
_port_t SE6432::_r2;
_port_t SE6432::_ck;
_port_t SE6432::_lt;
_port_t SE6432::_en;
#endif

/* SE6432 class constructor */

SE6432::SE6432(const uint8_t g1, const uint8_t g2, const uint8_t r1, const uint8_t r2, const uint8_t latch, const uint8_t clock, const uint8_t enable, const uint8_t abcd)
{
#if defined (__ARMEL__)
  _abcd.dev = PIN_MAP[abcd].gpio_device;
  _abcd.mask = PIN_MAP[abcd].gpio_bit;
  _g1.dev = PIN_MAP[g1].gpio_device;
  _g1.mask = PIN_MAP[g1].gpio_bit;
  _g2.dev = PIN_MAP[g2].gpio_device;
  _g2.mask = PIN_MAP[g2].gpio_bit;
  _r1.dev = PIN_MAP[r1].gpio_device;
  _r1.mask = PIN_MAP[r1].gpio_bit;
  _r2.dev = PIN_MAP[r2].gpio_device;
  _r2.mask = PIN_MAP[r2].gpio_bit;
  _lt.dev = PIN_MAP[latch].gpio_device;
  _lt.mask = PIN_MAP[latch].gpio_bit;
  _ck.dev = PIN_MAP[clock].gpio_device;
  _ck.mask = PIN_MAP[clock].gpio_bit;
  _en.dev = PIN_MAP[enable].gpio_device;
  _en.mask = PIN_MAP[enable].gpio_bit;

  gpio_set_mode(_abcd.dev, _abcd.mask, GPIO_OUTPUT_PP);
  gpio_set_mode(_g1.dev, _g1.mask, GPIO_OUTPUT_PP);
  gpio_set_mode(_g2.dev, _g2.mask, GPIO_OUTPUT_PP);
  gpio_set_mode(_r1.dev, _r1.mask, GPIO_OUTPUT_PP);
  gpio_set_mode(_r2.dev, _r2.mask, GPIO_OUTPUT_PP);
  gpio_set_mode(_ck.dev, _ck.mask, GPIO_OUTPUT_PP);
  gpio_set_mode(_lt.dev, _lt.mask, GPIO_OUTPUT_PP);
  gpio_set_mode(_en.dev, _en.mask, GPIO_OUTPUT_PP);
#elif defined (__PIC32MX__)
  _abcd.regs = (volatile p32_ioport *)portRegisters(digitalPinToPort(abcd));
  _g1.regs = (volatile p32_ioport *)portRegisters(digitalPinToPort(g1));
  _g2.regs = (volatile p32_ioport *)portRegisters(digitalPinToPort(g2));
  _r1.regs = (volatile p32_ioport *)portRegisters(digitalPinToPort(r1));
  _r2.regs = (volatile p32_ioport *)portRegisters(digitalPinToPort(r2));
  _ck.regs = (volatile p32_ioport *)portRegisters(digitalPinToPort(clock));
  _lt.regs = (volatile p32_ioport *)portRegisters(digitalPinToPort(latch));
  _en.regs = (volatile p32_ioport *)portRegisters(digitalPinToPort(enable));

  _abcd.mask = digitalPinToBitMask(abcd);
  _g1.mask = digitalPinToBitMask(g1);
  _g2.mask = digitalPinToBitMask(g2);
  _r1.mask = digitalPinToBitMask(r1);
  _r2.mask = digitalPinToBitMask(r2);
  _ck.mask = digitalPinToBitMask(clock);
  _lt.mask = digitalPinToBitMask(latch);
  _en.mask = digitalPinToBitMask(enable);

  _abcd.regs->tris.clr = _abcd.mask;
  _abcd.regs->odc.clr  = _abcd.mask;
  _g1.regs->tris.clr = _g1.mask;
  _g1.regs->odc.clr  = _g1.mask;
  _g2.regs->tris.clr = _g2.mask;
  _g2.regs->odc.clr  = _g2.mask;
  _r1.regs->tris.clr = _r1.mask;
  _r1.regs->odc.clr  = _r1.mask;
  _r2.regs->tris.clr = _r2.mask;
  _r2.regs->odc.clr  = _r2.mask;
  _ck.regs->tris.clr = _ck.mask;
  _ck.regs->odc.clr  = _ck.mask;
  _lt.regs->tris.clr = _lt.mask;
  _lt.regs->odc.clr  = _lt.mask;
  _en.regs->tris.clr = _en.mask;
  _en.regs->odc.clr  = _en.mask;
#endif
  _setup();
  clear();
}

SE6432::SE6432(volatile uint8_t *port, const uint8_t g1, const uint8_t g2, const uint8_t r1, const uint8_t r2, const uint8_t latch, const uint8_t clock, const uint8_t enable, volatile uint8_t *prow, const uint8_t abcd)
{
#if defined (__AVR__)
  _port = port;
  _g1 = 1 << (g1 & 7);
  _g2 = 1 << (g2 & 7);
  _r1 = 1 << (r1 & 7);
  _r2 = 1 << (r2 & 7);
  _lt = 1 << (latch & 7);
  _ck = 1 << (clock & 7);
  _en = 1 << (enable & 7);
  _abcd = 0x0f << (abcd & 7);
  _a = 1 << (abcd & 7);
  _prow = prow;

  _port--;
  _set(_g1);
  _set(_g2);
  _set(_r1);
  _set(_r2);
  _set(_lt);
  _set(_ck);
  _set(_en);
  _port++;
  _prow--;
  *_prow |= _abcd;
  _prow++;
#endif
  _setup();
  clear();
}

/* SE6432 class low level functions */

inline void SE6432::_set(_port_t port)
{
#if defined (__ARMEL__)
  port.dev->regs->BSRR = BIT(port.mask);
#elif defined (__PIC32MX__)
  port.regs->lat.set = port.mask;
#endif
}

inline void SE6432::_toggle(_port_t port)
{
#if defined (__ARMEL__)
  port.dev->regs->ODR = port.dev->regs->ODR ^ BIT(port.mask);
#elif defined (__PIC32MX__)
  port.regs->lat.inv = port.mask;
#endif
}

inline void SE6432::_reset(_port_t port)
{
#if defined (__ARMEL__)
  port.dev->regs->BRR = BIT(port.mask);
#elif defined (__PIC32MX__)
  port.regs->lat.clr = port.mask;
#endif
}

inline void SE6432::_set(register uint8_t val)
{
#ifdef USE_ASM
  asm (
    "ld      __tmp_reg__, %a0"  "\n"
    "or      __tmp_reg__,  %1"  "\n"
    "st      %a0, __tmp_reg__"  "\n"
    :
    : "e" (_port), "r" (val)
    :
  );
#else
  *_port |= val;
#endif
}

inline void SE6432::_toggle(register uint8_t val)
{
#ifdef USE_ASM
  asm (
    "ld      __tmp_reg__, %a0"  "\n"
    "eor     __tmp_reg__,  %1"  "\n"
    "st      %a0, __tmp_reg__"  "\n"
    :
    : "e" (_port), "r" (val)
    :
  );
#else
  *_port ^= val;
#endif
}

inline void SE6432::_reset(register uint8_t val)
{
#ifdef USE_ASM
  asm (
    "ld      __tmp_reg__, %a0"  "\n"
    "or      __tmp_reg__,  %1"  "\n"
    "eor     __tmp_reg__,  %1"  "\n"
    "st      %a0, __tmp_reg__"  "\n"
    :
    : "e" (_port), "r" (val)
    :
  );
#else
  *_port &= ~val;
#endif
}

inline void SE6432::_pulse(uint8_t num, _port_t port)
{
  while(num--)
  {
    _set(port);
    _toggle(port);
  }
}

void SE6432::_pulse(uint8_t num, uint8_t val)
{
  while(num--)
  {
    _set(val);
    _toggle(val);
  }
}

/* SE6432s based display initialization  */

void SE6432::_setup()
{
  x_max = 63;
  y_max = 31;
  fb_size = 256;
  g_fb = (uint8_t*) malloc(fb_size);
  r_fb = (uint8_t*) malloc(fb_size);
  setfont(FONT_5x7W);
  x_cur = 0;
  y_cur = 0;
}

/* select row */ 

inline void SE6432::_setrow(register uint8_t row)
{
#ifdef USE_ASM
  asm (
    "ld      __tmp_reg__, %a0"  "\n"
    "or      __tmp_reg__,  %1"  "\n"
    "eor     __tmp_reg__,  %1"  "\n"
    "st      %a0, __tmp_reg__"  "\n"
    "mul     %3, %2          "  "\n"
    "or      __tmp_reg__,  %3"  "\n"
    "st      %a0, __tmp_reg__"  "\n"
    :
    : "e" (_prow), "r" (_abcd), "r" (_a), "r" (row)
    :
  );
#else
    *_prow &= ~_abcd;
    *_prow |= _a * row;
#endif
}

/* write the framebuffer to the display - to be used after one or more textual or graphic functions */ 

void SE6432::sendframe() // 306 ASM 320 no ASM
{
  noInterrupts();
  uint8_t addr = 0;
  for (uint8_t row = 0; row < 16; row++) {
    for (uint8_t i = 0; i < 8; i++) {
      register uint8_t vg1 = g_fb[addr];
      register uint8_t vg2 = g_fb[addr + 128];
      register uint8_t vr1 = r_fb[addr];
      register uint8_t vr2 = r_fb[addr + 128];
#ifdef USE_ASM
  register uint8_t tmp;
  register uint8_t msb;
  asm (
    "ldi     %11, 128        "  "\n"
    "bb:                     "  "\n"
    "ld      __tmp_reg__, %a0"  "\n"

    "or      __tmp_reg__,  %1"  "\n"
    "eor     __tmp_reg__,  %1"  "\n"
    "st      %a0, __tmp_reg__"  "\n"

    "or      __tmp_reg__,  %2"  "\n"
    "or      __tmp_reg__,  %3"  "\n"
    "or      __tmp_reg__,  %4"  "\n"
    "or      __tmp_reg__,  %5"  "\n"
    "st      %a0, __tmp_reg__"  "\n"

    "mov     %10, %6         "  "\n"
    "and     %10, %11        "  "\n"
    "breq    bg1             "  "\n"
    "eor     __tmp_reg__,  %2"  "\n"
    "bg1:                    "  "\n"
    "mov     %10, %7         "  "\n"
    "and     %10, %11        "  "\n"
    "breq    bg2             "  "\n"
    "eor     __tmp_reg__,  %3"  "\n"
    "bg2:                    "  "\n"
    "mov     %10, %8         "  "\n"
    "and     %10, %11        "  "\n"
    "breq    br1             "  "\n"
    "eor     __tmp_reg__,  %4"  "\n"
    "br1:                    "  "\n"
    "mov     %10, %9         "  "\n"
    "and     %10, %11        "  "\n"
    "breq    br2             "  "\n"
    "eor     __tmp_reg__,  %5"  "\n"
    "br2:                    "  "\n"
    "st      %a0, __tmp_reg__"  "\n"

    "or      __tmp_reg__,  %1"  "\n"
    "st      %a0, __tmp_reg__"  "\n"

    "lsr     %11             "  "\n"
    "brne    bb              "  "\n"

    :
    : "e" (_port), "r" (_ck), "r" (_g1), "r" (_g2), "r" (_r1), "r" (_r2) ,"r" (vg1), "r" (vg2), "r" (vr1), "r" (vr2), "r" (tmp), "r" (msb)
    :
  );
#else
      uint8_t msb = 1 << 7;
      do {
        _reset(_ck);
        _set(_g1|_g2|_r1|_r2);
        if (vg1 & msb) _reset(_g1);
        if (vg2 & msb) _reset(_g2);
        if (vr1 & msb) _reset(_r1);
        if (vr2 & msb) _reset(_r2);
        _set(_ck);
      } while (msb >>= 1);
#endif
      addr++;
    }

    // set row
    _setrow(row);

    // disable display
    // latch the data
    // enable display
#ifdef USE_ASM
  asm (
    "ld      __tmp_reg__, %a0"  "\n"
    "or      __tmp_reg__,  %1"  "\n"
    "or      __tmp_reg__,  %2"  "\n"
    "st      %a0, __tmp_reg__"  "\n"
    "eor     __tmp_reg__,  %2"  "\n"
    "st      %a0, __tmp_reg__"  "\n"

    "or      __tmp_reg__,  %1"  "\n"
    "eor     __tmp_reg__,  %1"  "\n"
    "st      %a0, __tmp_reg__"  "\n"

    :
    : "e" (_port), "r" (_en), "r" (_lt)
    :
  );
#else
    _set(_en);
    _set(_lt|_en);
    _reset(_lt);
    _reset(_en);
#endif 
  }
  interrupts();
}

/* clear the display */

void SE6432::clear()
{
  x_cur = 0;
  y_cur = 0;
  memset(g_fb, 0, fb_size);
  memset(r_fb, 0, fb_size);
  sendframe();
}

inline void SE6432::_update_fb(uint8_t *ptr, uint8_t color, uint8_t x)
{
//  uint8_t val = 1 << (7 - x % 8);
  uint8_t val = 1 << (7 - _mod(x, 8));
//  uint8_t val = 1 << (~x & 7);
  (color) ? *ptr |= val : *ptr &= ~val;
}

/* put a single pixel in the coordinates x, y */

void SE6432::plot (uint8_t x, uint8_t y, uint8_t color)
{
  uint8_t addr;

  if (x > x_max)
    return;

  addr = (y << 3) + (x >> 3);
  _update_fb(&g_fb[addr], (color & GREEN), x);
  _update_fb(&r_fb[addr], (color & RED), x);

}

/* print a single character */

uint8_t SE6432::putchar(int x, int y, char c, uint8_t color, uint8_t attr, uint8_t bgcolor)
{
  uint16_t dots, msb;
  char col, row;

  uint8_t width = font_width;
  uint8_t height = font_height;

  if (x < -width || x > x_max + width || y < -height || y > y_max + height)
    return 0;

  if ((unsigned char) c >= 0xc0) c -= 0x41;
  c -= 0x20;
  msb = 1 << (height - 1);

  // some math with pointers... don't try this at home ;-)
  prog_uint8_t *addr = font + c * width;
  prog_uint16_t *waddr = wfont + c * width;

  for (col = 0; col < width; col++) {
    dots = (height > 8) ? pgm_read_word_near(waddr + col) : pgm_read_byte_near(addr + col);
    for (row = 0; row < height; row++) {
      if (dots & (msb >> row)) {
        plot(x + col, y + row, (color & MULTICOLOR) ? _rnd(1, 4) : color);
      } else {
        plot(x + col, y + row, bgcolor);
      }
    }
  }
  return ++width;
}

/* put a bitmap in the coordinates x, y */

void SE6432::putbitmap(int x, int y, prog_uint16_t *bitmap, uint8_t w, uint8_t h, uint8_t color)
{
  uint16_t dots, msb;
  char col, row;

  if (x < -w || x > x_max + w || y < -h || y > y_max + h)
    return;

  msb = 1 << (w - 1);
  for (row = 0; row < h; row++)
  {
    dots = pgm_read_word_near(&bitmap[row]);
    if (dots && color)
      for (uint8_t col = 0; col < w; col++)
      {
	if (dots & (msb >> col))
	  plot(x + col, y + row, color);
	else
	  plot(x + col, y + row, BLACK);
      }
  }
}

/* text only scrolling functions */

void SE6432::hscrolltext(int y, char *text, uint8_t color, int delaytime, int times, uint8_t dir, uint8_t attr, uint8_t bgcolor)
{
  uint8_t showcolor;
  int i, x, offs, len = strlen(text);
  uint8_t width = font_width;
  uint8_t height = font_height;
  width++;

  while (times) {
    for ((dir) ? x = - (len * width) : x = x_max; (dir) ? x <= x_max : x > - (len * width); (dir) ? x++ : x--)
    {
      for (i = 0; i < len; i++)
      {
        offs = width * i;
        (dir) ? offs-- : offs++;
        putchar(x + offs,  y, text[i], bgcolor, attr, bgcolor);
      }
      for (i = 0; i < len; i++)
      {
        showcolor = (color & RANDOMCOLOR) ? _rnd(1, 4) : color;
        showcolor = ((color & BLINK) && (x & 2)) ? BLACK : (showcolor & ~BLINK);
        offs = width * i;
        putchar(x + offs,  y, text[i], showcolor, attr, bgcolor);
      }
      sendframe();
      delay(delaytime);
    }
    times--;
  }
}

void SE6432::vscrolltext(int x, char *text, uint8_t color, int delaytime, int times, uint8_t dir, uint8_t attr, uint8_t bgcolor)
{
  uint8_t showcolor;
  int i, y, voffs, len = strlen(text);
  uint8_t offs;
  uint8_t width = font_width;
  uint8_t height = font_height;
  width++;

  while (times) {
    for ((dir) ? y = - font_height : y = y_max + 1; (dir) ? y <= y_max + 1 : y > - font_height; (dir) ? y++ : y--)
    {
      for (i = 0; i < len; i++)
      {
        offs = width * i;
        voffs = (dir) ? -1 : 1;
        putchar(x + offs,  y + voffs, text[i], bgcolor, attr, bgcolor);
      }
      for (i = 0; i < len; i++)
      {
        showcolor = (color & RANDOMCOLOR) ? _rnd(1, 4) : color;
        showcolor = ((color & BLINK) && (y & 2)) ? BLACK : (showcolor & ~BLINK);
        putchar(x + width * i,  y, text[i], showcolor, attr, bgcolor);
      }
      sendframe();
      delay(delaytime);
    }
    times--;
  }
}

/* choose/change font to use (for next putchar) */

void SE6432::setfont(uint8_t userfont)
{
  switch(userfont) {

#ifdef FONT_4x6
    case FONT_4x6:
	font = (prog_uint8_t *) &font_4x6[0];
	font_width = 4;
	font_height = 6;
	break;
#endif
#ifdef FONT_5x7
    case FONT_5x7:
	font = (prog_uint8_t *) &font_5x7[0];
	font_width = 5;
	font_height = 7;
	break;
#endif
#ifdef FONT_5x8
    case FONT_5x8:
	font = (prog_uint8_t *) &font_5x8[0];
	font_width = 5;
	font_height = 8;
	break;
#endif
#ifdef FONT_5x7W
    case FONT_5x7W:
	font = (prog_uint8_t *) &font_5x7w[0];
	font_width = 5;
	font_height = 8;
	break;
#endif
#ifdef FONT_6x10
    case FONT_6x10:
	wfont = (prog_uint16_t *) &font_6x10[0];
	font_width = 6;
	font_height = 10;
	break;
#endif
#ifdef FONT_6x12
    case FONT_6x12:
	wfont = (prog_uint16_t *) &font_6x12[0];
	font_width = 6;
	font_height = 12;
	break;
#endif
#ifdef FONT_6x13
    case FONT_6x13:
	wfont = (prog_uint16_t *) &font_6x13[0];
	font_width = 6;
	font_height = 13;
	break;
#endif
#ifdef FONT_6x13B
    case FONT_6x13B:
	wfont = (prog_uint16_t *) &font_6x13B[0];
	font_width = 6;
	font_height = 13;
	break;
#endif
#ifdef FONT_6x13O
    case FONT_6x13O:
	wfont = (prog_uint16_t *) &font_6x13O[0];
	font_width = 6;
	font_height = 13;
	break;
#endif
#ifdef FONT_6x9
    case FONT_6x9:
	wfont = (prog_uint16_t *) &font_6x9[0];
	font_width = 6;
	font_height = 9;
	break;
#endif
#ifdef FONT_7x13
    case FONT_7x13:
	wfont = (prog_uint16_t *) &font_7x13[0];
	font_width = 7;
	font_height = 13;
	break;
#endif
#ifdef FONT_7x13B
    case FONT_7x13B:
	wfont = (prog_uint16_t *) &font_7x13B[0];
	font_width = 7;
	font_height = 13;
	break;
#endif
#ifdef FONT_7x13O
    case FONT_7x13O:
	wfont = (prog_uint16_t *) &font_7x13O[0];
	font_width = 7;
	font_height = 13;
	break;
#endif
#ifdef FONT_7x14
    case FONT_7x14:
	wfont = (prog_uint16_t *) &font_7x14[0];
	font_width = 7;
	font_height = 14;
	break;
#endif
#ifdef FONT_7x14B
    case FONT_7x14B:
	wfont = (prog_uint16_t *) &font_7x14B[0];
	font_width = 7;
	font_height = 14;
	break;
#endif
#ifdef FONT_8x8
    case FONT_8x8:
	font = (prog_uint8_t *) &font_8x8[0];
	font_width = 8;
	font_height = 8;
	break;
#endif
#ifdef FONT_8x13
    case FONT_8x13:
	wfont = (prog_uint16_t *) &font_8x13[0];
	font_width = 8;
	font_height = 13;
	break;
#endif
#ifdef FONT_8x13B
    case FONT_8x13B:
	wfont = (prog_uint16_t *) &font_8x13B[0];
	font_width = 8;
	font_height = 13;
	break;
#endif
#ifdef FONT_8x13O
    case FONT_8x13O:
	wfont = (prog_uint16_t *) &font_8x13O[0];
	font_width = 8;
	font_height = 13;
	break;
#endif
#ifdef FONT_9x15
    case FONT_9x15:
	wfont = (prog_uint16_t *) &font_9x15[0];
	font_width = 9;
	font_height = 15;
	break;
#endif
#ifdef FONT_9x15B
    case FONT_9x15B:
	wfont = (prog_uint16_t *) &font_9x15b[0];
	font_width = 9;
	font_height = 15;
	break;
#endif
#ifdef FONT_8x16
    case FONT_8x16:
	wfont = (prog_uint16_t *) &font_8x16[0];
	font_width = 8;
	font_height = 16;
	break;
#endif
#ifdef FONT_8x16B
    case FONT_8x16B:
	wfont = (prog_uint16_t *) &font_8x16b[0];
	font_width = 8;
	font_height = 16;
	break;
#endif
#ifdef FONT_8x13BK
    case FONT_8x13BK:
	wfont = (prog_uint16_t *) &font_8x13bk[0];
	font_width = 8;
	font_height = 13;
	break;
#endif
  }
}

/* graphic primitives based on Bresenham's algorithms */

void SE6432::line(int x0, int y0, int x1, int y1, uint8_t color)
{
  int dx =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1; 
  int err = dx + dy, e2; /* error value e_xy */

  for(;;) {
    plot(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
    if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
  }
}

void SE6432::rect(int x0, int y0, int x1, int y1, uint8_t color)
{
  line(x0, y0, x0, y1, color); /* left line   */
  line(x1, y0, x1, y1, color); /* right line  */
  line(x0, y0, x1, y0, color); /* top line    */
  line(x0, y1, x1, y1, color); /* bottom line */
}

void SE6432::circle(int xm, int ym, int r, uint8_t color)
{
  int x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */ 
  do {
    plot(xm - x, ym + y, color); /*   I. Quadrant */
    plot(xm - y, ym - x, color); /*  II. Quadrant */
    plot(xm + x, ym - y, color); /* III. Quadrant */
    plot(xm + y, ym + x, color); /*  IV. Quadrant */
    r = err;
    if (r >  x) err += ++x * 2 + 1; /* e_xy+e_x > 0 */
    if (r <= y) err += ++y * 2 + 1; /* e_xy+e_y < 0 */
  } while (x < 0);
}

void SE6432::ellipse(int x0, int y0, int x1, int y1, uint8_t color)
{
  int a = abs(x1 - x0), b = abs(y1 - y0), b1 = b & 1; /* values of diameter */
  long dx = 4 * (1 - a) * b * b, dy = 4 * (b1 + 1) * a * a; /* error increment */
  long err = dx + dy + b1 * a * a, e2; /* error of 1.step */

  if (x0 > x1) { x0 = x1; x1 += a; } /* if called with swapped points */
  if (y0 > y1) y0 = y1; /* .. exchange them */
  y0 += (b + 1) / 2; /* starting pixel */
  y1 = y0 - b1;
  a *= 8 * a; 
  b1 = 8 * b * b;

  do {
    plot(x1, y0, color); /*   I. Quadrant */
    plot(x0, y0, color); /*  II. Quadrant */
    plot(x0, y1, color); /* III. Quadrant */
    plot(x1, y1, color); /*  IV. Quadrant */
    e2 = 2 * err;
    if (e2 >= dx) { x0++; x1--; err += dx += b1; } /* x step */
    if (e2 <= dy) { y0++; y1--; err += dy += a; }  /* y step */ 
  } while (x0 <= x1);

  while (y0 - y1 < b) {  /* too early stop of flat ellipses a=1 */
    plot(x0 - 1, ++y0, color); /* -> complete tip of ellipse */
    plot(x0 - 1, --y1, color); 
  }
}

void SE6432::bezier(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
{
  int sx = x0 < x2 ? 1 : -1, sy = y0 < y2 ? 1 : -1; /* step direction */
  int cur = sx * sy * ((x0 - x1) * (y2 - y1) - (x2 - x1) * (y0 - y1)); /* curvature */
  int x = x0 - 2 * x1 + x2, y = y0 - 2 * y1 + y2, xy = 2 * x * y * sx * sy;
                                /* compute error increments of P0 */
  long dx = (1 - 2 * abs(x0 - x1)) * y * y + abs(y0 - y1) * xy - 2 * cur * abs(y0 - y2);
  long dy = (1 - 2 * abs(y0 - y1)) * x * x + abs(x0 - x1) * xy + 2 * cur * abs(x0 - x2);
                                /* compute error increments of P2 */
  long ex = (1 - 2 * abs(x2 - x1)) * y * y + abs(y2 - y1) * xy + 2 * cur * abs(y0 - y2);
  long ey = (1 - 2 * abs(y2 - y1)) * x * x + abs(x2 - x1) * xy - 2 * cur * abs(x0 - x2);

  if (cur == 0) { line(x0, y0, x2, y2, color); return; } /* straight line */

  x *= 2 * x; y *= 2 * y;
  if (cur < 0) {                             /* negated curvature */
    x = -x; dx = -dx; ex = -ex; xy = -xy;
    y = -y; dy = -dy; ey = -ey;
  }
  /* algorithm fails for almost straight line, check error values */
  if (dx >= -y || dy <= -x || ex <= -y || ey >= -x) {        
    line(x0, y0, x1, y1, color);                /* simple approximation */
    line(x1, y1, x2, y2, color);
    return;
  }
  dx -= xy; ex = dx+dy; dy -= xy;              /* error of 1.step */

  for(;;) {                                         /* plot curve */
    plot(x0, y0, color);
    ey = 2 * ex - dy;                /* save value for test of y step */
    if (2 * ex >= dx) {                                   /* x step */
      if (x0 == x2) break;
      x0 += sx; dy -= xy; ex += dx += y; 
    }
    if (ey <= 0) {                                      /* y step */
      if (y0 == y2) break;
      y0 += sy; dx -= xy; ex += dy += x; 
    }
  }
}

/* returns the pixel value (RED, GREEN, ORANGE or 0/BLACK) of x, y coordinates */

uint8_t SE6432::getpixel(uint8_t x, uint8_t y)
{
  uint8_t g, r, val;
  uint16_t addr;

  val = 1 << (7 - _mod(x, 8));
  addr = (y << 3) + (x >> 3);
  g = g_fb[addr];
  r = r_fb[addr];
  return ((g & val) ? GREEN : BLACK) | ((r & val) ? RED : BLACK);
}

/* boundary flood fill with the seed in x, y coordinates */

void SE6432::_fill_r(uint8_t x, uint8_t y, uint8_t color)
{
  if (x > x_max) return;
  if (y > y_max) return;
  if (!getpixel(x, y))
  {
    plot(x, y, color);
    _fill_r(++x, y ,color);
    x--;
    _fill_r(x, y - 1, color);
    _fill_r(x, y + 1, color);
  }
}

void SE6432::_fill_l(uint8_t x, uint8_t y, uint8_t color)
{
  if (x > x_max) return;
  if (y > y_max) return;
  if (!getpixel(x, y))
  {
    plot(x, y, color);
    _fill_l(--x, y, color);
    x++;
    _fill_l(x, y - 1, color);
    _fill_l(x, y + 1, color);
  }
}

void SE6432::fill(uint8_t x, uint8_t y, uint8_t color)
{
  _fill_r(x, y, color);
  _fill_l(x - 1, y, color);
}

/* Print class extension: TBD */

#ifdef PRINT_NEW
size_t SE6432::write(uint8_t chr)
#else
void SE6432::write(uint8_t chr)
#endif
{
  uint8_t x, y;
  if (chr == '\n') {
    //y_cur += font_height;
  } else {
    //x_cur += putchar(x_cur, y_cur, chr, GREEN);
    //x_cur = 0;
    //y_cur = 0;
  }
  //sendframe();
#ifdef PRINT_NEW
  return 1;
#endif
}

#ifdef PRINT_NEW
size_t SE6432::write(const char *str)
#else
void SE6432::write(const char *str)
#endif
{
  uint8_t x, y;
  uint8_t len = strlen(str);
  if (x_cur + font_width <= x_max)
    for (uint8_t i = 0; i < len; i++)
    {
      if (str[i] == '\n') {
        y_cur += font_height;
        x_cur = 0;
        if (y_cur > y_max) break;
      } else {
        x_cur += putchar(x_cur, y_cur, str[i], GREEN);
        if (x_cur + font_width > x_max) break;
      }
    }
  //x_cur = 0;
  //y_cur = 0;
  sendframe();
#ifdef PRINT_NEW
  return len;
#endif
}

/* calculate frames per second speed, for benchmark */

void SE6432::profile() {
  const uint8_t frame_interval = 50;
  static unsigned long last_micros = 0;
  static uint8_t frames = 0;
  if (++frames == frame_interval) {
    fps = (frame_interval * 1000000L) / (micros() - last_micros);
    frames = 0;
    last_micros = micros();
  }
}
