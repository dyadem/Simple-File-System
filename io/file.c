// functions for creating, deleting, and reading directories

#include "file.h"
#include "disk.h"
#include <string.h>
#include <assert.h>

//vdisk management functions
int open_fs(char *fs_path)
{
	// Open vdisk
	vdisk = fopen(fs_path, "r+");

	if (vdisk == NULL) {
		fprintf(stderr, "Error opening file system!\n");
		exit(1);
	}
	return 0;

}

void close_fs(void)
{
	// Close vdisk
	int retval = fclose(vdisk);

	if (retval == 0) {
		// printf("File system closed successfully...\n");
	} else {
		fprintf(stderr, "File system could not be closed!\n");
	}
}

//disk initialization
int initFS(char *vdisk_path){
	superblock *sb;

	vdisk = fopen(vdisk_path, "wb+");
	if (vdisk == NULL){
		fprintf(stderr, "Error opening disk on init\n");
		return 1;
	}
	//create superblock

	sb = (superblock *)malloc(sizeof(superblock));
	if (sb == NULL) {
		printf("Malloc for superblock failed!\n");
		return 1;
	}
	sb->magic_number = MAGIC_NUMBER;
	sb->block_count = BLOCKS;
	sb->current_files = 1; //root dir
	sb->max_files = NUM_INODES;
	sb->allocated_blocks = 10;
	sb->next_free_block=9;
	sb->next_free_inode=0;
	//sb->max_files = MAX_FILES

	//create free block vector
	int ints = BLOCK_LIST_ENTRIES; // number of ints needed for 4096 blocks/8 bits per byte/4 bytes per int
	int block_list[ints];
	//initialize free blocks 0=used 1=free keep first ten used for metadata
	for (int i = 0; i < BLOCKS; i++){
		if (i < 9){
			//set first 9 blocks to 0=used for sb, free vectors and inodes
			ClearBit(block_list, i);
		}
		else{
			SetBit(block_list, i);
		}
	}

	//create free inode vector, use 128 ints to pad block
	int inode_list[ints];
	//initialize free inode vector 0=used 1=free
	for (int j = 0; j < BLOCKS; j++){
		//set all the inodes to 1=free
		if (j<NUM_INODES){
			SetBit(inode_list, j);
		}
		//fill up the block with 0s
		else {
			ClearBit(inode_list, j);
		}
	}

	//write blocks to disk
	if (fwrite(sb, 1, BLOCK_SIZE, vdisk) != BLOCK_SIZE){
		fprintf(stderr, "superblock write error\n");
		exit(1);
	}
	if (fwrite(block_list, 1, sizeof(block_list), vdisk) != sizeof(block_list)){
		fprintf(stderr, "free block vector write error\n");
		exit(1);
	}
	if (fwrite(inode_list, 1, sizeof(inode_list), vdisk) != sizeof(inode_list))	{
		fprintf(stderr, "free inode vector write error\n");
		exit(1);
	}

	// Build inodes
	// printf("Building %d inodes\n", sb->max_files);
	for (int k = 0; k < sb->max_files; k++){
		// printf("%zu\n",sizeof(inode));
		inode *iptr = (inode *)malloc(sizeof(inode));
		if (iptr == NULL){
			fprintf(stderr, "Inode malloc failure\n");
			exit(1);
		}
		// Initialize values
		iptr->inode_num = k;
		iptr->filesize = 0;
		//initialize default as 0=file
		iptr->directory_flag = 0;
		// set pointers to 0, otherwise they initialize with values (?)
		for (int l = 0; l < 10; l++){
			iptr->block_pointers[l] = 0;
		}
		iptr->indirect_pointer= 0;
		//write inodes
		if (fwrite(iptr, 1, sizeof(inode), vdisk) != sizeof(inode)){
			fprintf(stderr, "inode %d write error\n", k);
			exit(1);
		}
		free(iptr);
	}

	// printf("%zu \n", sizeof (struct inode));
	// printf("%lu \n", sizeof(inode));

	//initialize root inode for dir 0;
	// inode *root_inode = get_inode(0);
	// root_inode->filesize = 1;
	// root_inode->directory_flag = 1;
	//root indoe clear bit on free list

	//clean up init
	free(sb);
	fclose(vdisk);
	printf("Initializtion finished\n");
	return 0;
}

//root directory initialization
int init_root(){
	//open the disk for file editing
	if (open_fs(VDISK_PATH) !=0){
		exit(1);
	}

	superblock *spb = get_superblock();

	//update block_list

	//get next block number (should be block 9), update it to 0=used
	int *b_list = get_block_list();
	// printf("NFB Before: %d\n",spb->next_free_block);
	if (TestBit(b_list, spb->next_free_block) == 1)
		ClearBit(b_list, spb->next_free_block);
	else {
		fprintf(stderr, "next free block not available, check superblock!\n");
		exit(1);
	}
	// Write block list back to disk
	if (write_block_list(b_list) != 0) {
		fprintf(stderr, "Error writing block list for root dir \n");
		exit(1);
	}
	spb->next_free_block++;
	// printf("NDB After: %d\n",spb->next_free_block);

	//update inode_list

	//get next inode number (should be inode 0), update it to 0=used

	int *i_list = get_inode_list();
	// printf("NFI Before: %d\n",get_next_free_inode());
	if (TestBit(i_list, spb->next_free_inode) == 1)
		ClearBit(i_list, spb->next_free_inode);
	else{
		printf("next free inode not available, check superblock!\n");
		exit(1);
	}
	// Write inode list back to disk
	if (write_inode_list(i_list) != 0) {
		printf("Error writing inode list for root dir \n");
		exit(1);
	}
	spb->next_free_inode++;
	// printf("NFI After: %d\n",get_next_free_inode());

//TODO create block 9 with root directory entry

// TODO update superblock

	write_superblock(spb);

	close_fs();
	printf("Initialized root directory.\n");

	return 0;
}
