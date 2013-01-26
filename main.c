#define _BSD_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <unistd.h>
#include <glib.h>

// if open flag for no atime impact is not defined, define it as no-op
#ifndef O_NOATIME
# define O_NOATIME 0
#endif

// the maximun path length, tweak if needed.
#define FILENAME_LENGTH 1024

// the buffer size for md5 sum calculation: while most hard drives can
// read blocks of 512 bytes, most file systems have blocks of (at least)
// 4096 bytes 
#define MD5SUM_BUFFER_SIZE 4096

// size of buffer for full compare. the bigger, the better.
#define COMPARE_BUFFER_SIZE (1024*1024)

// this structure contains all the file information we need.
struct filedata;
struct filedata {
	long long inode;		// inode number
	long long size;			// file size
	char *md5start;			// md5sum of first 4 Kbytes
	char *name;			// file name
	struct filedata *equalto;	// this files is equal to referenced file
};

// this structure contains all the files with the same size in the array,
// and a hash table indexed by md5sum of first 4 kbytes.
struct group_same_size {
	GArray *arr;
	GHashTable *ht;
};

// from "man bash", QUOTING section:
//    Enclosing characters in double quotes preserves the literal value of all
//    characters within the quotes, with the exception of $, , \, and, when
//    history expansion is enabled, !. The  characters  $ and   retain their
//    special meaning within double quotes. The backslash retains its special
//    meaning only when followed by one of the following characters: $, , ", \,
//    or <newline>.  A double quote may be quoted within double quotes by
//    preceding it with a backslash. If enabled, history expansion will be
//    performed unless an !  appearing in double quotes is  escaped  using  a
//    backslash. The backslash preceding the !  is not removed.
static char *quote_filename(char *name, char *outbuf, int buflen)
{
	int i = 0, j = 0;
	
	do {
		switch (name[i]) {
		    case '$': 
		    case '\\':
		    case '`':
			outbuf[j++] = '\\';
			break;
		}
		outbuf[j++] = name[i++];	
	} while (name[i] != '\0' || j >= buflen - 2);
	
	if (name[i] != '\0') {
		fprintf(stderr, "output buffer too small to quote: %s.\n", name);
		return NULL;
	}
	outbuf[j] = '\0';
	return outbuf;
}

// do a full file compare, returns 1 if the file are identical, 0 if are different.
// this assumes that files are of same size. a safety check is done, but only
// after reading both files in full.
static int cmp_files(char *fn1, char *fn2)
{
	int f1, f2;
	char b1[COMPARE_BUFFER_SIZE];
	char b2[COMPARE_BUFFER_SIZE];
	int equal = 1;
	int l1, l2;
	
	f1 = open(fn1, O_RDONLY | O_NOATIME);
	if (f1 < 0) {
		perror("cmp_files(): something wrong opening file");
		fprintf(stderr, "file: %s.\n", fn1);
		return 0;
	}
	
	f2 = open(fn2, O_RDONLY | O_NOATIME);
	if (f2 < 0) {
		perror("cmp_files(): something wrong opening file");
		fprintf(stderr, "file: %s.\n", fn2);
		close(f1);
		return 0;
	}

	do {
		l1 = read(f1, b1, COMPARE_BUFFER_SIZE);
		l2 = read(f2, b2, COMPARE_BUFFER_SIZE);
		
		if (l1 != l2) {
			equal = 0;
			fprintf(stderr, "cmp_files(): something wrong: file sizes differ in %s and %s.\n", fn1, fn2);
			break;
		}
		
		if (memcmp(b1, b2, l1) != 0) {
			equal = 0;
			break;
		}
	} while (l1 > 0 && l2 > 0);
	
	close(f1);
	close(f2);
	
	return equal;
}

// calculate md5 sum of first bytes of file
static void calc_md5(struct filedata *fd)
{
        int n;
        MD5_CTX c;
        char buf[MD5SUM_BUFFER_SIZE];
        ssize_t bytes;
        unsigned char out[MD5_DIGEST_LENGTH];
        char md5[17];
	
	int f = open(fd->name, O_RDONLY);
	if (f < 0) {
		perror("calc_md5(): something wrong opening file: ");
		fprintf(stderr, "file: %s.\n", fd->name);
		close(f);
		return;
	}
	
        MD5_Init(&c);
        bytes = read(f, buf, MD5SUM_BUFFER_SIZE);
        close(f);
        if (bytes > 0)
                MD5_Update(&c, buf, bytes);
	
        MD5_Final(out, &c);
	
        for(n = 0; n < MD5_DIGEST_LENGTH; n++)
                sprintf(&md5[2*n], "%02x", out[n]);
        md5[2*MD5_DIGEST_LENGTH] = '\0';
        fd->md5start = strdup(md5);
	
        return;
}

