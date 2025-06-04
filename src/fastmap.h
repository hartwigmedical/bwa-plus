#ifndef FASTMAP_HPP
#define FASTMAP_HPP

#include <ctype.h>
#include <zlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include <fstream>
#include "bwa.h"
#include "bwamem.h"
#include "kthread.h"
#include "kvec.h"
#include "utils.h"
#include "bntseq.h"
#include "kseq.h"
#include "profiling.h"

KSEQ_DECLARE(gzFile)

typedef struct {
	kseq_t *ks, *ks2;
	mem_opt_t *opt;
	mem_pestat_t *pes0;
	int64_t n_processed;
	int copy_comment;
	int64_t my_ntasks;
	int64_t ntasks;
	int64_t task_size;
	int64_t actual_chunk_size;
	FILE *fp;
	uint8_t *ref_string;
	FMI_search *fmi;	
} ktp_aux_t;

typedef struct {
	ktp_aux_t *aux;
	int n_seqs;
	bseq1_t *seqs;
} ktp_data_t;

    
void *kopen(const char *fn, int *_fd);
int kclose(void *a);
int main_mem(int argc, char *argv[]);

#endif
