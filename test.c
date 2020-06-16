#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include "fifo_map.h"


typedef struct addr_info
{
	uint32_t ip;
	uint16_t port;
}
addr_info;


unsigned addr_key(void *userdata)
{
	addr_info *p = (addr_info *)userdata;
	return p->ip ^ p->port;
}

int addr_cmp(void *userdata1, void *userdata2)
{
	addr_info *p1 = (addr_info *)userdata1;
	addr_info *p2 = (addr_info *)userdata2;
	if (p1->ip == p2->ip && p1->port == p2->port)
		return 0;

	return -1;
}


//test
int main()
{
	int i;
	fifo_map fifo_table;

	fifo_map_init(&fifo_table, 100, 500, sizeof(addr_info), addr_key, addr_cmp);
	printf("init talbe node num: %u\n", fifo_map_num(&fifo_table));

	for (i=0; i<1000; i++)
	{
		addr_info *ainfo = (addr_info *)malloc(sizeof(addr_info));
		assert(ainfo);
		ainfo->ip = i % 100;
		ainfo->port = i % 100;
		fifo_map_add(&fifo_table, ainfo);
	}
	printf("add 100 node finish, talbe node num: %u\n", fifo_map_num(&fifo_table));
	assert(fifo_map_num(&fifo_table) == 100);

	fifo_map_clear(&fifo_table);
	printf("clear finish, talbe node num: %u\n", fifo_map_num(&fifo_table));
	assert(fifo_map_num(&fifo_table) == 0);

	for (i=0; i<1000; i++)
	{
		addr_info *ainfo = (addr_info *)malloc(sizeof(addr_info));
		assert(ainfo);
		ainfo->ip = i;
		ainfo->port = i;
		fifo_map_add(&fifo_table, ainfo);
	}
	printf("add 1000 node finish, talbe node num: %u\n", fifo_map_num(&fifo_table));
	assert(fifo_map_num(&fifo_table) == 500);

	printf("add and del test ...\n");
	addr_info ainfo, ret_ainfo;
	
	ainfo.ip = 1;
	ainfo.port = 1;
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo) == NULL);

	ainfo.ip = 499;
	ainfo.port = 499;
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo) == NULL);

	ainfo.ip = 500;
	ainfo.port = 500;
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo));
	fifo_map_del(&fifo_table, &ainfo);
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo) == NULL);

	ainfo.ip = 999;
	ainfo.port = 999;
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo));
	fifo_map_del(&fifo_table, &ainfo);
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo) == NULL);

	ainfo.ip = 600;
	ainfo.port = 600;
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo));
	fifo_map_del(&fifo_table, &ainfo);
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo) == NULL);

	assert(fifo_map_num(&fifo_table) == 497);

	ainfo.ip = 1000;
	ainfo.port = 1000;
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo) == NULL);
	assert(fifo_map_exist(&fifo_table, &ainfo) == 0);
	fifo_map_add(&fifo_table, &ainfo);
	assert(fifo_map_find(&fifo_table, &ainfo, &ret_ainfo));
	assert(fifo_map_exist(&fifo_table, &ainfo) == 1);
	assert(ret_ainfo.ip == 1000 && ret_ainfo.port == 1000);

	assert(fifo_map_num(&fifo_table) == 498);
	for (i=0; i<100000; i++)
	{
		addr_info *ainfo = (addr_info *)malloc(sizeof(addr_info));
		assert(ainfo);
		ainfo->ip = i % 100;
		ainfo->port = i % 100;
		fifo_map_add(&fifo_table, ainfo);
	}
	assert(fifo_map_num(&fifo_table) == 500);

	fifo_map_destroy(&fifo_table);

	printf("all test OK\n");
	
	return 0;
}


//gcc fifo_map.c test.c -lpthread
