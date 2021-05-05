#include "sound.h"
#include "../paging.h"
#include "../lib.h"
#include "../file_sys.h"
#include "../i8259.h"

/* Global Section */
volatile uint32_t total_samples; /* the remained unload date */
volatile uint32_t chunk_off;    /* index to-loaded data from the start */
// volatile uint8_t play_music = 0;
dentry_t music_dent;   
volatile uint8_t DMA_ADDR[Chunk_Size*2];
// #include "../timer.h"
uint8_t CH_Page_Port[4] = {0x87, 0x83, 0x81, 0x82};
uint8_t music_states = STOP;
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

/* ================================================================== final player part =========================================== */
/* Top envoke API */
void player(const uint8_t* music_name){
    cli();
    // uint8_t tmp[4096];
    // int i;
    // dentry_t music_dent;

    uint8_t  wav_info[36];
    uint16_t wav_file_len;         

    uint32_t wav_samples, sample_rate;
    uint16_t channels, bit_per_sample;

    /* Global Initialization  */
    if (music_states == PLAY){
        printf("Other music are still playing\n");
        return ;
    }else {
        /* pause or stop could go on */
        chunk_off = 0;
        total_samples = 0;
        music_states = PLAY;
    }
    

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

    // printf("Wav Samples: %d\n", wav_samples);
    // printf("Channels: %d\n", channels);
    // printf("Sample Rate: %d\n", sample_rate);
    // printf("Bit per Sample: %d\n", bit_per_sample);

    // read_data(music_dent.idx_inode, 44, tmp, 10);
    // printf("First ten Bytes:\n");
    // for (i = 0; i < 10; i ++){
    //     printf("Byte %d: %x\n", i, tmp[i]);
    // }

    /* Prepare Data */
    if (wav_samples > Chunk_Size){
        chunk_off = 2;
        if (wav_samples > Chunk_Size * 2){

            read_data(music_dent.idx_inode, 44, (uint8_t*)DMA_ADDR, Chunk_Size*2);
            // read_data(music_dent.idx_inode, 44, (uint8_t*)tmp, Chunk_Size*2);
            // memcpy((uint8_t*)DMA_ADDR , tmp, 4096 );
            
        } 
        else {
            read_data(music_dent.idx_inode, 44, (uint8_t*)DMA_ADDR, Chunk_Size);
        }
    } 
    else{
        read_data(music_dent.idx_inode, 44, (uint8_t*)DMA_ADDR, wav_samples);
    }

    enable_irq(DSP_IRQ);
    // inb(DSP_Read_buf_status);
    // printf("\nThe Master Mask values are %x \n", inb(MASTER_8259_DATA));

    /* Prepare to use DSP */
    reset_DSP();
    Set_Vol(0xA, 0xA);
    
    Turn_ON_SB16();

    DMAC_Setting(1, (uint32_t)DMA_ADDR, (uint16_t)Chunk_Size*2);

    // set input and output rate
    Set_Sample_Rate(sample_rate, 1);
    Set_Sample_Rate(sample_rate, 0);

    outb(0x40, DSP_Write);
    outb(65536 - (256000000/(sample_rate)), DSP_Write);

    /* Transfer mode */
    if (wav_samples > Chunk_Size){
        outb(0xC4, DSP_Write);  /* 8-bit auto-initialized output */
        outb(0x00, DSP_Write);  /* mono, 8 bit */

        // outb(0x48, DSP_Write);
        outb((uint8_t)((Chunk_Size - 1) & 0xFF), DSP_Write);              /* Low Byte */
        outb((uint8_t)(((Chunk_Size - 1) & 0xFF00) >> 8), DSP_Write);       /* High Byte */
        
        
        total_samples = wav_samples - Chunk_Size;

    } else {
        outb(0xC0, DSP_Write);  /* 8-bit single-cycle output */
        outb(0x00, DSP_Write);  /* mono, 8 bit */
        outb((uint8_t)(wav_samples & 0xFF), DSP_Write);             /* Low Byte */
        outb((uint8_t)(wav_samples & 0xFF00) >> 8, DSP_Write);      /* High Byte */
        
        total_samples = 0;
    }


    /* set irw */
    _set_irq();
    // inb(DSP_Read_buf_status);
    outb(0x1C, DSP_Write);
    // play_music = 1;
    sti();
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

// int handler_number = 0;
void sb16_handler(){
    uint8_t tmp;
    // printf("handler called %d:\n", handler_number);
    // handler_number++;
    outb(0x82, DSP_Mixer);
    // printf("Current 0x82 is %x\n", inb(DSP_Mixer_data));
    /* handler called when played done a chunk */
    tmp = inb(DSP_Read_buf_status);
    // printf("tmp = %x\n", tmp);

    if (total_samples > Chunk_Size){
        /* the remaining part is more than 1 chunk */
        /* prepare next next chunk */
        if (total_samples > Chunk_Size * 2){
            if (Chunk_Size != read_data(music_dent.idx_inode, 44 + Chunk_Size * chunk_off, (uint8_t*)(DMA_ADDR + Chunk_Size * (chunk_off % 2)), Chunk_Size)){
                printf("read data fail\n");
            }
            // read_data(music_dent.idx_inode, 44 + Chunk_Size * chunk_off, (uint8_t*)(DMA_ADDR + Chunk_Size * (chunk_off % 2)), Chunk_Size);
            chunk_off++;
        }
        else {
            read_data(music_dent.idx_inode, 44 + Chunk_Size * chunk_off, (uint8_t*)(DMA_ADDR + Chunk_Size * (chunk_off % 2)), total_samples - Chunk_Size);
            chunk_off++;
        }
        total_samples -= Chunk_Size;
    }
    else {
        total_samples = 0;
        /* set end sb16 mode */
        outb(0xDA, DSP_Write);
        disable_irq(DSP_IRQ); /* Turn off the irq */
        // play_music = 0;
        music_states = STOP;
    } 
    send_eoi(DSP_IRQ);
    return ;
}

void player_pause(){
    music_states = PAUSE;
    outb(0xD0, DSP_Write);
    printf("Music pause\n");
}

void player_stop(){
    total_samples = 0;
    music_states = STOP;
    /* set end sb16 mode */
    outb(0xDA, DSP_Write);
    disable_irq(DSP_IRQ); /* Turn off the irq */
    printf("Music stoped\n");
}


void player_goon(){
    music_states = PLAY;
    outb(0xD4, DSP_Write);
    printf("Music go on\n");
}

void pause_or_goon(){
    // if (music_states == STOP){
    //     printf("No music is playing\n");
    //     return ;
    // }
    switch (music_states)
    {
        case STOP:
            printf("No music is playing\n");
            return ;
            break;
        case PLAY:
            player_pause();
            break;
        case PAUSE:
            player_goon();
            break;

        default:
            break;
    }
    return ;
}



