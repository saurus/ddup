ddup: Detect duplicate files.
====

This simple utility reads from stdin a list of filenames, and check for duplicates files (i.e. with the same content). 

The default output is a list of "ln -f" commands, but it can be changed using the '-f' option and adding a new format string.

This program does not writes any change to disk.

I wrote this simple tool out of frustration on how slow and complex are other similar utilities are.

A simple way to run this program is something like:

    find . -type f | ddup > merge_files.sh

then you can review and run "merge_files.sh", or use it as a start for other scripts.

How this works
--------------

the algorithm is:

- read file names from standard input
- group files by size, skipping empty files and files with the same inode.
- when more than one file with the same size was found calculate the md5 of the first bytes, and create a hash table indexed by md5.
- when more than one file with the same md5 if found, do a full compare to check if are really the same file.
- for every file marked as a duplicate of another, write a "ln -f" command to standard output.

