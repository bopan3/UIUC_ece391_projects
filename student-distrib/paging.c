/* By F. J. */

#include "paging.h"

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
        page_table[i].P = ((i*_4KB_) == (VIDEO));       /* only the Video 4KB page is present when initialized */
        page_table[i].RW = 1;                           /* Read/Write enable */                           
        page_table[i].US = 0;                           /* kernel */
        page_table[i].PWT = 0;
        page_table[i].PCD = ((i*_4KB_) != (VIDEO));     /* disable cache only for video memory */

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
