/*! \file odfs.c
    \brief main odfs code
*/
/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/
#define FUSE_USE_VERSION 26

#define DEBUG 1

#include <fuse.h>
#include "odfs.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>

#include "fifo.h"
#include "process_path.h"

//static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/_new_";//TODO nepath

MDB_env *env_odfs_data;
MDB_env *env_odfs_posix;
MDB_dbi dbi_odfs_data,dbi_odfs_posix;
MDB_dbi sched_dbi;

pthread_t sched;
pthread_cond_t sched_cond;
struct fifo_t * sched_fifo;

struct odfs_tag_plugin * name_plugin;
struct odfs_tag_stack * tp_stack;

unsigned int nf_id;//TODO Zrobic to dobrze
int nf_flag;//TODO Zrobic to dobrze

int _get_newfile_id(const char * path,odfs_id_list_node *list) {
	if(nf_flag) {
		 add_id_to_list(nf_id, list);
	}
	return 0;
}

int _add_newfile_id(const char * path,odfs_id_list_node *list) {
	nf_id=list->odfs_id;
	nf_flag=1;
	return 0;
}

int _rm_newfile_id(const char * path,odfs_id_list_node *list) {
	nf_flag=0;
	return 0;
}

int odfs_run_plugin(struct odfs_tag_plugin * plugin, struct odfs_tag_query * query) {
	switch(query->context){
		case CTX_PLUGIN_INIT: return plugin->init ? plugin->init() : -1;
		case CTX_PLUGIN_GET: return plugin->init ? plugin->init() : -1;
		case CTX_PLUGIN_ADD: return plugin->add ? plugin->add(query->tag_string, query->list) : -1;
		case CTX_PLUGIN_DEL: return plugin->init ? plugin->init() : -1;
		case CTX_PLUGIN_FINI: return plugin->init ? plugin->init() : -1;
		case CTX_PLUGIN_RETAG: return plugin->init ? plugin->init() : -1;
		case CTX_PLUGIN_PP: return plugin->get ? plugin->get(query->tag_string, query->plugin_return_list) : -1;
		default: return -1;
	}
}

int odfs_run_plugin_stack(struct odfs_tag_stack * stack, struct odfs_tag_query * query) {
	odfs_tag_stack * current = stack;
	
	printf ("odfs_run_plugin_stack BEGIN\n");
	//while (current != NULL) { TODO Zrobic defoltowy plugin w odfs_init_plugin_stack
	while (current->plugin != NULL) {
		printf ("odfs_run_plugin_stack in while\n");
		if(query->tag_string!=NULL && query->plugin_name!=NULL && strcmp(query->plugin_name,current->plugin->name)==0) {
			printf ("odfs_run_plugin_stack current->plugin->name=%s ,context=%d\n",current->plugin->name, query->context);
			odfs_run_plugin(current->plugin, query);
		} else if(query->tag_string!=NULL && query->plugin_name==NULL && strcmp("name",current->plugin->name)==0) {
			printf ("odfs_run_plugin_stack current->plugin->name=name ,context=%d\n", query->context);
			odfs_run_plugin(current->plugin, query);
		} else {
			printf ("odfs_run_plugin_stack skip current->plugin->name=%s ,context=%d\n",current->plugin->name, query->context);
		}
	    	current = current->next;
	}
	printf ("odfs_run_plugin_stack END\n");
	return 0;
}

int odfs_init_plugin_stack(struct odfs_tag_stack ** stack) {

	struct  odfs_tag_stack * new =  malloc(sizeof(struct odfs_tag_stack));

	new->next=NULL;
	new->plugin=NULL;
	*stack=new;
	
	return 0;
}

int odfs_add_to_plugin_stack(struct odfs_tag_stack ** stack, struct odfs_tag_plugin * plugin) {

	struct  odfs_tag_stack * new =  malloc(sizeof(struct odfs_tag_stack));

	new->next=*stack;
	new->plugin=plugin;
	*stack=new;
	
	return 0;
}

