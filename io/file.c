// file.c functions for creating, deleting, and reading directories

#include "file.h"
#include "disk.h"
#include <string.h>
#include <assert.h>

//disk initialization
int initFS(char *vdisk_path){
	superblock *sb;

	vdisk = fopen(vdisk_path, "wb+");
	printf("vdisk path: %s\n",vdisk_path);
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	printf("cwd: %s\n",cwd);
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
		// set pointers to 0, otherwise they initialize with values from stack(?)
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
	// printf("NFB: %d\n",sb->next_free_block);
	// printf("NFB: %d\n",get_next_free_block());

	//initialize root inode for dir 0;
	// inode *root_inode = get_inode(0);
	// root_inode->filesize = 1;
	// root_inode->directory_flag = 1;
	//root indoe clear bit on free list

	//clean up init
	free(sb);
	fclose(vdisk);
	printf("Disk initializtion complete.\n");
	return 0;
}

//root directory initialization
int init_root(){

	//open the disk for file editing
	if (open_fs(VDISK_PATH) !=0){
		exit(1);
	}

	superblock *spb = get_superblock();
	direntry *dirent;
	direntry *dirent_self;

	// create block 9 with root directory entry
//TODO generic file_create() with all the updating built in
	// printf("sizeof name: %lu\n", sizeof(dirent->name));
	block *new_block = (block *)malloc(sizeof(block));

	dirent = (direntry *)malloc(sizeof(direntry));
	if (dirent == NULL) {
		fprintf(stderr, "Malloc for root direntry failed\n" );
		return 1;
	}
	//
	strncpy(dirent->name,"/",sizeof(dirent->name)-1);
	dirent->name[sizeof(dirent->name)-1] = 0; // ensure termination
	// int num = get_next_free_inode();
	// printf("Inode num: %d\n", num);
	dirent->inode_num = get_next_free_inode(); //should be 0
	int next_block = get_next_free_block();//should be 9 for root block
	int offset = BLOCK_SIZE*next_block; // 512*9 gets to start of block 9
	memcpy(new_block->data, dirent, sizeof(direntry));
	block_write(new_block,offset,sizeof(block));

	//write direntry for self ".""
	dirent_self = (direntry *)malloc(sizeof(direntry));
	if (dirent_self == NULL) {
		fprintf(stderr, "Malloc for root direntry self failed\n" );
		return 1;
	}
	strncpy(dirent_self->name,".",sizeof(dirent_self->name)-1);
	dirent_self->name[sizeof(dirent_self->name)-1] = 0; // ensure termination
	dirent_self->inode_num = get_next_free_inode(); //should be 0
	int offset2 = (BLOCK_SIZE*next_block) + sizeof(direntry); // 512*9 gets to start of block 9
	block_write(dirent_self,offset2,sizeof(direntry));

	free(new_block);

	// unpdate inode for root dir
	inode *iptr = get_inode(dirent->inode_num);
	iptr->filesize = sizeof(direntry)*2; //directory name "/" and ".""
	iptr->directory_flag = 1; //for directory
	iptr->block_pointers[0] = next_block;//pointer to directory block
	for (int i = 1; i < 10; i++){
		iptr->block_pointers[i] = 0;
	}
	write_inode(iptr);

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
	// printf("NFB After: %d\n",spb->next_free_block);

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
	// printf("NFI after: %d\n", spb->next_free_inode);
	// printf("NFI After: %d\n",get_next_free_inode());

	// update superblock
	spb->current_files++;
	spb->allocated_blocks++;

	//clean up
	free(dirent);

	write_superblock(spb);

	close_fs();
	printf("Initialized root directory.\n");

	return 0;
}

