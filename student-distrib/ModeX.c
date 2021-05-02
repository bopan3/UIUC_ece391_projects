#include "ModeX.h"
#include "text.h"
#include "paging.h"
#include "i8259.h"
#include "timer.h"
#include "keyboard.h"


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
// static void copy_image(unsigned char* img, unsigned short scr_addr);
// static void copy_status_bar(unsigned char* img, unsigned short scr_addr);


#define MEM_FENCE_WIDTH 256
#define MEM_FENCE_MAGIC 0xF3

// static unsigned char build[BUILD_BUF_SIZE + 2 * MEM_FENCE_WIDTH];
// static int img3_off;                /* offset of upper left pixel   */
// static unsigned char* img3;         /* pointer to upper left pixel  */
// static int show_x, show_y;          /* logical view coordinates     */

//                                     /* displayed video memory variables */
static unsigned char* mem_image;    /* pointer to start of video memory */
static unsigned char* mem_temp_v;    /* pointer to start (0x900000) of temp video memory for text screen when in modeX */


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
    mem_temp_v= (unsigned char*) TEMP_STOR_FOR_MODEX; //i.e. 0x900000
    /* copy 0xB8000--0xB8000+4KB*4 to 0x900000--0x900000+4KB*4 for storage of text screens */
    for (i = 0; i < _4KB_*4; i++) {mem_temp_v[i] = (VM_addr)[i];}
    /* set the start of mem_image of ModeX */
    mem_image= (unsigned char*) MODEX_STR_ADDR;
    VGA_blank(1);                               /* blank the screen      */
    set_seq_regs_and_reset(mode_X_seq, 0x63);   /* sequencer registers   */
    set_CRTC_registers(mode_X_CRTC);            /* CRT control registers */
    set_attr_registers(mode_X_attr);            /* attribute registers   */
    set_graphics_registers(mode_X_graphics);    /* graphics registers    */
    fill_palette();                             /* palette colors        */
    clear_screens();                            /* zero video memory     */
    VGA_blank(0);                               /* unblank the screen    */
    
    disable_irq(PIT_IRQ);        
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
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x2A },   /* palette 0x00 - 0x0F    */
        { 0x00, 0x2A, 0x00 },{ 0x00, 0x2A, 0x2A },   /* basic VGA colors       */
        { 0x2A, 0x00, 0x00 },{ 0x2A, 0x00, 0x2A },
        { 0x2A, 0x15, 0x00 },{ 0x2A, 0x2A, 0x2A },
        { 0x15, 0x15, 0x15 },{ 0x15, 0x15, 0x3F },
        { 0x15, 0x3F, 0x15 },{ 0x15, 0x3F, 0x3F },
        { 0x3F, 0x15, 0x15 },{ 0x3F, 0x15, 0x3F },
        { 0x3F, 0x3F, 0x15 },{ 0x3F, 0x3F, 0x3F },
        { 0x00, 0x00, 0x00 },{ 0x05, 0x05, 0x05 },   /* palette 0x10 - 0x1F    */
        { 0x08, 0x08, 0x08 },{ 0x0B, 0x0B, 0x0B },   /* VGA grey scale         */
        { 0x0E, 0x0E, 0x0E },{ 0x11, 0x11, 0x11 },
        { 0x14, 0x14, 0x14 },{ 0x18, 0x18, 0x18 },
        { 0x1C, 0x1C, 0x1C },{ 0x20, 0x20, 0x20 },
        { 0x24, 0x24, 0x24 },{ 0x28, 0x28, 0x28 },
        { 0x2D, 0x2D, 0x2D },{ 0x32, 0x32, 0x32 },
        { 0x38, 0x38, 0x38 },{ 0x3F, 0x3F, 0x3F },
        { 0x3F, 0x3F, 0x3F },{ 0x3F, 0x3F, 0x3F },   /* palette 0x20 - 0x2F    */    // PLAYER_CENTER_COLOR 0x20 @@ WALL_OUTLINE_COLOR  0x21
        { 0x00, 0x00, 0x3F },{ 0x00, 0x00, 0x00 },   /* wall and player colors */    // WALL_FILL_COLOR     0x22 
        { 0x3F, 0x00, 0x00 },{ 0x3F, 0x3F, 0x3F },                                   // STATUS_BACK_COLOR   0x24 @@ STATUS_TEXT_COLOR   0x25
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },{ 0x00, 0x00, 0x00 },
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
    memset(mem_image, 5, MODE_X_MEM_SIZE);
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

    /* restore 0xB8000--0xB8000+4KB*4 from 0x900000--0x900000+4KB*4 for restorage of text screens */
    for (j = 0; j < _4KB_*4; j++) {VM_addr[j] = (mem_temp_v)[j];}

    // if (clear_scr) {                            /* clear screens if needed */
    //     txt_scr = (unsigned long*)(mem_image + 0x18000);
    //     for (i = 0; i < 8192; i++)
    //         *txt_scr++ = 0x07200720;
    // }
    write_font_data();                          /* copy fonts to video mem */
    VGA_blank(0);                               /* unblank the screen      */
    enable_irq(PIT_IRQ);
}
