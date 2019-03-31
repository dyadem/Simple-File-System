#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "disk.h"
#include "file.h"

//helper functions for working with bit arrays
void  SetBit( int A[],  int k )
   {
      A[k/32] |= 1 << (k%32);  // Set the bit at the k-th position in A[i]
   }

void  ClearBit( int A[],  int k )
   {
      A[k/32] &= ~(1 << (k%32));
   }

int TestBit( int A[],  int k )
   {
      return ( (A[k/32] & (1 << (k%32) )) != 0 ) ;
   }


// Gets an inode from the "disk" based on the inumber
inode *get_inode(int inode_number){

	char buffer[BLOCK_SIZE];
	inode *ret;
	int block_number, block_offset, inode_offset;

	// check that inode number is valid
	if (inode_number >= NUM_INODES){
		fprintf(stderr, "Requested inode is invalid\n");
		exit(1);
	}

	//get block number of requested inode
	block_number = 3 + (inode_number / (BLOCK_SIZE / INODE_SIZE));
	block_offset = (block_number * BLOCK_SIZE);

	//move cursor to the top of inode block
	fseek(vdisk, block_offset, SEEK_SET);

	//read block into buffer
	fread(buffer, BLOCK_SIZE, 1, vdisk);

	//get inode offset
	inode_offset = inode_number % (BLOCK_SIZE / INODE_SIZE);

	//create a return inode
	ret = (inode *)malloc(sizeof(inode));
	if (ret == NULL) {
		printf("Malloc for getting inode failed!\n");
		return NULL;
	}
	//get bytes from disk
	memcpy(ret, &buffer[inode_offset * INODE_SIZE], INODE_SIZE);

	// printf("Returned inode number: %d\n", ret->inode_num);

	return ret;
}

int write_inode(inode *node){
	int inode_number, block_number, block_offset, inode_offset;

	//get inode number to write back to disk
	inode_number = node->inode_num;

	// Get block number to write inode back to
	block_number = 3 + (inode_number / (BLOCK_SIZE / INODE_SIZE));
	block_offset = (block_number * BLOCK_SIZE);

	// Move cursor to top of inode block
	fseek(vdisk, block_offset, SEEK_SET);

	// Calculate inode offset
	inode_offset = inode_number % (BLOCK_SIZE / INODE_SIZE);

	// Move cursor to top of inode
	fseek(vdisk, (inode_offset * INODE_SIZE), SEEK_CUR);

	// Write inode into disk
	// printf("Inode size: %zu\n", sizeof(inode));
	if (fwrite(node, 1, sizeof(inode), vdisk) != sizeof(inode)){
		fprintf(stderr, "free block vector write error\n");
		exit(1);
	}
	// Clean up
	free(node);

	return 0;
}

void* block_read(int offset)
{
	void* buffer = malloc(BLOCK_SIZE);

	// Move cursor to top of block
	fseek(vdisk, offset, SEEK_SET);

	// Read block into buffer
	fread(buffer, BLOCK_SIZE, 1, vdisk);

	return buffer;
}

int block_write(void* block, int offset, int data_size)
{

	// Move cursor to top of block
	fseek(vdisk, offset, SEEK_SET);

	// Write block to disk
	fwrite(block, 1, data_size, vdisk);

	return 0;
}

superblock* get_superblock()
{
	// Superblock is always the first block
	int block_offset = 0;
	superblock* sb;

	// Set up superblock struct
	sb = (superblock *)malloc(BLOCK_SIZE);
	if (sb == NULL) {
		fprintf(stderr, "Malloc for getting superblock failed!\n");
		exit(1);
	}

	// Read superblock from disk
	sb = (superblock*)block_read(block_offset);

	return sb;
}

int write_superblock(superblock* sb)
{
	// Superblock is always the first block
	int block_offset = 0;

	// Read superblock from disk
	if (block_write(sb, block_offset,BLOCK_SIZE) != 0) {
		fprintf(stderr, "Problem writing superblock!\n");
		exit(1);
	}

	// Clean up
	free(sb);

	return 0;
}

int* get_block_list()
{
	// Free block vector is always the second block
	int block_offset = BLOCK_SIZE;

	// Set up int array for 128 ints (512 bytes);
	int* block_list = malloc(BLOCK_LIST_ENTRIES);
	if (block_list == NULL) {
		fprintf(stderr, "Malloc failed for get free block vector\n");
		exit(1);
	}

	// Read free block vector from disk
	block_list = (int *)block_read(block_offset);

	return block_list;
}

int get_next_free_block(){
	int *b_list = get_block_list();
	for (int i=0; i < BLOCKS; i++){
		// printf("block_list: %d = %d\n",i, TestBit(b_list, i));
		if (TestBit(b_list, i) == 1){
			return i;
			exit(0);
		}
	}
	fprintf(stderr, "No free blocks available! Disk Full?\n");
	exit(1);
}

int write_block_list(int* block_list)
{
	int block_offset;

	// Free block vector is always the second block
	block_offset = BLOCK_SIZE;

	// Write free block vector to disk
	if (block_write(block_list, block_offset,BLOCK_SIZE) != 0) {
		fprintf(stderr, "Problem writing free block vector to disk\n");
		exit(1);
	}

	// Clean up
	free(block_list);

	return 0;
}

int* get_inode_list()
{
	// Free inode vector is always the third block
	int block_offset = BLOCK_SIZE*2;

	// Set up array for 3 ints (96 bits);
	int* inode_list = malloc(INODE_LIST_ENTRIES);
	if (inode_list == NULL) {
		fprintf(stderr, "Malloc failed for get free inode vector\n");
		exit(1);
	}
	// Read free inode vector from disk
	inode_list = (int *)block_read(block_offset);

	return inode_list;
}

int get_next_free_inode(){
	int *i_list = get_inode_list();
	for (int i=0; i < NUM_INODES; i++){
		// printf("here??\n");
		if (TestBit(i_list, i) == 1){
			return i;
			exit(0);
		}
	}
	fprintf(stderr, "No free blocks available! Disk Full?\n");
	exit(1);
}

int write_inode_list(int* inode_list)
{
	// Free inode vector is always the third block
	int block_offset = BLOCK_SIZE*2;
	// Write free inode vector to disk
	int data_size = INODE_LIST_ENTRIES*4; //inode data is 12 bytest long
	if (block_write(inode_list, block_offset, data_size) != 0) {
		fprintf(stderr, "Problem writing free inode vector to disk\n");
		exit(1);
	}

	// Clean up
	free(inode_list);

	return 0;
}





