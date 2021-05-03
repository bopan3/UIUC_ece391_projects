/* By F. J. */

#include "paging.h"
extern int32_t terminal_tick;
extern int32_t terminal_display; 
uint8_t task_use_vidmem = 0;            /* bitmap for the number of task using the user space vid mem */
uint8_t vidmem_bitmap[3] = {1, 2, 4};   /* for mask the using vidmap task */

/* paging_init
 *  Description: Initialize the paging dict and paging table, also mapping the video memory
 *  Input:  none    
 *  Output: none
 *  Side Effect: set the hardware to support paging mode
 *  (all setting refer to IA32 manual and descriptor.pdf from course website)
 */
void paging_init(void){
    int i;

    /* Initialize the page dict */
    for (i = 0; i < PD_SIZE; i++){
            switch(i){
                case 0: /* 0-4 MB */
                    /* first 4 MB to 4KB piece */
                    page_dict[i].P = 1;         /* make it present */
                    page_dict[i].RW = 1;        /* RW enable */
                    page_dict[i].US = 0;        /* for kernel */
                    page_dict[i].PWT = 0;       /* always write back policy */
                    page_dict[i].PCD = 1;       /* 1 for code and data */
                    page_dict[i].A = 0;         /* set to 1 by processor */

                    page_dict[i].bit6 = 0;      /* for 4KB */
                    page_dict[i].PS = 0;        /* for 4KB */
                    page_dict[i].G = 0;         /* ignored for 4KB */
                    page_dict[i].Avail = 0;     /* not used */
                    
                    /* setting address */
                    page_dict[i].bit12 = (((int) page_table) >> (ADDR_OFF)) & (_1BIT_);             /* Skip the 12 LSB */
                    page_dict[i].bit21_13 = (((int) page_table) >> (ADDR_OFF + 1 )) & (_9BIT_);     /* also skip bit12 */
                    page_dict[i].bit31_22 = (((int) page_table) >> (ADDR_OFF + 10 )) & (_10BIT_);   /* also skip bit21-12 */

                    break ;


                case 1: /* 4-8 MB */
                    /* kernel page, 4MB page */
                    page_dict[i].P = 1;         /* make it present */
                    page_dict[i].RW = 1;        /* RW enable */
                    page_dict[i].US = 0;        /* for kernel */
                    page_dict[i].PWT = 0;       /* always write back policy */
                    page_dict[i].PCD = 1;       /* 1 for code and data */
                    page_dict[i].A = 0;         /* set to 1 by processor */

                    page_dict[i].bit6 = 0;      /* set to 0 as Dirty for 4MB */
                    page_dict[i].PS = 1;        /* for 4MB */
                    page_dict[i].G = 1;         /* only for the kernel page */
                    page_dict[i].Avail = 0;     /* not used */
                    
                    /* setting address */
                    page_dict[i].bit12 = 0;     /* PAT not used */
                    page_dict[i].bit21_13 = 0;  /* reserved, must be 0 */
                    page_dict[i].bit31_22 = 1;  /* Physical Memory at 4MB */

                    break ;


                default: /* handle all rest PDE */
                    page_dict[i].P = 0;         /* make it not present */

                    /* The following setting is don't care */
                    page_dict[i].RW = 1;        /* RW enable */
                    page_dict[i].US = 0;        /* for kernel */
                    page_dict[i].PWT = 0;       /* always write back policy */
                    page_dict[i].PCD = 1;       /* 1 for code and data */
                    page_dict[i].A = 0;         /* set to 1 by processor */

                    page_dict[i].bit6 = 0;      /* set to 0 as Dirty for 4MB */
                    page_dict[i].PS = 0;        /* for 4MB */
                    page_dict[i].G = 0;         
                    page_dict[i].Avail = 0;     /* not used */
                    
                    /* address not matter */
                    page_dict[i].bit12 = 0;     /* PAT not used */
                    page_dict[i].bit21_13 = 0;  /* reserved, must be 0 */
                    page_dict[i].bit31_22 = 0;  /* Physical Memory don't care */
            }
    }
    
    /* Initialize the page table for 0-4 MB */
    /* Especially the Video Memory 4KB page */
    for (i = 0; i < PT_SIZE; i++){
//        page_table[i].P = ((i*_4KB_) == (VIDEO));       /* only the Video 4KB page is present when initialized */
        page_table[i].P = ((i <= (VIDEO / _4KB_) + 3) && (i >= ( VIDEO / _4KB_)));
        page_table[i].RW = 1;                           /* Read/Write enable */                           
        page_table[i].US = 0;                           /* kernel */
        page_table[i].PWT = 0;
//        page_table[i].PCD = ((i*_4KB_) != (VIDEO));     /* disable cache only for video memory */
        page_table[i].PCD = 1 - (i <= (VIDEO / _4KB_) + 3 && i >= ( VIDEO / _4KB_));

        page_table[i].A = 0;
        page_table[i].D = 0;                            /* Set by processor */
        page_table[i].PAT = 0;                          /* not used */
        page_table[i].G = 1;                            /* kernel */
        page_table[i].Avail = 0;                        /* not used */
        page_table[i].address = i;                      /* Physical Address MSB 20bits */
    }

    /* Enable paging mode in hardware */
    enable_paging_hw(page_dict);
    
}

