//
//  jobs.c
//  rpg
//
//  Created by George Watson on 01/08/2025.
//

#include "jobs.h"
#include <stdlib.h>

static void* work(void *arg) {
    struct job_pool *pool = (struct job_pool*)arg;
    for (;;) {
        pthread_mutex_lock(&pool->queue_mutex);
        while (pool->job_queue_head == NULL && !pool->shutdown)
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }
        struct job *job = pool->job_queue_head;
        if ((pool->job_queue_head = job->next) == NULL)
            pool->job_queue_tail = NULL;

        pthread_mutex_unlock(&pool->queue_mutex);
        job->function(job->arg);
        free(job);
    }
    return NULL;
}

bool pool_create(struct job_pool *pool, int num_threads) {
    if (!pool)
        return false;
    if (!(pool->threads = malloc(sizeof(pthread_t) * num_threads))) {
        free(pool);
        return NULL;
    }
    pool->thread_count = num_threads;
    pool->job_queue_head = NULL;
    pool->job_queue_tail = NULL;
    pool->shutdown = 0;
    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0 ||
        pthread_cond_init(&pool->queue_cond, NULL) != 0) {
        free(pool->threads);
        free(pool);
        return NULL;
    }
    for (int i = 0; i < num_threads; i++)
        if (pthread_create(&pool->threads[i], NULL, work, pool) != 0) {
            pool_destroy(pool);
            return NULL;
        }
    return pool;
}

void pool_destroy(struct job_pool *pool) {
    if (!pool)
        return;

    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
    for (int i = 0; i < pool->thread_count; i++)
        pthread_join(pool->threads[i], NULL);

    struct job *current = pool->job_queue_head;
    while (current) {
        struct job *next = current->next;
        free(current);
        current = next;
    }

    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_cond);
    free(pool->threads);
}

bool pool_add_job(struct job_pool *pool, void *arg, void (^function)(void*)) {
    if (!pool || !function)
        return false;

    struct job *job = malloc(sizeof(struct job));
    if (!job)
        return false;

    job->function = function;
    job->arg = arg;
    job->next = NULL;

    pthread_mutex_lock(&pool->queue_mutex);

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->queue_mutex);
        free(job);
        return false;
    }

    if (pool->job_queue_tail == NULL) {
        pool->job_queue_head = job;
        pool->job_queue_tail = job;
    } else {
        pool->job_queue_tail->next = job;
        pool->job_queue_tail = job;
    }

    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    return true;
}
