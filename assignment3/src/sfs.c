/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "params.h"
#include "block.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"

int count = 0;
///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//


///File Helper functions//////


int find_free_i_node(char type){

	struct r_node * root = (struct r_node *)malloc(sizeof(struct r_node));
	block_read(0, root);
	int i, counter;

	//Only requesting one i_node
	if (type == 0){
		for(i=2; i<500; i++){
			if(root-> free_blocks[i] == 0){
				root-> free_blocks[i] = 1;
				block_write(0, root);
				return i;
			}
		}
		return -1;

	}else if (type == 1){
		i = 2;
		counter = 0;
		while(counter<2 && i<500){
			if(root-> free_blocks[i] == 0){
				counter++;	
			}
			i++;
		}
		if (i<500){
			i--;
			root-> free_blocks[i] = 1;
			block_write(0, root);
			return i;
		}else{
			return -1;
		}
	}	

}

int find_i_node(struct i_node * curr ,char * path, char type){
	

	
	if(path[strlen(path)-1] == '/' && strlen(path) != 1){
		path[strlen(path)-1] = '\0';
	}
	if(path[0] != '/'){
		errno = ENOENT;
		return -1;	
	}
	if(strlen(path) == 1){
		return 1;
	}
	
	int i;
	char * str;
	char * temp = (char *)malloc(strlen(path)+1);
	struct i_node* buf = (struct i_node *)malloc(sizeof(struct i_node));
	int block_num;
	strcpy(temp, path);
	temp++;
	str = strchr(temp, '/');
	//free(temp);
	if(curr->first_child != -1){
		block_num = curr-> first_child;
		while(1){
			block_read(block_num, buf);
			if(strncmp(temp,buf->name, strlen(buf->name))== 0){
				if(strlen(temp)>strlen(buf->name)) {
						if(temp[strlen(buf->name)] == '/'){
							break;
						}
				}else{
					break;
				}	
			
			}
				block_num = buf->sibling;	
				if(block_num == -1){
					if (type == 0 && str == NULL){
						return curr-> i_node_num;
					}	
					//Incorrect path
		errno = ENOENT;
					return -1;
				}
			
		}
	}else{
		if (type == 0 && str == NULL && curr->type != 0){
			return curr-> i_node_num;
		}
		errno = ENOENT;
			return -1;
	}
	if(str == NULL){
		//find child that maches temp;
		//free(buf);
		if (type == 0){
		errno = ENOENT;
			return -1;
		}
		return block_num;
	}else{
		int rtn = find_i_node(buf, str, type);
	//	free(buf);
		return rtn;
	}
		
	



}



/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */


/*
 *
 *Things to figure out
 *  - Can we restrict the total number of i_nodes
 *  - Is create() the same as makeDir()
 *  - What stats do we need to return in getAttr(), what attributes from the stats struct
 *
 */

