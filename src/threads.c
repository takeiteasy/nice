//
//  threads.c
//  rpg
//
//  Created by George Watson on 23/07/2025.
//

#define PTHREAD_SHIM_IMPLEMENTATION
#include "threads.h"

static void* thrd_pool_worker(void *arg) {
    struct thrd_pool *pool = (struct thrd_pool*)arg;
    while (!pool->kill) {
        while (!pool->head && !pool->kill)
            pthread_cond_wait(&pool->work_cond, &pool->work_lock);
        if (pool->kill)
            break;

        struct thrd_work *work = pool->head;
        if (!work)
            break;
        if (!work->next) {
            pool->head = NULL;
            pool->tail = NULL;
        } else
            pool->head = work->next;
        pool->working_count++;

        pthread_mutex_lock(&pool->work_lock);
        work->func(work->arg);
        free(work);
        if (!pool->kill && !--pool->working_count && pool->head)
            pthread_cond_signal(&pool->working_cond);
        pthread_mutex_unlock(&pool->work_lock);
    }

    pool->thread_count--;
    pthread_cond_signal(&pool->working_cond);
    pthread_mutex_unlock(&pool->work_lock);
    return NULL;
}

bool thrd_pool_create(size_t maxThreads, struct thrd_pool *pool) {
    if (!maxThreads || !pool)
        return false;
    pthread_mutex_init(&pool->work_lock, NULL);
    pthread_cond_init(&pool->work_cond, NULL);
    pthread_cond_init(&pool->working_cond, NULL);
    pool->head = pool->tail = NULL;
    pool->thread_count = maxThreads;
    pthread_t _thrd;
    for (int i = 0; i < maxThreads; i++) {
        pthread_create(&_thrd, NULL, thrd_pool_worker, (void*)pool);
        pthread_detach(_thrd);
    }
    return true;
}

void thrd_pool_destroy(struct thrd_pool *pool) {
    if (!pool)
        return;
    pthread_mutex_lock(&pool->work_lock);
    struct thrd_work *work = pool->head;
    while (work) {
        struct thrd_work *tmp = work->next;
        free(work);
        work = tmp;
    }
    pthread_mutex_unlock(&pool->work_lock);
    thrd_pool_join(pool);
    pthread_mutex_destroy(&pool->work_lock);
    pthread_cond_destroy(&pool->work_cond);
    pthread_cond_destroy(&pool->working_cond);
    memset(pool, 0, sizeof(struct thrd_pool));
}

int thrd_pool_push_work(struct thrd_pool *pool, thrd_callback_t func, void *arg) {
    struct thrd_work *work = malloc(sizeof(struct thrd_work));
    work->arg = arg;
    work->func = func;
    work->next = NULL;
    pthread_mutex_lock(&pool->work_lock);
    if (!pool->head) {
        pool->head = work;
        pool->tail = pool->head;
    } else {
        pool->tail->next = work;
        pool->tail = work;
    }
    pthread_cond_broadcast(&pool->work_cond);
    pthread_mutex_unlock(&pool->work_lock);
    return 1;
}

int thrd_pool_push_work_priority(struct thrd_pool *pool, thrd_callback_t func, void *arg) {
    struct thrd_work *work = malloc(sizeof(struct thrd_work));
    work->arg = arg;
    work->func = func;
    work->next = NULL;
    pthread_mutex_lock(&pool->work_lock);
    if (!pool->head) {
        pool->head = work;
        pool->tail = pool->head;
    } else {
        work->next = pool->head;
        pool->head = work;
    }

    pthread_cond_broadcast(&pool->work_cond);
    pthread_mutex_unlock(&pool->work_lock);
    return 1;
}

void thrd_pool_join(struct thrd_pool *pool) {
    pthread_mutex_lock(&pool->work_lock);
    for (;;)
        if (pool->head ||
            (!pool->kill && pool->working_count > 0) ||
            (pool->kill && pool->thread_count > 0))
            pthread_cond_wait(&pool->working_cond, &pool->work_lock);
        else
            break;
    pthread_mutex_unlock(&pool->work_lock);
}

bool thrd_queue_create(struct thrd_queue *queue) {
    if (!queue)
        return false;
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    pthread_mutex_init(&queue->read_lock, NULL);
    pthread_mutex_init(&queue->write_lock, NULL);
    pthread_mutex_lock(&queue->read_lock);
    return true;
}

void thrd_queue_push(struct thrd_queue *queue, void *data) {
    struct thrd_queue_entry *item = malloc(sizeof(struct thrd_queue_entry));
    item->data = data;
    item->next = NULL;

    pthread_mutex_lock(&queue->write_lock);
    if (!queue->head) {
        queue->head = item;
        queue->tail = queue->head;
    } else {
        queue->tail->next = item;
        queue->tail = item;
    }
    queue->count++;

    pthread_mutex_unlock(&queue->read_lock);
    pthread_mutex_unlock(&queue->write_lock);
}

void* thrd_queue_pop(struct thrd_queue *queue) {
    if (!queue->head)
        return NULL;

    pthread_mutex_lock(&queue->read_lock);
    pthread_mutex_lock(&queue->write_lock);

    void *result = queue->head->data;
    struct thrd_queue_entry *tmp = queue->head;
    if (!(queue->head = tmp->next))
        queue->tail = NULL;
    free(tmp);

    if (--queue->count)
        pthread_mutex_unlock(&queue->read_lock);
    pthread_mutex_unlock(&queue->write_lock);
    return result;
}

void thrd_queue_destroy(struct thrd_queue *queue, thrd_callback_t func) {
    if (!queue)
        return;
    pthread_mutex_lock(&queue->read_lock);
    pthread_mutex_lock(&queue->write_lock);
    struct thrd_queue_entry *tmp = queue->head;
    while (tmp) {
        struct thrd_queue_entry *next = tmp->next;
        if (func)
            func(tmp->data);
        free(tmp);
        tmp = next;
    }
    pthread_mutex_unlock(&queue->write_lock);
    pthread_mutex_destroy(&queue->read_lock);
    pthread_mutex_destroy(&queue->write_lock);
    memset(queue, 0, sizeof(struct thrd_queue));
}