/* 
 * paging_set_user_mapping
 *   DESCRIPTION: for the use of execute system call, to set mapping from virtual to physical
 *   INPUTS: pid - index of task, indicate the address of user program chunks 
 *   OUTPUTS: none
 *   RETURN VALUE:  
 *   SIDE EFFECTS:  setting the physical address of virtual user program
 */
void paging_set_user_mapping(int32_t pid){
    /* first time setting mapping */
    if(page_dict[USER_PROG_ADDR].P == 0){
        page_dict[USER_PROG_ADDR].P = 1;         /* make it present */
        page_dict[USER_PROG_ADDR].RW = 1;        /* RW enable */
        page_dict[USER_PROG_ADDR].US = 1;        /* for user code */
        page_dict[USER_PROG_ADDR].PWT = 0;       /* always write back policy */
        page_dict[USER_PROG_ADDR].PCD = 1;       /* 1 for code and data */
        page_dict[USER_PROG_ADDR].A = 0;         /* set to 1 by processor */

        page_dict[USER_PROG_ADDR].bit6 = 0;      /* set to 0 as Dirty for 4MB */
        page_dict[USER_PROG_ADDR].PS = 1;        /* for 4MB */
        page_dict[USER_PROG_ADDR].G = 0;
        page_dict[USER_PROG_ADDR].Avail = 0;     /* not used */
        
        /* setting address */
        page_dict[USER_PROG_ADDR].bit12 = 0;     /* PAT not used */
        page_dict[USER_PROG_ADDR].bit21_13 = 0;  /* reserved, must be 0 */
        
    }
    page_dict[USER_PROG_ADDR].bit31_22 = pid+2;  /* start from 8MB */

    /* set video memory map */
    page_table[VIDEO_REGION_START_K].address = VIDEO_REGION_START_K +  (terminal_display != terminal_tick) * (terminal_tick + 1); /* set for kernel */
    page_table_vedio_mem[VIDEO_REGION_START_U].address =  VIDEO_REGION_START_K + (terminal_display != terminal_tick) * (terminal_tick + 1); /* set for user */

    TLB_flush();
}


/* 
 * paging_set_for_vedio_mem
 *   DESCRIPTION: set the page dic and page table for 4KB user level vedio memory
 *   INPUTS: virtual_addr_for_vedio - the start of virtual address for the vedio mem
 *           phys_addr_for_vedio   - the start of physical address for the vedio mem
 *   OUTPUTS: none
 *   RETURN VALUE:  
 *   SIDE EFFECTS:  map "the start of virtual address for the vedio mem" to "the start of physical address for the vedio mem" (physical vedio memory)
 */
