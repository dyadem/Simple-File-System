# **Assignment 3: File System**

This file system implementation is a simple but functional and elegant version of the early unix file system, with a fixed structure for metadata. The tradeoff for this kind of file system is definitely speed when reading and writing files, as the disk needs to seek back and forth between the file data blocks and the metadata blocks. 

Compile in the /A3 directory with the included makefile:  

`make`

To execute the disk initialization and run the tests:  
  
`./run`   

This will run main() in test_disk.c through initialization of the virtual disk, initializing the root directory, and testing file creation, writing files, and reading text files to the command line.
 
The code is modularized into the following files:

- **file.c** the api for file manipulation
- **disk.c** a library of lower level disk control functions

##**file.c**
The following functionality is impletmented:
###- **initFS()**  
The disk is initialized with the following blocks:  

**BLOCK 0.** Superblock  
**BLOCK 1.** A free-block bit array, list of binary flags 0=used, 1=free.  
**BLOCK 2.** A free-inode list of binary flags, 0=used 1=free.  
**BLOCKS 3-8.** fixed blocks of inodes, total = 96 inodes. This will limit the filesystem to 96 files across about 2Mb of space in the 4096 blocks of 512 bytes.    
**BLOCKS 10-4095.** available for file data and directories  

Files each contain a file_info as a descriptor with the file path, inode number and cursor offset.

###- **init_root()**
**BLOCK 9.** The root directory containing space for 16 directory entries which are 32 bytes, enough for a decent sized filename and the inode number. The first direntry is for the "/" denoting the root dir, the second is and entry for "." to be used for path lookup for current working directory, and the other 14 available could be filled with entries for files.  An indirection pointer to another block for more direntries can be added to increase capacity.

###- **create_file()**
Creates a new empty file in the current directory

###- **read_file()**
Prints the contents of a file on vdisk to the command line

##**disk.c**

This disk controller api has all the lower level functionality to set and maipulate the free inode and block vectors, inodes, directories and directory entries and data blocks on the disk. One particular design choice made for expediency was the implementation of the get\_inode\_from\_directory() function which allows read_file()to go directly to a file based on a search of the name of the file in the current directory and not only the path. See disk.h for a complete list of completed functions.