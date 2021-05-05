#include "rand.h"

uint32_t rand_seed = 0; /* seed for generate random number */

uint32_t rand(uint32_t input_seed, uint32_t maximum){
    rand_seed = rand_seed+ input_seed * 1103515245 +12345;
	return (uint32_t)(rand_seed) % (maximum+1); 
}

void update_seed(){
    rand_seed = rand_seed * 1103515245;
}


void set_seed(uint32_t new_seed){
    rand_seed = new_seed;
    return ;
}