void *sfs_init(struct fuse_conn_info *conn)
{
  
    	fprintf(stderr, "in bb-init\n");
    	log_msg("\nsfs_init()\n");
        
	conn-> max_write = BLOCK_SIZE;		
	conn-> async_read = 0;
	conn-> want = FUSE_CAP_EXPORT_SUPPORT;
	//Open the diskfile when main is called	
	disk_open((SFS_DATA) -> diskfile);
	struct r_node * root = (struct r_node*)malloc(BLOCK_SIZE);

	block_read(0,root);
	
	if (!(root->confirmation_number == 0x123CDF)){
		//The file system has not been initialized yet
		int i;
		root->confirmation_number = 0x123CDF;
		root->free_blocks[0] = 1;
		for(i = 1; i<500; i++){
			root->free_blocks[i] = 0;
		}
		struct i_node * i_root = (struct i_node *)malloc(sizeof(struct i_node));
		strcpy(i_root-> name, "/");
		i_root-> i_node_num = 1;
		i_root-> sibling = -1;
		i_root-> child = -1;
		i_root-> first_child = -1;
		i_root-> last_child = -1;
		i_root-> big_brother = -1;
		i_root-> parent = -1;
		i_root-> type = 1;
		i_root-> access = time(NULL);
		i_root-> modify = i_root-> access;
		i_root-> change = i_root-> access;
		i_root-> mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
		i_root-> file_size = sizeof(struct i_node);
		root-> free_blocks[1] = 1;
		root-> counter = 0;
		block_write(1,i_root); 
	}
	
	block_write(0, root);		
	log_conn(conn);
	log_fuse_context(fuse_get_context());
		
	

    	return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata)
{
    log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
    	int retstat = 0;
    	char fpath[PATH_MAX];
    
    	log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",path, statbuf);
        
	char * new_path = (char *)malloc(sizeof(strlen(path)+1));	
	strcpy(new_path, path);	
	//Get i_node from path
	struct i_node * attr_buf = (struct i_node *)malloc(512);
	struct i_node * root_node = (struct i_node *)malloc(512);
	block_read(1,root_node);
	unsigned int block_num = find_i_node(root_node, new_path, 1);
	if(block_num != -1){
	log_msg("\n Block num: %d\n", block_num);
		block_read(block_num, attr_buf);
		statbuf->st_size = attr_buf->file_size;	
		if (attr_buf-> type == 1){
			statbuf->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
		}else{
			statbuf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
		}
		statbuf->st_ino = block_num;
		statbuf->st_nlink = 1;
		statbuf->st_gid = 0;		
		statbuf->st_uid = 0;
		statbuf->st_blocks = 1;
		statbuf->st_atime = attr_buf-> access;
		statbuf->st_ctime = attr_buf-> change;
		statbuf->st_mtime = attr_buf-> modify;
		statbuf->st_blksize = BLOCK_SIZE;
		
	}else{
		return -errno;
	}
	
	//free(new_path);
	//free(attr_buf);
	//free(root_node);

	log_msg("About to return from get attr\n");
    	return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    	int retstat = 0;
    	log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n", path, mode, fi);
    
   	char * new_path = (char *)malloc(sizeof(strlen(path)+1));
    	struct i_node * root_node = (struct i_node *)malloc(512);
	struct i_node * dir_node = (struct i_node *)malloc(512);
	struct i_node * child_node = (struct i_node *)malloc(512);
	struct i_node * new_node = (struct i_node *)malloc(512);
	struct r_node * root = (struct r_node *)malloc(512);

	block_read(0, root);

	char * name;
	strcpy(new_path, path);
	name = strrchr(new_path, '/');
	name++;
	if (name[strlen(name) -1] == '/'){
		name[strlen(name) -1] = '\0';	
	}


	block_read(1, root_node);
	int dir_block_num = find_i_node(root_node, new_path, 0);
	if (dir_block_num != -1){
		block_read(dir_block_num, dir_node);
		int open_index1;
		int open_index2;
		open_index1 = find_free_i_node(1);
		if (open_index1 == -1){
			//There are not two open i_node blocks
	//		free(root_node);
	//		free(dir_node);
	//		free(new_path);
	//		free(child_node);
	//		free(new_node);
			return -1;
		}else{
			open_index2 = find_free_i_node(0);
		}

		if (dir_node-> last_child != -1){
			block_read(dir_node-> last_child, child_node);
			child_node-> sibling = open_index1;
			new_node-> big_brother = child_node->i_node_num; 
			dir_node-> last_child = open_index1;
			block_write(child_node-> i_node_num, child_node);
		}else{
			dir_node-> first_child = open_index1;
			dir_node-> last_child = open_index1;
			new_node-> big_brother = -1;
		}

		root-> counter++;
		new_node-> ino = root-> counter;
		new_node-> parent = dir_node->i_node_num;
		new_node-> data_block = open_index2;
		new_node-> i_node_num = open_index1;
		new_node-> sibling = -1;
		new_node-> child = -1;
		new_node-> access = time(NULL);
		new_node-> modify = new_node-> access;
		new_node-> change = new_node-> access;
		new_node-> first_child = -1;
		new_node-> last_child = -1;
		new_node-> type = 0;
		new_node-> is_open = 0;
		new_node-> mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
		strcpy(new_node-> name, name);
		//fi-> fh = open_index1;

		block_write(dir_block_num, dir_node);
		block_write(open_index1, new_node);

	}else{
	//	free(root_node);
	//	free(dir_node);
	//	free(child_node);
	//	free(new_node);
	//	free(new_path);
		return -1;
	}
		
	
	
    
	//free(root_node);
	//free(new_path);
	//free(dir_node);
	//free(child_node);
	//free(new_node);
	sfs_open(path,fi);    
    	return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path){

	int retstat = 0;
	log_msg("sfs_unlink(path=\"%s\")\n", path);
	struct i_node * rem_node = (struct i_node *)malloc(512);
	struct i_node * root_node = (struct i_node *)malloc(512);
	struct i_node * parent_node = (struct i_node *)malloc(512);
	struct i_node * bb_node = (struct i_node *)malloc(512);
	struct i_node * lb_node = (struct i_node *)malloc(512);
	struct r_node * root = (struct r_node *)malloc(512);
	char * new_path = (char *)malloc(strlen(path)+1);
	strcpy(new_path, path);	

	block_read(1, root_node);
	int i_node_index = find_i_node(root_node, new_path,1);
	block_read(0, root);
	block_read(i_node_index, rem_node);
	block_read(rem_node->parent, parent_node);	
	int data_index = rem_node-> data_block;

	root->free_blocks[data_index] = 0;
	root->free_blocks[i_node_index] = 0;
	rem_node->data_block = -1;

	//Shuffle parent and siblings links
	if(rem_node-> big_brother != -1 && rem_node-> sibling != -1){
		block_read(rem_node-> big_brother, bb_node);	
		block_read(rem_node-> sibling, lb_node);
		bb_node-> sibling = rem_node-> sibling;
		lb_node-> big_brother = rem_node-> big_brother;
		block_write(bb_node-> i_node_num, bb_node);
		block_write(lb_node-> i_node_num, lb_node);
	}else if(parent_node-> first_child == i_node_index && rem_node-> sibling != -1){
		block_read(rem_node->sibling, lb_node);
		parent_node-> first_child = rem_node-> sibling;
		lb_node-> big_brother = -1;
log_msg("In test case\n");
		block_write(parent_node-> i_node_num, parent_node);
		block_write(lb_node-> i_node_num, lb_node);
	}else if(parent_node-> last_child == i_node_index && rem_node-> big_brother != -1){
		block_read(rem_node->big_brother, bb_node);
		parent_node-> last_child = bb_node-> i_node_num;
		bb_node-> sibling = -1;
		block_write(parent_node-> i_node_num, parent_node);
		block_write(bb_node-> i_node_num, bb_node);
	}else if(rem_node-> sibling == -1 && rem_node-> big_brother == -1){
		parent_node-> last_child = -1;
		parent_node-> first_child = -1;
		block_write(rem_node-> parent, parent_node);
	}else{
		//we are beat
	}



	block_write(i_node_index, rem_node);
	block_write(0, root);
	//free(rem_node);
	//free(bb_node);
	//free(lb_node);
	//free(root);
	//free(parent_node);
	//free(root_node);
	//free(new_path);


	return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi){
	int retstat = 0;
    	log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n", path, fi);

	struct i_node * open_node = (struct i_node *)malloc(512);
	struct i_node * root_node = (struct i_node *)malloc(512);
	char * new_path = (char *)malloc(strlen(path)+1);
	strcpy(new_path, path);
	block_read(1,root_node);
	const int index = find_i_node(root_node, new_path, 1);
	log_msg("open index: %d\n", index);
	if (index != -1){
		block_read(index, open_node);
	}else{
		//sfs_create(path,O_CREAT,fi);
		return -errno;
	}

	
	open_node-> is_open = 1;
	block_write(index,open_node);
    	//free(open_node);
	//free(root_node);
	//free(new_path);
	log_msg("\nindex %d\n", index);
    	return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi){
    	int retstat = 0;
    	log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",path, fi);
    
/*	struct i_node * open_node = (struct i_node *)malloc(sizeof(struct i_node));
	struct i_node * root_node = (struct i_node *)malloc(sizeof(struct i_node));
	char * new_path = (char *)malloc(strlen(path)+1);
	strcpy(new_path, path);
	block_read(1,root_node);
	const int index = find_i_node(root_node, new_path, 1);
	if (index != -1){
		block_read(index, open_node);
	}else{
		return -errno;
	}
	open_node-> is_open = 0;
	block_write(index,open_node);
    	//free(open_node);
	//free(root_node);
	//free(new_path);
*/
    	return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    	int retstat = 0;
    	log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);

	int new_size;
	if(size>512){
		new_size = 512;
	}else{
		new_size = size;
	}

	struct i_node * open_node = (struct i_node *)malloc(512);
	struct i_node * root_node = (struct i_node *)malloc(512);
	char * new_path = (char *)malloc(strlen(path)+1);
	char * new_buf = (char *)malloc(512);
	strcpy(new_path, path);
	block_read(1,root_node);
	int index = find_i_node(root_node, new_path, 1);
	if (index != -1){
		block_read(index, open_node);
	}else{
		return 0;
	}
    	if ((int)offset > 511){
		return 0;
	}
	if (open_node-> is_open == 0){
		return 0;
	}

	int dif;
	if((int)offset + (int)new_size > 511){
	log_msg("size_t: %zu and size_int %d\n", size, (int)size);
		dif = 512 - offset;
	}else{
		dif = new_size;
	}
	log_msg("Dif variable: %d\n", dif);
	block_read(open_node->data_block, new_buf);
	memcpy(buf, &new_buf[offset], dif);


	open_node-> access = time(NULL);
	block_write(index, open_node);
