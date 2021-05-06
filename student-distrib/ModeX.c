#include "ModeX.h"
#include "text.h"
#include "paging.h"
#include "i8259.h"
#include "timer.h"
#include "keyboard.h"
#include "desktop.h"
#include "blocks.h"
#include "sys_calls.h"
#include "vedio.h"
#include "./dev/sound.h"


#define SCROLL_SIZE             (SCROLL_X_WIDTH * SCROLL_Y_DIM)
#define SCREEN_SIZE             (SCROLL_SIZE * 4 + 1)
#define BUILD_BUF_SIZE          (SCREEN_SIZE + 20000)
#define BUILD_BASE_INIT         ((BUILD_BUF_SIZE - SCREEN_SIZE) / 2)

/* Mode X and general VGA parameters */
#define MODEX_STR_ADDR          0xA0000
#define TEMP_STOR_FOR_MODEX     0x9000000
#define VID_MEM_SIZE            131072
#define MODE_X_MEM_SIZE         65536
#define NUM_SEQUENCER_REGS      5
#define NUM_CRTC_REGS           25
#define NUM_GRAPHICS_REGS       9
#define NUM_ATTR_REGS           22
#define WHITE                   0x3F

static void VGA_blank(int blank_bit);
static void set_seq_regs_and_reset(unsigned short table[NUM_SEQUENCER_REGS], unsigned char val);
static void set_CRTC_registers(unsigned short table[NUM_CRTC_REGS]);
static void set_attr_registers(unsigned char table[NUM_ATTR_REGS * 2]);
static void set_graphics_registers(unsigned short table[NUM_GRAPHICS_REGS]);
static void fill_palette();
static void write_font_data();
static void copy_image(unsigned char* img, unsigned short scr_addr);
static void copy_status_bar(unsigned char* img, unsigned short scr_addr);


#define MEM_FENCE_WIDTH 0 // we do not need mem-fence for mp3
#define MEM_FENCE_MAGIC 0xF3

static unsigned char build[BUILD_BUF_SIZE + 2 * MEM_FENCE_WIDTH];
static int img3_off;                /* offset of upper left pixel   */
static unsigned char* img3;         /* pointer to upper left pixel  */
static int show_x, show_y;          /* logical view coordinates     */

                                    /* displayed video memory variables */
static unsigned char* mem_image;    /* pointer to start of video memory */
static unsigned short target_img;   /* offset of displayed screen image */

/* pointer to start (0x900000) of temp video memory for text screen when in modeX */
static unsigned char* mem_temp_v;    

int32_t in_modex=0;
extern volatile int32_t terminal_tick;      /* for the active running terminal, default the first terminal */
extern volatile int32_t terminal_display;   /* for the displayed terminal, only change when function-key pressed */
unsigned char palette_RGB_vedio[256][3]= {
{0xd8 ,0xd9 ,0xfb},
{0xcc ,0xcc ,0xf5},
{0xc9 ,0xc4 ,0xec},
{0x98 ,0x94 ,0xab},
{0xb6 ,0xa5 ,0xaa},
{0x93 ,0x7a ,0x89},
{0xab ,0xa5 ,0xad},
{0xa9 ,0xa5 ,0xbb},
{0xd2 ,0xa8 ,0xac},
{0x93 ,0x89 ,0x95},
{0xb1 ,0xae ,0xd2},
{0x7b ,0x88 ,0xb5},
{0xc2 ,0xb9 ,0xdb},
{0x46 ,0x38 ,0x42},
{0x32 ,0x30 ,0x44},
{0x44 ,0x30 ,0x2c},
{0x26 ,0x1e ,0x21},
{0xa7 ,0x94 ,0x99},
{0x64 ,0x3d ,0x35},
{0x36 ,0x3f ,0x6a},
{0x86 ,0x78 ,0x7d},
{0x96 ,0x66 ,0x66},
{0x77 ,0x55 ,0x56},
{0x88 ,0x66 ,0x67},
{0x77 ,0x65 ,0x68},
{0x89 ,0x64 ,0x54},
{0xbb ,0xc0 ,0xec},
{0xcc ,0xd8 ,0xfe},
{0xb9 ,0x9b ,0xa9},
{0xf2 ,0xc9 ,0xd1},
{0xd7 ,0xcc ,0xed},
{0xaf ,0x86 ,0x74},
{0x58 ,0x6c ,0xa8},
{0x98 ,0x76 ,0x78},
{0x67 ,0x55 ,0x57},
{0xda ,0xd6 ,0xea},
{0xa7 ,0x75 ,0x7e},
{0xa7 ,0x84 ,0x89},
{0xa8 ,0x8a ,0x97},
{0xd6 ,0xcd ,0xf9},
{0x77 ,0x48 ,0x47},
{0x89 ,0x5a ,0x66},
{0x76 ,0x69 ,0x76},
{0x45 ,0x46 ,0x67},
{0xbb ,0x98 ,0x99},
{0x99 ,0x6a ,0x76},
{0x79 ,0x78 ,0x98},
{0x7d ,0x77 ,0x88},
{0x67 ,0x46 ,0x46},
{0x56 ,0x65 ,0x99},
{0xee ,0xe7 ,0xf8},
{0x54 ,0x48 ,0x55},
{0xcb ,0xc4 ,0xcc},
{0x47 ,0x54 ,0x86},
{0x56 ,0x44 ,0x46},
{0x67 ,0x58 ,0x65},
{0x54 ,0x56 ,0x77},
{0x78 ,0x54 ,0x48},
{0x79 ,0x74 ,0x78},
{0x88 ,0x69 ,0x76},
{0x44 ,0x41 ,0x57},
{0xb9 ,0x87 ,0x89},
{0xff ,0xf7 ,0xfd},
{0x78 ,0x7b ,0xa6},
{0x77 ,0x59 ,0x65},
{0x66 ,0x75 ,0xa9},
{0x99 ,0xa4 ,0xc1},
{0xb7 ,0x84 ,0x96},
{0x66 ,0x67 ,0x88},
{0x43 ,0x49 ,0x76},
{0x73 ,0x63 ,0x58},
{0x48 ,0x52 ,0x78},
{0x67 ,0x79 ,0xb7},
{0x54 ,0x59 ,0x87},
{0xb6 ,0x9c ,0xb7},
{0x6a ,0x65 ,0x68},
{0x57 ,0x54 ,0x67},
{0x69 ,0x75 ,0x99},
{0x98 ,0x75 ,0x68},
{0x69 ,0x66 ,0x76},
{0x59 ,0x53 ,0x58},
{0xec ,0xde ,0xf8},
{0xa8 ,0x77 ,0x67},
{0x48 ,0x42 ,0x45},
{0x93 ,0x84 ,0x82},
{0x84 ,0x52 ,0x49},
{0x58 ,0x65 ,0x89},
{0xd7 ,0xc4 ,0xca},
{0xa5 ,0x6b ,0x78},
{0x66 ,0x49 ,0x54},
{0x77 ,0x4a ,0x53},
{0x66 ,0x6a ,0x97},
{0xdd ,0xd6 ,0xde},
{0x9e ,0x8c ,0xa8},
{0xb9 ,0x96 ,0x8a},
{0xb6 ,0xbf ,0xda},
{0x76 ,0x6a ,0x87},
{0x54 ,0x49 ,0x66},
{0xee ,0xe7 ,0xef},
{0xdc ,0xe8 ,0xff},
{0xf9 ,0xdb ,0xec},
{0x49 ,0x5a ,0x91},
{0x67 ,0x55 ,0x49},
{0x65 ,0x59 ,0x76},
{0xbe ,0xa5 ,0x9a},
{0xaa ,0x95 ,0x88},
{0xe8 ,0xce ,0xe4},
{0x8b ,0x73 ,0x6e},
{0x85 ,0x4a ,0x54},
{0xf8 ,0xde ,0xf8},
{0xf7 ,0xe7 ,0xee},
{0x59 ,0x57 ,0x4a},
{0x84 ,0x4d ,0x35},
{0x84 ,0x4a ,0x49},
{0xa6 ,0x6d ,0x53},
{0xe2 ,0xbc ,0xbe},
{0x94 ,0x59 ,0x5f},
{0x59 ,0x63 ,0x79},
{0x9e ,0x6b ,0x6b},
{0xc0 ,0x8c ,0xa2},
{0xb1 ,0x7a ,0x82},
{0x49 ,0x52 ,0x6a},
{0xc8 ,0x92 ,0x84},
{0x69 ,0x74 ,0x8a},
{0x75 ,0x7b ,0xb8},
{0x39 ,0x4b ,0x7f},
{0x4a ,0x63 ,0x9e},
{0x63 ,0x6b ,0xa7},
{0x53 ,0x5a ,0x94},
{0x77 ,0x5a ,0x75},
{0xce ,0xd6 ,0xec},
{0xad ,0xb5 ,0xc4},
{0x75 ,0x6b ,0x95},
{0x6b ,0x81 ,0xc7},
{0x42 ,0x4a ,0x84},
{0x5a ,0x73 ,0xa0},
{0x79 ,0x73 ,0x6b},
{0x5f ,0x63 ,0x4e},
{0x64 ,0x5a ,0x86},
{0xfc ,0xc8 ,0xdd},
{0x54 ,0x4a ,0x74},
{0x95 ,0x5b ,0x44},
{0x97 ,0x4a ,0x5b},
{0xb6 ,0x79 ,0x65},
{0xe8 ,0xf2 ,0xff},
{0x9f ,0x57 ,0x6b},
{0x4a ,0x5f ,0xaa},
{0xb9 ,0xb5 ,0xad},
{0x9f ,0x8c ,0xb6},
{0x66 ,0x4a ,0x64},
{0x4a ,0x63 ,0x89},
{0xfe ,0xe0 ,0xdb},
{0x5a ,0x73 ,0x8c},
{0x76 ,0x4a ,0x63},
{0x9f ,0x50 ,0x5a},
{0xe9 ,0xce ,0xf8},
{0xce ,0xd6 ,0xde},
{0x4a ,0x52 ,0x59},
{0xaf ,0x6a ,0x7e},
{0x6b ,0x73 ,0x7a},
{0x75 ,0x39 ,0x46},
{0xc4 ,0x92 ,0xb5},
{0x8f ,0x43 ,0x4a},
{0xa2 ,0x9c ,0xad},
{0x35 ,0x35 ,0x2f},
{0xf7 ,0xce ,0xf1},
{0xa2 ,0x9c ,0xb5},
{0xb3 ,0xad ,0xb4},
{0xa3 ,0x9c ,0xc2},
{0xad ,0x9c ,0xad},
{0xb3 ,0xad ,0xc0},
{0x76 ,0x3b ,0x29},
{0xbb ,0x83 ,0x63},
{0xbf ,0xb5 ,0xc4},
{0xa2 ,0x9c ,0xa4},
{0x65 ,0x4a ,0x73},
{0x6c ,0x6b ,0xa5},
{0xff ,0xef ,0xff},
{0x5a ,0x72 ,0xb5},
{0xad ,0x9c ,0xb5},
{0xb5 ,0xa5 ,0xb5},
{0xa0 ,0x94 ,0xa0},
{0xd7 ,0xd6 ,0xd5},
{0xa5 ,0x94 ,0xad},
{0xbf ,0xad ,0xc1},
{0xf7 ,0xef ,0xff},
{0xb0 ,0xa5 ,0xc4},
{0x84 ,0x52 ,0x52},
{0xbe ,0xad ,0xb3},
{0xf7 ,0xef ,0xf7},
{0x97 ,0x9e ,0x9d},
{0xad ,0x9c ,0xa3},
{0xf7 ,0xe7 ,0xf7},
{0xb5 ,0xb5 ,0xc1},
{0xc6 ,0x83 ,0x6c},
{0xad ,0x9c ,0xc5},
{0xc6 ,0xc6 ,0xde},
{0x8c ,0x5a ,0x5a},
{0xa5 ,0x94 ,0xb5},
{0xff ,0xef ,0xf7},
{0xce ,0xc6 ,0xd6},
{0xaf ,0x94 ,0xa5},
{0xef ,0xde ,0xef},
{0xce ,0xc6 ,0xde},
{0xd6 ,0xce ,0xde},
{0x84 ,0x52 ,0x5b},
{0xae ,0x94 ,0xad},
{0x84 ,0x5a ,0x5a},
{0x42 ,0x50 ,0x92},
{0xb5 ,0xb5 ,0xb5},
{0xd6 ,0xc3 ,0xd5},
{0xe5 ,0xde ,0xef},
{0xe7 ,0xd6 ,0xe6},
{0xde ,0xce ,0xe1},
{0xd6 ,0xce ,0xd6},
{0x4b ,0x4b ,0x4a},
{0xbe ,0xa5 ,0xb1},
{0xc6 ,0xc6 ,0xd6},
{0xc3 ,0xb5 ,0xb4},
{0xf7 ,0xe7 ,0xff},
{0x84 ,0x5a ,0x4d},
{0x8d ,0x52 ,0x5c},
{0xd6 ,0xc5 ,0xe1},
{0x9d ,0x94 ,0xc0},
{0xbf ,0xa5 ,0xc1},
{0xe6 ,0xde ,0xe7},
{0x8c ,0x52 ,0x51},
{0xce ,0xce ,0xe4},
{0xe7 ,0xd6 ,0xf0},
{0xff ,0xe7 ,0xff},
{0xae ,0x94 ,0xb5},
{0x8c ,0x5a ,0x51},
{0xff ,0xe7 ,0xf6},
{0xce ,0xce ,0xd5},
{0xc2 ,0xbd ,0xc7},
{0xef ,0xd6 ,0xf0},
{0xf0 ,0xd6 ,0xe5},
{0xde ,0xce ,0xd5},
{0xe0 ,0xc5 ,0xdf},
{0xe3 ,0xc1 ,0xd2},
{0xef ,0xde ,0xe6},
{0xad ,0x94 ,0xc0},
{0xc6 ,0xce ,0xe4},
{0xc6 ,0xce ,0xd5},
{0xb5 ,0xbd ,0xcd},
{0xc7 ,0xbd ,0xb5},
{0xad ,0xb5 ,0xad},
{0xb5 ,0xbd ,0xb5},
{0x4a ,0x4a ,0x84},
{0x84 ,0x84 ,0x88},
{0xbd ,0xc6 ,0xc5},
{0xff ,0xf7 ,0xef},
{0xce ,0xd6 ,0xce},
{0x9b ,0x80 ,0x5f},
{0x00 ,0x00 ,0x00},
{0xff ,0xff ,0xff}};
        // /**/
        // { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x2A },   /* palette 0x00 - 0x0F    */
        // { 0x00, 0x2A, 0x00 },{ 0x00, 0x2A, 0x2A },   /* basic VGA colors       */
        // { 0x2A, 0x00, 0x00 },{ 0x2A, 0x00, 0x2A },
        // { 0x2A, 0x15, 0x00 },{ 0x2A, 0x2A, 0x2A },
        // { 0x15, 0x15, 0x15 },{ 0x15, 0x15, 0x3F },
        // { 0x15, 0x3F, 0x15 },{ 0x15, 0x3F, 0x3F },
        // { 0x3F, 0x15, 0x15 },{ 0x3F, 0x15, 0x3F },
        // { 0x3F, 0x3F, 0x15 },{ 0x3F, 0x3F, 0x3F }}


