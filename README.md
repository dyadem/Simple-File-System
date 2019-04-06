# **Assignment 3: File System**
**UVic CSC360 Spring 2019**  
**Jason Sanche V00349530 LabID:137**

I decided to start with a fixed inode structure with the intention that I could improve this to a log structured file system in the future. The code is modularized into disk.c, a library of lower level disk control functions, and file.c which is the api for file manipulation functions.

**Compile with the included makefile and "./run"** This will run 'main' in test_disk.c through initialization of the virtual disk, setting the root directory and testing with file creation, writing files, and reading text files to the command line. This much is implemented and functional.

The disk is initialized with the following blocks:
**BLOCK 0.** Superblock
**BLOCK 1.** A free-block bit array, list of binary flags 0=used, 1=free.
**BLOCK 2.** A free-inode list of binary flags, 0=used 1=free.
**BLOCKS 3-8.** fixed blocks of inodes, total = 96 inodes. This will limit the filesystem to 96 files across about 2Mb of space in the 4096 blocks of 512 bytes.
**BLOCK 9.** The root directory containing space for 16 directory entries which are 32 bytes, enough for a decent sized filename and the inode number. The first direntry is for the "/" denoting the root dir, the second is and entry for "." to be used for path lookup for current working directory, and the other 14 available could be filled with entries for files. TODO: create an indirection pointer to another block for more direntries so that more than 14 files could exist in a directory.
**BLOCKS 10-4095.** available for data and directories





