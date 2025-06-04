#ifndef KTHREAD_HPP
#define KTHREAD_HPP

#include <stdint.h>
#include "macro.h"
#include "bwamem.h"
#include <pthread.h>

// ----------------
//  kt_pipeline() 
// -------------------

struct ktp_t;
struct mem_opt_t;
struct worker_t;

typedef struct {
	struct ktp_t *pl;
	int64_t index;
	int step;
	void *data;
	worker_t *w;
	mem_opt_t *opt;
	int i;
} ktp_worker_t;

typedef struct ktp_t {
	void *shared;
	int (*func)(void*);
	int64_t index;
	int n_workers, n_steps;
	ktp_worker_t *workers;
	pthread_mutex_t mutex;
	pthread_cond_t cv;
} ktp_t;

// ---------------
// kt_for() 
// ---------------

struct kt_for_t;

typedef struct {
	struct kt_for_t *t;
	long i;
} ktf_worker_t;

typedef struct kt_for_t {
	int n_threads;
	long n;
	ktf_worker_t *w;
	void (*func)(void*, int, int, int);
	void *data;
} kt_for_t;


void kt_pipeline(int n_threads, int (*func)(void*), void *shared_data, int n_steps);
void kt_for(void (*func)(void*,int,int,int), void *data, int n);
#endif
