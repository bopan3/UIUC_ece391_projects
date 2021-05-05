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

typedef struct pellete_struct {
    unsigned char RGB[[256][3];
} pellete_struct_t;


/////* Puts the VGA into mode X. [clears (or preset) video memory] */
extern int32_t switch_to_modeX();

/////* Put VGA into text mode 3 (color text).  [may clear screens; writes font data to video memory] */
extern void set_text_mode_3(int clear_scr);


/* set logical view window coordinates */
extern void set_view_window(int scr_x, int scr_y);

/* show the logical view window on the monitor */
extern void show_screen();

/* clear the video memory in mode X */
extern void clear_screens();

/*
 * draw a 12x12 block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not drawn)
 */
extern void draw_full_block(int pos_x, int pos_y, unsigned char* blk);

/* draw a horizontal line at vertical pixel y within the logical view window */
extern int draw_horiz_line(int y);

/* draw a vertical line at horizontal pixel x within the logical view window */
extern int draw_vert_line(int x);

/* refresh the status bar on screen */
extern void refresh_bar(int level, int num_fruit, int time);

/*
 * draw a 12x12 block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not drawn). And we only draw the position where the corresponding position in mask is 1 
 * (before we draw it we first store the old map into the corresponding position in restore block)
 */
extern void draw_full_block_with_mask(int pos_x, int pos_y, unsigned char* blk, unsigned char* mask, unsigned char* restore_block);
/*
 * restore a 12x12 block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not drawn). And we only restore the position where the corresponding position in mask is 1 
 */
extern void restore_full_block_with_mask(int pos_x, int pos_y, unsigned char* blk, unsigned char* mask, unsigned char* restore_block);
/*
 * draw a 18x320 block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not drawn). And we only transparent the position where the corresponding position in mask is STATUS_TEXT_COLOR 
 */
extern void draw_fruit_text_with_mask(int pos_x, int pos_y, unsigned char* mask, unsigned char* restore_block); 
/*
 * restore a 18x320 block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not drawn). And we only transparent the position where the corresponding position in mask is STATUS_TEXT_COLOR 
 */
extern void restore_fruit_text_with_mask(int pos_x, int pos_y, unsigned char* mask, unsigned char* restore_block);
/*
 * set the palette color for given index 
 */
extern void set_palette_color(unsigned char color_index, unsigned char* RGB);

extern void switch_another_screen();
extern void clear_screens_manul();
extern void change_top_left();
extern unsigned char* get_block_img(int32_t block_name);
extern void refresh_mp4(unsigned char* pt_2_mp4_buffer);
extern void copy_mp4(unsigned char* img, unsigned short scr_addr);
#endif
