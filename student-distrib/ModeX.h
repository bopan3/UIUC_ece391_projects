#ifndef _ModeX_h_
#define _ModeX_h_

#include "types.h"
#include "x86_desc.h"
#include "lib.h"


/* 
 * IMAGE  is the whole screen in mode X: 320x200 pixels in our flavor.
 * SCROLL is the scrolling region of the screen.
 *
 * X_DIM   is a horizontal screen dimension in pixels.
 * X_WIDTH is a horizontal screen dimension in 'natural' units
 *         (addresses, characters of text, etc.)
 * Y_DIM   is a vertical screen dimension in pixels.
 */
#define IMAGE_X_DIM     320   /* pixels; must be divisible by 4             */
#define IMAGE_Y_DIM     200-18   /* pixels    18 = text hight+ 2 pixel.     */
#define IMAGE_X_WIDTH   (IMAGE_X_DIM / 4)          /* addresses (bytes)     */
#define SCROLL_X_DIM    IMAGE_X_DIM                /* full image width      */
#define SCROLL_Y_DIM    IMAGE_Y_DIM                /* full image width      */
#define SCROLL_X_WIDTH  (IMAGE_X_DIM / 4)          /* addresses (bytes)     */
#define NUM_NONTRANS_COLOR      64      //number of non-tranparent colors in the palette

extern int32_t switch_to_modeX();


/* clear the video memory in mode X */
extern void clear_screens();
#endif