// static unsigned short target_img;   /* offset of displayed screen image */

/*
 * macro used to target a specific video plane or planes when writing
 * to video memory in mode X; bits 8-11 in the mask_hi_bits enable writes
 * to planes 0-3, respectively
 */
#define SET_WRITE_MASK(mask_hi_bits)                                \
do {                                                                \
    asm volatile ("                                               \n\
        movw $0x03C4, %%dx    /* set write mask */                \n\
        movb $0x02, %b0                                           \n\
        outw %w0, (%%dx)                                          \n\
        "                                                           \
        : /* no outputs */                                          \
        : "a"((mask_hi_bits))                                       \
        : "edx", "memory"                                           \
    );                                                              \
} while (0)

/* macro used to write a byte to a port */
#define OUTB(port, val)                                             \
do {                                                                \
    asm volatile ("outb %b1, (%w0)"                                 \
        : /* no outputs */                                          \
        : "d"((port)), "a"((val))                                   \
        : "memory", "cc"                                            \
    );                                                              \
} while (0)

/* macro used to write two bytes to two consecutive ports */
#define OUTW(port, val)                                             \
do {                                                                \
    asm volatile ("outw %w1, (%w0)"                                 \
        : /* no outputs */                                          \
        : "d"((port)), "a"((val))                                   \
        : "memory", "cc"                                            \
    );                                                              \
} while (0)

/* macro used to write an array of two-byte values to two consecutive ports */
#define REP_OUTSW(port, source, count)                              \
do {                                                                \
    asm volatile ("                                               \n\
        1: movw 0(%1), %%ax                                       \n\
        outw %%ax, (%w2)                                          \n\
        addl $2, %1                                               \n\
        decl %0                                                   \n\
        jne 1b                                                    \n\
        "                                                           \
        : /* no outputs */                                          \
        : "c"((count)), "S"((source)), "d"((port))                  \
        : "eax", "memory", "cc"                                     \
    );                                                              \
} while (0)

/* macro used to write an array of one-byte values to two consecutive ports */
#define REP_OUTSB(port, source, count)                              \
do {                                                                \
    asm volatile ("                                               \n\
        1: movb 0(%1), %%al                                       \n\
        outb %%al, (%w2)                                          \n\
        incl %1                                                   \n\
        decl %0                                                   \n\
        jne 1b                                                    \n\
        "                                                           \
        : /* no outputs */                                          \
        : "c"((count)), "S"((source)), "d"((port))                  \
        : "eax", "memory", "cc"                                     \
    );                                                              \
} while (0)

/*
 * set_palette_color
 *   DESCRIPTION: set the palette color for one index 
 *   INPUTS: color_index -- the index of color we want to set
 *           RGB -- the intensity of red/green/blue
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set the palette color for given index 
 */
void set_palette_color(unsigned char color_index, unsigned char* RGB){
    /* Start writing at color 0. */
    OUTB(0x03C8, color_index);
    /* Write all 64 colors from array. */
    REP_OUTSB(0x03C9, RGB,  3 ); // RGB(3)
}

/* VGA register settings for mode X */
static unsigned short mode_X_seq[NUM_SEQUENCER_REGS] = {
    0x0100, 0x2101, 0x0F02, 0x0003, 0x0604
};
// here we change the compare line field to （200-18）*2-1 i.e. (pixel hight of X mode - pixel hight of bar) * （(number of horizontal scan line for screen)/ (hight of pixels for screen)） - 1
static unsigned short mode_X_CRTC[NUM_CRTC_REGS] = {
    0x5F00, 0x4F01, 0x5002, 0x8203, 0x5404, 0x8005, 0xBF06, 0x1F07,
    0x0008, 0x0109, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x9C10, 0x8E11, 0x8F12, 0x2813, 0x0014, 0x9615, 0xB916, 0xE317,
    0x6B18
};
static unsigned char mode_X_attr[NUM_ATTR_REGS * 2] = {
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03,
    0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B,
    0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F,
    0x10, 0x41, 0x11, 0x00, 0x12, 0x0F, 0x13, 0x00,
    0x14, 0x00, 0x15, 0x00
};
static unsigned short mode_X_graphics[NUM_GRAPHICS_REGS] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x4005, 0x0506, 0x0F07,
    0xFF08
};

/* VGA register settings for text mode 3 (color text) */
static unsigned short text_seq[NUM_SEQUENCER_REGS] = {
    0x0100, 0x2001, 0x0302, 0x0003, 0x0204
};
static unsigned short text_CRTC[NUM_CRTC_REGS] = {
    0x5F00, 0x4F01, 0x5002, 0x8203, 0x5504, 0x8105, 0xBF06, 0x1F07,
    0x0008, 0x4F09, 0x0D0A, 0x0E0B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x9C10, 0x8E11, 0x8F12, 0x2813, 0x1F14, 0x9615, 0xB916, 0xA317,
    0xFF18
};
static unsigned char text_attr[NUM_ATTR_REGS * 2] = {
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03,
    0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B,
    0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F,
    0x10, 0x0C, 0x11, 0x00, 0x12, 0x0F, 0x13, 0x08,
    0x14, 0x00, 0x15, 0x00
};
static unsigned short text_graphics[NUM_GRAPHICS_REGS] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x1005, 0x0E06, 0x0007,
    0xFF08
};


