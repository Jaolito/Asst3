/*
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  There are a couple of symbols that need to be #defined before
  #including all the headers.
*/

#ifndef _PARAMS_H_
#define _PARAMS_H_

// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26
#define FUSE_USE_VERSION 26

// need this to get pwrite().  I have to use setvbuf() instead of
// setlinebuf() later in consequence.
#define _XOPEN_SOURCE 500


// maintain bbfs state in here
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
	
struct sfs_state {
    FILE *logfile;
    char *diskfile;
    struct i_node * current_node;
    struct r_node * root;
};
#define SFS_DATA ((struct sfs_state *) fuse_get_context()->private_data)



struct i_node {
	int data_block;
	int i_node_num;
	char name [100];
	char type;
	char is_open;
	int file_size;
	mode_t mode;
	uint32_t ino;
	int first_child;
	time_t access, modify, change;
	int last_child;
	int big_brother;
	int parent;
	int sibling;
	int child;
	/*
 	*
 	*	Could potentially need to include time
 	*
 	*/
	
} i_node;

//Bookeeping node to track state of file system
struct r_node {
	char free_blocks [500];	
	int confirmation_number;
	uint32_t counter;
 } r_node;



struct file_descriptor {
	uint32_t i_node_id;
	unsigned int index;
	unsigned int tags;
	unsigned int open;
} file_descriptor;





#endif



