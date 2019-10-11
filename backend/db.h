/*
 * db.h
 *      Author: aagapi
 */

#ifndef BACKEND_DB_H_
#define BACKEND_DB_H_

#include "skiplist.h"
#include "fastrand.h"
#include "failure_detector/vector_clock.h"

#include <pthread.h>
#include <unistd.h>

typedef void *WORD;

// High level API:

#define DB_TYPE_CHAR 0
#define DB_TYPE_INT16 1
#define DB_TYPE_INT32 2
#define DB_TYPE_INT64 3
#define DB_TYPE_FLOAT32 4
#define DB_TYPE_FLOAT64 5

typedef struct db_schema {
	int * col_types;
	int no_cols;

	int * primary_key_idxs;
	int no_primary_keys;

	int * clustering_key_idxs;
	int no_clustering_keys;

	int * index_key_idxs;
	int no_index_keys;
} db_schema_t;

typedef struct db_table {
	WORD table_key;
	db_schema_t * schema;
	skiplist_t * rows;
	skiplist_t ** indexes;

	skiplist_t * row_tombstones;

	pthread_mutex_t* lock;
} db_table_t;

// Queues:

typedef struct queue_callback_args
{
	WORD table_key;
	WORD queue_id;

	WORD consumer_id;
	WORD shard_id;
	WORD app_id;

	int status;
} queue_callback_args;

typedef struct queue_callback
{
	void (*callback)(queue_callback_args *);
	pthread_mutex_t * lock;
	pthread_cond_t * signal;
} queue_callback;

typedef struct consumer_state {
	WORD consumer_id;
	WORD shard_id;
	WORD app_id;

	long private_read_head;
	long private_consume_head;

	vector_clock * prh_version;
	vector_clock * pch_version;

	short notified;

	queue_callback* callback;;
} consumer_state;

// Cells:

typedef struct db_cell {
	WORD key;
	skiplist_t * cells;
	WORD * column_array;
	int no_columns;

	// Queue metadata:
	skiplist_t * consumer_state; // TO DO: Change to hash table, add indexing by shard_id and app_id
	long no_entries;
	pthread_mutex_t* enqueue_lock;
	pthread_mutex_t* read_lock;
	pthread_mutex_t* subscribe_lock;

	vector_clock * version;

	struct db_cell_t * _next;
} db_cell_t;

typedef db_cell_t db_row_t;

typedef struct db {
    skiplist_t * tables;
    skiplist_t * txn_state;
} db_t;

// DB high level API:

// DB, schema and table manipulation:

db_t * get_db();
int db_delete_db(db_t * db);
int db_dump_db(db_t * db);

db_schema_t* db_create_schema(int * col_types, int no_cols, int * primary_key_idxs, int no_primary_keys, int * clustering_key_idxs, int no_clustering_keys, int * index_key_idxs, int no_index_keys);
void free_schema(db_schema_t * schema);
int db_create_table(WORD table_key, db_schema_t* schema, db_t * db, unsigned int * fastrandstate);
int db_create_index(int new_index, WORD table_key, db_t * db, unsigned int * fastrandstate);
int db_delete_table(WORD table_key, db_t * db);

// DB queries:

int db_insert(WORD * column_values, int no_cols, WORD table_key, db_t * db, unsigned int * fastrandstate);
int db_insert_transactional(WORD * column_values, int no_cols, vector_clock * version, WORD table_key, db_t * db, unsigned int * fastrandstate);
int db_update(int * col_idxs, int no_cols, WORD * column_values, WORD table_key, db_t * db);
int db_update_transactional(int * col_idxs, int no_cols, WORD * column_values, vector_clock * version, WORD table_key, db_t * db);
db_row_t* db_search(WORD* primary_keys, WORD table_key, db_t * db);
int db_range_search(WORD* start_primary_keys, WORD* end_primary_keys, snode_t** start_row, snode_t** end_row, WORD table_key, db_t * db);
int db_range_search_copy(WORD* start_primary_keys, WORD* end_primary_keys, db_row_t** rows, WORD table_key, db_t * db);
db_row_t* db_search_clustering(WORD* primary_keys, WORD* clustering_keys, int no_clustering_keys, WORD table_key, db_t * db);
int db_range_search_clustering(WORD* primary_keys, WORD* start_clustering_keys, WORD* end_clustering_keys, int no_clustering_keys, snode_t** start_row, snode_t** end_row, WORD table_key, db_t * db);
WORD* db_search_columns(WORD* primary_keys, WORD* clustering_keys, int* column_idxs, int no_columns, WORD table_key, db_t * db);
db_row_t* db_search_index(WORD index_key, int idx_idx, WORD table_key, db_t * db);
int db_range_search_index(int idx_idx, WORD start_idx_key, WORD end_idx_key, snode_t** start_row, snode_t** end_row, WORD table_key, db_t * db);
int db_delete_row(WORD* primary_keys, WORD table_key, db_t * db);
int db_delete_row_transactional(WORD* primary_keys, vector_clock * version, WORD table_key, db_t * db);
// TO DO: int db_delete_cell(WORD* keys, int no_primary_keys, int no_clustering_keys, WORD table_key, db_t * db);
int db_delete_by_index(WORD index_key, int idx_idx, WORD table_key, db_t * db);
int db_verify_cell_version(WORD* primary_keys, int no_primary_keys, WORD* clustering_keys, int no_clustering_keys, WORD table_key, vector_clock * version, db_t * db);
int db_verify_row_range_version(WORD* start_primary_keys, WORD* end_primary_keys, int no_primary_keys, WORD table_key,
									long * range_result_keys, vector_clock ** range_result_versions, int no_range_results, db_t * db);
