#include "mm.h"

static unsigned short mem_map[PAGING_PAGES] = {0, };

int remain_page = PAGE_ENTRY;

unsigned long get_free_page() {
    for(int i=0; i<PAGING_PAGES; i++) {
        if(mem_map[i] == 0) {
            mem_map[i] = 1;
            return LOW_MEMORY + i*PAGE_SIZE;
        }
    }
    return 0;
}

unsigned long get_free_page_id() {
    for(int i=0; i<PAGING_PAGES; i++) {
        if(mem_map[i] == 0) {
            mem_map[i] = 1;
            return i;
        }
    }
    return 0;
}

void free_page(unsigned long p) {
    mem_map[(p - LOW_MEMORY) / PAGE_SIZE] = 0;
}

unsigned long get_user_page(int pid) {
	return LOW_USER_STACK + (pid - 1) * PAGE_SIZE;
}

void init_page_struct(){
	// reset page struct
	int i = 0;	
	for(;i<FIRST_AVAILIBLE_PAGE;i++){
		page[i].used = PRESERVE;
		remain_page--;
	}
		
	for(;i<PAGE_ENTRY;i++){
		page[i].used = NOT_USED;
	}
}
