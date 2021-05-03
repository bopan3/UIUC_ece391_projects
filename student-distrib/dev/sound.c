#include "sound.h"
#include "../paging.h"
#include "../lib.h"
#include "../file_sys.h"
/* Global Section */

// #include "../timer.h"
uint8_t CH_Page_Port[4] = {0x87, 0x83, 0x81, 0x82};

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

void sound_player(int32_t address, int32_t sample_rate, int32_t length){
    // uint8_t tmp;
    reset_DSP();
    
    
    // Load sound data to memory
    /* TODO */

    // Set master volume
    Set_Vol(0xA, 0xA);

    // Turn speaker on
    Turn_ON_SB16();

    // Program ISA DMA to transfer
    Program_DMA_8b(1, address, length);

    // set input and output rate
    Set_Sample_Rate(sample_rate, 1);
    Set_Sample_Rate(sample_rate, 0);

    length--;
    /* transfer mode */
    outb(0xC0, DSP_Write);
    outb(0x00, DSP_Write);  /* data type */
    outb((length & 0xFF00) >> 8, DSP_Write);    /* High Byte */
    outb((uint8_t)(length & 0xFF), DSP_Write);  /* Low Byte */
    
}


void reset_DSP(){
    uint32_t i = 0;
    // Reset DSP
    outb(1, DSP_Reset);
    // asm volatile (
    //     "movb $0x86, %%AH;"
    //     "movw $0, %%CX;"
    //     "movw $0xFFFF, %%DX;"
    //     "int $0x15;"
    //     : /* No output */
    //     : /* No input */
    //     : "eax", "ecx", "edx"
    // );
    while (++i <5000000){}

    outb(0, DSP_Reset);

    while ((inb(DSP_Read_buf_status) & 0x80) == 0){}
    while ((inb(DSP_Read) != 0xAA)){}
}



/* DMAC use physical address only */
void Program_DMA_8b(int8_t chan_num, uint32_t address, uint16_t length){
    uint8_t tmp;
    length--;

    /* 1.  Disable channel*/
    outb(0x04 + chan_num, DMAC1_W_MaskReg);

    /* 2. Write any value to fli-flop port */
    outb(1, DMAC1_FF);

    /* 3. send transfer mode */
    outb(AutoMode + chan_num, DMAC1_W_Mode);

    /* 4. send page number */
    tmp = (uint8_t)((address & 0x00FF0000) >> 16);
    outb(tmp, CH_Page_Port[(uint8_t)chan_num]);

    /* 5. send low bits of position */
    tmp = (uint8_t)((address & 0x000000FF));
    outb(tmp, 0x2 * chan_num);

    /* 6. send high bits of position */
    tmp = (uint8_t)((address & 0x0000FF00) >> 8);
    outb(tmp, 0x2 * chan_num);

    /* 7. send low bits of length of data */
    tmp = (uint8_t)(length & 0x00FF);
    outb(tmp, 0x2 * chan_num + 1);

    /* 8. send high bits o length of data */
    tmp = (uint8_t)((length & 0xFF00) >> 8);
    outb(tmp, 0x2 * chan_num + 1);

    /* 9. enable channel number  */
    outb(chan_num, DMAC1_W_MaskReg);

    return ;
}

void Set_Sample_Rate(int32_t sample_rate, int8_t input_b){
    /* set input if 1 */
    outb(0x41 + input_b, DSP_Write);
    outb((uint8_t)((sample_rate & 0xFF00) >> 8), DSP_Write);
    outb((uint8_t)(sample_rate & 0x00FF), DSP_Write);
}



/* test function for SB16 */
void test_play_music(){
    dentry_t music_dent;
    uint8_t  wav_buf[_64K_];
    uint8_t  wav_info[36];
    uint16_t wav_file_len;         

    uint32_t wav_samples, sample_rate;
    uint16_t channels, bit_per_sample;



    if (read_dentry_by_name((uint8_t*)"rickroll10s-PCM6k.wav", &music_dent) == -1){
        printf("fail to find the file\n");
        return ;
    }
    wav_file_len = get_file_size(music_dent.idx_inode);
    printf("wav file with length %d\n", wav_file_len);
    read_data(music_dent.idx_inode, 0, wav_info, 36);
    
    wav_samples = *(uint32_t*)(wav_info+4) - 36;
    sample_rate = *(uint32_t*)(wav_info+24);
    channels = *(uint16_t*)(wav_info+22);
    bit_per_sample =  *(uint16_t*)(wav_info+34);
    printf("Wav Samples: %d\n", wav_samples);
    printf("Channels: %d\n", channels);
    printf("Sample Rate: %d\n", sample_rate);
    printf("Bit per Sample: %d\n", bit_per_sample);


    // read_data(music_dent, 44, (uint8_t*)0xB00000, wav_samples);
    read_data(music_dent.idx_inode, 44, wav_buf, wav_samples);

    sound_player((uint32_t)wav_buf, sample_rate, wav_samples);
    printf("Play Done\n");
    return ;
}