/*
 * switch_to_modeX
 *   DESCRIPTION: Puts the VGA into mode X.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: clears (or preset) video memory
 */
extern int32_t switch_to_modeX(){
    int32_t i;
    uint8_t* VM_addr = (uint8_t*)(VIDEO);  
    uint8_t* VM_ACCESS_addr = (uint8_t*)(VIRTUAL_ADDR_AlWAYS_ACCESS_VEDIO_PAGE);  
    mem_temp_v= (unsigned char*) TEMP_STOR_FOR_MODEX; //i.e. 0x900000
    // sti();
    // WARNING_PCS();
    // cli();
    // /* set video memory map */
    // page_table[VIDEO_REGION_START_K].address = VIDEO_REGION_START_K +  (terminal_display != terminal_tick) * (terminal_tick + 1) +in_modex*(TEMP_ADDR_VEDIO_PAGE-VIDEO)/_4KB_; /* set for kernel */
    // page_table_vedio_mem[VIDEO_REGION_START_U].address =  VIDEO_REGION_START_K + (terminal_display != terminal_tick) * (terminal_tick + 1) +in_modex*(TEMP_ADDR_VEDIO_PAGE-VIDEO)/_4KB_; /* set for user */
    
    /* copy 0xB9000--0xB8000+4KB*4 to 0x9000000+4KB--0x9000000+4KB*4 for storage of text screens */
    for (i = _4KB_; i < _4KB_*4; i++) {mem_temp_v[i] = (VM_addr)[i];}
    /* copy physical 0xB8000--0xB8000+4KB to 0x9800000--0x9800000+4KB for storage of text screens */
    for (i = 0; i < _4KB_; i++) {mem_temp_v[i] = (VM_ACCESS_addr)[i];}
    /* set the start of mem_image of ModeX */
    mem_image= (unsigned char*) MODEX_STR_ADDR;

    ////
    /* Initialize the logical view window to position (0,0). */
    show_x = show_y = 0;
    img3_off = BUILD_BASE_INIT;
    img3 = build + img3_off + MEM_FENCE_WIDTH;

    /* Set up the memory fence on the build buffer. */
    for (i = 0; i < MEM_FENCE_WIDTH; i++) {
        build[i] = MEM_FENCE_MAGIC;
        build[BUILD_BUF_SIZE + MEM_FENCE_WIDTH + i] = MEM_FENCE_MAGIC;
    }

    /* One display page goes at the start of video memory. */
    target_img = 18* IMAGE_X_WIDTH; // 18 = text hight+ 2 pixel.  Here we just add offset for the bar

    VGA_blank(1);                               /* blank the screen      */
    set_seq_regs_and_reset(mode_X_seq, 0x63);   /* sequencer registers   */
    set_CRTC_registers(mode_X_CRTC);            /* CRT control registers */
    set_attr_registers(mode_X_attr);            /* attribute registers   */
    set_graphics_registers(mode_X_graphics);    /* graphics registers    */
    fill_palette();                             /* palette colors        */
    clear_screens();                            /* zero video memory     */
    VGA_blank(0);                               /* unblank the screen    */

    in_modex=1;
    
    /* Return success. */
    return 0;
}


/*
 * VGA_blank
 *   DESCRIPTION: Blank or unblank the VGA display.
 *   INPUTS: blank_bit -- set to 1 to blank, 0 to unblank
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
static void VGA_blank(int blank_bit) {
    /*
     * Move blanking bit into position for VGA sequencer register
     * (index 1).
     */
    blank_bit = ((blank_bit & 1) << 5);

    asm volatile ("                                                    \n\
        movb $0x01, %%al         /* Set sequencer index to 1.       */ \n\
        movw $0x03C4, %%dx                                             \n\
        outb %%al, (%%dx)                                              \n\
        incw %%dx                                                      \n\
        inb  (%%dx), %%al        /* Read old value.                 */ \n\
        andb $0xDF, %%al         /* Calculate new value.            */ \n\
        orl  %0, %%eax                                                 \n\
        outb %%al, (%%dx)        /* Write new value.                */ \n\
        movw $0x03DA, %%dx       /* Enable display (0x20->P[0x3C0]) */ \n\
        inb  (%%dx), %%al        /* Set attr reg state to index.    */ \n\
        movw $0x03C0, %%dx       /* Write index 0x20 to enable.     */ \n\
        movb $0x20, %%al                                               \n\
        outb %%al, (%%dx)                                              \n\
        "
        :
        : "g"(blank_bit)
        : "eax", "edx", "memory"
    );
}

/*
 * set_seq_regs_and_reset
 *   DESCRIPTION: Set VGA sequencer registers and miscellaneous output
 *                register; array of registers should force a reset of
 *                the VGA sequencer, which is restored to normal operation
 *                after a brief delay.
 *   INPUTS: table -- table of sequencer register values to use
 *           val -- value to which miscellaneous output register should be set
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
static void set_seq_regs_and_reset(unsigned short table[NUM_SEQUENCER_REGS], unsigned char val) {
    /*
     * Dump table of values to sequencer registers.  Includes forced reset
     * as well as video blanking.
     */
    REP_OUTSW(0x03C4, table, NUM_SEQUENCER_REGS);

    /* Delay a bit... */
    {volatile int ii; for (ii = 0; ii < 10000; ii++);}

    /* Set VGA miscellaneous output register. */
    OUTB(0x03C2, val);

    /* Turn sequencer on (array values above should always force reset). */
    OUTW(0x03C4, 0x0300);
}

/*
 * set_CRTC_registers
 *   DESCRIPTION: Set VGA cathode ray tube controller (CRTC) registers.
 *   INPUTS: table -- table of CRTC register values to use
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
static void set_CRTC_registers(unsigned short table[NUM_CRTC_REGS]) {
    /* clear protection bit to enable write access to first few registers */
    OUTW(0x03D4, 0x0011);
    REP_OUTSW(0x03D4, table, NUM_CRTC_REGS);
}

/*
 * set_attr_registers
 *   DESCRIPTION: Set VGA attribute registers.  Attribute registers use
 *                a single port and are thus written as a sequence of bytes
 *                rather than a sequence of words.
 *   INPUTS: table -- table of attribute register values to use
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
static void set_attr_registers(unsigned char table[NUM_ATTR_REGS * 2]) {
    /* Reset attribute register to write index next rather than data. */
    asm volatile ("inb (%%dx),%%al"
        :
        : "d"(0x03DA)
        : "eax", "memory"
    );
    REP_OUTSB(0x03C0, table, NUM_ATTR_REGS * 2);
}

/*
 * set_graphics_registers
 *   DESCRIPTION: Set VGA graphics registers.
 *   INPUTS: table -- table of graphics register values to use
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
static void set_graphics_registers(unsigned short table[NUM_GRAPHICS_REGS]) {
    REP_OUTSW(0x03CE, table, NUM_GRAPHICS_REGS);
}

/*
 * fill_palette
 *   DESCRIPTION: Fill VGA palette with necessary colors for the maze game.
 *                Only the first 64 (of 256) colors are written.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes the first 64 palette colors
 */
static void fill_palette() {
    /* 6-bit RGB (red, green, blue) values for first 64 colors */
    static unsigned char palette_RGB[NUM_NONTRANS_COLOR*2][3] = {
        /**/
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x2A },   /* palette 0x00 - 0x0F    */
        { 0x00, 0x2A, 0x00 },{ 0x00, 0x2A, 0x2A },   /* basic VGA colors       */
        { 0x2A, 0x00, 0x00 },{ 0x2A, 0x00, 0x2A },
        { 0x2A, 0x15, 0x00 },{ 0x2A, 0x2A, 0x2A },
        { 0x15, 0x15, 0x15 },{ 0x15, 0x15, 0x3F },
        { 0x15, 0x3F, 0x15 },{ 0x15, 0x3F, 0x3F },
        { 0x3F, 0x15, 0x15 },{ 0x3F, 0x15, 0x3F },
        { 0x3F, 0x3F, 0x15 },{ 0x3F, 0x3F, 0x3F },
        /*------------- Used by GUI -------------*/
        /*
         * 0x10 - color of icon
         * 0x24 - status bar backgroud color
         * 0x25 - status bar text color
         */
        { 0xA8, 0xA8, 0xA8 },{ 0xA8, 0xA8, 0xA8 },   /* palette 0x10 - 0x1F    */
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },   
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        /*---------------------------------------*/
        { 0x3F, 0x3F, 0x3F },{ 0x3F, 0x3F, 0x3F },   /* palette 0x20 - 0x2F    */    // PLAYER_CENTER_COLOR 0x20 @@ WALL_OUTLINE_COLOR  0x21
        { 0x00, 0x00, 0x3F },{ 0x00, 0x00, 0x00 },   /* User define color  */        // WALL_FILL_COLOR     0x22 
        { 0xA8, 0xA8, 0xA8 },{ 0x00, 0x00, 0x00 },                                   // STATUS_BACK_COLOR   0x24 @@ STATUS_TEXT_COLOR   0x25
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        /*---------------------------------------*/
        { 0x10, 0x08, 0x00 },{ 0x18, 0x0C, 0x00 },   /* palette 0x30 - 0x3F    */
        { 0x20, 0x10, 0x00 },{ 0x28, 0x14, 0x00 },   /* browns for maze floor  */
        { 0x30, 0x18, 0x00 },{ 0x38, 0x1C, 0x00 },
        { 0x3F, 0x20, 0x00 },{ 0x3F, 0x20, 0x10 },
        { 0x20, 0x18, 0x10 },{ 0x28, 0x1C, 0x10 },
        { 0x3F, 0x20, 0x10 },{ 0x38, 0x24, 0x10 },
        { 0x3F, 0x28, 0x10 },{ 0x3F, 0x2C, 0x10 },
        { 0x3F, 0x30, 0x10 },{ 0x3F, 0x20, 0x10 },
