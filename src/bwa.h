#ifndef BWA_H_
#define BWA_H_

#include <stdint.h>
#include <zlib.h>
#include "bntseq.h"
#include "bwt.h"
#include "macro.h"

#define BWA_IDX_BWT 0x1
#define BWA_IDX_BNS 0x2
#define BWA_IDX_PAC 0x4
#define BWA_IDX_ALL 0x7

#define BWA_CTL_SIZE 0x10000

typedef struct {
	// bwt2_t   *bwt2;
	bwt_t    *bwt; // FM-index
	bntseq_t *bns; // information on the reference sequences
	uint8_t  *pac; // the actual 2-bit encoded reference sequences with 'N' converted to a random base

	int    is_shm;
	int64_t l_mem;
	uint8_t  *mem;
} bwaidx_t;


typedef struct {
	int l_seq, id;
	char *name, *comment, *seq, *qual, *sam;
} bseq1_t;

extern int bwa_verbose;
extern char bwa_rg_id[256];

#ifdef __cplusplus
extern "C" {
#endif
    bseq1_t *bseq_read_orig(int64_t chunk_size, int *n_, void *ks1_, void *ks2_, int64_t *s);

    bseq1_t *bseq_read(int64_t chunk_size, int *n_, void *ks1_,
                       void *ks2_, FILE* fpp, int len,
                       int64_t *sz);
    
    bseq1_t *bseq_read_one_fasta_file(int64_t chunk_size, int *n_, gzFile fp, int64_t *s);
    
    void bseq_classify(int n, bseq1_t *seqs, int m[2], bseq1_t *sep[2]);
    
	void bwa_fill_scmat(int a, int b, int8_t mat[25]);
    
	uint32_t *bwa_gen_cigar(const int8_t mat[25], int q, int r, int w_,
							int64_t l_pac, const uint8_t *pac, int l_query,
							uint8_t *query, int64_t rb, int64_t re,
							int *score, int *n_cigar, int *NM);
	
	uint32_t *bwa_gen_cigar2(const int8_t mat[25], int o_del, int e_del,
							 int o_ins, int e_ins, int w_, int64_t l_pac,
							 const uint8_t *pac, int l_query, uint8_t *query,
							 int64_t rb, int64_t re, int *score,
							 int *n_cigar, int *NM);

	int bwa_idx_build(const char *fa, const char *prefix);

	char *bwa_idx_infer_prefix(const char *hint);
	bwt_t *bwa_idx_load_bwt(const char *hint);
	bwt2_t *bwa_idx_load_bwt2(const char *hint);
	
	bwaidx_t *bwa_idx_load_from_shm(const char *hint);
	bwaidx_t *bwa_idx_load_from_disk(const char *hint, int which);
	bwaidx_t *bwa_idx_load(const char *hint, int which);
	
	void bwa_idx_destroy(bwaidx_t *idx);
	void bwa_print_sam_hdr(const bntseq_t *bns, const char *hdr_line, FILE *fp);
	char *bwa_set_rg(const char *s);
	char *bwa_insert_header(const char *s, char *hdr);
#ifdef __cplusplus
}
#endif

#endif
