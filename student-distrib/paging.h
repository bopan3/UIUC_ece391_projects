#ifndef _paging_h_
#define _paging_h_

#include "types.h"
#include "x86_desc.h"
#include "lib.h"

#define PD_SIZE         1024            // 2^10 = 1024
#define PT_SIZE         1024            // page table size, 4KB / 4B = 1024
#define _4KB_           4096            // 4K Bytes to aline to   
#define _4MB_           0x400000        // 4M Bytes to aline to   
#define _4B_            0x4             // 4  Bytes to aline to  
#define VIDEO           0xB8000         // from lib.c, addr of video memory
#define _1BIT_          0x1             // mask to get 1 bit
#define _9BIT_          0x1FF           // mask to get 9 bits
#define _10BIT_         0x3FF           // mask to get 10 bits
#define ADDR_OFF        12              // skip 12 LSB to get address filed

#define USER_PROG_ADDR 32               /* 128 MB / 4MB per entry */
#define VIDEO_REGION_START_K (VIDEO / _4KB_)
#define VIDEO_REGION_START_U 0

/* Set the reg in hardware to enable page mode.  
 * Description: This macro takes a 32-bit address which points to 
 *              the page_dict initialized, and set cr0, cr3, cr4 to enable page mode
 * Input: a 32-bit address which points to the page_dict
 * Output: None
 * Return: None
 * Reference: IA32 Manual chap 3.
 */
#define enable_paging_hw(page_dict)         \
do {                                        \
    asm volatile ("                                               \n\
        /* Set cr3 reg with the dict address */                   \n\
        movl    %0, %%eax                                         \n\
        movl    %%eax, %%cr3                                      \n\
                                                                  \n\
        /* Set PSE flag, cr4 bit4 to support mixed size*/         \n\
        movl    %%cr4, %%eax                                      \n\
        orl     $0x00000010, %%eax                                \n\
        movl    %%eax, %%cr4                                      \n\
                                                                  \n\
        /* Enable PG flag, cr0 bit31 */                           \n\
        movl    %%cr0, %%eax                                      \n\
        orl     $0x80000000, %%eax                                \n\
        movl    %%eax, %%cr0                                      \n\
        "                                                           \
        : /* no outputs */                                          \
        : "r"((page_dict))                                          \
        : "eax"                                                     \
    );                                                              \
} while (0);

/*   TLB_flush
 * Description: This macro reload cr3, to flush the cache
 * Input: None
 * Output: None
 * Return: None
 * Reference: reload cr3, cache flushed 
 */
#define TLB_flush()                 \
do {                                \
    asm volatile ("               \n\
    movl %%cr3, %%eax             \n\
    movl %%eax, %%cr3             \n\
    "                               \
    :   /* no outputs */            \
    :   /* no inputs */             \
    :   "eax"                       \
    );                              \
}while (0);



/* Structure for page dict and page table */
/* page table only used for 4KB Page */
typedef struct __attribute__((packed)) PTE_desc {
    uint32_t P                  :1; /* present */   
    uint32_t RW                 :1; /* read/write */
    uint32_t US                 :1; /* user/supervisor */
    uint32_t PWT                :1; /* write-through */
    uint32_t PCD                :1; /* cache disabled */
    uint32_t A                  :1; /* accessed */
    uint32_t D                  :1; /* Dirty */
    uint32_t PAT                :1; /* Page Table Attr. index */
    uint32_t G                  :1; /* Global page */
    uint32_t Avail              :3; /* available for system program's use */
    uint32_t address            :20; 
} PTE;


/* Combined PDT struct for both 4KB and 4MB cases */
typedef struct __attribute__((packed)) PDE_desc {
    /* Common filed on bits 5:0 */
    uint32_t P                  :1; /* present */   
    uint32_t RW                 :1; /* read/write */
    uint32_t US                 :1; /* user/supervisor */
    uint32_t PWT                :1; /* write-through */
    uint32_t PCD                :1; /* cache disabled */
    uint32_t A                  :1; /* accessed */

    /* bit 6:   4MB page - dirty 
                4KB page - set to 0 */
    uint32_t bit6               :1;

    uint32_t PS                 :1; /* Page size:  4MB page, 1 \ 4KB page, 0 */
    uint32_t G                  :1; /* Global page */
    uint32_t Avail              :3; /* available for system program's use */

    /* 4MB page: bit 31-22, addr; bit21-13, reserved; bit12, page table attr index */
    /* 4KB page: bit 31-12, addr */
    uint32_t bit12              :1;
    uint32_t bit21_13           :9; 
    uint32_t bit31_22           :10;
} PDE;


volatile PDE page_dict[PD_SIZE] __attribute__((aligned (_4KB_)));    /* Page Dict */
volatile PTE page_table[PT_SIZE] __attribute__((aligned (_4KB_)));   /* Page Table for first chunk */
volatile PTE page_table_vedio_mem[PT_SIZE] __attribute__((aligned (_4KB_)));   /* Page Table for vedio memory */

/* function prototype */
void paging_init(void);
extern void paging_set_user_mapping(int32_t pid);
extern void paging_set_for_vedio_mem(int32_t virtual_addr_for_vedio, int32_t phys_addr_for_vedio);
extern void paging_restore_for_vedio_mem(int32_t virtual_addr_for_vedio);

#endif