void paging_set_for_vedio_mem(int32_t virtual_addr_for_vedio, int32_t phys_addr_for_vedio){
    int i;
    int dict_idx = virtual_addr_for_vedio/_4MB_;
    int table_idx = (virtual_addr_for_vedio << (10)) >> 22 ; // left shift 10 bits first and then right shift 22 bits to extract the second 10 bits in the virtual address
    
    /* setting page dic entry*/
    if (page_dict[dict_idx].P == 0){
        page_dict[dict_idx].P = 1;         /* make it present */
        page_dict[dict_idx].RW = 1;        /* RW enable */
        page_dict[dict_idx].US = 1;        /* for user code */
        page_dict[dict_idx].PWT = 0;       /* always write back policy */
        page_dict[dict_idx].PCD = 0;       /* disable cache only for video memory */
        page_dict[dict_idx].A = 0;         /* set to 1 by processor */

        page_dict[dict_idx].bit6 = 0;      /* for 4KB */
        page_dict[dict_idx].PS = 0;        /* for 4KB */
        page_dict[dict_idx].G = 0;         /* ignored for 4KB */
        page_dict[dict_idx].Avail = 0;     /* not used */
        
        /* setting address */
        page_dict[dict_idx].bit12 = (((int) page_table_vedio_mem) >> (ADDR_OFF)) & (_1BIT_);             /* Skip the 12 LSB */
        page_dict[dict_idx].bit21_13 = (((int) page_table_vedio_mem) >> (ADDR_OFF + 1 )) & (_9BIT_);     /* also skip bit12 */
        page_dict[dict_idx].bit31_22 = (((int) page_table_vedio_mem) >> (ADDR_OFF + 10 )) & (_10BIT_);   /* also skip bit21-12 */
    }
    
    /* setting page table entry*/
    for (i = 0; i < PT_SIZE; i++){
        page_table_vedio_mem[i].P = ( i == (table_idx));       /* only the Video 4KB page is present when initialized */
        page_table_vedio_mem[i].RW = 1;                           /* Read/Write enable */                           
        page_table_vedio_mem[i].US = 1;                           /* user */
        page_table_vedio_mem[i].PWT = 0;
        page_table_vedio_mem[i].PCD = 0;     /* disable cache only for video memory */

        page_table_vedio_mem[i].A = 0;
        page_table_vedio_mem[i].D = 0;                            /* Set by processor */
        page_table_vedio_mem[i].PAT = 0;                          /* not used */
        page_table_vedio_mem[i].G = 0;                            /* user */
        page_table_vedio_mem[i].Avail = 0;                        /* not used */
        page_table_vedio_mem[i].address = phys_addr_for_vedio>>12 ;     /* Physical Address MSB 20bits (so we need to right shift by 12) */  
    }
    TLB_flush();
    task_use_vidmem = task_use_vidmem | vidmem_bitmap[terminal_tick];

}

/* 
 * paging_restore_for_vedio_mem
 *   DESCRIPTION: restore the page dic and page table for 4KB user level vedio memory
 *   INPUTS: virtual_addr_for_vedio - the start of virtual address for the vedio mem
 *   OUTPUTS: none
 *   RETURN VALUE:  
 *   SIDE EFFECTS:  unmap "the start of virtual address for the vedio mem" to "0xB8000" (physical vedio memory)
 */
void paging_restore_for_vedio_mem(int32_t virtual_addr_for_vedio){
    int i;
    int dict_idx = virtual_addr_for_vedio/_4MB_;
    // int table_idx = (virtual_addr_for_vedio << (10)) >> 22 ; // left shift 10 bits first and then right shift 22 bits to extract the second 10 bits in the virtual address
    switch (terminal_tick)
    {
        case 0:
            task_use_vidmem = task_use_vidmem & 0xFE; /* set 0 for bit 0 */
            break;
        case 1:
            task_use_vidmem = task_use_vidmem & 0xFD; /* set 0 for bit 1 */
            break;
        case 2:
            task_use_vidmem = task_use_vidmem & 0xFB; /* set 0 for bit 2 */
            break;
    }

    if(task_use_vidmem == 0){
        /* setting page dic entry*/
        page_dict[dict_idx].P = 0;         /* make it not present */        
        /* setting page table entry*/
        for (i = 0; i < PT_SIZE; i++){
            page_table_vedio_mem[i].P = 0;       /* make all not present */ 
        }
        TLB_flush();
    }
    
}
