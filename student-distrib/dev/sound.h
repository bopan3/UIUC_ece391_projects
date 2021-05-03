#include "../types.h"

#define CH2_PORT 0x42
#define PIT_MODE_REG 0x43
#define PCS_CMDW 0xB6
#define PCS_PORT 0x61
#define WARNING_PCS() do{beep(700, 20);} while(0)

#define beat 100     /* ticks length */
#define gap (beat * 5 / 20)
#define PITCH(Fre, P_Len) do{                   \
        beep(Fre, P_Len-gap);                   \
        timer_wait(gap);                        \
 } while (0)

/* 
C - do - 261.6HZ
D - re - 293.6HZ
E - mi - 329.6HZ
F - fa - 349.2HZ
G - sol- 392HZ
A - la - 440HZ
B - si - 493.8HZ 
*/
// #define DO() PITCH(262)
// #define RE() PITCH(294)
// #define MI() PITCH(330)
// #define FA() PITCH(349)
// #define SO() PITCH(392)
// #define LA() PITCH(440)
// #define SI() PITCH(494)

#define DO 262
#define RE 294
#define MI 330
#define FA 349
#define SO 392
#define LA 440
#define SI 494


/* ================== PC Speaker ================= */
extern void timer_wait(int ticks);
void play_sound(uint32_t nFrequence);
void nosound();
void beep(uint32_t fre, uint32_t wait);

void little_star();

/* ================== SB16 ================= */
#define DSP_Reset 0x226
#define DSP_Read  0x22A
#define DSP_Write 0x22C
#define DSP_Read_buf_status 0x22E
#define DSP_INT_ACK 0x22F