/////////////////////////below is Transparent///////////////////////////////////////////
        { (0x00/2+WHITE/2), (0x00/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x00/2 + WHITE/2), (0x00/2+WHITE/2), (0x2A/2+WHITE/2) },   /* palette 0x00 - 0x0F    */
        { (0x00/2+WHITE/2), (0x2A/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x00/2 + WHITE/2), (0x2A/2+WHITE/2), (0x2A/2+WHITE/2) },   /* basic VGA colors       */
        { (0x2A/2+WHITE/2), (0x00/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x2A/2 + WHITE/2), (0x00/2+WHITE/2), (0x2A/2+WHITE/2) },
        { (0x2A/2+WHITE/2), (0x15/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x2A/2 + WHITE/2), (0x2A/2+WHITE/2), (0x2A/2+WHITE/2) },
        { (0x15/2+WHITE/2), (0x15/2 + WHITE/2), (0x15/2 + WHITE/2) },{ (0x15/2 + WHITE/2), (0x15/2+WHITE/2), (0x3F/2+WHITE/2) },
        { (0x15/2+WHITE/2), (0x3F/2 + WHITE/2), (0x15/2 + WHITE/2) },{ (0x15/2 + WHITE/2), (0x3F/2+WHITE/2), (0x3F/2+WHITE/2) },
        { (0x3F/2+WHITE/2), (0x15/2 + WHITE/2), (0x15/2 + WHITE/2) },{ (0x3F/2 + WHITE/2), (0x15/2+WHITE/2), (0x3F/2+WHITE/2) },
        { (0x3F/2+WHITE/2), (0x3F/2 + WHITE/2), (0x15/2 + WHITE/2) },{ (0x3F/2 + WHITE/2), (0x3F/2+WHITE/2), (0x3F/2+WHITE/2) },
        { (0x00/2+WHITE/2), (0x00/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x05/2 + WHITE/2), (0x05/2+WHITE/2), (0x05/2+WHITE/2) },   /* palette 0x10 - 0x1F    */
        { (0x08/2+WHITE/2), (0x08/2 + WHITE/2), (0x08/2 + WHITE/2) },{ (0x0B/2 + WHITE/2), (0x0B/2+WHITE/2), (0x0B/2+WHITE/2) },   /* VGA grey scale         */
        { (0x0E/2+WHITE/2), (0x0E/2 + WHITE/2), (0x0E/2 + WHITE/2) },{ (0x11/2 + WHITE/2), (0x11/2+WHITE/2), (0x11/2+WHITE/2) },
        { (0x14/2+WHITE/2), (0x14/2 + WHITE/2), (0x14/2 + WHITE/2) },{ (0x18/2 + WHITE/2), (0x18/2+WHITE/2), (0x18/2+WHITE/2) },
        { (0x1C/2+WHITE/2), (0x1C/2 + WHITE/2), (0x1C/2 + WHITE/2) },{ (0x20/2 + WHITE/2), (0x20/2+WHITE/2), (0x20/2+WHITE/2) },
        { (0x24/2+WHITE/2), (0x24/2 + WHITE/2), (0x24/2 + WHITE/2) },{ (0x28/2 + WHITE/2), (0x28/2+WHITE/2), (0x28/2+WHITE/2) },
        { (0x2D/2+WHITE/2), (0x2D/2 + WHITE/2), (0x2D/2 + WHITE/2) },{ (0x32/2 + WHITE/2), (0x32/2+WHITE/2), (0x32/2+WHITE/2) },
        { (0x38/2+WHITE/2), (0x38/2 + WHITE/2), (0x38/2 + WHITE/2) },{ (0x3F/2 + WHITE/2), (0x3F/2+WHITE/2), (0x3F/2+WHITE/2) },
        { (0x3F/2+WHITE/2), (0x3F/2 + WHITE/2), (0x3F/2 + WHITE/2) },{ (0x3F/2 + WHITE/2), (0x3F/2+WHITE/2), (0x3F/2+WHITE/2) },   /* palette 0x20 - 0x2F    */
        { (0x00/2+WHITE/2), (0x00/2 + WHITE/2), (0x3F/2 + WHITE/2) },{ (0x00/2 + WHITE/2), (0x00/2+WHITE/2), (0x00/2+WHITE/2) },   /* wall and player colors */
        { (0x00/2+WHITE/2), (0x00/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x00/2 + WHITE/2), (0x00/2+WHITE/2), (0x00/2+WHITE/2) },
        { (0x00/2+WHITE/2), (0x00/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x00/2 + WHITE/2), (0x00/2+WHITE/2), (0x00/2+WHITE/2) },
        { (0x00/2+WHITE/2), (0x00/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x00/2 + WHITE/2), (0x00/2+WHITE/2), (0x00/2+WHITE/2) },
        { (0x00/2+WHITE/2), (0x00/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x00/2 + WHITE/2), (0x00/2+WHITE/2), (0x00/2+WHITE/2) },
        { (0x00/2+WHITE/2), (0x00/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x00/2 + WHITE/2), (0x00/2+WHITE/2), (0x00/2+WHITE/2) },
        { (0x00/2+WHITE/2), (0x00/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x00/2 + WHITE/2), (0x00/2+WHITE/2), (0x00/2+WHITE/2) },
        { (0x10/2+WHITE/2), (0x08/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x18/2 + WHITE/2), (0x0C/2+WHITE/2), (0x00/2+WHITE/2) },   /* palette 0x30 - 0x3F    */
        { (0x20/2+WHITE/2), (0x10/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x28/2 + WHITE/2), (0x14/2+WHITE/2), (0x00/2+WHITE/2) },   /* browns for maze floor  */
        { (0x30/2+WHITE/2), (0x18/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x38/2 + WHITE/2), (0x1C/2+WHITE/2), (0x00/2+WHITE/2) },
        { (0x3F/2+WHITE/2), (0x20/2 + WHITE/2), (0x00/2 + WHITE/2) },{ (0x3F/2 + WHITE/2), (0x20/2+WHITE/2), (0x10/2+WHITE/2) },
        { (0x20/2+WHITE/2), (0x18/2 + WHITE/2), (0x10/2 + WHITE/2) },{ (0x28/2 + WHITE/2), (0x1C/2+WHITE/2), (0x10/2+WHITE/2) },
        { (0x3F/2+WHITE/2), (0x20/2 + WHITE/2), (0x10/2 + WHITE/2) },{ (0x38/2 + WHITE/2), (0x24/2+WHITE/2), (0x10/2+WHITE/2) },
        { (0x3F/2+WHITE/2), (0x28/2 + WHITE/2), (0x10/2 + WHITE/2) },{ (0x3F/2 + WHITE/2), (0x2C/2+WHITE/2), (0x10/2+WHITE/2) },
        { (0x3F/2+WHITE/2), (0x30/2 + WHITE/2), (0x10/2 + WHITE/2) },{ (0x3F/2 + WHITE/2), (0x20/2+WHITE/2), (0x10/2+WHITE/2) },
    };

    /* Start writing at color 0. */
    OUTB(0x03C8, 0x00);

    /* Write all 64 colors from array. */
    REP_OUTSB(0x03C9, palette_RGB, 64 * 3 * 2); // number of non-transparent color(64) * RGB(3) *2(transparent+non-transparent)
}

/*
 * fill_palette
 *   DESCRIPTION: Fill VGA palette with necessary colors for the maze game.
 *                Only the first 64 (of 256) colors are written.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes the first 64 palette colors
 */
void fill_palette_vedio() {
    /* 6-bit RGB (red, green, blue) values for first 64 colors */
    /* Start writing at color 0. */
    OUTB(0x03C8, 0x00);
    /* Write all 64 colors from array. */
    REP_OUTSB(0x03C9, palette_RGB_vedio, 64 * 3 * 4); // number of non-transparent color(64) * RGB(3) *2(transparent+non-transparent)
}

/*
 * clear_screens
 *   DESCRIPTION: Fills the video memory with zeroes.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: fills all 256kB of VGA video memory with zeroes
 */
void clear_screens() {
    /* Write to all four planes at once. */
    SET_WRITE_MASK(0x0F00);

    /* Set 64kB to zero (times four planes = 256kB). */
    memset(mem_image, 0, MODE_X_MEM_SIZE/2);//// try the only write 32KB  
       
}



/*
 * write_font_data
 *   DESCRIPTION: Copy font data into VGA memory, changing and restoring
 *                VGA register values in order to do so.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: leaves VGA registers in final text mode state
 */
static void write_font_data() {
    int i;                /* loop index over characters                   */
    int j;                /* loop index over font bytes within characters */
    unsigned char* fonts; /* pointer into video memory                    */

    /* Prepare VGA to write font data into video memory. */
    OUTW(0x3C4, 0x0402);
    OUTW(0x3C4, 0x0704);
    OUTW(0x3CE, 0x0005);
    OUTW(0x3CE, 0x0406);
    OUTW(0x3CE, 0x0204);

    /* Copy font data from array into video memory. */
    for (i = 0, fonts = mem_image; i < 256; i++) {
        for (j = 0; j < 16; j++)
            fonts[j] = font_data[i][j];
        fonts += 32; /* skip 16 bytes between characters */
    }

    /* Prepare VGA for text mode. */
    OUTW(0x3C4, 0x0302);
    OUTW(0x3C4, 0x0304);
    OUTW(0x3CE, 0x1005);
    OUTW(0x3CE, 0x0E06);
    OUTW(0x3CE, 0x0004);
}

/*
 * set_text_mode_3
 *   DESCRIPTION: Put VGA into text mode 3 (color text).
 *   INPUTS: clear_scr -- if non-zero, clear screens; otherwise, do not
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: may clear screens; writes font data to video memory
 */
