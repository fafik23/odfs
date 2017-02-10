#ifndef ODFS
#define ODFS
#include <fuse.h>
#include <lmdb.h>
#include <stdlib.h>
#include "fifo.h"
#include <time.h>

#define ODFS_DATA_BLOCK_SIZE 4080
#define ODFS_DISPLAYNAME_SIZE 128
#define MAX_ID 40

/*
typedef struct odfs_id_list {
	unsigned int number_of_ids; // 0 means empty list
	unsigned int id_list[MAX_ID];
} odfs_id_list;
*/


typedef struct odfs_id_list_node odfs_id_list_node;

/** \struct odfs_id_list_node
*   \brief nod listy przechowujacej id plikow
*   \warning moze zostac zamieniona na dwukierunkowa
*/
struct odfs_id_list_node
{
        unsigned int odfs_id;
        odfs_id_list_node *next;
};


typedef struct
{
        unsigned int number_of_ids; // 0 means empty list
        odfs_id_list_node *next;
} odfs_id_list_head;

typedef enum {
	CTX_PLUGIN_INIT = 0, 
	CTX_PLUGIN_GET, 
	CTX_PLUGIN_ADD, 
	CTX_PLUGIN_DEL,
	CTX_PLUGIN_FINI,
	CTX_PLUGIN_RETAG,
	CTX_PLUGIN_PP,
} odfs_tag_plugin_context;

typedef struct odfs_key {
	unsigned int odfs_file_id;
	unsigned int odfs_file_part;
} odfs_key;
//typedef unsigned int odfs_file_id;

typedef struct odfs_posix_data{
	struct stat odfs_posix_stat ;
	unsigned int odfs_file_id;
	char display_name[ODFS_DISPLAYNAME_SIZE];
} odfs_posix_data;

typedef struct odfs_file {
	unsigned int id;
	struct odfs_posix_data opd_cache;
	MDB_txn *write_txn;
	MDB_dbi write_dbi;
	MDB_dbi read_dbi;
	MDB_txn *read_txn;
	pthread_mutex_t read_lock;
	fifo_t write_fifo;
	off_t offset;
} odfs_file;

typedef struct odfs_wrequest {
	MDB_val key;
	MDB_val data;
	int data_free_flag;
	//pthread_cond_t flag;
	volatile int flag;
} odfs_wrequest;

typedef struct odfs_tag_plugin {
	char name[100];
	int (*get)(char *,struct odfs_id_list_node *) ;
	int (*add)(char *,struct odfs_id_list_node *) ;
	//int (*add)(unsigned int,char *) ;
	int (*del)(unsigned int,char *) ;
	int (*list_tag)(char **) ;
	int (*run)(odfs_file * file,odfs_tag_plugin_context) ;
	int (*init)() ;
	int (*fini)() ;
} odfs_tag_plugin;

typedef struct odfs_tag_stack {
	struct  odfs_tag_plugin * plugin;
	struct odfs_tag_stack * next;
} odfs_tag_stack;

/** \struct odfs_tag_query
*   \brief ...
*   \warning to nie 'konflikt w strukturze i process path'
*   \warning to okazja na wyroznajacy sie kod
*/
typedef struct odfs_tag_query {
	char * plugin_name;
	char * tag_string;
	odfs_id_list_node * list;
	odfs_id_list_node *plugin_return_list; //konflikt w strukturze i process path
	odfs_tag_plugin_context context;
} odfs_tag_query;


int odfs_run_plugin(struct odfs_tag_plugin * plugin, struct odfs_tag_query * query);
int odfs_run_plugin_stack(struct odfs_tag_stack * stack, struct odfs_tag_query * query);
int odfs_init_plugin_stack(struct odfs_tag_stack ** stack);
int odfs_add_to_plugin_stack(struct odfs_tag_stack ** stack, struct odfs_tag_plugin * plugin);

static inline struct odfs_file *get_odfs_file(struct fuse_file_info *fi)
{
	return (struct odfs_file *) (uintptr_t) fi->fh;
}

//    Initialize the filesystem. This function can often be left unimplemented, but it can be a handy way to perform one-time setup such as allocating variable-sized data structures or initializing a new filesystem. The fuse_conn_info structure gives information about what features are supported by FUSE, and can be used to request certain capabilities (see below for more information). The return value of this function is available to all file operations in the private_data field of fuse_context. It is also passed as a parameter to the destroy() method. (Note: see the warning under Other Options below, regarding relative pathnames.) 
void* odfs_init(struct fuse_conn_info *conn);
//    Called when the filesystem exits. The private_data comes from the return value of init. 
void odfs_destroy(void* private_data);
 //   Return file attributes. The "stat" structure is described in detail in the stat(2) manual page. For the given pathname, this should fill in the elements of the "stat" structure. If a field is meaningless or semi-meaningless (e.g., st_ino) then it should be set to 0 or given a "reasonable" value. This call is pretty much required for a usable filesystem. 