int _odfs_truncate(unsigned int id, off_t offset) {
	int res = 0;
	int rc;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn *txn;
	struct odfs_posix_data  opd;

	key.mv_size = sizeof(unsigned int);
	key.mv_data = &id;
	rc = mdb_txn_begin(env_odfs_posix, NULL, 0, &txn);
	rc = mdb_open(txn, NULL, 0, &dbi);
	rc = mdb_get(txn, dbi, &key, &data);

	if(rc==MDB_NOTFOUND){
		printf("odfs_truncate rc==MDB_NOTFOUND \n");
		res = -ENOENT;
	} else {
		memcpy(&opd,data.mv_data,sizeof(struct odfs_posix_data));
		opd.odfs_posix_stat.st_size=offset;
	}
	data.mv_size=sizeof(struct odfs_posix_data);
	data.mv_data=&opd;
	rc = mdb_put(txn, dbi, &key, &data, 0);
	mdb_txn_commit(txn);
	return res;
}

int _odfs_load_plugin(char * plugin_path, struct  odfs_tag_plugin ** plugin) {
	
	*plugin = malloc(sizeof(struct  odfs_tag_plugin));
	void *handle;

	handle = dlopen(plugin_path, RTLD_LAZY);

	if (!handle) {
		fprintf(stderr, "_odfs_load_plugin dlopen(%s) %s\n", plugin_path, dlerror());
		return 2;
	}
	
	(*plugin)->get = dlsym(handle, "get");
	if ((*plugin)->get == NULL ) fprintf(stderr, "_odfs_load_plugin get == NULL\n");
	(*plugin)->add = dlsym(handle, "add");
	if ((*plugin)->add == NULL ) fprintf(stderr, "_odfs_load_plugin add == NULL\n");
	(*plugin)->del = dlsym(handle, "del");
	if ((*plugin)->del == NULL ) fprintf(stderr, "_odfs_load_plugin del == NULL\n");
	(*plugin)->run = dlsym(handle, "run");
	if ((*plugin)->run == NULL ) fprintf(stderr, "_odfs_load_plugin run == NULL\n");
	

	strcpy((*plugin)->name,(const char *)dlsym(handle, "plugin_name"));
	fprintf(stderr, "_odfs_load_plugin loaded %s as %s\n", plugin_path, (*plugin)->name);
	
	(*plugin)->init = dlsym(handle, "init");
	(*plugin)->fini = dlsym(handle, "fini");
	if ( dlerror() != NULL)   return 1;

	return 0;
}

int odfs_statfs(const char* path, struct statvfs* stbuf ){

	MDB_stat mst;
	MDB_envinfo mei;
	int rc;
//struct statvfs {
//               unsigned long  f_bsize;    /* file system block size */
//               unsigned long  f_frsize;   /* fragment size */
//               fsblkcnt_t     f_blocks;   /* size of fs in f_frsize units */
//               fsblkcnt_t     f_bfree;    /* # free blocks */
//               fsblkcnt_t     f_bavail;   /* # free blocks for unprivileged users */
//               fsfilcnt_t     f_files;    /* # inodes */
//               fsfilcnt_t     f_ffree;    /* # free inodes */
//               fsfilcnt_t     f_favail;   /* # free inodes for unprivileged users */
//               unsigned long  f_fsid;     /* file system ID */
//               unsigned long  f_flag;     /* mount flags */
//               unsigned long  f_namemax;  /* maximum filename length */
//           };
	rc = mdb_env_stat(env_odfs_posix, &mst);
	rc = mdb_env_info(env_odfs_posix, &mei);
	printf("Environment Info\n");
	printf("  Map address: %p\n", mei.me_mapaddr);
	printf("  Map size: %zu\n", mei.me_mapsize);
	printf("  Page size: %u\n", mst.ms_psize);
	printf("  Max pages: %zu\n", mei.me_mapsize / mst.ms_psize);
	printf("  Number of pages used: %zu\n", mei.me_last_pgno+1);
	printf("  Last transaction ID: %zu\n", mei.me_last_txnid);
	printf("  Max readers: %u\n", mei.me_maxreaders);
	printf("  Number of readers used: %u\n", mei.me_numreaders);
	printf("  Page size: %u\n", mst.ms_psize);
	printf("  Tree depth: %u\n", mst.ms_depth);
	printf("  Branch pages: %zu\n", mst.ms_branch_pages);
	printf("  Leaf pages: %zu\n", mst.ms_leaf_pages);
	printf("  Overflow pages: %zu\n", mst.ms_overflow_pages);
	printf("  Entries: %zu\n", mst.ms_entries);

	stbuf->f_bsize=ODFS_DATA_BLOCK_SIZE;
	stbuf->f_bfree=mst.ms_leaf_pages;

	return rc;
}

