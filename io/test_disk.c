#include <stdlib.h>
#include <stdio.h>
#include "file.h"
#include "disk.h"
#include <assert.h>

int test_disk(){

	//open the disk
	if (open_fs(VDISK_PATH) !=0){
		exit(1);
	}

//test next free block
	superblock *spb = get_superblock();
	assert(spb->next_free_block == get_next_free_block());
	assert(spb->next_free_inode == get_next_free_inode());
//test write inode

	// printf("Testing inode write to inode 3. Size: %zu\n", sizeof(inode));
	// inode *iptr = (inode *)malloc(sizeof(inode));
	// if (iptr == NULL){
	// 	fprintf(stderr, "Inode malloc failure\n");
	// 	exit(1);
	// }
	// iptr->inode_num = 3;
	// iptr->filesize = 612;
	// iptr->directory_flag = 1;
	// iptr->block_pointers[0] = 44;
	// iptr->block_pointers[1] = 2;
	// write_inode(iptr);

//test get inode

	// for (int i = 0; i < 112; i++){
	// 	inode *inod = get_inode(i);
	// 	printf("%d\n",inod->inode_num);
	// }

	// inode *node = get_inode(3);
	// node->inode_num = 2;
	// write_inode(node);

	close_fs();
	return 0;
}

int main(){
	// printf("testing Makefile\n");..
	char *vdisk_path = VDISK_PATH;
	initFS(vdisk_path);
	test_disk();
	init_root();
	test_disk();

	return 0;
}