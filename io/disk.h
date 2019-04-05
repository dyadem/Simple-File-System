#ifndef disk_H
#define disk_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAGIC_NUMBER 0x5ccdcb8a
#define BLOCKS 4096
#define BLOCK_SIZE 512
#define INODE_SIZE 32
#define	NUM_INODES 96 // 6 blocks
#define DIRENTRY_SIZE 16
#define BLOCK_LIST_ENTRIES 128 //No of ints in the list BLOCKS/8/4
#define INODE_LIST_ENTRIES 3 //No of ints in the list NUM_INODES/8/4 bytes per int
//#define MAX_FILES ??
#define VDISK_PATH "disk/vdisk"

FILE *vdisk;

/************* structs for vdisk ******************/

// disk file descriptor
typedef struct file_info
{
	// file path
	char filepath[100];
	// inode
	int inumber;
	// byte offset
	int cursor;
} file_info;

typedef struct superblock
{
	// assigns on init
	int magic_number;

	// size of disk in blocks
	int block_count;

	// file count
	int current_files;

	// count of allocated blocks
	int allocated_blocks;

	// count of max number of files
	int max_files;

	// keep track of next free block
	int next_free_block;

	// keep track of next free inode
	int next_free_inode;

	// padding to make the block 512 bytes
	char padding[BLOCK_SIZE - ((sizeof(int) * 7))];
} superblock;

// typedef struct free_block
// {
// 	// pointer to the next free block
// 	int next_free;
// 	int prev_free;
// 	// Free status
// 	char data[BLOCK_SIZE - (sizeof(int) * 2)];
// } free_block;

typedef struct inode
{
	//inode number starting at 0
	int inode_num;
	//filesize
	int filesize;//keeps track of offset of direntry in dir block by incrementing by 16 max: 512/16 = 32
	// flag: 0 for file 1 for dir
	short directory_flag;
	//first ten data blocks
	short block_pointers[10];
	//first indirect pointer
	short indirect_pointer;
} inode;

typedef struct direntry{
	char name[12];
	int inode_num;
} direntry;

typedef struct block {
	// Size of block, 512 bytes
	char data[BLOCK_SIZE];
} block;
/************* function definitions *****************/

void  SetBit( int A[],  int k ); //DONE
void  ClearBit( int A[],  int k ); //DONE
int TestBit( int A[],  int k ); //DONE

inode *get_inode(int inode_number); //DONE
int write_inode(inode *node); //DONE
int path_to_inode(char* path); //DONE

void* block_read(int offset); //DONE
int block_write(void* block, int offset, int data_size); //DONE

superblock* get_superblock(); //DONE
int write_superblock(superblock* sb); //DONE

int* get_block_list(); //DONE
int get_next_free_block(); //DONE
int write_block_list(int* block_list); //DONE
int set_block_list(int k);//DONE
int clear_block_list(int k);//DONE

int* get_inode_list(); //DONE
int get_next_free_inode(); //DONE
int write_inode_list(int* inode_list); //DONE
int set_inode_list(int k);//DONE
int clear_inode_list(int k);//DONE

#endif