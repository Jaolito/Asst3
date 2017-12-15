# Simple File System


* Andrew Macneille - apm145
* Justin Wu - jw879
* Timothy Katzgrau - tjk155

##### Tested on : interpreter.cs.rutgers.edu




## Testing


## Design

Our design features a root node which keeps track of the current state of the system, such as remembering which blocks are free and which are used.


```c
struct r_node {
    char free_blocks [500];
    int confirmation_number;
} r_node;
```

We have designed our system to use 500 blocks, each of 512 bytes.  Each iNode is directly mapped to blocks in the diskfile based on whether it is an iNode for a file or directory.  If the iNode is for a file, it will point to blocks on the disk designated for files. A directory iNode, however, will point to both file blocks and other directories (other iNodes).  This essentially means that we can have at most 500 iNodes.

Inside the iNode you will find the data block it points to, an iNode id, name, type (Directory or File), a boolean is_open, file size and access mode (Read, Write). In addition to these, we also keep track of the relationships between iNodes.

```c
    struct i_node {
    int data_block;
    int i_node_num;
    char name [100];
    char type;
    char is_open;
    int file_size;
    mode_t mode;
    int first_child;
    int last_child;
    int big_brother;
    int parent;
    int sibling;
    int child;
} i_node;
```

We store these in the variables  first child, last child, big brother, parent, sibling and child. These variables are all of type int and reference another iNodes id.  These relationships come in handy for when we  create, read and delete directories and files.

Since our system uses 500 blocks and each block is 512 bytes, 500 * 512 = 256,000 bytes = 256 kb. This is the maximum storage capacity of our file system.