int db_verify_cell_range_version(WORD* primary_keys, int no_primary_keys, WORD* start_clustering_keys, WORD* end_clustering_keys, int no_clustering_keys, WORD table_key,
									long * range_result_keys, vector_clock ** range_result_versions, int no_range_results, db_t * db);
int db_verify_index_version(WORD index_key, int idx_idx, WORD table_key, vector_clock * version, db_t * db);
int db_verify_index_range_version(int idx_idx, WORD start_idx_key, WORD end_idx_key,
									long * range_result_keys, vector_clock ** range_result_versions, int no_range_results, WORD table_key, db_t * db);

// Lower level API:

db_row_t * create_db_row(WORD * column_values, db_schema_t * schema, unsigned int * fastrandstate);
void free_db_row(db_row_t * row, db_schema_t * schema);
void long_row_to_string(db_row_t* row, char * to_string, int * len);
void print_long_row(db_row_t* row);

int table_insert(WORD * column_values, int no_cols, vector_clock * version, db_table_t * table, unsigned int * fastrandstate);
int table_update(int * col_idxs, int no_cols, WORD * column_values, vector_clock * version, db_table_t * table);
db_row_t* table_search(WORD* primary_keys, db_table_t * table);
int table_range_search(WORD* start_primary_keys, WORD* end_primary_keys, snode_t** start_row, snode_t** end_row, db_table_t * table);
int table_range_search_copy(WORD* start_primary_keys, WORD* end_primary_keys, db_row_t** rows, db_table_t * table);
db_row_t* table_search_clustering(WORD* primary_keys, WORD* clustering_keys, int no_clustering_keys, db_table_t * table);
int table_range_search_clustering(WORD* primary_keys, WORD* start_clustering_keys, WORD* end_clustering_keys, int no_clustering_keys, snode_t** start_row, snode_t** end_row, db_table_t * table);
WORD* table_search_columns(WORD* primary_keys, WORD* clustering_keys, int* column_idxs, int no_columns, db_table_t * table);
db_row_t* table_search_index(WORD index_key, int idx_idx, db_table_t * table);
int table_range_search_index(int idx_idx, WORD start_idx_key, WORD end_idx_key, snode_t** start_row, snode_t** end_row, db_table_t * table);
int table_delete_row(WORD* primary_keys, vector_clock * version, db_table_t * table);
int table_delete_by_index(WORD index_key, int idx_idx, db_table_t * table);
int table_verify_cell_version(WORD* primary_keys, int no_primary_keys, WORD* clustering_keys, int no_clustering_keys, vector_clock * version, db_table_t * table);
int table_verify_row_range_version(WORD* start_primary_keys, WORD* end_primary_keys, int no_primary_keys,
										long * range_result_keys, vector_clock ** range_result_versions, int no_range_results, db_table_t * table);
int table_verify_cell_range_version(WORD* primary_keys, int no_primary_keys, WORD* start_clustering_keys, WORD* end_clustering_keys, int no_clustering_keys,
										long * range_result_keys, vector_clock ** range_result_versions, int no_range_results, db_table_t * table);
int table_verify_index_version(WORD index_key, int idx_idx, vector_clock * version, db_table_t * table);
int table_verify_index_range_version(int idx_idx, WORD start_idx_key, WORD end_idx_key,
										long * range_result_keys, vector_clock ** range_result_versions, int no_range_results, db_table_t * table);

#endif /* BACKEND_DB_H_ */