extern void set_text_mode_3(int clear_scr) {
    unsigned long* txt_scr;     /* pointer to text screens in video memory */
    int i;                      /* loop over text screen words             */
    int32_t j;
    uint8_t* VM_addr = (uint8_t*)(VIDEO);  
    uint8_t* VM_ACCESS_addr = (uint8_t*)(VIRTUAL_ADDR_AlWAYS_ACCESS_VEDIO_PAGE);  
    mem_temp_v= (unsigned char*) TEMP_STOR_FOR_MODEX; //i.e. 0x900000

    VGA_blank(1);                               /* blank the screen        */
    /*
     * The value here had been changed to 0x63, but seems to work
     * fine in QEMU (and VirtualPC, where I got it) with the 0x04
     * bit set (VGA_MIS_DCLK_28322_720).
     */
    set_seq_regs_and_reset(text_seq, 0x67);     /* sequencer registers     */
    set_CRTC_registers(text_CRTC);              /* CRT control registers   */
    set_attr_registers(text_attr);              /* attribute registers     */
    set_graphics_registers(text_graphics);      /* graphics registers      */
    fill_palette();                             /* palette colors          */

    /* restore 0xB9000--0xB8000+4KB*4 from 0x9000000+4KB--0x9000000+4KB*4 for restorage of text screens */
    for (j = _4KB_; j < _4KB_*4; j++) {VM_addr[j] = (mem_temp_v)[j];}
    /* restore physical 0xB8000--0xB8000+4KB from 0x9800000--0x9800000+4KB for storage of text screens */
    for (i = 0; i < _4KB_; i++) {VM_ACCESS_addr[i] = (mem_temp_v)[i];}

    // //reload VGA
    // // /* copy 0xB8000--0xB8000+4KB*4 to 0x9000000--0x9000000+4KB*4 for storage of text screens */
    // for (i = 0; i < _4KB_*4; i++) {mem_temp_v[i] = (VM_addr)[i];}
    // /* restore 0xB8000--0xB8000+4KB*4 from 0x9000000--0x9000000+4KB*4 for restorage of text screens */
    // for (j = 0; j < _4KB_*4; j++) {VM_addr[j] = (mem_temp_v)[j];}

    if (clear_scr) {                            /* clear screens if needed */
        txt_scr = (unsigned long*)(mem_image + 0x18000);
        for (i = 0; i < 8192; i++)
            *txt_scr++ = 0x07200720;
    }
    write_font_data();                          /* copy fonts to video mem */
    VGA_blank(0);                               /* unblank the screen      */
    in_modex=0;

}



/*
 * set_view_window
 *   DESCRIPTION: Set the logical view window, moving its location within
 *                the build buffer if necessary to keep all on-screen data
 *                in the build buffer.  If the location within the build
 *                buffer moves, this function copies all data from the old
 *                window that are within the new screen to the appropriate
 *                new location, so only data not previously on the screen
 *                must be drawn before calling show_screen.
 *   INPUTS: (scr_x,scr_y) -- new upper left pixel of logical view window
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: may shift position of logical view window within build
 *                 buffer
 */
void set_view_window(int scr_x, int scr_y) {
    int old_x, old_y;       /* old position of logical view window           */
    int start_x, start_y;   /* starting position for copying from old to new */
    int end_x, end_y;       /* ending position for copying from old to new   */
    int start_off;          /* offset of copy start relative to old build    */
                            /*    buffer start position                      */
    int length;             /* amount of data to be copied                   */
    int i;                  /* copy loop index                               */
    unsigned char* start_addr;  /* starting memory address of copy     */
    unsigned char* target_addr; /* destination memory address for copy */

    /* Record the old position. */
    old_x = show_x;
    old_y = show_y;

    /* Keep track of the new view window. */
    show_x = scr_x;
    show_y = scr_y;

    /*
     * If the new view window fits within the boundaries of the build
     * buffer, we need move nothing around.
     */
    if (img3_off + (scr_x >> 2) + scr_y * SCROLL_X_WIDTH >= 0 &&
        img3_off + 3 * SCROLL_SIZE +
        ((scr_x + SCROLL_X_DIM - 1) >> 2) +
        (scr_y + SCROLL_Y_DIM - 1) * SCROLL_X_WIDTH < BUILD_BUF_SIZE)
        return;

    /*
     * If the new screen does not overlap at all with the old screen, none
     * of the old data need to be saved, and we can simply reposition the
     * valid window of the build buffer in the middle of that buffer.
     */
    if (scr_x <= old_x - SCROLL_X_DIM || scr_x >= old_x + SCROLL_X_DIM ||
        scr_y <= old_y - SCROLL_Y_DIM || scr_y >= old_y + SCROLL_Y_DIM) {
        img3_off = BUILD_BASE_INIT - (scr_x >> 2) - scr_y * SCROLL_X_WIDTH;
        img3 = build + img3_off + MEM_FENCE_WIDTH;
        return;
    }

    /*
     * Any still-visible portion of the old screen should be retained.
     * Rather than clipping exactly, we copy all contiguous data between
     * a clipped starting point to a clipped ending point (which may
     * include non-visible data).
     *
     * The starting point is the maximum (x,y) coordinates between the
     * new and old screens.  The ending point is the minimum (x,y)
     * coordinates between the old and new screens (offset by the screen
     * size).
     */
    if (scr_x > old_x) {
        start_x = scr_x;
        end_x = old_x;
    }
    else {
        start_x = old_x;
        end_x = scr_x;
    }
    end_x += SCROLL_X_DIM - 1;
    if (scr_y > old_y) {
        start_y = scr_y;
        end_y = old_y;
    }
    else {
        start_y = old_y;
        end_y = scr_y;
    }
    end_y += SCROLL_Y_DIM - 1;

    /*
     * We now calculate the starting and ending addresses for the copy
     * as well as the new offsets for use with the build buffer.  The
     * length to be copied is basically the ending offset minus the starting
     * offset plus one (plus the three screens in between planes 3 and 0).
     */
    start_off = (start_x >> 2) + start_y * SCROLL_X_WIDTH;
    start_addr = img3 + start_off;
    length = (end_x >> 2) + end_y * SCROLL_X_WIDTH + 1 - start_off + 3 * SCROLL_SIZE;
    img3_off = BUILD_BASE_INIT - (show_x >> 2) - show_y * SCROLL_X_WIDTH;
    img3 = build + img3_off + MEM_FENCE_WIDTH;
    target_addr = img3 + start_off;

    /*
     * Copy the relevant portion of the screen from the old location to the
     * new one.  The areas may overlap, so copy direction is important.
     * (You should be able to explain why!)
     */
    if (start_addr < target_addr)
        for (i = length; i-- > 0; )
            target_addr[i] = start_addr[i];
    else
        for (i = 0; i < length; i++)
            target_addr[i] = start_addr[i];
}

/*
 * show_screen
 *   DESCRIPTION: Show the logical view window on the video display.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: copies from the build buffer to video memory;
 *                 shifts the VGA display source to point to the new image
 */
void show_screen() {
    unsigned char* addr;    /* source address for copy             */
    int p_off;              /* plane offset of first display plane */
    int i;                  /* loop index over video planes        */

    /*
     * Calculate offset of build buffer plane to be mapped into plane 0
     * of display.
     */
    p_off = (3 - (show_x & 3));

    /* Switch to the other target screen in video memory. */
    target_img ^= 0x4000;

    /* Calculate the source address. */
    addr = img3 + (show_x >> 2) + show_y * SCROLL_X_WIDTH;

    /* Draw to each plane in the video memory. */
    for (i = 0; i < 4; i++) {
        SET_WRITE_MASK(1 << (i + 8));
        copy_image(addr + ((p_off - i + 4) & 3) * SCROLL_SIZE + (p_off < i), target_img);
    }

    /*
     * Change the VGA registers to point the top left of the screen
     * to the video memory that we just filled.
     */
    OUTW(0x03D4, (target_img & 0xFF00) | 0x0C);
    OUTW(0x03D4, ((target_img & 0x00FF) << 8) | 0x0D);

}

extern void switch_another_screen(){
    target_img ^= 0x4000;
    /*
     * Change the VGA registers to point the top left of the screen
     * to the video memory that we just filled.
     */
    OUTW(0x03D4, (target_img & 0xFF00) | 0x0C);
    OUTW(0x03D4, ((target_img & 0x00FF) << 8) | 0x0D);
}


extern void clear_screens_manul() {
    /* Write to all four planes at once. */
    SET_WRITE_MASK(0x0F00);

    /* Set 64kB to zero (times four planes = 256kB). */
    memset(mem_image, 6, MODE_X_MEM_SIZE/2);//// try the only write 32KB  
       
}
extern void change_top_left(){
        /*
     * Change the VGA registers to point the top left of the screen
     * to the video memory that we just filled.
     */
    OUTW(0x03D4, (VIDEO & 0xFF00) | 0x0C);
    OUTW(0x03D4, ((VIDEO & 0x00FF) << 8) | 0x0D);
}

/*
 * draw_vert_line
 *   DESCRIPTION: Draw a vertical map line into the build buffer.  The
 *                line should be offset from the left side of the logical
 *                view window screen by the given number of pixels.
 *   INPUTS: x -- the 0-based pixel column number of the line to be drawn
 *                within the logical view window (equivalent to the number
 *                of pixels from the leftmost pixel to the line to be
 *                drawn)
 *   OUTPUTS: none
 *   RETURN VALUE: Returns 0 on success.  If x is outside of the valid
 *                 SCROLL range, the function returns -1.
 *   SIDE EFFECTS: draws into the build buffer
 */
int draw_vert_line(int x) {
    unsigned char buf[SCROLL_Y_DIM];    /* buffer for graphical image of line */
    unsigned char* addr;                /* address of first pixel in build    */
                                        /*     buffer (without plane offset)  */
    int p_off;                          /* offset of plane of first pixel     */
    int i;                              /* loop index over pixels             */

    /* Check whether requested line falls in the logical view window. */
    if (x < 0 || x >= SCROLL_X_DIM)
        return -1;

    /* Adjust x to the logical row value. */
    x += show_x;

    /* Get the image of the line. */
    fill_vert_buffer (x, show_y, buf);

    /* Calculate starting address in build buffer. */
    addr = img3 + (x >> 2) + show_y * SCROLL_X_WIDTH;

    /* Calculate plane offset of first pixel. */
    p_off = (3 - (x & 3));

    /* Copy image data into appropriate planes in build buffer. */
    for (i = 0; i < SCROLL_Y_DIM; i++) {
        addr[p_off * SCROLL_SIZE] = buf[i];
        addr=addr+SCROLL_X_WIDTH;
    }
    /* Return success. */
    return 0;
}

