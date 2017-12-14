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

Next, we have x inodes. Each inode is directly mapped to blocks in the diskfile. Inside the iNode you will find the data block it points to, an iNode id, name, type, a boolean is_open, file size and mode. In addition to these, we also keep track of the relationships between iNodes.

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
