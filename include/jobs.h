//
//  jobs.h
//  rpg
//
//  Created by George Watson on 01/08/2025.
//

#pragma once

#include "pthread_shim.h"
#include <stdlib.h>
#include <stdbool.h>

struct job {
    void (^function)(void*);
    void *arg;
    struct job *next;
};

struct job_pool {
    pthread_t *threads;          // Array of worker threads
    struct job *job_queue_head;     // Head of task queue
    struct job *job_queue_tail;     // Tail of task queue
    pthread_mutex_t queue_mutex; // Mutex for task queue
    pthread_cond_t queue_cond;   // Condition variable for task queue
    int thread_count;            // Number of threads in pool
    int shutdown;                // Shutdown flag
};

bool pool_create(struct job_pool *pool, int num_threads);
void pool_destroy(struct job_pool *pool);
bool pool_add_job(struct job_pool *pool, void *arg, void (^function)(void*));