/*
 * draw_horiz_line
 *   DESCRIPTION: Draw a horizontal map line into the build buffer.  The
 *                line should be offset from the top of the logical view
 *                window screen by the given number of pixels.
 *   INPUTS: y -- the 0-based pixel row number of the line to be drawn
 *                within the logical view window (equivalent to the number
 *                of pixels from the top pixel to the line to be drawn)
 *   OUTPUTS: none
 *   RETURN VALUE: Returns 0 on success.  If y is outside of the valid
 *                 SCROLL range, the function returns -1.
 *   SIDE EFFECTS: draws into the build buffer
 */
int draw_horiz_line(int y) {
    unsigned char buf[SCROLL_X_DIM];    /* buffer for graphical image of line */
    unsigned char* addr;                /* address of first pixel in build    */
                                        /*     buffer (without plane offset)  */
    int p_off;                          /* offset of plane of first pixel     */
    int i;                              /* loop index over pixels             */

    /* Check whether requested line falls in the logical view window. */
    if (y < 0 || y >= SCROLL_Y_DIM)
        return -1;

    /* Adjust y to the logical row value. */
    y += show_y;

    /* Get the image of the line. */
    fill_horiz_buffer (show_x, y, buf);

    /* Calculate starting address in build buffer. */
    addr = img3 + (show_x >> 2) + y * SCROLL_X_WIDTH;

    /* Calculate plane offset of first pixel. */
    p_off = (3 - (show_x & 3));

    /* Copy image data into appropriate planes in build buffer. */
    for (i = 0; i < SCROLL_X_DIM; i++) {
        addr[p_off * SCROLL_SIZE] = buf[i];
        if (--p_off < 0) {
            p_off = 3;
            addr++;
        }
    }

    /* Return success. */
    return 0;
}

/*
 * copy_image
 *   DESCRIPTION: Copy one plane of a screen from the build buffer to the
 *                video memory.
 *   INPUTS: img -- a pointer to a single screen plane in the build buffer
 *           scr_addr -- the destination offset in video memory
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: copies a plane from the build buffer to video memory
 */
static void copy_image(unsigned char* img, unsigned short scr_addr) {
    /*
     * memcpy is actually probably good enough here, and is usually
     * implemented using ISA-specific features like those below,
     * but the code here provides an example of x86 string moves
     */
    asm volatile ("                                             \n\
        cld                                                     \n\
        movl $16000,%%ecx                                       \n\
        rep movsb    /* copy ECX bytes from M[ESI] to M[EDI] */ \n\
        "
        : /* no outputs */
        : "S"(img), "D"(mem_image + scr_addr)
        : "eax", "ecx", "memory"
    );
}


extern uint16_t yyyy;
extern uint8_t  mon;
extern uint8_t  dd;
extern uint8_t  hh;
extern uint8_t  mm;
extern uint8_t  ss;
extern uint8_t  ww;
extern uint32_t rand_seed;
extern void update_time();
/*
 * refresh_bar
 *   DESCRIPTION: 1. convert "level num_fruit time" infos into a string with 320/8=40 chracters
 *                2. call text_to_graphics to render the string into graphic 
 *                      (result in a tex_buffer of size (320*18) i.e. width of bar * hight_of_bar)
 *                3. copu the data format in tex_buffer into tex_VGA_buffer in a format friendly to VGA
 *                4. do copy_status_bar four times (for each plane) to copy the status bar to the start of vedeo_mem
 *   INPUTS: level -- current levels
 *           num_fruit -- number of fruits in maze now
 *           time -- seconds since start
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: copies a plane from the build buffer to video memory
 */
void refresh_bar(int level, int num_fruit, int time){
    static unsigned char tex_buffer[320*18];                  //buffer for text graphic. size = width of bar * hight_of_bar
    static unsigned char tex_VGA_buffer[4*(SCROLL_X_WIDTH*18)]; /* buffer for graphical image in VGA friendly farmat
                                                          size = num_plane*(SCROLL_X_WIDTH* hight_of_bar)*/
    int tex_buffer_idx; // idx of the pixel in tex_buffer
    int tex_VGA_buffer_idx; // idx of the pixel in tex_VGA_buffer
    int p_off;  // the index of the plane
    int pixel_idx; //the index of pixel in a plane
    unsigned char* addr; //address to the start of the plane we want to copy
    int i;

    // // char string_text[]="               TEAM18 OS                "; //just rubbish number to make room for 40 chars    
    // char string_text[41];
    // char* text_1 = 'TEAM18 OS';
    // string_text[40] = '\0';
    // for (i = 0; i < 40; i++) {
    //     string_text[i] = ' ';
    // }
    // // *(string_text + (40 - strlen(text_1))) = text_1;

    //1. convert "level num_fruit time" infos into a string with 320/8=40 chracters
    //char string_text[]="0123456789012345678901234567890123456789"; //just rubbish number to make room for 40 chars    
    //   char string_text[]="               TEAM18 OS                "; //just rubbish number to make room for 40 chars    
    // char string_text[]  =   " 2021-05-06    TEAM18 OS                ";
    char string_text[40] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    update_time();
    itoa(yyyy+2000, string_text+1, 10);
    string_text[5] = '-';
    if (mon < 10){
        itoa(0, string_text+6, 10);
        itoa(mon, string_text+7, 10);
    }else{
        itoa(mon, string_text+6, 10);
    }
    string_text[8] = '-';
    if (dd < 10){
        itoa(0, string_text+9, 10);
        itoa(dd, string_text+10, 10);
    }else{
        itoa(dd, string_text+9, 10);
    }

    string_text[15] = 'T';
    string_text[16] = 'E';
    string_text[17] = 'A';
    string_text[18] = 'M';
    string_text[19] = '1';
    string_text[20] = '8';
    // string_text[] = "";
    if (hh < 10){
        itoa(0, string_text+26, 10);
        itoa(hh, string_text+27, 10);
    }else{
        itoa(hh, string_text+26, 10);
    }
    string_text[28] = ':';
    if (mm < 10){
        itoa(0, string_text+29, 10);
        itoa(mm, string_text+30, 10);
    }else{
        itoa(mm, string_text+29, 10);
    }
    string_text[31] = ':';
    if (ss < 10){
        itoa(0, string_text+32, 10);
        itoa(ss, string_text+33, 10);
    }else{
        itoa(ss, string_text+32, 10);
    }

    
    // itoa(rand_seed%100, string_text[36], 10);

    // itoa()
    // itoa(mon)
    // itoa()
    for (i = 0; i < 39; i++){
        if (string_text[i] == '\0'){
            string_text[i] = ' ';
        }
    }
    //2. call text_to_graphics to render the string into graphic 
    text_to_graphics(string_text,tex_buffer);

    //3. copu the data format in tex_buffer into tex_VGA_buffer in a format friendly to VGA
    for ( p_off=0; p_off<4;p_off++) {  //loop through 4 planes 
        for ( pixel_idx=0; pixel_idx< SCROLL_X_WIDTH*18; pixel_idx++){ //loop through every pixel in this plane (num_of_pixel_in_plane=SCROLL_X_WIDTH*18)
            tex_buffer_idx = p_off + pixel_idx*4; // 4 planes
            tex_VGA_buffer_idx = p_off* (SCROLL_X_WIDTH*18)+ pixel_idx;  //(num_of_pixel_in_plane=SCROLL_X_WIDTH*18)
            tex_VGA_buffer[tex_VGA_buffer_idx]=tex_buffer[tex_buffer_idx]; 
            //tex_VGA_buffer[tex_VGA_buffer_idx]= (tex_buffer[tex_buffer_idx]==COLOR_TEXT)?  a : c;
        }
    }

    //4. do copy_status_bar four times (for each plane) to copy the status bar to the start of vedeo_mem

    /* Draw to each plane in the video memory. */
    for ( p_off = 0; p_off < 4; p_off++) {
        SET_WRITE_MASK(1 << (p_off + 8)); // set musk for the plane we want
        addr=tex_VGA_buffer+p_off*SCROLL_X_WIDTH*18; //18 is the hight of the bar
        copy_status_bar(addr, 0x0000); //(num_of_pixel_in_plane=SCROLL_X_WIDTH*18) 0x0000=start of mem
    }
}

extern void refresh_mp4(unsigned char* pt_2_mp4_buffer){
    //static unsigned char tex_buffer[320*18];                  //buffer for text graphic. size = width of bar * hight_of_bar
    static unsigned char tex_VGA_buffer[4*(SCROLL_X_WIDTH*182)]; /* buffer for graphical image in VGA friendly farmat
                                                          size = num_plane*(SCROLL_X_WIDTH* hight_of_bar)*/
    int tex_buffer_idx; // idx of the pixel in tex_buffer
    int tex_VGA_buffer_idx; // idx of the pixel in tex_VGA_buffer
    int p_off;  // the index of the plane
    int pixel_idx; //the index of pixel in a plane
    unsigned char* addr; //address to the start of the plane we want to copy
    //3. copu the data format in pt_2_mp4_buffer(tex_buffer) into tex_VGA_buffer in a format friendly to VGA
    for ( p_off=0; p_off<4;p_off++) {  //loop through 4 planes 
        for ( pixel_idx=0; pixel_idx< SCROLL_X_WIDTH*(200-18); pixel_idx++){ //loop through every pixel in this plane (num_of_pixel_in_plane=SCROLL_X_WIDTH*18)
            tex_buffer_idx = p_off + pixel_idx*4; // 4 planes
            tex_VGA_buffer_idx = p_off* (SCROLL_X_WIDTH*(200-18))+ pixel_idx;  //(num_of_pixel_in_plane=SCROLL_X_WIDTH*18)
            tex_VGA_buffer[tex_VGA_buffer_idx]=pt_2_mp4_buffer[tex_buffer_idx]; 
            //tex_VGA_buffer[tex_VGA_buffer_idx]= (tex_buffer[tex_buffer_idx]==COLOR_TEXT)?  a : c;
        }
    }  
    //4. do copy_mp4 four times (for each plane) to copy the status bar to the start of vedeo_mem


    /* Switch to the other target screen in video memory. */
    target_img ^= 0x4000;


    /* Draw to each plane in the video memory. */
    for ( p_off = 0; p_off < 4; p_off++) {
        SET_WRITE_MASK(1 << (p_off + 8)); // set musk for the plane we want
        addr=tex_VGA_buffer+p_off*SCROLL_X_WIDTH*182; //18 is the hight of the bar
        copy_mp4(addr, target_img); //(num_of_pixel_in_plane=SCROLL_X_WIDTH*(200-18)) 
    }  

        /*
     * Change the VGA registers to point the top left of the screen
     * to the video memory that we just filled.
     */
    OUTW(0x03D4, (target_img & 0xFF00) | 0x0C);
    OUTW(0x03D4, ((target_img & 0x00FF) << 8) | 0x0D);
}