int create_file(char* filename){
	printf("creating file: %s\n",filename);
	int block_number;
	int block_offset;
	inode *new_inode;
	block *new_block;
	file_info *fileinfo;

	//open the disk
	if (open_fs(VDISK_PATH) !=0){
		exit(1);
	}

	superblock *sb = get_superblock();
/*
	// Get file name from end of path
	path_length = strlen(path);
	for (i = 0; i < path_length; i++) {
		if (path[i] == '/') {
			slash_index = i;
		}
	}
	//replace filename with null termination to get the path to file
	new_path = strdup(path);
	new_path[slash_index] = '\0';

	memcpy(filename, &new_path[slash_index + 1], 12);

	// printf("slash index: %d\n", slash_index);
	// printf("%s\n", filename );
	*/
	// Get inode for file
	int inode_num = sb->next_free_inode;

	// Get next available inode for the file
	// printf("next free inode: %d\n",sb->next_free_inode);
	new_inode = get_inode(inode_num);
	new_inode->directory_flag = 0;
	new_inode->filesize = 0;//empty file  TODO: add file descriptor

	// Get block to use for the file
	new_block = (block *)malloc(sizeof(block));
	memset(new_block->data, 0, sizeof(new_block->data));
	block_number = get_next_free_block();
	// printf("next free block %d\n",block_number);

	// write file descriptor to the new block
	fileinfo = (file_info *)malloc(sizeof(file_info));
	//generate path: only handles root dir for now
	char path[] = "/"; //TODO get path from current_dir
	char * fullpath = (char *) malloc(1 + strlen(path)+ strlen(filename) );
	strcpy(fullpath, path);
	strcat(fullpath,filename);
	strcpy(fileinfo->filepath, fullpath);
	fileinfo->inumber = inode_num;
	fileinfo->cursor = 0;

	// write fileinfo to new block
	memcpy(new_block->data,fileinfo,sizeof(file_info));

	block_offset = (block_number) * BLOCK_SIZE;
	// Write new block back to disk
	if (block_write(new_block, block_offset, BLOCK_SIZE) != 0) {
		fprintf(stderr, "Problems writing new block back to disk\n" );
		free(new_block);
		free(sb);
		exit(1);
	}

	// Add block number to its inode
	//TODO improve for allocating multiple blocks for larger files
	memset(new_inode->block_pointers, 0, sizeof(new_inode->block_pointers));
	for (int j = 0; j < 10; j++) {
		// printf("inode bp at %d: %d\n", j, new_inode->block_pointers[j]);
		if (new_inode->block_pointers[j] == 0) {
			new_inode->block_pointers[j] = block_number;
			// printf("inode bp at %d: %d\n", j, new_inode->block_pointers[j]);
			break;
		}
	}

	// add filesize to inode
	new_inode->filesize = sizeof(direntry);

	// Write new inode to disk
	if (write_inode(new_inode) != 0) {
		fprintf(stderr, "Problems writing new inode to disk!\n" );
		free(new_inode);
		free(sb);
		exit(1);
	}

	// Update free block vector
	set_block_list(block_number);

	// Update inode list vector
	set_inode_list(sb->next_free_inode);

	//add direntry to directory block
	int current_dir = 0; // only handle root dir for now. TODO: add CWD();
	add_file_direntry(current_dir,filename,inode_num);

	// Update Superblock
	sb->next_free_block++;
	sb->next_free_inode++;
	sb->allocated_blocks++;

	// Clean up
	free(new_block);
	free(fileinfo);
	free(fullpath);

	// Write Superblock back to disk
	if(write_superblock(sb) != 0) {
		printf("Problems writing superblock to disk!\n");
		free(sb);
		return 1;
	}

	//close file
	close_fs();
	printf("file created: %s\n",filename );
	return 0;

}

