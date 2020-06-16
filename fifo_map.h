#ifndef __FIFO_MAP_H__
#define __FIFO_MAP_H__


#include <pthread.h>


typedef struct common_node_t
{
	struct common_node_t *list_prev, *list_next;
	struct common_node_t *hash_prev, *hash_next;
	unsigned hash_id;
	void *user_data;
}
common_node;


typedef struct common_node_list_t
{
	common_node *head, *tail;
	unsigned cur_num;
	pthread_rwlock_t rwlock;
}
common_node_list;


int common_node_list_init(common_node_list *list, unsigned node_num, unsigned user_data_size);

void common_node_list_destroy(common_node_list *list);

int common_node_list_add(common_node_list *list, common_node *node);

common_node * common_node_list_pop(common_node_list *list);



typedef unsigned (* node_key_fun)(void *userdata);
typedef int (* node_cmp_fun)(void *userdata1, void *userdata2);


typedef struct map_list_t
{
	common_node *list;
	pthread_rwlock_t rwlock;
}
map_list;

typedef struct fifo_map_t
{
	unsigned BUCKET_SIZE;
	unsigned USER_DATA_SIZE;
	unsigned USER_DATA_MAX_NUM;
	
	map_list **bucket_ptr;

	node_key_fun key_fun;
	node_cmp_fun cmp_fun;

	common_node_list used_list;
	common_node_list free_list;
}
fifo_map;


int fifo_map_init(fifo_map *mp, 
	unsigned bucket_size, unsigned user_data_num, unsigned user_data_size,
	node_key_fun key_fun, node_cmp_fun cmp_fun);

void fifo_map_destroy(fifo_map *mp);

int fifo_map_add(fifo_map *mp, void *user_data);

int fifo_map_del(fifo_map *mp, void *user_data);

void fifo_map_clear(fifo_map *mp);

void * fifo_map_find(fifo_map *mp, void *user_data, void *ret_user_data);

int fifo_map_exist(fifo_map *mp, void *user_data);

unsigned fifo_map_num(fifo_map *mp);








#endif
