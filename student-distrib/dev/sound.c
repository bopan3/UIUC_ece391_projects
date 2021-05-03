#include "sound.h"
#include "../lib.h"
// #include "../timer.h"

/* ================== PC Speaker ================= */
/* Adapted from https://wiki.osdev.org/PC_Speaker  */
void play_sound(uint32_t nFrequence) {
 	uint32_t Div;
 	uint8_t tmp;
 
    //Set the PIT to the desired frequency
 	Div = 1193180 / nFrequence;
 	outb(PCS_CMDW, PIT_MODE_REG);
 	outb((uint8_t) (Div), CH2_PORT);
 	outb((uint8_t) (Div >> 8), CH2_PORT);
 
    //And play the sound using the PC speaker
 	tmp = inb(PCS_PORT);
  	if (tmp != (tmp | 3)) {
 		outb(tmp | 3, PCS_PORT);
 	}
 }
 
//make it shutup
void nosound() {
 	uint8_t tmp = inb(PCS_PORT) & 0xFC;
 	outb(tmp, PCS_PORT);
 }

 
//Make a beep
void beep(uint32_t fre, uint32_t wait) {\
 	 play_sound(fre);
 	 timer_wait(wait);
 	 nosound();
 }

void little_star(){
    // DO();
    // DO();
    // SO();
    // SO();
    // LA();
    // SO();
    PITCH(DO, beat);
    PITCH(DO, beat);
    PITCH(SO, beat);
    PITCH(SO, beat);
    PITCH(LA, beat);
    PITCH(LA, beat);
    PITCH(SO, beat * 2);
}


/* ================== SB16 ================= */

void play_music(){
    // uint8_t tmp;

    // Reset DSP
    outb(1, DSP_Reset);
    asm volatile (
        "movb $0x86, %%AH;"
        "movw $0, %%CX;"
        "movw $FFFF, %%DX;"
        "int $0x15;"
        : /* No output */
        : /* No input */
        : "eax", "ebx", "edx"
    )
    outb(0, DSP_Reset);

    while ((inb(DSP_Read_buf_status) & 0x80) == 0){}
    while ((inb(DSP_Read) != 0xAA)){}
    
    // Load sound data to memory
    // Set master volume
    // Turn speaker on
    // Program ISA DMA to transfer
    // Set time constant. Notice that the Sound Blaster 16 is able to use sample rates instead of time constants using command 0x41 instead of 0x40.
    // You can calculate the time constant like this: Time constant = 65536 - (256000000 / (channels * sampling rate))
    // Set output sample rate
    // Write transfer mode to DSP
    // Write type of sound data
    // Write data length to DSP(Low byte/High byte) (You must calculate LENGTH-1 e.g. if is your real length 0x0FFF, you must send 0xFE and 0x0F)
}