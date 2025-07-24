//
//  threads.h
//  rpg
//
//  Created by George Watson on 23/07/2025.
//

#pragma once

#include "pthread_shim.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct thrd_work {
    void(*func)(void*);
    void *arg;
    struct thrd_work *next;
};

struct thrd_pool {
    struct thrd_work  *head, *tail;
    pthread_mutex_t work_lock;
    pthread_cond_t work_cond, working_cond;
    size_t working_count, thread_count;
    int kill;
};

bool thrd_pool_create(size_t maxThreads, struct thrd_pool *dst);
void thrd_pool_destroy(struct thrd_pool *pool);
int thrd_pool_push_work(struct thrd_pool *pool, void(*func)(void*), void *arg);
// Adds work to the front of the queue
int thrd_pool_push_work_priority(struct thrd_pool *pool, void(*func)(void*), void *arg);
void thrd_pool_join(struct thrd_pool *pool);

struct thrd_queue_entry {
    void *data;
    struct thrd_queue_entry *next;
};

struct thrd_queue {
    struct thrd_queue_entry *head, *tail;
    pthread_mutex_t readLock, writeLock;
    size_t count;
};

bool thrd_queue_create(struct thrd_queue* dst);
void thrd_queue_push(struct thrd_queue *queue, void *data);
void* thrd_queue_pop(struct thrd_queue *queue);
void thrd_queue_destroy(struct thrd_queue *queue);
