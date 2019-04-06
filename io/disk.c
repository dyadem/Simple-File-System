#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "disk.h"
#include "file.h"

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
		fprintf(stderr, "free inode vector write error %d\n",inode_number);
		exit(1);
	}
	// Clean up
	free(node);

	return 0;
}

int path_to_inode(char* path)
{
	if (!path) {
		return -1;
	}
	if (strlen(path) == 0 || strcmp(path, "/") == 0) {
		return 0;
	}

	// TODO: other directory paths
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

	// int padding_size = BLOCK_SIZE - ((offset%512) + data_size);

	// printf("Padding size: %d\n",padding_size );
	// char *padding = (char*) malloc(padding_size);
	// printf("sizeof padding: %lu\n", sizeof(padding) );
	// memset(padding, '0', padding_size);
	// printf("sizeof padding: %lu\n", sizeof(padding) );
	// fseek(vdisk, offset+data_size, SEEK_SET);
	// printf("sizeof padding: %lu\n", sizeof(padding) );
	// fwrite(padding, 1, padding_size, vdisk);

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

int set_block_list(int k)
{
	int *list;

	// Get list
	list = get_block_list();

	// Check bit: 1 = available slot
	if (TestBit(list, k) == 1) {
		ClearBit(list, k);
	} else {
		printf("Block already set in list!\n");
		return 1;
	}

	// Write list back to disk
	if (write_block_list(list) != 0) {
		fprintf(stderr, "Problems writing block list to disk!\n" );
		exit(1);
	}

	return 0;
}

int clear_block_list(int k)
{
	int *list;

	// Get list
	list = get_block_list();

	// Check bit: 0 = unavailable slot
	if (TestBit(list, k) == 0) {
		SetBit(list, k);
	} else {
		fprintf(stderr, "Block list bit already cleared!\n" );
		exit(1);
	}

	// Write list back to disk
	if (write_block_list(list) != 0) {
		fprintf(stderr, "Problems writing inode list to disk!\n" );
		exit(1);
	}

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
	fprintf(stderr, "No free inodes available! Disk Full?\n");
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

int set_inode_list(int k)
{
	int *list;

	// Get list
	list = get_inode_list();

	// Check bit: 1 = available slot
	if (TestBit(list, k) == 1) {
		ClearBit(list, k);
	} else {
		fprintf(stderr, "Inode already set in list!\n" );
		exit(1);
	}

	// Write list back to disk
	if (write_inode_list(list) != 0) {
		fprintf(stderr, "Problems writing inode list to disk!\n" );
		exit(1);
	}

	return 0;
}

int clear_inode_list(int k)
{
	int *list;

	// Get list
	list = get_inode_list();

	// Check bit: 0 = unavailable slot
	if (TestBit(list, k) == 0) {
		SetBit(list, k);
	} else {
		fprintf(stderr, "Inode list bit already cleared!\n" );
		exit(1);
	}

	// Write list back to disk
	if (write_inode_list(list) != 0) {
		fprintf(stderr, "Problems writing inode list to disk!\n" );
		exit(1);
	}

	return 0;
}

int add_file_direntry(int current_dir,char* filename,int inode_num){
	inode *dir_node;
	//allocate direntry in memory
	direntry *dir_entry = (direntry *)malloc(sizeof(direntry));
	if (dir_entry == NULL) {
		fprintf(stderr,"Malloc for dir_entry failed!\n");
		exit(1);
	}
	//check name is not over max
	size_t len = strlen(filename);
	int max_len = sizeof(dir_entry->name);
	if (len>=max_len){
		fprintf(stderr, "Error. Filename too long! Max length is %d characters\n", max_len);
		exit(1);
	}
	//get inode for the directory block
	dir_node = get_inode(current_dir);
	//get block number of the directory block
	int dir_block_num = dir_node->block_pointers[0];
	// printf("directory filesize: %d\n", dir_node->filesize);
	int dir_offset = (512*(dir_block_num)) + dir_node->filesize;
	// printf("dir offset: %d\n",dir_offset);
	//dir_block = block_read(dir_offset);

	memset(dir_entry->name, 0, sizeof(dir_entry->name));
	strcpy(dir_entry->name, filename);
	dir_entry->inode_num = inode_num;

	// memcpy(&dir_block->data[dirents * 16], dir_entry, sizeof(direntry));

	// Write directory entry to disk
	if (block_write(dir_entry, dir_offset, sizeof(direntry)) != 0) {
		fprintf(stderr, "Problem writing directory block to disk in write_file!\n" );
		exit(1);
	}

	//update dir inode number of direntries
	dir_node->filesize += sizeof(direntry);
	// printf("directory filesize: %d\n", dir_node->filesize);

	// Write dir inode back to disk
	if (write_inode(dir_node) != 0) {
		fprintf(stderr, "Problems writing path inode back to disk!\n");
		exit(1);
	}
	free(dir_entry);
	return(0);
}

int get_inode_num_from_direntry(char* filename){// only handles root dir TODO
	direntry *dir_entry = (direntry *)malloc(sizeof(direntry));
	int block_offset = 512*9; // root dir is 10th block
	char* dir_block = block_read(block_offset);
	char name[28];
	int inode_num;
	int dirent_offset = 0; //add for each iteration 32
	// //iterate through block of dirents 512/32 = 16
	for (int i=0; i < 16; i++){
			strcpy(name,dir_block+dirent_offset);
			// compare the name in direntry with target filename
			if (strcmp(name,filename) == 0){ // found the file!
				memcpy(dir_entry,dir_block+dirent_offset,32);
				inode_num = dir_entry->inode_num;
				free(dir_entry);
				free(dir_block);
				// printf("inode num: %d\n", inode_num);
				return(inode_num);
				break;
			}
			dirent_offset += 32;
		}
	free(dir_entry);
	free(dir_block);
	fprintf(stderr, "Error finding file inode!%s\n", filename );
	exit(-5);
}




