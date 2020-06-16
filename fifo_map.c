#include <stdio.h>
#include <strings.h>
#include <assert.h>
#include "fifo_map.h"


#define GET_USER_DATA_HASH(mp, user_data) ((mp)->key_fun((user_data)) % (mp)->BUCKET_SIZE)

	
int common_node_list_init(common_node_list *list, unsigned node_num, unsigned user_data_size)
{
	int i;

	bzero(list, sizeof(common_node_list));

	if (pthread_rwlock_init(&(list->rwlock),NULL) != 0)
		return -1;

	for (i=0; i<node_num; i++)
	{
		common_node *node = (common_node *)malloc(sizeof(common_node));
		if (!node)
			return -1;

		node->user_data = malloc(user_data_size);
		if (!node->user_data)
			return -1;

		bzero(node, user_data_size);
		
		if (list->head)
		{
			node->list_prev = list->tail;
			list->tail->list_next = node;
			list->tail = node;
		}
		else
		{
			list->head = list->tail = node;
		}

		list->cur_num ++;
	}

	return list->cur_num;
}

void common_node_list_destroy(common_node_list *list)
{
	common_node *node;
	pthread_rwlock_wrlock(&(list->rwlock));
	while (list->head)
	{
		node = list->head;
		list->head = list->head->list_next;
		free(node->user_data);
		free(node);
	}
	pthread_rwlock_unlock(&(list->rwlock));
	pthread_rwlock_destroy(&(list->rwlock));
}

int common_node_list_add(common_node_list *list, common_node *node)
{
	pthread_rwlock_wrlock(&(list->rwlock));
	if (list->head)
	{
		node->list_prev = list->tail;
		node->list_next = NULL;
		list->tail->list_next = node;
		list->tail = node;
	}
	else
	{
		list->head = list->tail = node;
	}

	list->cur_num ++;
	pthread_rwlock_unlock(&(list->rwlock));

	return list->cur_num;
}


int common_node_list_del(common_node_list *list, common_node *node)
{
	pthread_rwlock_wrlock(&(list->rwlock));
	if (!list->head)
	{
		assert(NULL == list->head);
		assert(list->cur_num == 0);
		pthread_rwlock_unlock(&(list->rwlock));
		return 0;
	}
	
	if (list->head == node)
	{
		list->head = list->head->list_next;
		if (list->head)
			list->head->list_prev = NULL;
		else
			list->tail = NULL;
	}
	else
	{
		if (list->tail == node)
			list->tail = node->list_prev;
		node->list_prev->list_next = node->list_next;
		if (node->list_next)
			node->list_next->list_prev = node->list_prev;
	}

	list->cur_num --;
	pthread_rwlock_unlock(&(list->rwlock));

	return list->cur_num;
}


common_node * common_node_list_pop(common_node_list *list)
{
	common_node *node = NULL;
	
	pthread_rwlock_wrlock(&(list->rwlock));
	if (list->head)
	{
		node = list->head;
		list->head = list->head->list_next;
		if (list->head)
			list->head->list_prev = NULL;
		else
			list->tail = NULL;

		list->cur_num --;
	}
	pthread_rwlock_unlock(&(list->rwlock));

	return node;
}

unsigned common_node_list_num(common_node_list *list)
{
	return list->cur_num;
}



int fifo_map_init(fifo_map *mp, 
	unsigned bucket_size, unsigned user_data_num, unsigned user_data_size,
	node_key_fun key_fun, node_cmp_fun cmp_fun)
{
	int i;
	
	bzero(mp, sizeof(fifo_map));
	mp->BUCKET_SIZE = bucket_size;
	mp->USER_DATA_SIZE = user_data_size;
	mp->USER_DATA_MAX_NUM = user_data_num;
	
	mp->bucket_ptr = (map_list **)malloc(bucket_size * sizeof(map_list *));
	if (!mp->bucket_ptr)
		return -1;
	for (i=0; i<bucket_size; i++)
	{
		map_list *ml = (map_list *)malloc(sizeof(map_list));
		if (!ml)
			return -1;
		ml->list = NULL;
		if (pthread_rwlock_init(&(ml->rwlock), NULL) != 0)
			return -1;
		
		mp->bucket_ptr[i] = ml;
	}

	mp->key_fun = key_fun;
	mp->cmp_fun = cmp_fun;

	common_node_list_init(&mp->used_list, 0, user_data_size);
	if (common_node_list_init(&mp->free_list, user_data_num, user_data_size) != user_data_num)
		return -1;

	return 0;
}

void fifo_map_destroy(fifo_map *mp)
{
	int i;
	
	for (i=0; i<mp->BUCKET_SIZE; i++)
	{
		map_list *ml = mp->bucket_ptr[i];
		if (!ml)
			continue;
		pthread_rwlock_destroy(&(ml->rwlock));
		free(ml);
		mp->bucket_ptr[i] = NULL;
	}
	free(mp->bucket_ptr);
	common_node_list_destroy(&mp->free_list);
	common_node_list_destroy(&mp->used_list);
}