/* Top envoke API */
void player(const uint8_t* music_name){
    dentry_t music_dent;
    uint8_t  wav_buf[_64K_];
    uint8_t  wav_info[36];
    uint16_t wav_file_len;         

    uint32_t wav_samples, sample_rate;
    uint16_t channels, bit_per_sample;

    /* Get file dentry */
    if (read_dentry_by_name(music_name, &music_dent) == -1){
        printf("fail to find the music file\n");
        return ;
    }

    /* Get wave info */
    wav_file_len = get_file_size(music_dent.idx_inode);
    read_data(music_dent.idx_inode, 0, wav_info, 36);
    wav_samples = *(uint32_t*)(wav_info+4) - 36;
    sample_rate = *(uint32_t*)(wav_info+24);
    channels = *(uint16_t*)(wav_info+22);
    bit_per_sample =  *(uint16_t*)(wav_info+34);

    printf("Wav Samples: %d\n", wav_samples);
    printf("Channels: %d\n", channels);
    printf("Sample Rate: %d\n", sample_rate);
    printf("Bit per Sample: %d\n", bit_per_sample);

    /* Prepare Data */
    if (wav_samples > Chunk_Size){
        read_data(music_dent.idx_inode, 44, (uint8_t*)DMA_ADDR, Chunk_Size);
    }else{
        read_data(music_dent.idx_inode, 44, (uint8_t*)DMA_ADDR, wav_samples);
    }
    /* Prepare to use DSP */
    reset_DSP();
    Set_Vol(0xA, 0xA);
    
    Turn_ON_SB16();

    DMAC_Setting(1, (uint32_t)DMA_ADDR, (uint16_t)Chunk_Size*2);

    // set input and output rate
    Set_Sample_Rate(sample_rate, 1);
    Set_Sample_Rate(sample_rate, 0);

    /* Transfer mode */
    if (wav_samples > Chunk_Size){
        outb(0xC6, DSP_Write);  /* 8-bit auto-initialized output */
        outb(0x00, DSP_Write);  /* mono, 8 bit */
        outb((uint8_t)(Chunk_Size & 0xFF00) >> 8, DSP_Write);       /* High Byte */
        outb((uint8_t)(Chunk_Size & 0xFF), DSP_Write);              /* Low Byte */
    } else {
        outb(0xC0, DSP_Write);  /* 8-bit single-cycle output */
        outb(0x00, DSP_Write);  /* mono, 8 bit */
        outb((uint8_t)(wav_samples & 0xFF00) >> 8, DSP_Write);      /* High Byte */
        outb((uint8_t)(wav_samples & 0xFF), DSP_Write);             /* Low Byte */
    }

    return ;
    

}


void DMAC_Setting(int8_t chan_num, uint32_t address, uint16_t length){
    uint8_t tmp;
    length--;
    /* 1.  Disable channel*/
    outb(0x04 + chan_num, DMAC1_W_MaskReg);
    /* 2. Write any value to fli-flop port */
    outb(1, DMAC1_FF);
    /* 3. send transfer mode */
    outb(AutoMode + chan_num, DMAC1_W_Mode);
    /* 4. send page number */
    tmp = (uint8_t)((address & 0x00FF0000) >> 16);
    outb(tmp, CH_Page_Port[(uint8_t)chan_num]);
    /* 5. send low bits of position */
    tmp = (uint8_t)((address & 0x000000FF));
    outb(tmp, 0x2 * chan_num);
    /* 6. send high bits of position */
    tmp = (uint8_t)((address & 0x0000FF00) >> 8);
    outb(tmp, 0x2 * chan_num);
    /* 7. send low bits of length of data */
    tmp = (uint8_t)(length & 0x00FF);
    outb(tmp, 0x2 * chan_num + 1);
    /* 8. send high bits o length of data */
    tmp = (uint8_t)((length & 0xFF00) >> 8);
    outb(tmp, 0x2 * chan_num + 1);
    /* 9. enable channel number  */
    outb(chan_num, DMAC1_W_MaskReg);

    return ;
}

void _set_irq(){
    outb(0x80, DSP_Mixer);
    outb(0x02,DSP_Mixer_data);  /* for IRQ 5 */
}
void sb16_handler(){

}