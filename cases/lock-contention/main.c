#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t wait_lock;
pthread_cond_t wait_cond;

pthread_mutex_t counter_lock;
pthread_cond_t counter_cond;
static unsigned counter;

static void dec_counter(void)
{
    pthread_mutex_lock(&counter_lock);
    while (counter == 0) {
        fprintf(stderr, "B running->block\n");
        pthread_cond_wait(&counter_cond, &counter_lock);
        fprintf(stderr, "B try to wake up A\n");
        pthread_cond_signal(&wait_cond);
    }
    counter -= 1;
    pthread_mutex_unlock(&counter_lock);
}

static void inc_counter(void)
{
    pthread_mutex_lock(&counter_lock);
    if (counter == 0) {
        sleep(10);
        fprintf(stderr, "C try to wake up B\n");
        pthread_cond_signal(&counter_cond);
    }
    counter += 1;
    pthread_mutex_unlock(&counter_lock);
}

void *thread_wait(void *arg)
{
    long max = (long)arg;
    long i;

    while(1) {
        fprintf(stderr, "pthread A runnning->block\n");
        pthread_cond_wait(&wait_cond, &wait_lock);
        fprintf(stderr, "pthread A block->runnable\n");
        for (i = 0; i < max; i++) {
            ;
        }
    }
}

void *thread_main(void *arg)
{
    long i = (long)arg;
    if (i == 0) {
        while (1) {
            dec_counter();
        }
    } else {
        while (1) {
            inc_counter();
        }
    }
    return NULL;
}

int main(void)
{
    pthread_t t[2];
    pthread_t wait;
    long wait_wl = 10000;

    long i;
    for (i = 0; i < (long)2; i++) {
        pthread_create(&t[i], NULL, thread_main, (void*)i);
        pthread_setname_np(t[i], i == 0 ? "B" : "C");
    }

    pthread_create(&wait, NULL, thread_wait, (void*)wait_wl);
    pthread_setname_np(wait, "A");

    for (i = 0; i < 2; i++) {
        pthread_join(t[i], NULL);
    }

    pthread_join(wait, NULL);

    pthread_cond_destroy(&counter_cond);
    pthread_cond_destroy(&wait_cond);

    return 0;
}