//	free(open_node);
//	free(root_node);
//	free(new_path);
	

    	return dif;
   
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    	int retstat = 0;
    	log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n", path, buf, size, offset, fi);
    
	
	struct i_node * open_node = (struct i_node *)malloc(512);
	struct i_node * root_node = (struct i_node *)malloc(512);
	char * new_path = (char *)malloc(strlen(path)+1);
	char new_buf [512];
	strcpy(new_path, path);
	block_read(1,root_node);
	int index = find_i_node(root_node, new_path, 1);
	if (index != -1){
		block_read(index, open_node);
	}else{
		return 0;
	}
    	if ((int)offset > 511){
		return 0;
	}
	if (open_node-> is_open == 0){
		return 0;
	}

	int dif;
	if((int)offset + (int)size > 511){
		dif = 512 - offset;
	}else{
		dif = size;
	}
	block_read(open_node->data_block, new_buf);
	memcpy(&new_buf[offset], buf, dif);
	open_node-> access = time(NULL);
	open_node-> modify = open_node-> access;
	open_node-> change = open_node-> access;

	block_write(open_node->data_block, new_buf);
//	free(open_node);
//	free(root_node);
//	free(new_path);
	

    	return dif;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
   
   	char * new_path = (char *)malloc(sizeof(strlen(path)+1));
    	struct i_node * root_node = (struct i_node *)malloc(512);
	struct i_node * dir_node = (struct i_node *)malloc(512);
	struct i_node * child_node = (struct i_node *)malloc(512);
	struct i_node * new_node = (struct i_node *)malloc(512);
	struct r_node * root = (struct r_node *)malloc(512);

	block_read(0, root);

	char * name;
	strcpy(new_path, path);
	name = strrchr(new_path, '/');
	name++;
	if (name[strlen(name) -1] == '/'){
		name[strlen(name) -1] = '\0';	
	}


	block_read(1, root_node);
	int dir_block_num = find_i_node(root_node, new_path, 0);
	if (dir_block_num != -1){
		block_read(dir_block_num, dir_node);
		int open_index1;
		open_index1 = find_free_i_node(0);
		if (open_index1 == -1){
			//There are not two open i_node blocks
	//		free(root_node);
	//		free(dir_node);
	//		free(new_path);
	//		free(child_node);
	//		free(new_node);
			return -1;
		}

		if (dir_node-> last_child != -1){
			block_read(dir_node-> last_child, child_node);
			child_node-> sibling = open_index1;
			new_node-> big_brother = child_node->i_node_num; 
			dir_node-> last_child = open_index1;
			block_write(child_node-> i_node_num, child_node);
		}else{
			dir_node-> first_child = open_index1;
			dir_node-> last_child = open_index1;
			new_node-> big_brother = -1;
		}

		root-> counter++;
		new_node-> ino = root-> counter;
		new_node-> parent = dir_node->i_node_num;
		new_node-> data_block = -1;
		new_node-> i_node_num = open_index1;
		new_node-> sibling = -1;
		new_node-> child = -1;
		new_node-> access = time(NULL);
		new_node-> modify = new_node-> access;
		new_node-> change = new_node-> access;
		new_node-> first_child = -1;
		new_node-> last_child = -1;
		new_node-> type = 1;
		new_node-> is_open = 0;
		new_node-> mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
		strcpy(new_node-> name, name);
		//fi-> fh = open_index1;

		block_write(dir_block_num, dir_node);
		block_write(open_index1, new_node);

	}else{
	//	free(root_node);
	//	free(dir_node);
	//	free(child_node);
	//	free(new_node);
	//	free(new_path);
		return -1;
	}
		
	
	
    
	//free(root_node);
	//free(new_path);
	//free(dir_node);
	//free(child_node);
	//free(new_node);
    

    
    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
    int retstat = 0;
    log_msg("sfs_rmdir(path=\"%s\")\n",
	    path);
	struct i_node * rem_node = (struct i_node *)malloc(512);
	struct i_node * root_node = (struct i_node *)malloc(512);
	struct i_node * parent_node = (struct i_node *)malloc(512);
	struct i_node * bb_node = (struct i_node *)malloc(512);
	struct i_node * lb_node = (struct i_node *)malloc(512);
	struct r_node * root = (struct r_node *)malloc(512);
	char * new_path = (char *)malloc(strlen(path)+1);
	strcpy(new_path, path);	

	block_read(1, root_node);
	int i_node_index = find_i_node(root_node, new_path,1);
	block_read(0, root);
	block_read(i_node_index, rem_node);
	block_read(rem_node->parent, parent_node);	
	if (rem_node-> first_child != -1){
		errno = ENOTEMPTY;
		return -1;
	}
	root->free_blocks[i_node_index] = 0;
	rem_node->data_block = -1;
	

	//Shuffle parent and siblings links
	if(rem_node-> big_brother != -1 && rem_node-> sibling != -1){
		block_read(rem_node-> big_brother, bb_node);	
		block_read(rem_node-> sibling, lb_node);
		bb_node-> sibling = rem_node-> sibling;
		lb_node-> big_brother = rem_node-> big_brother;
		block_write(bb_node-> i_node_num, bb_node);
		block_write(lb_node-> i_node_num, lb_node);
	}else if(parent_node-> first_child == i_node_index && rem_node-> sibling != -1){
		block_read(rem_node->sibling, lb_node);
		parent_node-> first_child = rem_node-> sibling;
		lb_node-> big_brother = -1;
log_msg("In test case\n");
		block_write(parent_node-> i_node_num, parent_node);
		block_write(lb_node-> i_node_num, lb_node);
	}else if(parent_node-> last_child == i_node_index && rem_node-> big_brother != -1){
		block_read(rem_node->big_brother, bb_node);
		parent_node-> last_child = bb_node-> i_node_num;
		bb_node-> sibling = -1;
		block_write(parent_node-> i_node_num, parent_node);
		block_write(bb_node-> i_node_num, bb_node);
	}else if(rem_node-> sibling == -1 && rem_node-> big_brother == -1){
		parent_node-> last_child = -1;
		parent_node-> first_child = -1;
		block_write(rem_node-> parent, parent_node);
	}else{
		//we are beat
	}



	block_write(i_node_index, rem_node);
	block_write(0, root);
	//free(rem_node);
	//free(bb_node);
	//free(lb_node);
	//free(root);
	//free(parent_node);
	//free(root_node);
	//free(new_path);
    
    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){

    	int retstat = 0;
	
    
    log_msg("sfs_readdir(path=\"%s\")\n",
	    path);
	    //Do we need to return the full path?
    	char * new_path = (char *)malloc(sizeof(strlen(path)));
	struct i_node * root_node = (struct i_node *)malloc(512);
	struct i_node * read_node = (struct i_node *)malloc(512);
	struct i_node * child_node = (struct i_node *)malloc(512);
	struct stat * stat_buf;
	block_read(1, root_node);
	strcpy(new_path, path);

	int block_num = find_i_node(root_node, new_path, 1);
	block_read(block_num, read_node);
	
	int child_block = read_node-> first_child;
	if (child_block == -1){
		//No files in directory
	}else{
		block_read(child_block, child_node);
		while(1){
			stat_buf = (struct stat *)malloc(sizeof(struct stat));
			stat_buf-> st_ino = child_node-> ino;
			stat_buf-> st_mode = child_node-> mode;
			if(filler(buf, child_node->name, stat_buf, 0) == 1){
		//		free(child_node);
    				log_msg("ERROR\n");
		//		free(root_node);
		//		free(read_node);
				return -ENOMEM;
			}else{
				child_block = child_node->sibling;
				if(child_block != -1){
					block_read(child_block, child_node);
				}else{
					break;
				}
				
			}
			
		}
	}
	//free(child_node);
	//free(root_node);
	//free(read_node);
	return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi){
    int retstat = 0;

    
    return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage()
{
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{

    int fuse_stat;
    struct sfs_state *sfs_data;
    
    // sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
	perror("main calloc");
	abort();
    }

    // Pull the diskfile and save it in internal data
    	sfs_data->diskfile = argv[argc-2];
    	argv[argc-2] = argv[argc-1];
    	argv[argc-1] = NULL;
    	argc--;
    
    	sfs_data->logfile = log_open();
	


    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
