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