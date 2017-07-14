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
int last_frame_counter = 0;
int fifo_counter = 0;
int* seech_array;
int is_random;
int nframes;
int npages;
char* type = NULL;
char *virtmem;
char *physmem;
struct page_table* pt;

void random_fault_handler(struct page_table *pt, int page);
void fifo_fault_handler(struct page_table *pt, int page);
void second_chance_fault_handler(struct page_table *pt, int page);

void page_fault_handler( struct page_table *pt, int page ){
	page_faults++;

	if (!strcmp(type, "rand")){
		random_fault_handler(pt,page);
	}
	else if (!strcmp(type, "fifo")){
		fifo_fault_handler(pt,page);
	}
	else if (!strcmp(type, "sech")){
		second_chance_fault_handler(pt,page);
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

	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	pt = page_table_create( npages, nframes, page_fault_handler );

	if (npages <= nframes){
			for (int i = 0; i < npages; i++) {
				page_table_set_entry(pt,i, i, PROT_READ );
			}
	}

	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	virtmem = page_table_get_virtmem(pt);
	physmem = page_table_get_physmem(pt);
	seech_array = (int*)calloc(nframes, sizeof(int));

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

	// seeding 
	if (!is_random){
		srand(time(NULL));
		is_random = 1;
	}

	int frame;
	int bits;
	int random_frame = (rand()) % nframes;

	page_table_get_entry(pt, page, &frame, &bits);

	if(bits){
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
	}

	else if (!bits){

		// if full
		if (last_frame_counter == nframes){

			for (int i = 0; i < npages; i++){
				page_table_get_entry(pt, i, &frame, &bits);

				if (frame == random_frame){

					disk_write(disk, i, &physmem[frame* PAGE_SIZE]);
					disk_read(disk, page, &physmem[frame* PAGE_SIZE]);
					page_table_set_entry(pt, page, frame, PROT_READ);
					page_table_set_entry(pt, i, 0, 0);
					disk_reads++;
					disk_writes++;
					break;

				}
			}
		}
		// if not full
		else{
			page_table_set_entry(pt,page, last_frame_counter, PROT_READ);
			disk_read(disk, page, &physmem[last_frame_counter* PAGE_SIZE]);
			last_frame_counter++;
			disk_reads++;
		}

	}
	else{
		printf("Fault in random page handler\n");
		exit(1);
	}
}


void fifo_fault_handler(struct page_table *pt, int page){

	int frame;
	int bits;

	page_table_get_entry(pt, page, &frame, &bits);

	if(bits){
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
	}

	else if (!bits){

		// if full
		if (last_frame_counter == nframes){

			for (int i = 0; i < npages; i++){
				page_table_get_entry(pt, i, &frame, &bits);

				if (frame == fifo_counter%nframes){

					if ((bits ^ (PROT_READ|PROT_WRITE))==0){
						disk_write(disk, i, &physmem[frame* PAGE_SIZE]);
						disk_read(disk, page, &physmem[frame* PAGE_SIZE]);
						page_table_set_entry(pt, page, frame, PROT_READ);
						page_table_set_entry(pt, i, 0, 0);
						disk_writes++;
					}
					else{
						disk_read(disk, page, &physmem[frame* PAGE_SIZE]);
						page_table_set_entry(pt, page, frame, PROT_READ);
						page_table_set_entry(pt, i, 0, 0);
					}
					
					disk_reads++;
					fifo_counter += 1;
					break;

				}
			}
		}
		// if not full
		else{
			page_table_set_entry(pt,page, last_frame_counter, PROT_READ);
			disk_read(disk, page, &physmem[last_frame_counter* PAGE_SIZE]);
			last_frame_counter++;
			disk_reads++;
		}

	}
	else{
		printf("Fault in random page handler\n");
		exit(1);
	}

}

void second_chance_fault_handler(struct page_table *pt, int page){

	int frame;
	int bits;

	page_table_get_entry(pt, page, &frame, &bits);

	if(bits){
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
		seech_array[frame] = (seech_array[frame] + 1) % 2;
	}

	else if (!bits){
		// if full
		if (last_frame_counter == nframes){

			for (int i = 0; i < npages; i++){

				while (seech_array[i] == 1) {
					seech_array[i] = 0;
					fifo_counter+=1;
				}
				page_table_get_entry(pt, i, &frame, &bits);

				if (frame == fifo_counter%nframes){

					if ((bits ^ (PROT_READ|PROT_WRITE))==0){
						disk_write(disk, i, &physmem[frame* PAGE_SIZE]);
						disk_read(disk, page, &physmem[frame* PAGE_SIZE]);
						page_table_set_entry(pt, page, frame, PROT_READ);
						page_table_set_entry(pt, i, 0, 0);
						disk_writes++;
					}
					else{
						disk_read(disk, page, &physmem[frame* PAGE_SIZE]);
						page_table_set_entry(pt, page, frame, PROT_READ);
						page_table_set_entry(pt, i, 0, 0);
					}
					
					disk_reads++;
					fifo_counter += 1;
					seech_array[i] = 1;
					break;

				}
			}
		}
		// if not full
		else{
			page_table_set_entry(pt,page, last_frame_counter, PROT_READ);
			disk_read(disk, page, &physmem[last_frame_counter* PAGE_SIZE]);
			seech_array[last_frame_counter] = 1; 
			last_frame_counter++;
			disk_reads++;
		}

	}
	else{
		printf("Fault in random page handler\n");
		exit(1);
	}
}