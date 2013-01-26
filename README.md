ddup
====

Detect duplicate files.

This simple utility reads from stdin a list of filenames, and check for duplicates files (i.e. with the same content). The output is a list of "ln -f" commands.

This program does not writes any change to disk.

A simple way to run this program is something like:

    find . -type f | ddup > merge_files.sh

then you can review and run "merge_files.sh", or use it as a start for other scripts.
