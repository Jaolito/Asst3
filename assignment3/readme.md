# Simple File System


* Andrew Macneille - apm145
* Justin Wu - jw879
* Timothy Katzgrau - tjk155

##### Tested on : interpreter.cs.rutgers.edu


## Testing
This program works for all required functionality. touch, echo, cat, rm, cd, and ls. A problem we faced is that ls has random behavior when run on a directory with more than 2 members. We could not pinpoint the issue, but ls works perfectly fine with 1 or 2 members. However, more members will be read with incorrect mode values that we did not assign. The structure of the file system remains intact and ls will output all the files, but it disconnects and breaks the mounted directory. 

## Design
For this project we have chosen to implement the extended directory operations for the functions <br>
int mkdir(char* path) <br>
int rmdir(char* path)

Our design features a root node which keeps track of the current state of the system, such as remembering which blocks are free and which are used.


```c
struct r_node {
    char free_blocks [500];
    int confirmation_number;
} r_node;
```

We have designed our system to use 500 blocks, each of 512 bytes.  Each iNode is directly mapped to blocks in the diskfile based on whether it is an iNode for a file or directory.  If the iNode is for a file, it will point to blocks on the disk designated for files. A directory iNode, however, will point to both file blocks and other directories (other iNodes).  This essentially means that we can have at most 500 iNodes.

Inside the iNode you will find the data block it points to, an iNode id, name, type (Directory or File), a boolean is_open, file size and access mode (Read, Write), and a timestamp for when the node was last accessed, modified and changed. In addition to these, we also keep track of the relationships between iNodes.

```c
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
} i_node;
```

We store these relationships in the variables  first child, last child, big brother, parent, sibling and child. These variables are all of type int and reference another iNodes id.  These relationships come in handy for when we create, read and delete directories and files because a file system is a hierarchy of data.  In order to keep it organized, we must know the relationships between all iNodes.

Since our system uses 500 blocks and each block is 512 bytes, 500 * 512 = 256,000 bytes = 256 kb. This is the maximum storage capacity of our file system.

## Helper Functions

### int find_free_i_node(char type)
This function finds open blocks within the system to use for the creation of a new directory or datafile.

### int find_i_node(struct i_node * curr ,char * path, char type)
This function finds the block number of the given path or the parent on a create or mkdir call.

