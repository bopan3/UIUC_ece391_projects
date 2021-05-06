#include "video_player.h"
#include "../file_sys.h"
#include "../ModeX.h"
#include "../lib.h"
/* global section */
uint32_t frame_index; 
dentry_t vid_dent;
uint32_t vid_width, vid_height, frame_num, frame_rate, palette_num;
uint8_t video_status = PLAY_VID;
extern unsigned char palette_RGB_vedio[256][3];




void video_player(const uint8_t* video_name){
    cli();
    
    uint32_t vid_file_len;
    uint8_t  vid_info_buf[vid_buf_size];
    // uint8_t  palette_buf[256*3];
    
    uint8_t tmp[320*182];

    if (read_dentry_by_name(video_name, &vid_dent) == -1){
        printf("fail to find the video file\n");
        return ;
    }

    /* get video info */
    vid_file_len = get_file_size(vid_dent.idx_inode);
    read_data(vid_dent.idx_inode, 0, vid_info_buf, vid_buf_size);
    vid_width = *(uint32_t*)vid_info_buf;
    vid_height = *(uint32_t*)(vid_info_buf + 4);
    frame_num = *(uint32_t*)(vid_info_buf + 8);
    frame_rate = *(uint32_t*)(vid_info_buf + 12);
    palette_num = *(uint32_t*)(vid_info_buf + 16);

    printf("Video width: %d\n", vid_width);
    printf("Video height: %d\n", vid_height);
    printf("Frame number: %d\n", frame_num);
    printf("Frame rate: %d\n", frame_rate);
    printf("Palette Entry: %d\n", palette_num);

    read_data(vid_dent.idx_inode, 20, (uint8_t*)palette_RGB_vedio, palette_num);
    read_data(vid_dent.idx_inode, 20+palette_num, tmp, vid_width * vid_height);
    fill_palette_vedio();

    refresh_mp4(tmp);


    frame_index = 1; /* the next to be displayed  */
    video_status = PLAY_VID;
    return ;
}

/* update the video frame by interrupt */
void video_handler(){
    uint8_t tmp[320*182];
    /*  */
    if (frame_index < frame_num){
        read_data(vid_dent.idx_inode, 20+palette_num, tmp, vid_width * vid_height);
        frame_index++;
    }
}