int odfs_truncate(const char *path, off_t offset) {
	int res = 0;
//TODO usuwanie starych blokow
	odfs_id_list_node * list;
	init_id_list(&list);

	printf("odfs_truncate of %s  to %jd \n",path,offset);
	process_path(path, list, tp_stack);
	int count=count_odfs_id_list(list);
	if (count==1) {
		res=_odfs_truncate((list->odfs_id),offset);
	} else {
		printf("odfs_truncate list.number_of_ids!=1 \n");
		res = -ENOENT;
	}
	return res;
}

int odfs_unlink(const char* path){
	int res = 0;
	int rc;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn *txn;
	struct odfs_posix_data  opd;
	odfs_id_list_node * list;

	init_id_list(&list);

	process_path(path, list, tp_stack);

	int count=count_odfs_id_list(list);
	if (count==1) {
		key.mv_size = sizeof(unsigned int);
		key.mv_data = &(list->odfs_id);
		printf("odfs_unlink of %s  id %u \n",path,list->odfs_id);
		_odfs_truncate(list->odfs_id, 0);
		rc = mdb_txn_begin(env_odfs_posix, NULL, 0, &txn);
		rc = mdb_open(txn, NULL, 0, &dbi);
		rc = mdb_del(txn, dbi, &key, &data);
	
		if(rc != 0){
			printf("odfs_unlink mdb_del retuned rc!=0 \n");
			res = -ENOENT;
		} 
		data.mv_size=sizeof(struct odfs_posix_data);
		data.mv_data=&opd;
		mdb_txn_commit(txn);
	} else {
		printf("odfs_unlink list.number_of_ids!=1 \n");
		res = -ENOENT;
	}
	return res;
}

int odfs_getattr(const char *path, struct stat *stbuf) {
	int res = 0;

	int rc;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn *txn;

	printf("odfs_getattr of %s \n",path);

	memset(stbuf, 0, sizeof(struct stat));

	odfs_id_list_node * list;
	init_id_list(&list);
	display_odfs_id_list(list);
	process_path(path, list, tp_stack);
	int count=count_odfs_id_list(list);
	printf("odfs_getattr count=%d \n",count);
	if (strcmp(path, "/") == 0 ||strcmp(path, hello_path) == 0) {
		printf("odfs_getattr of X \n");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		stbuf->st_uid = 1000;
		stbuf->st_gid = 1000;
	} else if (count>1) {
		printf("odfs_getattr of XX \n");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 1+5;//TODO
		stbuf->st_uid = 1000;
		stbuf->st_gid = 1000;
	} else if (count==1) {
		printf("odfs_getattr of XXX \n");
		rc = mdb_txn_begin(env_odfs_posix, NULL, MDB_RDONLY, &txn);
		rc = mdb_open(txn, NULL, 0, &dbi);
		key.mv_size = sizeof(unsigned int);
		key.mv_data = &(list->odfs_id);
		rc = mdb_get(txn, dbi, &key, &data);

		if(rc==MDB_NOTFOUND){
			res = -ENOENT;
		} else {
			memcpy(stbuf,data.mv_data,sizeof(struct stat)); //TODO zrobic rzutowanie
		}
		mdb_txn_abort(txn);
	} else {	
		res=-ENOENT;
	}

	display_odfs_id_list(list);
	return res;

}

int odfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi) {
	(void) offset;
	(void) fi;
	odfs_id_list_node * list, *list_tofree;
	int i;
        char  name_buf[100];//TODO

	init_id_list(&list);
	list_tofree=list;

	printf("odfs_readdir: Start\n");

	if (strcmp(path, "/") == 0) {

		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		filler(buf, hello_path + 1, NULL, 0);
		int rc;
		MDB_dbi dbi;
		MDB_val key, data;
		MDB_txn *txn;
		MDB_cursor *cursor;
		
		rc = mdb_txn_begin(env_odfs_posix, NULL, MDB_RDONLY, &txn);
		rc = mdb_open(txn, NULL, 0, &dbi);
		rc = mdb_cursor_open(txn, dbi, &cursor);
		while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		//	printf("key: %p %.*s, data: %p %.*s\n",
		//		key.mv_data,  (int) key.mv_size,  (int *) key.mv_data,
		//		data.mv_data, (int) data.mv_size, (char *) data.mv_data);
			sprintf(name_buf ,"id=%u", *(unsigned int *) (key.mv_data));
			filler(buf, name_buf, NULL, 0);
		}
		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
	} else {
		process_path(path, list, tp_stack);
		while(list->next!=NULL){
			sprintf(name_buf ,"id=%u", list->odfs_id);
			filler(buf, name_buf, NULL, 0);
			list=list->next;
		}
	}
	return 0;
}