// callback called for every element in the hash table indexed by
// md5sum hash: if there is more than one file, checks if is really
// the same file. 
static void do_hashes(G_GNUC_UNUSED gpointer key, gpointer value, G_GNUC_UNUSED gpointer user_data)
{
	GArray *arr = (GArray *)value;
	
	if (arr->len < 2)
		return; // nothing to see here, move along...
	
	for (unsigned int i = 0; i < arr->len; i++)
	{
		struct filedata *fd1 = g_array_index(arr, struct filedata *, i);
		for (unsigned int j = i+1; j < arr->len; j++) 
		{
			struct filedata *fd2 = g_array_index(arr, struct filedata *, j);
			
			if (cmp_files(fd1->name, fd2->name))
			{
				// equals
				fd2->equalto = fd1;
				break;
			}
		}
	}
	
	char outbuf[2*FILENAME_LENGTH+2];
	for (unsigned int i = 0; i < arr->len; i++)
	{
		struct filedata *fd = g_array_index(arr, struct filedata *, i);
		
		if (fd->equalto != NULL)
			printf("ln -f \"%s\" \"%s\" # %lld\t%lld\t%s\n", 
			       quote_filename(fd->equalto->name, outbuf, 2*FILENAME_LENGTH+2), 
			       quote_filename(fd->name, outbuf, 2*FILENAME_LENGTH+2), 
			       fd->inode, fd->size, fd->md5start);
	}
}

// callback called for every element in the hash table indexed by file size.
// if there is more than one element, calculates the (partial) md5sum and
// fills the hash table indexed by md5. 
static void dump_array(G_GNUC_UNUSED gpointer key, gpointer value, G_GNUC_UNUSED gpointer user_data)
{
	struct group_same_size *g = (struct group_same_size *)value;
	
	if (g->arr->len < 2)
		return;  // nothing to see here, move along...
	
	g->ht = g_hash_table_new(g_str_hash, g_str_equal);
	for (unsigned int i = 0; i < g->arr->len; i++)
	{
		struct filedata *fd = &g_array_index(g->arr, struct filedata, i);
		calc_md5(fd);
		
		if (fd->md5start != NULL) {
			GArray *arr = g_hash_table_lookup(g->ht, fd->md5start);
			if (!arr)
			{
				arr = g_array_new(FALSE, FALSE, sizeof(struct filedata *));
				g_hash_table_insert(g->ht, fd->md5start, arr);
			}
			g_array_append_val(arr, fd);
		}
	}
	
	g_hash_table_foreach(g->ht, do_hashes, NULL);
}

// how this works:
// - read file names from standard input
// - group files by size, skipping files with the same inode
// - when more than one file with the same size was found calculate the md5
//   of the first bytes, and create a hash table indexed by md5.
// - when more than one file with the same md5 if found, do a full compare
//   to check if are really the same file.
int main(void)
{
	char name[FILENAME_LENGTH];
	struct stat st;
	GHashTable *htInodes = g_hash_table_new(NULL, NULL);
	GHashTable *htSize = g_hash_table_new(NULL, NULL);
	struct filedata fd;
	
	while (fgets(name, FILENAME_LENGTH, stdin)) {
		int len = strlen(name);
		if (len == 0) continue;
		if (name[len-1] != '\n')
		{
			fprintf(stderr, "skipping too long filename: %s\n", name);
			continue;
		}
		name[len-1] = '\0';
		if (stat(name, &st) < 0)
		{
			perror("cannot stat file");
			fprintf(stderr, "file: %s\n", name);
			continue;
		}
		
		if (!S_ISREG(st.st_mode))
		{
			fprintf(stderr, "not a regular file: %s\n", name);
			continue;
		}
		
		if (st.st_size == 0)
			continue; // no zero length files
		
		if (g_hash_table_contains(htInodes, (gpointer)st.st_ino))
			continue; // this inode already exists
		
		g_hash_table_insert(htInodes, (gpointer)st.st_ino, NULL);
		
		fd.inode = (long long)st.st_ino;
		fd.size = (long long)st.st_size;
		fd.md5start = NULL;
		fd.name = strdup(name);
		fd.equalto = NULL;
		
		struct group_same_size *g = g_hash_table_lookup(htSize, (gpointer)fd.size);
		if (!g)
		{
			g = malloc(sizeof(struct group_same_size));
			g->arr = g_array_new(FALSE, FALSE, sizeof(struct filedata));
			g->ht = NULL;
			g_hash_table_insert(htSize, (gpointer)fd.size, g);
		}
		g_array_append_val(g->arr, fd);
		
		
	}
	fprintf(stderr, "DEBUG: done, do dump...\n");
	// dump
    
	g_hash_table_foreach(htSize, dump_array, NULL);
	
	return 0;
}