int odfs_getattr(const char* path, struct stat* stbuf);
  //  As getattr, but called when fgetattr(2) is invoked by the user program. 
int odfs_fgetattr(const char* path, struct stat* stbuf);
   // This is the same as the access(2) system call. It returns -ENOENT if the path doesn't exist, -EACCESS if the requested permission isn't available, or 0 for success. Note that it can be called on files, directories, or any other object that appears in the filesystem. This call is not required but is highly recommended. 
int odfs_access(const char* path, int mask);
    //If path is a symbolic link, fill buf with its target, up to size. See readlink(2) for how to handle a too-small buffer and for error codes. Not required if you don't support symbolic links. NOTE: Symbolic-link support requires only readlink and symlink. FUSE itself will take care of tracking symbolic links in paths, so your path-evaluation code doesn't need to worry about it. 
int odfs_readlink(const char* path, char* buf, size_t size);
    //Open a directory for reading. 
int odfs_opendir(const char* path, struct fuse_file_info* fi);
    //Return one or more directory entries (struct dirent) to the caller. This is one of the most complex FUSE functions. It is related to, but not identical to, the readdir(2) and getdents(2) system calls, and the readdir(3) library function. Because of its complexity, it is described separately below. Required for essentially any filesystem, since it's what makes ls and a whole bunch of other things work. 
int odfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
    //Make a special (device) file, FIFO, or socket. See mknod(2) for details. This function is rarely needed, since it's uncommon to make these objects inside special-purpose filesystems. 
int odfs_mknod(const char* path, mode_t mode, dev_t rdev);
    //Create a directory with the given name. The directory permissions are encoded in mode. See mkdir(2) for details. This function is needed for any reasonable read/write filesystem. 
int odfs_mkdir(const char* path, mode_t mode);
    //Remove (delete) the given file, symbolic link, hard link, or special node. Note that if you support hard links, unlink only deletes the data when the last hard link is removed. See unlink(2) for details. 
int odfs_unlink(const char* path);
    //Remove the given directory. This should succeed only if the directory is empty (except for "." and ".."). See rmdir(2) for details. 
int odfs_rmdir(const char* path);
    //Create a symbolic link named "from" which, when evaluated, will lead to "to". Not required if you don't support symbolic links. NOTE: Symbolic-link support requires only readlink and symlink. FUSE itself will take care of tracking symbolic links in paths, so your path-evaluation code doesn't need to worry about it. 
int odfs_symlink(const char* to, const char* from);
    //Rename the file, directory, or other object "from" to the target "to". Note that the source and target don't have to be in the same directory, so it may be necessary to move the source to an entirely new directory. See rename(2) for full details. 
int odfs_rename(const char* from, const char* to);
    //Create a hard link between "from" and "to". Hard links aren't required for a working filesystem, and many successful filesystems don't support them. If you do implement hard links, be aware that they have an effect on how unlink works. See link(2) for details. 
int odfs_link(const char* from, const char* to);
    //Change the mode (permissions) of the given object to the given new permissions. Only the permissions bits of mode should be examined. See chmod(2) for details. 
int odfs_chmod(const char* path, mode_t mode);
    //Change the given object's owner and group to the provided values. See chown(2) for details. NOTE: FUSE doesn't deal particularly well with file ownership, since it usually runs as an unprivileged user and this call is restricted to the superuser. It's often easier to pretend that all files are owned by the user who mounted the filesystem, and to skip implementing this function. 
int odfs_chown(const char* path, uid_t uid, gid_t gid);
    //Truncate or extend the given file so that it is precisely size bytes long. See truncate(2) for details. This call is required for read/write filesystems, because recreating a file will first truncate it. 
int odfs_truncate(const char* path, off_t size);
    //As truncate, but called when ftruncate(2) is called by the user program. 
int odfs_ftruncate(const char* path, off_t size);
    //Update the last access time of the given object from ts[0] and the last modification time from ts[1]. Both time specifications are given to nanosecond resolution, but your filesystem doesn't have to be that precise; see utimensat(2) for full details. Note that the time specifications are allowed to have certain special values; however, I don't know if FUSE functions have to support them. This function isn't necessary but is nice to have in a fully functional filesystem. 
