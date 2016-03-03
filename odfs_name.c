/*! \file odfs_name.c
    \brief odfs name plugin code
*/
#include "odfs.h"
#include "liblmdb/lmdb.h"
#include <string.h>
#include <stdio.h>
#include "process_path.h"

#define DEBUG 1

MDB_env *env_odfs_tag_name;
MDB_dbi dbi_odfs_tag_name;
const char plugin_name[] = "name";


/**
* \fc odfs plugin name, get()
*
* (x) - return value
*/
int get(char * tag_value,struct odfs_id_list_node * list){
   	if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_name: get for tag_value=%s strlen= %lu\n",__FILE__,__LINE__,tag_value,strlen(tag_value) ); }

	if( strlen(tag_value) == 0 )
	{
		return 0;
	}

	int rc;
	int val;
	MDB_val key, data;
	MDB_txn *txn;
	MDB_cursor *cursor;
	unsigned int flags=MDB_SET;

	char * tofree,*tag_value_buf,*token;
        
        odfs_id_list_node * test;
        test=list;

	tofree = tag_value_buf = strdup(tag_value);
	if ((token = strsep(&tag_value_buf, "=")) != NULL && strcmp(token,"id")==0 && tag_value_buf) {
		val = strtol(tag_value_buf, &tag_value_buf, 10);
   		if(DEBUG) { fprintf(stderr, "%s:%d debug: name get, val=%u to list\n",__FILE__,__LINE__,(unsigned int ) val ); }

		if (*tag_value_buf=='\0')  {
   			if(DEBUG) { fprintf(stderr, "%s:%d debug: name get, add to list val=%u to list\n",__FILE__,__LINE__,(unsigned int ) val ); }

			add_id_to_list(val, list);
			//list->id_list[list->number_of_ids] = (unsigned int) val ;
			//list->number_of_ids++;
		}
		//printf("%ld\n", val); 

   		if(DEBUG) { fprintf(stderr, "%s:%d debug: name get, free tofree\n",__FILE__,__LINE__); }
		free(tofree);
		return 0;
	} else {
		free(tofree);
	}

   	if(DEBUG) { fprintf(stderr, "%s:%d debug: name get, mdb_txn_begin operation start \n",__FILE__,__LINE__); }
	rc = mdb_txn_begin(env_odfs_tag_name, NULL, MDB_RDONLY, &txn);
	if (rc) {
   		if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_tag_name: get: mdb_txn_begin: rc= (%d) \n",__FILE__,__LINE__, rc ); }
   		if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_tag_name: get: mdb_txn_begin: mdb_strerror(rc)=) %s\n",__FILE__,__LINE__, mdb_strerror(rc) ); }
	}
   	if(DEBUG) { fprintf(stderr, "%s:%d debug: name get, mdb_open operation start \n",__FILE__,__LINE__); }
	rc = mdb_open(txn, NULL, MDB_DUPSORT , &dbi_odfs_tag_name);
	if (rc) {
   		if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_tag_name: get: mdb_open: (%d) %s\n",__FILE__,__LINE__, rc, mdb_strerror(rc) ); }
	}
   	if(DEBUG) { fprintf(stderr, "%s:%d debug: name get, mdb_cursor_open operation start \n",__FILE__,__LINE__); }
	rc = mdb_cursor_open(txn, dbi_odfs_tag_name, &cursor);
	if (rc) {
   		if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_tag_name: get: mdb_cursor_open: (%d) %s\n",__FILE__,__LINE__, rc, mdb_strerror(rc) ); }
	}
        key.mv_data=tag_value;
	key.mv_size=strlen(tag_value); 

	while ((rc = mdb_cursor_get(cursor, &key, &data,  flags)) == 0) {
		//fprintf(stderr,"odfs_name get : key: %p %.*s, data: %p %u\n",
		//	key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
		//	data.mv_data,  *((unsigned int *) data.mv_data));
   		if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_name get : key: %p %.*s, data: %p %u\n", 
                            __FILE__,__LINE__, key.mv_data,  (int) key.mv_size,  (char *) key.mv_data, 
                            data.mv_data,  *((unsigned int *) data.mv_data) ); }
		//list->id_list[list->number_of_ids]=*((unsigned int *)data.mv_data);
		add_id_to_list(*((unsigned int *)data.mv_data), list);
		//list->number_of_ids++;
		flags=MDB_NEXT_DUP;
	}
	if (rc) {
   		if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_tag_name: mdb_cursor_get: (%d) %s\n",__FILE__,__LINE__, rc, mdb_strerror(rc) ); }
	}
	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);
	mdb_close(env_odfs_tag_name, dbi_odfs_tag_name);

	display_odfs_id_list(test);
	return 0;
}

/**
* \fc odfs plugin name, add()
*
* (x) - return value
*/
int add(char * tag_value,struct odfs_id_list_node * list){
	int rc;
	MDB_val key, data;
	MDB_txn *txn;

	unsigned int id = list->odfs_id;

   	if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_name: add: tag_value=%s to id=%u \n",__FILE__,__LINE__, tag_value, id ); }

        key.mv_data=tag_value;
	key.mv_size=strlen(tag_value); 

	data.mv_data=&id;
	data.mv_size=sizeof(unsigned int);

	rc = mdb_txn_begin(env_odfs_tag_name, NULL, 0, &txn);
	rc = mdb_open(txn, NULL, MDB_DUPSORT , &dbi_odfs_tag_name);
	rc = mdb_put(txn, dbi_odfs_tag_name, &key, &data, 0);

	if (rc) {
   		if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_tag_name: add: mdb_put: (%d) %s\n",__FILE__,__LINE__, rc, mdb_strerror(rc) ); }
	}
	rc = mdb_txn_commit(txn);
	if (rc) {
   		if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_tag_name: add: mdb_txn_commit: (%d) %s\n",__FILE__,__LINE__, rc, mdb_strerror(rc) ); }
	}

	mdb_close(env_odfs_tag_name, dbi_odfs_tag_name);

   	if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_name: add: end \n",__FILE__,__LINE__ ); }
	return 0;
}

/**
* \fc odfs plugin name, del()
*
* (x) - return value
*/
int del(unsigned int id,char * tag_value){
   	if(DEBUG) { fprintf(stderr, "%s:%d debug: odfs_name: del \n",__FILE__,__LINE__ ); }
	return 0;
}

/**
* \fc odfs plugin name, init()
*
* (x) - return value
*/
int init() {
	mdb_env_create(&env_odfs_tag_name);
	mdb_env_set_mapsize(env_odfs_tag_name, 1024*1024*40);
	mdb_env_open(env_odfs_tag_name, "./testdb_bart/tag/name", MDB_FIXEDMAP , 0664);
	return 0;
}

/**
* \fc odfs plugin name, fini()
*
* (x) - return value
*/
int fini(){
	mdb_env_close(env_odfs_tag_name);
	return 0;
}


