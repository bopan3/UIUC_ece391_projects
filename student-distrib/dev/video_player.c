#include "video_player.h"
#include "../file_sys.h"
#include "../lib.h"

void video_player(const uint8_t* video_name){
    cli();
    dentry_t vid_dent;
    uint32_t vid_file_len;
    uint8_t  vid_info_buf[vid_buf_size];
    uint32_t vid_width, vid_height, frame_num, frame_rate, palette_num;
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
    return ;
}

