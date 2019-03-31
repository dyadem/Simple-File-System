# **Assignment 3: File System**
**UVic CSC360 Spring 2019**  
**Jason Sanche V00349530 LabID:137**

To start with, I decided to keep the fs simple and have a fixed inode structure. The disk is initialized with the following blocks:  
0. Superblock 
1. A free-block list of binary flags 
2. A free-inode list of binary flags
3-8. fixed blocks of inodes, total = 96 inodes
9. root directory	