/*
 * copy_status_bar
 *   DESCRIPTION: Copy one plane of a screen from the tex_VGA_buffer to the
 *                video memory.
 *   INPUTS: img -- a pointer to a single screen plane in the build buffer
 *           scr_addr -- the destination offset in video memory
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: copies a plane from the tex_VGA_buffer to video memory
 */
static void copy_status_bar(unsigned char* img, unsigned short scr_addr) {
    /*
     * memcpy is actually probably good enough here, and is usually
     * implemented using ISA-specific features like those below,
     * but the code here provides an example of x86 string moves
     */
    asm volatile ("                                             \n\
        cld                                                     \n\
        movl $1440,%%ecx                                       \n\
        rep movsb    /* copy ECX bytes from M[ESI] to M[EDI] */ \n\
        "
        : /* no outputs */
        : "S"(img), "D"(mem_image + scr_addr)
        : "eax", "ecx", "memory"
    );
    // 1440=80*18=pixels in one plane for the bar
}

/*
 * copy_mp4
 *   DESCRIPTION: Copy one plane of a screen from the tex_VGA_buffer to the
 *                video memory.
 *   INPUTS: img -- a pointer to a single screen plane in the build buffer
 *           scr_addr -- the destination offset in video memory
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: copies a plane from the tex_VGA_buffer to video memory
 */
extern void copy_mp4(unsigned char* img, unsigned short scr_addr) {
    /*
     * memcpy is actually probably good enough here, and is usually
     * implemented using ISA-specific features like those below,
     * but the code here provides an example of x86 string moves
     */
    asm volatile ("                                             \n\
        cld                                                     \n\
        movl $14560,%%ecx                                       \n\
        rep movsb    /* copy ECX bytes from M[ESI] to M[EDI] */ \n\
        "
        : /* no outputs */
        : "S"(img), "D"(mem_image + scr_addr)
        : "eax", "ecx", "memory"
    );
    // 14560=(200-18)*80=pixels in one plane for mp4
}

/*
 * draw_full_block
 *   DESCRIPTION: Draw a BLOCK_X_DIM x BLOCK_Y_DIM block at absolute
 *                coordinates.  Mask any portion of the block not inside
 *                the logical view window.
 *   INPUTS: (pos_x,pos_y) -- coordinates of upper left corner of block
 *           blk -- image data for block (one byte per pixel, as a C array
 *                  of dimensions [BLOCK_Y_DIM][BLOCK_X_DIM])
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: draws into the build buffer
 */
void draw_full_block(int pos_x, int pos_y, unsigned char* blk) {
    int dx, dy;          /* loop indices for x and y traversal of block */
    int x_left, x_right; /* clipping limits in horizontal dimension     */
    int y_top, y_bottom; /* clipping limits in vertical dimension       */

    /* If block is completely off-screen, we do nothing. */
    if (pos_x + BLOCK_X_DIM <= show_x || pos_x >= show_x + SCROLL_X_DIM ||
        pos_y + BLOCK_Y_DIM <= show_y || pos_y >= show_y + SCROLL_Y_DIM)
        return;

    /* Clip any pixels falling off the left side of screen. */
    if ((x_left = show_x - pos_x) < 0)
        x_left = 0;
    /* Clip any pixels falling off the right side of screen. */
    if ((x_right = show_x + SCROLL_X_DIM - pos_x) > BLOCK_X_DIM)
        x_right = BLOCK_X_DIM;
    /* Skip the first x_left pixels in both screen position and block data. */
    pos_x += x_left;
    blk += x_left;

    /*
     * Adjust x_right to hold the number of pixels to be drawn, and x_left
     * to hold the amount to skip between rows in the block, which is the
     * sum of the original left clip and (BLOCK_X_DIM - the original right
     * clip).
     */
    x_right -= x_left;
    x_left = BLOCK_X_DIM - x_right;

    /* Clip any pixels falling off the top of the screen. */
    if ((y_top = show_y - pos_y) < 0)
        y_top = 0;
    /* Clip any pixels falling off the bottom of the screen. */
    if ((y_bottom = show_y + SCROLL_Y_DIM - pos_y) > BLOCK_Y_DIM)
        y_bottom = BLOCK_Y_DIM;
    /*
     * Skip the first y_left pixel in screen position and the first
     * y_left rows of pixels in the block data.
     */
    pos_y += y_top;
    blk += y_top * BLOCK_X_DIM;
    /* Adjust y_bottom to hold the number of pixel rows to be drawn. */
    y_bottom -= y_top;

    /* Draw the clipped image. */
    for (dy = 0; dy < y_bottom; dy++, pos_y++) {
        for (dx = 0; dx < x_right; dx++, pos_x++, blk++)
            *(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH +
            (3 - (pos_x & 3)) * SCROLL_SIZE) = *blk;
        pos_x -= x_right;
        blk += x_left;
    }
}

/*
 * draw_full_block_with_mask
 *   DESCRIPTION:  * draw a 12x12 block with upper left corner at logical position
 *                (pos_x,pos_y); any part of the block outside of the logical view window
 *               is clipped (cut off and not drawn). And we only draw the position where the corresponding position in mask is 1 
 *                  (before we draw it we first store the old map into the corresponding position in restore block)
 *   INPUTS: (pos_x,pos_y) -- coordinates of upper left corner of block
 *           blk -- image data for block (one byte per pixel, as a C array
 *                  of dimensions [BLOCK_Y_DIM][BLOCK_X_DIM])
 *           mask -- the data for mask of the player
 *           restore_block -- the place for us to store the old map data
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: draws into the build buffer (with mask), store the old map before the draw
 */
void draw_full_block_with_mask(int pos_x, int pos_y, unsigned char* blk, unsigned char* mask, unsigned char* restore_block) {
    int dx, dy;          /* loop indices for x and y traversal of block */
    int x_left, x_right; /* clipping limits in horizontal dimension     */
    int y_top, y_bottom; /* clipping limits in vertical dimension       */

    /* If block is completely off-screen, we do nothing. */
    if (pos_x + BLOCK_X_DIM <= show_x || pos_x >= show_x + SCROLL_X_DIM ||
        pos_y + BLOCK_Y_DIM <= show_y || pos_y >= show_y + SCROLL_Y_DIM)
        return;

    /* Clip any pixels falling off the left side of screen. */
    if ((x_left = show_x - pos_x) < 0)
        x_left = 0;
    /* Clip any pixels falling off the right side of screen. */
    if ((x_right = show_x + SCROLL_X_DIM - pos_x) > BLOCK_X_DIM)
        x_right = BLOCK_X_DIM;
    /* Skip the first x_left pixels in both screen position and block data and mask and restore bloack. */
    pos_x += x_left;
    blk += x_left;
    mask += x_left;
    restore_block += x_left;


    /*
     * Adjust x_right to hold the number of pixels to be drawn, and x_left
     * to hold the amount to skip between rows in the block, which is the
     * sum of the original left clip and (BLOCK_X_DIM - the original right
     * clip).
     */
    x_right -= x_left;
    x_left = BLOCK_X_DIM - x_right;

    /* Clip any pixels falling off the top of the screen. */
    if ((y_top = show_y - pos_y) < 0)
        y_top = 0;
    /* Clip any pixels falling off the bottom of the screen. */
    if ((y_bottom = show_y + SCROLL_Y_DIM - pos_y) > BLOCK_Y_DIM)
        y_bottom = BLOCK_Y_DIM;
    /*
     * Skip the first y_left pixel in screen position and the first
     * y_left rows of pixels in the block data (and mask and restore block).
     */
    pos_y += y_top;
    blk += y_top * BLOCK_X_DIM;
    mask += y_top * BLOCK_X_DIM;
    restore_block += y_top * BLOCK_X_DIM;
    /* Adjust y_bottom to hold the number of pixel rows to be drawn. */
    y_bottom -= y_top;

    /* Draw the clipped image. */
    for (dy = 0; dy < y_bottom; dy++, pos_y++) {
        for (dx = 0; dx < x_right; dx++, pos_x++, blk++, mask++, restore_block++)
            // store the old map to "restore bloack" and draw the player if *musk==1
            if (*mask==1){
                *restore_block = *(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE);
                *(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE) = *blk;
            }
        pos_x -= x_right;
        blk += x_left;
        mask += x_left;
        restore_block += x_left;
    }
}
/*
 * restore_full_block_with_mask
 *   DESCRIPTION:  * restore a 12x12 block with upper left corner at logical position
 *                (pos_x,pos_y); any part of the block outside of the logical view window
 *               is clipped (cut off and not drawn). And we only restore the position where the corresponding position in mask is 1 
 *   INPUTS: (pos_x,pos_y) -- coordinates of upper left corner of block
 *           blk -- image data for block (one byte per pixel, as a C array
 *                  of dimensions [BLOCK_Y_DIM][BLOCK_X_DIM])
 *           mask -- the data for mask of the player
 *           restore_block -- the place for us to store the old map data
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: restore the old map to the build buffer
 */
