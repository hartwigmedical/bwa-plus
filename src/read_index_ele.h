#ifndef __INDEX_ELE_HPP
#define __INDEX_ELE_HPP

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
#include "bntseq.h"
#include "macro.h"

#define BWA_IDX_BWT 0x1
#define BWA_IDX_BNS 0x2
#define BWA_IDX_PAC 0x4
#define BWA_IDX_ALL 0x7

typedef struct {
	bntseq_t *bns; // information on the reference sequences
	uint8_t  *pac; // the actual 2-bit encoded reference sequences with 'N' converted to a random base
	int    is_shm;
	int64_t l_mem;
	uint8_t  *mem;
} bwaidx_fm_t;


class indexEle {
	
public:
	bwaidx_fm_t *idx;
	
	indexEle();
	~indexEle();
	void bwa_idx_load_ele(const char *hint, int which);
	char *bwa_idx_infer_prefix(const char *hint);	
};
#endif
