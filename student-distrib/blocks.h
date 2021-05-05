/*
 * tab:4
 *
 * blocks.h - header file for maze block image definitions
 *
 * "Copyright (c) 2004-2009 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:        Steve Lumetta
 * Version:       2
 * Creation Date: Thu Sep  9 22:15:40 2004
 * Filename:      blocks.h
 * History:
 *    SL    1    Thu Sep  9 22:15:40 2004
 *        First written.
 *    SL    2    Sat Sep 12 12:06:20 2009
 *        Integrated original release back into main code base.
 */

#ifndef BLOCKS_H
#define BLOCKS_H

/* 
 * block image dimensions in pixels; images are defined as contiguous
 * sets of bytes in blocks.s
 */
#define BLOCK_X_DIM 12
#define BLOCK_Y_DIM 12

/* 
 * CAUTION!  The directions below are listed clockwise to allow the use
 * of modular arithmetic for turns, e.g., turning right is +1 mod 4.  The
 * directions also correspond to the order of the player blocks in the
 * enumeration below and in the blocks.s file.  The last direction of
 * motion must be used for the images--no "stopped" picture is provided.
 */
typedef enum {
    DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT, 
    NUM_DIRS,
    DIR_STOP = NUM_DIRS
} dir_t;

/*
 * CAUTION!  These colors are used as constants rather than symbols
 * in blocks.s.
 */
#define PLAYER_CENTER_COLOR 0x20
#define WALL_OUTLINE_COLOR  0x21
#define WALL_FILL_COLOR     0x22
#define STATUS_BACK_COLOR   0x24  
#define STATUS_TEXT_COLOR   0x25


/* 
 * CAUTION!  The order of blocks in this enumeration must match the
 * order in blocks.s.
 */
enum {
    /* User defined blocks */
    MOUSE_CURSOR,
    MOUSE_CURSOR_MASK_SOLID,
    MOUSE_CURSOR_MASK_TRANS,
    BACKGROUND,
    ICON_EDGE_1,
    ICON_EDGE_2,
    ICON_EDGE_3,
    ICON_EDGE_4,
    ICON_EDGE_5,    // Same as backgroud block
    ICON_EDGE_6,
    ICON_EDGE_7,
    ICON_EDGE_8,
    ICON_EDGE_9,
    ICON_EDGE_MASK_1,
    ICON_EDGE_MASK_2,
    ICON_EDGE_MASK_3,
    ICON_EDGE_MASK_4,
    ICON_EDGE_MASK_5,
    ICON_EDGE_MASK_6,
    ICON_EDGE_MASK_7,
    ICON_EDGE_MASK_8,
    ICON_EDGE_MASK_9,
    ICON_TEST_1,
    ICON_TEST_2,
    ICON_TEST_3,
    ICON_TEST_4,
    ICON_TEST_5,    // Same as backgroud block
    ICON_TEST_6,
    ICON_TEST_7,
    ICON_TEST_8,
    ICON_TEST_9,
    /* MP */
    ICON_MP_1,
    ICON_MP_2,
    ICON_MP_3,
    ICON_MP_4,
    ICON_MP_5,
    ICON_MP_6,
    ICON_MP_7,
    ICON_MP_8,
    ICON_MP_9,
    /* Counter */
    ICON_COUNTER_1,
    ICON_COUNTER_2,
    ICON_COUNTER_3,
    ICON_COUNTER_4,
    ICON_COUNTER_5,
    ICON_COUNTER_6,
    ICON_COUNTER_7,
    ICON_COUNTER_8,
    ICON_COUNTER_9,
    /* Pingpong */
    ICON_PINGPONG_1,
    ICON_PINGPONG_2,
    ICON_PINGPONG_3,
    ICON_PINGPONG_4,
    ICON_PINGPONG_5,
    ICON_PINGPONG_6,
    ICON_PINGPONG_7,
    ICON_PINGPONG_8,
    ICON_PINGPONG_9,
    /* Video */
    ICON_VIDEO_1,
    ICON_VIDEO_2,
    ICON_VIDEO_3,
    ICON_VIDEO_4,
    ICON_VIDEO_5,
    ICON_VIDEO_6,
    ICON_VIDEO_7,
    ICON_VIDEO_8,
    ICON_VIDEO_9,
    /* Fish */
    ICON_FISH_1,
    ICON_FISH_2,
    ICON_FISH_3,
    ICON_FISH_4,
    ICON_FISH_5,
    ICON_FISH_6,
    ICON_FISH_7,
    ICON_FISH_8,
    ICON_FISH_9,
    /* Must at the end */
    NUM_BLOCKS
};

/*
 * CAUTION: Fruits are encoded using several bits in the maze_bit_t
 * bit vector.  Make sure that enough bits are used to cover the
 * number of fruits defined here.
 */
#define NUM_FRUIT_TYPES (LAST_FRUIT - BLOCK_FRUIT_1 + 1)

/* the array of block pictures defined in blocks.s */
extern unsigned char blocks[NUM_BLOCKS][BLOCK_Y_DIM][BLOCK_X_DIM];

#endif /* BLOCKS_H */