int fifo_map_add(fifo_map *mp, void *user_data)
{
	//find same node
	unsigned idx = GET_USER_DATA_HASH(mp, user_data);
	map_list *aml = mp->bucket_ptr[idx];
	common_node *node = NULL;
	pthread_rwlock_wrlock(&(aml->rwlock));
	node = aml->list;
	while (node)
	{
		if (mp->cmp_fun(node->user_data, user_data) == 0)
		{
			pthread_rwlock_unlock(&(aml->rwlock));
			return 1;
		}
		node = node->hash_next;
	}
	pthread_rwlock_unlock(&(aml->rwlock));

	node = common_node_list_pop(&mp->free_list);
	if (!node)
	{
		node = common_node_list_pop(&mp->used_list);
		assert(node);
		//unlink from hash map
		map_list *dml = mp->bucket_ptr[node->hash_id];
		pthread_rwlock_wrlock(&(dml->rwlock));
		if (dml->list == node)
		{
			assert(NULL == node->hash_prev);
			dml->list = dml->list->hash_next;
			if (dml->list)
				dml->list->hash_prev = NULL;
		}
		else
		{
			node->hash_prev->hash_next = node->hash_next;
			if (node->hash_next)
				node->hash_next->hash_prev = node->hash_prev;
		}
		pthread_rwlock_wrlock(&(dml->rwlock));
	}

	//add node to map
	memcpy(node->user_data, user_data, mp->USER_DATA_SIZE);
	pthread_rwlock_wrlock(&(aml->rwlock));
	node->hash_prev = NULL;
	node->hash_next = aml->list;
	if (aml->list)
		aml->list->hash_prev = node;
	aml->list = node;

	pthread_rwlock_unlock(&(aml->rwlock));

	common_node_list_add(&mp->used_list, node);
	
	return 0;
}

int fifo_map_del(fifo_map *mp, void *user_data)
{
	//find same node
	unsigned idx = GET_USER_DATA_HASH(mp, user_data);
	map_list *ml = mp->bucket_ptr[idx];
	common_node *node = NULL;
	pthread_rwlock_wrlock(&(ml->rwlock));
	node = ml->list;
	while (node)
	{
		if (mp->cmp_fun(node->user_data, user_data) == 0)
		{
			break;
		}
		node = node->hash_next;
	}

	if (!node)
	{
		pthread_rwlock_unlock(&(ml->rwlock));
		return -1;
	}

	//delete node from map
	if (ml->list == node)
	{
		assert(NULL == node->hash_prev);
		ml->list = ml->list->hash_next;
		if (ml->list)
			ml->list->hash_prev = NULL;
	}
	else
	{
		node->hash_prev->hash_next = node->hash_next;
		if (node->hash_next)
			node->hash_next->hash_prev = node->hash_prev;
	}
	pthread_rwlock_unlock(&(ml->rwlock));

	//delet node from used list
	common_node_list_del(&mp->used_list, node);
	common_node_list_add(&mp->free_list, node);
	
	return 0;
}


void fifo_map_clear(fifo_map *mp)
{
	common_node *node = NULL;
	
	node = common_node_list_pop(&mp->used_list);
	while (node)
	{
		//unlink from hash map
		map_list *dml = mp->bucket_ptr[node->hash_id];
		pthread_rwlock_wrlock(&(dml->rwlock));
		if (dml->list == node)
		{
			assert(NULL == node->hash_prev);
			dml->list = dml->list->hash_next;
			if (dml->list)
				dml->list->hash_prev = NULL;
		}
		else
		{
			node->hash_prev->hash_next = node->hash_next;
			if (node->hash_next)
				node->hash_next->hash_prev = node->hash_prev;
		}
		pthread_rwlock_wrlock(&(dml->rwlock));

		common_node_list_add(&mp->free_list, node);

		node = common_node_list_pop(&mp->used_list);
	}
}


void * fifo_map_find(fifo_map *mp, void *user_data, void *ret_user_data)
{
	unsigned idx = GET_USER_DATA_HASH(mp, user_data);
	map_list *ml = mp->bucket_ptr[idx];
	common_node *node = NULL;
	pthread_rwlock_wrlock(&(ml->rwlock));
	node = ml->list;
	while (node)
	{
		if (mp->cmp_fun(node->user_data, user_data) == 0)
		{
			if (ret_user_data)
				memcpy(ret_user_data, node->user_data, mp->USER_DATA_SIZE);

			pthread_rwlock_unlock(&(ml->rwlock));
			return node->user_data;
		}
		node = node->hash_next;
	}
	pthread_rwlock_unlock(&(ml->rwlock));
	return NULL;
}

int fifo_map_exist(fifo_map *mp, void *user_data)
{
	if (fifo_map_find(mp, user_data, NULL))
		return 1;
	return 0;
}


unsigned fifo_map_num(fifo_map *mp)
{
	return common_node_list_num(&mp->used_list);
}



