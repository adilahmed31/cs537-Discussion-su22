#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "mapreduce.h"
#include "hashmap.h"
#include <time.h>

struct kv {
    char* key;
    char* value;
};

struct kv_list {
    struct kv** elements;
    size_t num_elements;
    size_t size;
};

struct kv_list kvl;
size_t kvl_counter;

void init_kv_list(size_t size) {
    kvl.elements = (struct kv**) malloc(size * sizeof(struct kv*));
    kvl.num_elements = 0;
    kvl.size = size;
}

void add_to_list(struct kv* elt) {
    if (kvl.num_elements == kvl.size) {
	kvl.size *= 2;
	kvl.elements = realloc(kvl.elements, kvl.size * sizeof(struct kv*));
    }
    kvl.elements[kvl.num_elements++] = elt;
}

char* get_func(char* key, int partition_number) {
    if (kvl_counter == kvl.num_elements) {
	return NULL;
    }
    struct kv *curr_elt = kvl.elements[kvl_counter];
    if (!strcmp(curr_elt->key, key)) {
	kvl_counter++;
	return curr_elt->value;
    }
    return NULL;
}

int cmp(const void* a, const void* b) {
    char* str1 = (*(struct kv **)a)->key;
    char* str2 = (*(struct kv **)b)->key;
    return strcmp(str1, str2);
}

void MR_Emit(char* key, char* value)
{
    struct kv *elt = (struct kv*) malloc(sizeof(struct kv));
    if (elt == NULL) {
	printf("Malloc error! %s\n", strerror(errno));
	exit(1);
    }
    elt->key = strdup(key);
    elt->value = strdup(value);
    add_to_list(elt);

    return;
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    return 0;
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers,
	    Reducer reduce, int num_reducers, Partitioner partition)
{
    struct timespec start, end;
    double cpu_time_used;
    

    init_kv_list(10);
    int i;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 1; i < argc; i++) {
	(*map)(argv[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (end.tv_sec - start.tv_sec);
    cpu_time_used += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("** Map stage took %.3f secs **\n", cpu_time_used);


    clock_gettime(CLOCK_MONOTONIC, &start);
    qsort(kvl.elements, kvl.num_elements, sizeof(struct kv*), cmp);
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (end.tv_sec - start.tv_sec);
    cpu_time_used += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("** Sort stage took %.3f secs **\n", cpu_time_used);

    // note that in the single-threaded version, we don't really have
    // partitions. We just use a global counter to keep it really simple
    kvl_counter = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (kvl_counter < kvl.num_elements) {
	(*reduce)((kvl.elements[kvl_counter])->key, get_func, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    cpu_time_used = (end.tv_sec - start.tv_sec);
    cpu_time_used += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("** Reduce stage took %.3f secs **\n", cpu_time_used);
}