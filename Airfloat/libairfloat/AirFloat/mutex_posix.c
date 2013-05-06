//
//  mutex_posix.c
//  AirFloat
//
//  Copyright (c) 2013, Kristian Trenskow All rights reserved.
//
//  Redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following
//  conditions are met:
//
//  Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//  Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution. THIS SOFTWARE IS PROVIDED BY THE
//  COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
//  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
//  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
//  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#if (__APPLE__)

#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

#include "log.h"
#include "thread.h"
#include "mutex.h"

#define MAX_NAME_LENGTH 250

struct mutex_t {
    pthread_mutex_t mutex;
    bool locked;
    char name[MAX_NAME_LENGTH];
    thread_p destroy_thread;
    pthread_cond_t destroy_condition;
    pthread_mutex_t destroy_mutex;
};

struct mutex_t* mutex_create() {
    
    struct mutex_t* m = (struct mutex_t*)malloc(sizeof(struct mutex_t));
    bzero(m, sizeof(struct mutex_t));
    
    pthread_mutex_init(&m->mutex, NULL);
    
    return m;
    
}

void _mutex_destroy(struct mutex_t* m) {
    
    pthread_mutex_destroy(&m->mutex);
    free(m);
    
}

void _mutex_destroy_delayed(void* ctx) {
    
    struct mutex_t* m = (struct mutex_t*)ctx;
    
    struct timeval tv;
    
    gettimeofday(&tv, NULL);

#if DEBUG
    tv.tv_sec += 10;
#else
    tv.tv_sec++;
#endif
    
    struct timespec req = {tv.tv_sec, tv.tv_usec * 1000};
    
    pthread_mutex_lock(&m->destroy_mutex);
    pthread_cond_timedwait(&m->destroy_condition, &m->destroy_mutex, &req);
    pthread_mutex_unlock(&m->destroy_mutex);
    
    pthread_cond_destroy(&m->destroy_condition);
    pthread_mutex_destroy(&m->destroy_mutex);
    
    thread_p thread = m->destroy_thread;
    
    _mutex_destroy(m);
    
    thread_destroy(thread);
    
    log_message(LOG_INFO, "Mutex %p was destroyed after delay.", m);
    
}

void mutex_destroy(struct mutex_t* m, bool delay) {
    
    if (delay) {
        
        pthread_cond_init(&m->destroy_condition, NULL);
        pthread_mutex_init(&m->destroy_mutex, NULL);
        
        m->destroy_thread = thread_create(_mutex_destroy_delayed, m);
        
    } else
        _mutex_destroy(m);
    
}

bool mutex_trylock(struct mutex_t* m) {
    
    return (pthread_mutex_trylock(&m->mutex) == 0);
    
}

void mutex_lock(struct mutex_t* m) {
    
    if (m != NULL) {
        pthread_mutex_lock(&m->mutex);
#ifdef DEBUG
        pthread_getname_np(pthread_self(), m->name, MAX_NAME_LENGTH);
        m->locked = true;
#endif
    }
    
}

void mutex_unlock(struct mutex_t* m) {
    
    if (m != NULL) {
#ifdef DEBUG
        m->locked = false;
        m->name[0] = '\0';
#endif
        pthread_mutex_unlock(&m->mutex);
    }
    
}

pthread_mutex_t* mutex_pthread(struct mutex_t* m) {
    
    return &m->mutex;
    
}

#endif
