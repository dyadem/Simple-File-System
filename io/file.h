//file.h userspace api

// common errors
#define ERR_DISK_FULL -1
#define ERR_MAX_FILES -2
#define ERR_FILE_EXISTS -3
#define ERR_PAST_END -4
#define ERR_FILE_NOT_FOUND -5
#define ERR_INVALID_PATH -6
#define ERR_TOO_MANY_FILES_OPEN -7
#define ERR_FILE_NOT_OPEN -8
#define ERR_MAX_FILESIZE -9

// #define MAX_FILESIZE 2093056

#define SUCCESS 0
#define LSEEK_FROM_CURRENT 0
#define LSEEK_ABSOLUTE 1
#define LSEEK_END 2

//initializes the virtual disk with superblock,
//free-block and free inode vector and inode blocks
int initFS(char *vdisk_path);

// Initializes block 9 with the root directory
int init_root();

// creates a new empty file in the current directory
int create_file();

// writes a file to disk
int write_new_file(char *file, char *path);

// prints the contents of a file from disk to the command line
int read_file(char *filename);

// removes a file
int file_delete(char *path); //TODO

// makes a new directory
int file_mkdir(char *path); //TODO

// removes the directory
int file_rmdir(char *path); //TODO

// lists the contents of the directory
char **file_listdir(char *path); //TODO
