Name: Kamal Acharya
Email: acharya.24@wright.edu

Project Directory contains the following files as listed by command ls -l
total 884
-rw-rw-r-- 1 kamal kamal   3267 Mar  7 11:16 answers.txt
-rw------- 1 kamal kamal   2902 Apr  2  2007 bitvector.cpp
-rw-rw-r-- 1 kamal kamal  37416 Mar 30 09:49 bitvector.o
-rw------- 1 kamal kamal  65536 Mar 30 20:10 D1.dsk
-rw------- 1 kamal kamal  10080 Mar 30 13:04 directory.cpp
-rw-rw-r-- 1 kamal kamal  57408 Mar 30 14:41 directory.o
-rw------- 1 kamal kamal    232 Mar 20 10:38 diskParams.dat
-rw------- 1 kamal kamal   4199 Apr  2  2007 file.cpp
-rw-rw-r-- 1 kamal kamal  40384 Mar 30 09:49 file.o
-rw------- 1 kamal kamal   7674 Mar 30 09:48 fs33types.hpp
-rw-rw-r-- 1 kamal kamal  11559 Mar  9 21:57 gdbSession.txt
-rw-rw-r-- 1 kamal kamal    179 Mar 30 16:36 Improvements.txt
-rw------- 1 kamal kamal   7569 Apr  2  2007 inodes.cpp
-rw-rw-r-- 1 kamal kamal  46640 Mar 30 09:49 inodes.o
-rw------- 1 kamal kamal    722 Jul  7  2020 Makefile
-rw------- 1 kamal kamal   3641 Apr  2  2007 mount.cpp
-rw-rw-r-- 1 kamal kamal  62872 Mar 30 09:49 mount.o
-rwxrwxr-x 1 kamal kamal 229608 Mar 30 20:09 P0
-rw-rw-r-- 1 kamal kamal   3198 Mar 30 20:12 ReadMe.txt
-rw------- 1 kamal kamal  22420 Mar 30 20:09 shell.cpp
-rw------- 1 kamal kamal   9044 Oct 29  2020 shell.cpp~
-rw-rw-r-- 1 kamal kamal 113208 Mar 30 20:09 shell.o
-rw------- 1 kamal kamal   3811 Mar 25 15:28 simdisk.cpp
-rw-rw-r-- 1 kamal kamal  41512 Mar 30 09:49 simdisk.o
-rw-rw-r-- 1 kamal kamal    293 Mar 30 20:10 testscript.txt
-rw-r--r-- 1 kamal kamal   2718 Sep 24  2020 typescript
-rw------- 1 kamal kamal    112 Apr  2  2007 user.cpp
-rw------- 1 kamal kamal   7364 Mar 23 10:42 volume.cpp
-rw-rw-r-- 1 kamal kamal  48608 Mar 30 09:49 volume.o






As I was new to operating system and progrmming in unix process, it was tough task for me. I have gone through
number of youtube channels that guide me alot. And ofcourse I repeatly watch the class lecture in which our professor
has given the description. I have also discussed many times with the professor during the office hour which was also very helpful for me.
 As of now my shell has following features:
1. It can successfully handle the redirection even for the commands having arguments
2. Piping is implemented successfully for up to three commands using two pipes
3. Background execution is also implemented.
4. mkdir command for making the directory
5. rmdir command for removing the directory
6. cd command to change the directory
7. ls command to list the content of the directory
8. ls -lR command to list the content recursively
9. rm -fr command to delete all the contents recursively
10. pwd command to find current working directory
11. mv command to move the file and the directory


Problems:
1. In piping only upto three commands are supported
2. Piping,redirection or background execution all are not supported in single command.
4. cd and pwd command behave badly when used after mov command.

I have demonstrated all the above features through the TestScript.txt file which contains the various commands that can
be used to test the shell and its robustness.