void restore_full_block_with_mask(int pos_x, int pos_y, unsigned char* blk, unsigned char* mask, unsigned char* restore_block) {
    int dx, dy;          /* loop indices for x and y traversal of block */
    int x_left, x_right; /* clipping limits in horizontal dimension     */
    int y_top, y_bottom; /* clipping limits in vertical dimension       */

    /* If block is completely off-screen, we do nothing. */
    if (pos_x + BLOCK_X_DIM <= show_x || pos_x >= show_x + SCROLL_X_DIM ||
        pos_y + BLOCK_Y_DIM <= show_y || pos_y >= show_y + SCROLL_Y_DIM)
        return;

    /* Clip any pixels falling off the left side of screen. */
    if ((x_left = show_x - pos_x) < 0)
        x_left = 0;
    /* Clip any pixels falling off the right side of screen. */
    if ((x_right = show_x + SCROLL_X_DIM - pos_x) > BLOCK_X_DIM)
        x_right = BLOCK_X_DIM;
    /* Skip the first x_left pixels in both screen position and block data and mask and restore bloack. */
    pos_x += x_left;
    blk += x_left;
    mask += x_left;
    restore_block += x_left;


    /*
     * Adjust x_right to hold the number of pixels to be drawn, and x_left
     * to hold the amount to skip between rows in the block, which is the
     * sum of the original left clip and (BLOCK_X_DIM - the original right
     * clip).
     */
    x_right -= x_left;
    x_left = BLOCK_X_DIM - x_right;

    /* Clip any pixels falling off the top of the screen. */
    if ((y_top = show_y - pos_y) < 0)
        y_top = 0;
    /* Clip any pixels falling off the bottom of the screen. */
    if ((y_bottom = show_y + SCROLL_Y_DIM - pos_y) > BLOCK_Y_DIM)
        y_bottom = BLOCK_Y_DIM;
    /*
     * Skip the first y_left pixel in screen position and the first
     * y_left rows of pixels in the block data (and mask and restore block).
     */
    pos_y += y_top;
    blk += y_top * BLOCK_X_DIM;
    mask += y_top * BLOCK_X_DIM;
    restore_block += y_top * BLOCK_X_DIM;
    /* Adjust y_bottom to hold the number of pixel rows to be drawn. */
    y_bottom -= y_top;

    /* Draw the clipped image. */
    for (dy = 0; dy < y_bottom; dy++, pos_y++) {
        for (dx = 0; dx < x_right; dx++, pos_x++, blk++, mask++, restore_block++)
            // store the old map to "restore bloack" and draw the player if *musk==1
            if (*mask==1){
                *(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE) = *restore_block;
            }
        pos_x -= x_right;
        blk += x_left;
        mask += x_left;
        restore_block += x_left;
    }
}

/*
 * draw_fruit_text_with_mask
 *   DESCRIPTION:  * draw a 18x320 block with upper left corner at logical position
 *                (pos_x,pos_y); any part of the block outside of the logical view window
 *               is clipped (cut off and not drawn). And we only change the position where the corresponding position in text_mask is STATUS_TEXT_COLOR
 *                  (before we draw it we first store the old map into the corresponding position in restore block)
 *   INPUTS: (pos_x,pos_y) -- coordinates of upper left corner of block
 *           mask -- the data for mask of text
 *           restore_block -- the place for us to store the old map data
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: draws into the build buffer (with mask), store the old map before the draw
 */
void draw_fruit_text_with_mask(int pos_x, int pos_y, unsigned char* mask, unsigned char* restore_block) {
    int dx, dy;          /* loop indices for x and y traversal of block */
    int x_left, x_right; /* clipping limits in horizontal dimension     */
    int y_top, y_bottom; /* clipping limits in vertical dimension       */

    /* If block is completely off-screen, we do nothing. */
    if (pos_x + BLOCK_FRT_TXT_X_DIM <= show_x || pos_x >= show_x + SCROLL_X_DIM ||
        pos_y + BLOCK_FRT_TXT_Y_DIM <= show_y || pos_y >= show_y + SCROLL_Y_DIM)
        return;

    /* Clip any pixels falling off the left side of screen. */
    if ((x_left = show_x - pos_x) < 0)
        x_left = 0;
    /* Clip any pixels falling off the right side of screen. */
    if ((x_right = show_x + SCROLL_X_DIM - pos_x) > BLOCK_FRT_TXT_X_DIM)
        x_right = BLOCK_FRT_TXT_X_DIM;
    /* Skip the first x_left pixels in both screen position and block data and mask and restore bloack. */
    pos_x += x_left;
    mask += x_left;
    restore_block += x_left;


    /*
     * Adjust x_right to hold the number of pixels to be drawn, and x_left
     * to hold the amount to skip between rows in the block, which is the
     * sum of the original left clip and (BLOCK_X_DIM - the original right
     * clip).
     */
    x_right -= x_left;
    x_left = BLOCK_FRT_TXT_X_DIM - x_right;

    /* Clip any pixels falling off the top of the screen. */
    if ((y_top = show_y - pos_y) < 0)
        y_top = 0;
    /* Clip any pixels falling off the bottom of the screen. */
    if ((y_bottom = show_y + SCROLL_Y_DIM - pos_y) > BLOCK_FRT_TXT_Y_DIM)
        y_bottom = BLOCK_FRT_TXT_Y_DIM;
    /*
     * Skip the first y_left pixel in screen position and the first
     * y_left rows of pixels in the block data (and mask and restore block).
     */
    pos_y += y_top;
    mask += y_top * BLOCK_FRT_TXT_X_DIM;
    restore_block += y_top * BLOCK_FRT_TXT_X_DIM;
    /* Adjust y_bottom to hold the number of pixel rows to be drawn. */
    y_bottom -= y_top;

    /* Draw the clipped image. */
    for (dy = 0; dy < y_bottom; dy++, pos_y++) {
        for (dx = 0; dx < x_right; dx++, pos_x++, mask++, restore_block++)
            // store the old map to "restore bloack" and draw the player if *musk==1
            if (*mask==STATUS_TEXT_COLOR && *(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE)<64){
                // here "(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE)" is just the pointer to the current pixel in the build buffer
                *restore_block = *(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE);
                *(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE) = *(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE)+NUM_NONTRANS_COLOR;
            }
        pos_x -= x_right;
        mask += x_left;
        restore_block += x_left;
    }
}

/*
 * restore_fruit_text_with_mask
 *   DESCRIPTION:  * restore a 18x320 block with upper left corner at logical position
 *                (pos_x,pos_y); any part of the block outside of the logical view window
 *               is clipped (cut off and not drawn). And we only restore the position where the corresponding position in text_mask is STATUS_TEXT_COLOR
 *   INPUTS: (pos_x,pos_y) -- coordinates of upper left corner of block
 *           mask -- the data for mask of text
 *           restore_block -- the place for us to store the old map data
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: draws into the build buffer (with mask), restore the old map before the draw
 */
void restore_fruit_text_with_mask(int pos_x, int pos_y, unsigned char* mask, unsigned char* restore_block) {
    int dx, dy;          /* loop indices for x and y traversal of block */
    int x_left, x_right; /* clipping limits in horizontal dimension     */
    int y_top, y_bottom; /* clipping limits in vertical dimension       */

    /* If block is completely off-screen, we do nothing. */
    if (pos_x + BLOCK_FRT_TXT_X_DIM <= show_x || pos_x >= show_x + SCROLL_X_DIM ||
        pos_y + BLOCK_FRT_TXT_Y_DIM <= show_y || pos_y >= show_y + SCROLL_Y_DIM)
        return;

    /* Clip any pixels falling off the left side of screen. */
    if ((x_left = show_x - pos_x) < 0)
        x_left = 0;
    /* Clip any pixels falling off the right side of screen. */
    if ((x_right = show_x + SCROLL_X_DIM - pos_x) > BLOCK_FRT_TXT_X_DIM)
        x_right = BLOCK_FRT_TXT_X_DIM;
    /* Skip the first x_left pixels in both screen position and block data and mask and restore bloack. */
    pos_x += x_left;
    mask += x_left;
    restore_block += x_left;


    /*
     * Adjust x_right to hold the number of pixels to be drawn, and x_left
     * to hold the amount to skip between rows in the block, which is the
     * sum of the original left clip and (BLOCK_X_DIM - the original right
     * clip).
     */
    x_right -= x_left;
    x_left = BLOCK_FRT_TXT_X_DIM - x_right;

    /* Clip any pixels falling off the top of the screen. */
    if ((y_top = show_y - pos_y) < 0)
        y_top = 0;
    /* Clip any pixels falling off the bottom of the screen. */
    if ((y_bottom = show_y + SCROLL_Y_DIM - pos_y) > BLOCK_FRT_TXT_Y_DIM)
        y_bottom = BLOCK_FRT_TXT_Y_DIM;
    /*
     * Skip the first y_left pixel in screen position and the first
     * y_left rows of pixels in the block data (and mask and restore block).
     */
    pos_y += y_top;
    mask += y_top * BLOCK_FRT_TXT_X_DIM;
    restore_block += y_top * BLOCK_FRT_TXT_X_DIM;
    /* Adjust y_bottom to hold the number of pixel rows to be drawn. */
    y_bottom -= y_top;

    /* Draw the clipped image. */
    for (dy = 0; dy < y_bottom; dy++, pos_y++) {
        for (dx = 0; dx < x_right; dx++, pos_x++, mask++, restore_block++)
            // store the old map to "restore bloack" and draw the player if *musk==1
            if (*mask==STATUS_TEXT_COLOR){
                // here "(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE)" is just the pointer to the current pixel in the build buffer
                *(img3 + (pos_x >> 2) + pos_y * SCROLL_X_WIDTH + (3 - (pos_x & 3)) * SCROLL_SIZE) = *restore_block;
            }
        pos_x -= x_right;
        mask += x_left;
        restore_block += x_left;
    }
}

/* 
 * get_block_img
 *   DESCRIPTION: Get  a pointer to the graphical image for the block.
 *   INPUTS: block_name -- macro name of the block
 *   OUTPUTS: none
 *   RETURN VALUE: a pointer to an image of a BLOCK_X_DIM x BLOCK_Y_DIM
 *                 block of data with one byte per pixel laid out as a
 *                 C array of dimension [BLOCK_Y_DIM][BLOCK_X_DIM]
 *   SIDE EFFECTS: none
 */
unsigned char* get_block_img(int32_t block_name) {
    return (unsigned char*)blocks[block_name];
}



