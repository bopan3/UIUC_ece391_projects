#ifndef DESKTOP_H
#define DESKTOP_H

#include "modex.h"
#include "types.h"

#define SHOW_MIN       0  /* hide the last six pixels of boundary */

/*
 * Define maze minimum and maximum dimensions.  The description of make_maze
 * in maze.c gives details on the layout of the maze.  Minimum values are
 * chosen to ensure that a maze fills the scrolling region of the screen.
 * Maximum values are somewhat arbitrary.
 */
#define MAZE_MIN_X_DIM ((SCROLL_X_DIM + (BLOCK_X_DIM - 1) + 2 * SHOW_MIN) / (2 * BLOCK_X_DIM))
#define MAZE_MAX_X_DIM 50
#define MAZE_MIN_Y_DIM ((SCROLL_Y_DIM + (BLOCK_Y_DIM - 1) + 2 * SHOW_MIN) / (2 * BLOCK_Y_DIM))
#define MAZE_MAX_Y_DIM 30

/* 
 * maze array index calculation macro; maze dimensions are valid only
 * after a call to make_maze
 */
#define MAZE_INDEX(a,b) ((a) + ((b) + 1) * maze_x_dim * 2)

// #ifndef __GLOBAL__
// #define __GLOBAL__
// #endif

/* bit vector of properties for spaces in the maze */
typedef enum {
    MAZE_NONE           = 0,    /* empty                                    */
    MAZE_WALL           = 1,    /* wall                                     */
    MAZE_FRUIT_1        = 2,    /* fruit (3-bit field, with 0 for no fruit) */
    MAZE_FRUIT_2        = 4,
    MAZE_FRUIT_3        = 8,  
    LAST_MAZE_FRUIT_BIT = MAZE_FRUIT_3,
    MAZE_FRUIT          = (MAZE_FRUIT_1 | MAZE_FRUIT_2 | MAZE_FRUIT_3),
    MAZE_EXIT           = 16,   /* exit from maze                           */
    MAZE_REACH          = 128   /* seen already (not shrouded in mist)      */
} maze_bit_t;

// variables borrowed from mazegame.c
typedef struct {
    /* dynamic values within a level -- you may want to add more... */
    unsigned int map_x, map_y;   /* current upper left display pixel */
    int is_ModX;
} game_info_t;

/* create a desktop and place UI for it */
extern int make_desktop(int x_dim, int y_dim);

/* fill a buffer with the pixels for a horizontal line of the maze */
extern void fill_horiz_buffer(int x, int y, unsigned char buf[SCROLL_X_DIM]);

/* fill a buffer with the pixels for a vertical line of the maze */
extern void fill_vert_buffer(int x, int y, unsigned char buf[SCROLL_Y_DIM]);

extern int32_t desktop_open(const uint8_t* filename);

extern int32_t desktop_close(int32_t fd);

extern int32_t desktop_read(int32_t fd, void* buf, int32_t nbytes);

extern int32_t desktop_write(int32_t fd, const void* buf, int32_t nbytes);

void init_game_info();

#endif /* MAZE_H */