int odfs_open(const char *path, struct fuse_file_info *fi) {
	odfs_id_list_node * list;
	init_id_list(&list);
	process_path(path, list, tp_stack);
	struct odfs_file *of;
	
	printf("odfs_open: Start\n");
	int count=count_odfs_id_list(list);
	//if (list->next==NULL && list->odfs_id != ODFS_NULL_ID) {
	if (count==1) {
		of=malloc(sizeof(odfs_file));
		memset(of,0,sizeof(odfs_file));
		printf("open: list.number_of_ids==1\n");
		of->id =list->odfs_id ;
		of->write_txn = NULL ;
		pthread_mutex_init(&(of->read_lock), NULL);
		of->read_txn = NULL ;
		odfs_getattr(path, &of->opd_cache.odfs_posix_stat);
		of->offset=of->opd_cache.odfs_posix_stat.st_size;
		fi->fh = (unsigned long) of;
		if (strncmp(path, hello_path,strlen(hello_path)) == 0) {
			_rm_newfile_id(path,list);
		}

		fifo_init(&of->write_fifo);
		fifo_add(sched_fifo,(void *)of);

		printf("odfs_open: END\n");
		return 0;
	//} else if (list->next==NULL && list->odfs_id == ODFS_NULL_ID) {
	} else if (count==0) {
		return -ENOENT;
	} else if (count>1) {
		return -EACCES;
	}
	if (strcmp(path, hello_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

int odfs_release(const char* path, struct fuse_file_info *fi){

	struct odfs_file * of = get_odfs_file(fi);
	//int rc;

	printf("odfs_release: Start\n");

	if(of->write_txn != NULL) {
		//TODO porzadek
		printf("odfs_release: mdb_txn_commit\n");
		/*rc = mdb_txn_commit(of->write_txn);
		if (rc) {
			fprintf(stderr, "odfs_release: mdb_txn_commit: (%d) %s\n", rc, mdb_strerror(rc));
			return -EACCES;
		}*/
		of->write_txn = NULL ;
		//mdb_close(env_odfs_data,of->write_dbi);
	}
	pthread_mutex_lock(&(of->read_lock));
	if(of->read_txn != NULL) {
		mdb_txn_reset(of->read_txn);
		of->read_txn = NULL ;
		mdb_close(env_odfs_data,of->read_dbi);
		//TODO
	}
	pthread_mutex_destroy(&(of->read_lock));
	fifo_del(sched_fifo,(void *)of);
	fifo_free(&of->write_fifo );

	printf("odfs_release: set size to %jd\n",of->offset);
	_odfs_truncate(of->id, of->offset);
	printf("odfs_release: X\n");
	free(of);	
	printf("odfs_release: XX\n");
	fi=NULL;
	printf("odfs_release: XXX\n");
	return 0;
}

int odfs_write(const char *path, const char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi) {
	int rc;
	int part_size,part_offset;
	char  * part_buf;
	size_t commit_byte=0;
	odfs_key file_part_key;
	size_t to_write =size;
	struct odfs_file * of = get_odfs_file(fi);
	int i=0;
	int wrs_size;
	int wrs_done;
	//MDB_val key, data;


	odfs_wrequest * wr;

	file_part_key.odfs_file_id=of->id;


	if(of->read_txn==NULL) {
		fprintf(stderr, "odfs_write: Create new TXT\n");
		pthread_mutex_lock(&(of->read_lock));
		if(of->read_txn==NULL) {
			rc = mdb_txn_begin(env_odfs_data, NULL, MDB_RDONLY, &of->read_txn);
			if (rc) {
				fprintf(stderr, "odfs_write: mdb_txn_begin (%d) %s\n", rc, mdb_strerror(rc));
				return -EACCES;
			}
			rc = mdb_open(of->read_txn, NULL, MDB_INTEGERKEY ,&of->read_dbi);
			if (rc) {
				fprintf(stderr, "odfs_write: mdb_open (%d) %s\n", rc, mdb_strerror(rc));
				return -EACCES;
			}
		}
		pthread_mutex_unlock(&(of->read_lock));
	}
	//fprintf(stderr, "odfs_write: size=%zu offset=%jd id=%u \n",size,offset,of->id);
	if (of->offset < offset+size) {
		of->offset= offset+size;
	}
	wrs_size =  (size+(offset%ODFS_DATA_BLOCK_SIZE))/ODFS_DATA_BLOCK_SIZE + (((size+(offset%ODFS_DATA_BLOCK_SIZE))%ODFS_DATA_BLOCK_SIZE) ? 1 : 0);
	//odfs_wrequest * * wrs = malloc(sizeof(odfs_wrequest *) * wrs_size);
	odfs_wrequest *  wrs[wrs_size];

	while (to_write >0)	{
		wr = malloc(sizeof(odfs_wrequest));
		wrs[i]=wr;
		i++;
		wr->key.mv_size=sizeof(odfs_key);
		wr->key.mv_data=&file_part_key;
		file_part_key.odfs_file_part=offset/ODFS_DATA_BLOCK_SIZE;
		//fprintf(stderr,"odfs_write: file_part_key.odfs_file_part=%u file_part_key.odfs_file_part=%u\n",file_part_key.odfs_file_part, file_part_key.odfs_file_part);
		part_offset=offset%ODFS_DATA_BLOCK_SIZE;
		part_size= (part_offset+to_write < ODFS_DATA_BLOCK_SIZE) ? part_offset+to_write : ODFS_DATA_BLOCK_SIZE;
		//fprintf(stderr,"odfs_write: to_write=%zu\n",to_write);
		if(part_offset || to_write < ODFS_DATA_BLOCK_SIZE) {//TODO Sprawdzic
		//	fprintf(stderr,"odfs_write:(part_offset || size < ODFS_DATA_BLOCK_SIZE)\n");
			pthread_mutex_lock(&(of->read_lock));
			rc = mdb_get(of->read_txn, of->read_dbi, &wr->key, &wr->data);
			pthread_mutex_unlock(&(of->read_lock));
		//	fprintf(stderr,"odfs_write:mdb_get: (%d) %s\n", rc, mdb_strerror(rc));
			part_buf=malloc(ODFS_DATA_BLOCK_SIZE);
			wr->data_free_flag=1;
			if(rc==MDB_NOTFOUND) {
		//		fprintf(stderr,"odfs_write: rc==MDB_NOTFOUND file_part_key.odfs_file_part=%u \n",file_part_key.odfs_file_part);
				memset(part_buf,0,part_offset);
				memcpy(part_buf+part_offset,buf,part_size-part_offset);
				wr->data.mv_data=part_buf;
				wr->data.mv_size=part_size;

			} else {
		//		fprintf(stderr,"odfs_write: memcpy old data wr->data.mv_size=%zu\n",wr->data.mv_size);
				memcpy(part_buf,wr->data.mv_data,wr->data.mv_size); //TODO copy to much
				memcpy(part_buf+part_offset,buf,part_size-part_offset);
				wr->data.mv_data=part_buf;
				wr->data.mv_size=part_size;
			}
		} else {
			wr->data.mv_size=ODFS_DATA_BLOCK_SIZE;
			wr->data.mv_data=(char *)buf;
			wr->data_free_flag=0;
		}	
	//	wr.key=key;
	//	wr.data=data;
//		printf("odfs_write: add to write fifo\n");
		wr->flag=1; //TODO Narazie do testow 
		fifo_add(&of->write_fifo,(void *)wr);
		
		buf=buf+part_size-part_offset;
		to_write=to_write-part_size+part_offset;
		offset=offset+part_size-part_offset;
		commit_byte=commit_byte+part_size-part_offset;
	}
	printf("odfs_write: i=%d wrs_size=%d \n",i,wrs_size);
	//mdb_txn_reset(of->read_txn);
	printf("odfs_write: wait for sched:\n");
/*	while (1) {
		wrs_done=0;
		for(i=0;i<wrs_size;i++) wrs_done=wrs_done+wrs[i]->flag;
		if(! wrs_done) break;
	}*/
	//free(wrs);
	fifo_broadcast(sched_fifo);
	fifo_wait(&of->write_fifo);
	printf("odfs_write: END\n");
	return size;
}

int odfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi) {
        int rc;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn *txn;
	size_t part_size,part_offset;
	odfs_key file_part_key;
	struct odfs_file *of = get_odfs_file(fi);

	//printf("odfs_read: Start\n");
	rc = mdb_txn_begin(env_odfs_data, NULL, MDB_RDONLY, &txn);
	rc = mdb_open(txn, NULL, MDB_INTEGERKEY , &dbi);

	file_part_key.odfs_file_id=of->id;
	key.mv_size=sizeof(odfs_key);
	key.mv_data=&file_part_key;
	size_t to_read =size;

	//fprintf(stderr, "odfs_read %lu: from id=%lu \n, offset =%ld ",size ,fi->fh,offset);
	while (to_read )	{
		file_part_key.odfs_file_part=offset/ODFS_DATA_BLOCK_SIZE;
		part_offset=offset%ODFS_DATA_BLOCK_SIZE;

		rc = mdb_get(txn, dbi, &key, &data);
		part_size= (to_read + part_offset < data.mv_size) ? to_read : data.mv_size-part_offset;
		if (part_size == 0 ) break;
		
		//fprintf(stderr, "odfs_read from file_part_key.odfs_file_part= %u: to_read=%lu,part_size=%zu,part_offset=%zu \n",file_part_key.odfs_file_part,to_read,part_size,part_offset);

		if(rc==MDB_NOTFOUND) {
			memset(buf,0,part_size);
		} else {
			memcpy(buf,data.mv_data+part_offset,part_size);
		}	

		buf=buf+part_size;
		to_read=to_read-part_size;
		offset=offset+part_size;
	}


	mdb_txn_abort(txn);
	mdb_close(env_odfs_data, dbi);

	return size;
}

int odfs_mknod(const char *path, mode_t mode, dev_t dev) {

	int retstat = 0;
	struct odfs_tag_query  query;
	char *token, *path_buf, *tofree;

	odfs_id_list_node * list;
	init_id_list(&list);

	printf("odfs_mknode: Start\n");

	if (S_ISREG(mode)) {
		printf("odfs_mknod: S_ISREG\n");
	        int rc;
        	unsigned int id;
		MDB_dbi dbi;
		MDB_val key, data;
		MDB_txn *txn;
		MDB_cursor *cursor;
		odfs_posix_data x;
        	
		x.odfs_posix_stat.st_mode = S_IFREG | 0444;
		x.odfs_posix_stat.st_nlink = 1;
		x.odfs_posix_stat.st_size = 0;
		x.odfs_posix_stat.st_uid = 1000;
		x.odfs_posix_stat.st_gid = 1000;

		rc = mdb_txn_begin(env_odfs_posix, NULL, 0, &txn);
		rc = mdb_open(txn, NULL, MDB_INTEGERKEY , &dbi);
		rc = mdb_cursor_open(txn, dbi, &cursor);
		if ( mdb_cursor_get(cursor, &key, &data, MDB_LAST  ) ==  MDB_NOTFOUND ) {
			id=0;
			printf ("XXX first id=0\n");
		} else {
			id= *(unsigned int *)(key.mv_data)+1;
			printf ("key.mv_data=%u id=%u\n",*((unsigned int *)key.mv_data), id);
		}

		key.mv_size = sizeof(unsigned int);
		key.mv_data = &id;
		data.mv_size = sizeof(odfs_posix_data);
		data.mv_data = &x;

		rc = mdb_put(txn, dbi, &key, &data, MDB_NOOVERWRITE);
		mdb_cursor_close(cursor);
		if (rc) {
			fprintf(stderr, "mdb_put: (%d) %s\n", rc, mdb_strerror(rc));
			rc = mdb_txn_commit(txn);
			return -EACCES;
		}
		rc = mdb_txn_commit(txn);
		if (rc) {
			fprintf(stderr, "mdb_txn_commit: (%d) %s\n", rc, mdb_strerror(rc));
			return -EACCES;
		}
		mdb_close(env_odfs_posix, dbi);
		//mdb_env_close(env_odfs_posix);
		fprintf(stderr, "Before add(id) from %s plugin \n", name_plugin->name);

		tofree = path_buf = strdup(path);
		query.context=CTX_PLUGIN_ADD;
		query.plugin_name="name";
		query.list=list;
		list->odfs_id=id;

		_add_newfile_id(path,list);
		while ((token = strsep(&path_buf, "/")) != NULL) {
			if (strcmp(token, hello_path+1) == 0) continue;
			if(token + 1 != path_buf) {
				query.tag_string=token;	
				odfs_run_plugin_stack(tp_stack, &query);
			} 
		}
		free(tofree);
    } 
    else
    {
        retstat=EACCES;  
    }

    return retstat;
}

/**
* \fc odfs plugin name, odfs_utimens()
*
* (x) - return value
*/
int odfs_utimens(const char* path, const struct timespec ts[2])
{
	return 0;
}

void make_write(void *arg1 ,void *arg2) {
	int rc=0;
	struct odfs_wrequest * wr = (struct odfs_wrequest * ) arg1;
	struct odfs_file * of = (struct odfs_file *)arg2;
	//puts("make_write: Start");

	if(wr->flag){
		//fprintf(stderr, "make_write:  wr->flag=%u \n", wr->flag);
		rc = mdb_put(of->write_txn, of->write_dbi, &wr->key, &wr->data, 0);
		if (rc) {
			fprintf(stderr, "make_write: mdb_put: (%d) %s\n", rc, mdb_strerror(rc));
		}
		wr->flag=0;
	}
}

//int odfs_utimens(const char* path, const struct timespec ts[2] )
//{
//	if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_utimes\n",__FILE__,__LINE__); }
//	return 0;
//}



void proc_file(void *arg1 ,void *arg2) {
	struct odfs_file * of = (struct odfs_file *)arg1;
	MDB_txn *txn= (MDB_txn *) arg2;
	//puts("proc_file: Start");
	of->write_txn=txn;
	of->write_dbi=sched_dbi;
	fifo_exec(&of->write_fifo,make_write,of);//TODO Czy tu niema wicigu??
	fifo_broadcast(&of->write_fifo);
//	puts("proc_file: END");
}

void *odfs_sched(void *arg) {
	int rc;
	MDB_txn *txn;
	const struct timespec delay={0,100000};

	puts("odfs_sched: Before INF LOOP\n");
	while (1) {
		//printf("odfs_sched: LOOP\n");
		rc = mdb_txn_begin(env_odfs_data, NULL, 0, &txn);
		if (rc) {
			fprintf(stderr, "odfs_sched: mdb_txn_begin (%d) %s\n", rc, mdb_strerror(rc));
		}
		rc = mdb_open(txn, NULL, MDB_INTEGERKEY , &sched_dbi);
		if (rc) {
			fprintf(stderr, "odfs_sched: mdb_open (%d) %s\n", rc, mdb_strerror(rc));
		}
		int i=0;
		while (i++<1000) {
			fifo_exec(sched_fifo,proc_file,txn);
			pthread_mutex_lock(&(sched_fifo->lock));
			pthread_cond_timedwait(&sched_fifo->cond,&sched_fifo->lock,&delay);
			pthread_mutex_unlock(&(sched_fifo->lock));
			
		}
		rc = mdb_txn_commit(txn);
		if (rc) {
			fprintf(stderr, "odfs_sched: mdb_txn_commit: (%d) %s\n", rc, mdb_strerror(rc));
		}
		mdb_close(env_odfs_data, sched_dbi);
		//sleep(1);//TODO troche heurystki
	}
}

void* odfs_init(struct fuse_conn_info *conn) {
	printf("odfs_init: Start\n");
	conn->want |= FUSE_CAP_BIG_WRITES;
	conn->max_write=4080*3*1000;
	sched_fifo=malloc(sizeof(struct fifo_t));
	fifo_init(sched_fifo);
	pthread_cond_init (&sched_cond, NULL);
	pthread_create (&sched,NULL,odfs_sched,NULL);
	
	return NULL;
}

static struct fuse_operations odfs_oper = {
	.getattr	= odfs_getattr,
	.readdir	= odfs_readdir,
	.truncate	= odfs_truncate,
	.open		= odfs_open,
	.release	= odfs_release,
	.unlink		= odfs_unlink,
	.read		= odfs_read,
	.write		= odfs_write,
	.init		= odfs_init,
	.mknod		= odfs_mknod,
	.utimens	= odfs_utimens,
};

int main(int argc, char *argv[])
{
	mdb_env_create(&env_odfs_data);
	mdb_env_set_mapsize(env_odfs_data, 4294967296);
	mdb_env_open(env_odfs_data, "./testdb_bart/data", MDB_FIXEDMAP /*|MDB_NOSYNC*/, 0664);
	mdb_env_create(&env_odfs_posix);
	mdb_env_set_mapsize(env_odfs_posix, 4294967296);
	mdb_env_open(env_odfs_posix, "./testdb_bart/posix", MDB_FIXEDMAP /*|MDB_NOSYNC*/, 0664);

	_odfs_load_plugin("./odfs_name.so", & name_plugin);
	(name_plugin->init)();
	odfs_init_plugin_stack(&tp_stack);
	odfs_add_to_plugin_stack(&tp_stack, name_plugin) ;
	return fuse_main(argc, argv, &odfs_oper, NULL);
}