int odfs_utimens(const char* path, const struct timespec ts[2] );
    //Open a file. If you aren't using file handles, this function should just check for existence and permissions and return either success or an error code. If you use file handles, you should also allocate any necessary structures and set fi->fh. In addition, fi has some other fields that an advanced filesystem might find useful; see the structure definition in fuse_common.h for very brief commentary. 
int odfs_open(const char* path, struct fuse_file_info* fi) ;
    //Read size bytes from the given file into the buffer buf, beginning offset bytes into the file. See read(2) for full details. Returns the number of bytes transferred, or 0 if offset was at or beyond the end of the file. Required for any sensible filesystem. 
int odfs_read(const char* path, char *buf, size_t size, off_t offset, struct fuse_file_info* fi);
    //As for read above, except that it can't return 0. 
int odfs_write(const char* path, const char *buf, size_t size, off_t offset, struct fuse_file_info* fi);
    //Return statistics about the filesystem. See statvfs(2) for a description of the structure contents. Usually, you can ignore the path. Not required, but handy for read/write filesystems since this is how programs like df determine the free space. 
int odfs_statfs(const char* path, struct statvfs* stbuf );
    //This is the only FUSE function that doesn't have a directly corresponding system call, although close(2) is related. Release is called when FUSE is completely done with a file; at that point, you can free up any temporarily allocated data structures. The IBM document claims that there is exactly one release per open, but I don't know if that is true. 
int odfs_release(const char* path, struct fuse_file_info *fi);
    //This is like release, except for directories. 
int odfs_releasedir(const char* path, struct fuse_file_info *fi);
    //Flush any dirty information about the file to disk. If isdatasync is nonzero, only data, not metadata, needs to be flushed. When this call returns, all file data should be on stable storage. Many filesystems leave this call unimplemented, although technically that's a Bad Thing since it risks losing data. If you store your filesystem inside a plain file on another filesystem, you can implement this by calling fsync(2) on that file, which will flush too much data (slowing performance) but achieve the desired guarantee. 
int odfs_fsync(const char* path, int isdatasync, struct fuse_file_info* fi);
    //Like fsync, but for directories. 
int odfs_fsyncdir(const char* path, int isdatasync, struct fuse_file_info* fi);
    //Called on each close so that the filesystem has a chance to report delayed errors. Important: there may be more than one flush call for each open. Note: There is no guarantee that flush will ever be called at all! 
int odfs_flush(const char* path, struct fuse_file_info* fi);
    //Perform a POSIX file-locking operation. See details below. 
int odfs_lock(const char* path, struct fuse_file_info* fi, int cmd, struct flock* locks);
    //Support the ioctl(2) system call. As such, almost everything is up to the filesystem. On a 64-bit machine, FUSE_IOCTL_COMPAT will be set for 32-bit ioctls. The size and direction of data is determined by _IOC_*() decoding of cmd. For _IOC_NONE, data will be NULL; for _IOC_WRITE data is being written by the user; for _IOC_READ it is being read, and if both are set the data is bidirectional. In all non-NULL cases, the area is _IOC_SIZE(cmd) bytes in size. 
int odfs_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi, unsigned int flags, void* data);
    //Poll for I/O readiness. If ph is non-NULL, when the filesystem is ready for I/O it should call fuse_notify_poll (possibly asynchronously) with the specified ph; this will clear all pending polls. The callee is responsible for destroying ph with fuse_pollhandle_destroy() when ph is no longer needed. 
int odfs_poll(const char* path, struct fuse_file_info* fi, struct fuse_pollhandle* ph, unsigned* reventsp);
/*    //This function is similar to bmap(9). If the filesystem is backed by a block device, it converts blockno from a file-relative block number to a device-relative block. It isn't entirely clear how the blocksize parameter is intended to be used. 
int setxattr(const char* path, const char* name, const char* value, size_t size, int flags)
    //Set an extended attribute. See setxattr(2). This should be implemented only if HAVE_SETXATTR is true. 
int getxattr(const char* path, const char* name, char* value, size_t size)
    //Read an extended attribute. See getxattr(2). This should be implemented only if HAVE_SETXATTR is true. 
int listxattr(const char* path, const char* list, size_t size)
    //List the names of all extended attributes. See listxattr(2). This should be implemented only if HAVE_SETXATTR is true. 
*/

#endif