int write_new_file(char *file, char *path){
	FILE *f;
	char *contents;
	long file_size;
	int block_offset, block_number, inode_num;
	inode *new_inode;
	block *new_block;
	superblock *sb;
	file_info *fileinfo;
	char *filename = file; //must be in CWD on host machine for now

	//open the disk
	if (open_fs(VDISK_PATH) !=0){
		exit(1);
	}

	sb = get_superblock();

	printf("Writing new file: %s\n",file );
	//open target file
	f = fopen(file, "rb");
	fseek(f, 0, SEEK_END);

	//check size
	file_size = ftell(f);
	rewind(f);
	// printf("filesize: %ld\n",file_size );

	//read file into memory
	contents = malloc(file_size + 1);
	if (contents == NULL) {
		fprintf(stderr,"Malloc for contents failed!\n");
		exit(1);
	}
	fread(contents, 1, file_size, f);
	fclose(f);

	//null terminations
	contents[file_size] = 0;
	// printf("Contents: %s\n", contents);
	//Get file inode
	inode_num = get_next_free_inode();
	new_inode = get_inode(inode_num);
	new_inode->directory_flag = 0; //set 0 for file
	new_inode->filesize = file_size;

	//get new block
	block_number = get_next_free_block();
	new_block = (block *)malloc(sizeof(block));
	if (new_block == NULL) {
		fprintf(stderr,"Malloc for new_block failed!\n");
		exit(1);
	}
	memset(new_block->data, 0, sizeof(new_block->data));
	// printf("next free block %d\n",block_number);

	// write fileinfo descriptor to the new block
	fileinfo = (file_info *)malloc(sizeof(file_info));
	//path = "/filename.ex"; TODO handle multiple directory tree
	// char * fullpath = (char *) malloc(1 + strlen(path)+ strlen(filename) );
	// if (fullpath == NULL) {
	// 	fprintf(stderr,"Malloc for fullpath failed!\n");
	// 	exit(1);
	// }
	// strcpy(fullpath, path);
	// strcat(fullpath,filename);
	strcpy(fileinfo->filepath, path);
	fileinfo->inumber = inode_num;
	fileinfo->cursor = 0;

	// copy fileinfo to new block in memory
	memcpy(new_block->data,fileinfo,sizeof(file_info));

	// copy contents to new block in memory with offset (ie. after fileinfo)
	if (file_size < 512-sizeof(file_info)){
		//OK small enough for one block
			memcpy(&new_block->data[sizeof(file_info)],contents, file_size);
	}
	else{
		fprintf(stderr, "FS can't handle multi block files yet!\n");
		//TODO handle larger files
		exit(1);
	}

	block_offset = (block_number) * BLOCK_SIZE;
	// Write new block with fileinfo to disk
	if (block_write(new_block, block_offset, BLOCK_SIZE) != 0) {
		fprintf(stderr, "Problems writing new block back to disk\n" );
		exit(1);
	}

	//update inode block pointer
	new_inode->block_pointers[0] = block_number;

	// Write new inode to disk
	write_inode(new_inode);

	// Update free block vector
	set_block_list(block_number);

	// Update inode list vector
	set_inode_list(sb->next_free_inode);

	//add direntry
	int current_dir = 0;//only handling root dir TODO: add CWD
	add_file_direntry(current_dir,filename,inode_num);

	//update superblock
	sb->next_free_block++;
	sb->next_free_inode++;
	sb->allocated_blocks++;

	// Write Superblock back to disk
	if(write_superblock(sb) != 0) {
		printf("Problems writing superblock to disk!\n");
		free(sb);
		return 1;
	}

	// Clean up
	free(contents);
	free(new_block);
	free(fileinfo);
	// free(fullpath);

	//close file
	close_fs();
	printf("exiting file write: %s\n",filename );
	return 0;
}

int read_file(char *filename){

	char * contents;
	int filesize;
	int inode_num, block_offset;
	inode *file_inode;
	block *file_block;

	printf("reading file: %s\n",filename );
	//open the disk
	if (open_fs(VDISK_PATH) !=0){
		exit(1);
	}

	//get inode of file from direntry
	inode_num = get_inode_num_from_direntry(filename);
	file_inode = get_inode(inode_num);

	//get filesize
	filesize = file_inode->filesize;
	contents = malloc(filesize+1);

	//get block
	block_offset = (file_inode->block_pointers[0] * BLOCK_SIZE) + sizeof(file_info);
	file_block = (block *)block_read(block_offset);

	//get contents of block
	memcpy(contents, &file_block->data, filesize);

	//print contents to command line
	printf("Contents of file: \n%s\n", contents);

	//clean up
	free(file_block);
	free(contents);
	free(file_inode);

	close_fs();
	printf("exiting file read: %s\n",filename );
	return 0;
}
