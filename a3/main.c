/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct disk* disk = NULL;
int page_faults = 0;
int disk_reads = 0;
int disk_writes = 0;
int nframes;
int npages;
char* type = NULL;
char *virtmem;
char *physmem;

typedef struct 
{
	int page;
	int bits;
} frame;

frame* frame_table = NULL;

void random_fault_handler(struct page_table *pt, int page);

void remove_page(struct page_table *pt, int frame_number){

	if (frame_table[frame_number].bits & PROT_WRITE){
		disk_write(disk, frame_table[frame_number].page, &physmem[frame_number * PAGE_SIZE]);
		disk_writes++;
	}

	page_table_set_entry(pt, frame_table[frame_number].page, frame_number, 0);
	frame_table[frame_number].bits = 0;
}

int find_freeframe(){
	for (int i = 0; i < nframes; i++){

		if (frame_table[i].bits == 0){
			return i;
		}
	}

	return -1;
}

void page_fault_handler( struct page_table *pt, int page ){
	page_faults++;

	if (!strcmp(type, "rand")){
		random_fault_handler(pt,page);
	}
}

int main( int argc, char *argv[] ){
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|lru> <sort|scan|focus>\n");
		return 1;
	}

	npages = atoi(argv[1]);
	nframes = atoi(argv[2]);
	type = argv[3];
	const char *program = argv[4];
	disk = disk_open("myvirtualdisk",npages);

	frame_table = calloc(nframes,sizeof(frame));

	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	virtmem = page_table_get_virtmem(pt);
	physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[4]);

	}

	page_table_delete(pt);
	disk_close(disk);

	printf("Disk Reads: %d\n", disk_reads);
	printf("Disk Writes: %d\n", disk_writes);
	printf("Page Faults: %d\n", page_faults);

	return 0;
}

void random_fault_handler(struct page_table *pt, int page){

	int frame;
	int bits;
	int free_frame;

	page_table_get_entry(pt, page, &frame, &bits);

	if (!bits){
		bits = PROT_READ;
		free_frame = find_freeframe();

		if(free_frame < 0){
			free_frame = (int) lrand48() % nframes;
			remove_page(pt, free_frame);
		}

		disk_read(disk, page, &physmem[free_frame * PAGE_SIZE]);
		disk_reads++;
	}
	else if (bits & PROT_READ){
		bits = PROT_READ | PROT_WRITE;
		free_frame = frame;
	}
	else{
		printf("Fault in random page handler\n");
		exit(1);
	}

	page_table_set_entry(pt, page, free_frame, bits);

	frame_table[free_frame].page = page;
	frame_table[free_frame].bits = bits;
}

