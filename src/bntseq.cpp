#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "bntseq.h"
#include "utils.h"
#include "macro.h"

#include "kseq.h"
KSEQ_DECLARE(gzFile)

#include "khash.h"
KHASH_MAP_INIT_STR(str, int)

#ifdef USE_MALLOC_WRAPPERS
#  include "malloc_wrap.h"
#endif

extern uint64_t tprof[LIM_R][LIM_C];

unsigned char nst_nt4_table[256] = {
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 5 /*'-'*/, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4
};

void bns_dump(const bntseq_t *bns, const char *prefix)
{
	char str[PATH_MAX];
	FILE *fp;
	int i;
    //assert(strlen(prefix) + 4 < 1024);
	{ // dump .ann
		strcpy_s(str, PATH_MAX, prefix); strcat_s(str, PATH_MAX, ".ann");
		fp = xopen(str, "w");
		err_fprintf(fp, "%lld %d %u\n", (long long)bns->l_pac, bns->n_seqs, bns->seed);
		for (i = 0; i != bns->n_seqs; ++i) {
			bntann1_t *p = bns->anns + i;
			err_fprintf(fp, "%d %s", p->gi, p->name);
			if (p->anno[0]) err_fprintf(fp, " %s\n", p->anno);
			else err_fprintf(fp, "\n");
			err_fprintf(fp, "%lld %d %d\n", (long long)p->offset, p->len, p->n_ambs);
		}
		err_fflush(fp);
		err_fclose(fp);
	}
	{ // dump .amb
		strcpy_s(str, PATH_MAX, prefix); strcat_s(str, PATH_MAX, ".amb");
		fp = xopen(str, "w");
		err_fprintf(fp, "%lld %d %u\n", (long long)bns->l_pac, bns->n_seqs, bns->n_holes);
		for (i = 0; i != bns->n_holes; ++i) {
			bntamb1_t *p = bns->ambs + i;
			err_fprintf(fp, "%lld %d %c\n", (long long)p->offset, p->len, p->amb);
		}
		err_fflush(fp);
		err_fclose(fp);
	}
}

bntseq_t *bns_restore_core(const char *ann_filename, const char* amb_filename, const char* pac_filename)
{
	char str[8193];
	FILE *fp;
	const char *fname;
	bntseq_t *bns;
	long long xx;
	int i;
	int scanres;
	bns = (bntseq_t*)calloc(1, sizeof(bntseq_t));
	assert(bns != 0);
	{ // read .ann
		fp = xopen(fname = ann_filename, "r");
		scanres = fscanf(fp, "%lld%d%u", &xx, &bns->n_seqs, &bns->seed);
		assert(bns->n_seqs >= 0 && bns->n_seqs <= INT_MAX);
		if (scanres != 3) goto badread;
		bns->l_pac = xx;
		bns->anns = (bntann1_t*)calloc(bns->n_seqs, sizeof(bntann1_t));
        assert(bns->anns != NULL);
		for (i = 0; i < bns->n_seqs; ++i) {
			bntann1_t *p = bns->anns + i;
			char *q = str;
			int c;
			// read gi and sequence name
			scanres = fscanf(fp, "%u%8192s", &p->gi, str);
			if (scanres != 2) goto badread;
			p->name = strdup(str);
			// read fasta comments 
			while (q - str < sizeof(str) - 1 && (c = fgetc(fp)) != '\n' && c != EOF) *q++ = c;
			while (c != '\n' && c != EOF) c = fgetc(fp);
			if (c == EOF) {
				scanres = EOF;
				goto badread;
			}
			*q = 0;
			assert(strlen(str) < 8192);
			if (q - str > 1 && strcmp(str, " (null)") != 0) p->anno = strdup(str + 1); // skip leading space
			else p->anno = strdup("");
			// read the rest
			scanres = fscanf(fp, "%lld%d%d", &xx, &p->len, &p->n_ambs);
			if (scanres != 3) goto badread;
			p->offset = xx;
		}
		err_fclose(fp);
	}
	{ // read .amb
		int64_t l_pac;
		int32_t n_seqs;
		fp = xopen(fname = amb_filename, "r");
		scanres = fscanf(fp, "%lld%d%d", &xx, &n_seqs, &bns->n_holes);
		assert(bns->n_holes >= 0 && bns->n_holes <= INT_MAX);
		if (scanres != 3) goto badread;
		l_pac = xx;
		xassert(l_pac == bns->l_pac && n_seqs == bns->n_seqs, "inconsistent .ann and .amb files.");
        if(bns->n_holes){
            bns->ambs = (bntamb1_t*)calloc(bns->n_holes, sizeof(bntamb1_t));
            assert(bns->ambs != NULL);
        }
        else{
            bns->ambs = 0;
        }
		for (i = 0; i < bns->n_holes; ++i) {
			bntamb1_t *p = bns->ambs + i;
			scanres = fscanf(fp, "%lld%d%8192s", &xx, &p->len, str);
			if (scanres != 3) goto badread;
			p->offset = xx;
			p->amb = str[0];
		}
		err_fclose(fp);
	}
	{ // open .pac
		bns->fp_pac = xopen(pac_filename, "rb");
	}
	return bns;

 badread:
	if (EOF == scanres) {
		err_fatal(__func__, "Error reading %s : %s\n", fname, ferror(fp) ? strerror(errno) : "Unexpected end of file");
	}
	err_fatal(__func__, "Parse error reading %s\n", fname);
}

bntseq_t *bns_restore(const char *prefix)
{  
	char ann_filename[PATH_MAX], amb_filename[PATH_MAX], pac_filename[PATH_MAX], alt_filename[PATH_MAX];
	FILE *fp;
	bntseq_t *bns;
	//assert(strlen(prefix) + 4 < 1024);
	strcpy_s(ann_filename, PATH_MAX, prefix); strcat_s(ann_filename, PATH_MAX, ".ann");
	strcpy_s(amb_filename, PATH_MAX, prefix); strcat_s(amb_filename, PATH_MAX, ".amb");
	strcpy_s(pac_filename, PATH_MAX, prefix); strcat_s(pac_filename, PATH_MAX, ".pac");
	bns = bns_restore_core(ann_filename, amb_filename, pac_filename);
	if (bns == 0) return 0;
    strcpy_s(alt_filename, PATH_MAX, prefix); strcat_s(alt_filename, PATH_MAX, ".alt");
	if ((fp = fopen(alt_filename, "r")) != 0) { // read .alt file if present
		char str[1024];
		khash_t(str) *h;
		int c, i, absent;
		khint_t k;
		h = kh_init(str);
        assert(h != NULL);
		for (i = 0; i < bns->n_seqs; ++i) {
			k = kh_put(str, h, bns->anns[i].name, &absent);
			kh_val(h, k) = i;
		}
		i = 0;
		while ((c = fgetc(fp)) != EOF) {
			if (c == '\t' || c == '\n' || c == '\r') {
				str[i] = 0;
				if (str[0] != '@') {
					k = kh_get(str, h, str);
					if (k != kh_end(h))
						bns->anns[kh_val(h, k)].is_alt = 1;
				}
				while (c != '\n' && c != EOF) c = fgetc(fp);
				i = 0;
			} else str[i++] = c; // FIXME: potential segfault here
		}
		kh_destroy(str, h);
		fclose(fp);
	}
	return bns;
}

void bns_destroy(bntseq_t *bns)
{
	if (bns == 0) return;
	else {
		int i;
		if (bns->fp_pac) err_fclose(bns->fp_pac);
		free(bns->ambs);
		for (i = 0; i < bns->n_seqs; ++i) {
			free(bns->anns[i].name);
			free(bns->anns[i].anno);
		}
		free(bns->anns);
		free(bns);
	}
}

#define _set_pac(pac, l, c) ((pac)[(l)>>2] |= (c)<<((~(l)&3)<<1))
#define _get_pac(pac, l) ((pac)[(l)>>2]>>((~(l)&3)<<1)&3)

static uint8_t *add1(const kseq_t *seq, bntseq_t *bns, uint8_t *pac, int64_t *m_pac, int *m_seqs, int *m_holes, bntamb1_t **q)
{
	bntann1_t *p;
	int i, lasts;
	if (bns->n_seqs == *m_seqs) {
		*m_seqs <<= 1;
		bns->anns = (bntann1_t*)realloc(bns->anns, *m_seqs * sizeof(bntann1_t));
        assert(bns->anns != NULL);
	}
	p = bns->anns + bns->n_seqs;
	p->name = strdup((char*)seq->name.s);
	p->anno = seq->comment.l > 0? strdup((char*)seq->comment.s) : strdup("(null)");
	p->gi = 0; p->len = seq->seq.l;
	p->offset = (bns->n_seqs == 0)? 0 : (p-1)->offset + (p-1)->len;
	p->n_ambs = 0;
	for (i = lasts = 0; i < seq->seq.l; ++i) {
		int c = nst_nt4_table[(int)seq->seq.s[i]];
		if (c >= 4) { // N
			if (lasts == seq->seq.s[i]) { // contiguous N
				++(*q)->len;
			} else {
				if (bns->n_holes == *m_holes) {
					(*m_holes) <<= 1;
					bns->ambs = (bntamb1_t*)realloc(bns->ambs, (*m_holes) * sizeof(bntamb1_t));
				}
				*q = bns->ambs + bns->n_holes;
				(*q)->len = 1;
				(*q)->offset = p->offset + i;
				(*q)->amb = seq->seq.s[i];
				++p->n_ambs;
				++bns->n_holes;
			}
		}
		lasts = seq->seq.s[i];
		{ // fill buffer
			if (c >= 4) c = lrand48()&3;
			if (bns->l_pac == *m_pac) { // double the pac size
				*m_pac <<= 1;
				pac = (uint8_t*) realloc(pac, *m_pac/4);
				memset(pac + bns->l_pac/4, 0, (*m_pac - bns->l_pac)/4);
			}
			_set_pac(pac, bns->l_pac, c);
			++bns->l_pac;
		}
	}
	++bns->n_seqs;
	return pac;
}

int64_t bns_fasta2bntseq(gzFile fp_fa, const char *prefix, int for_only)
{
	extern void seq_reverse(int len, ubyte_t *seq, int is_comp); // in bwaseqio.c
	kseq_t *seq;
	char name[PATH_MAX];
	bntseq_t *bns;
	uint8_t *pac = 0;
	int32_t m_seqs, m_holes;
	int64_t ret = -1, m_pac, l;
	bntamb1_t *q;
	FILE *fp;

	// initialization
	seq = kseq_init(fp_fa);
	bns = (bntseq_t*)calloc(1, sizeof(bntseq_t));
    assert(bns != NULL);
	bns->seed = 11; // fixed seed for random generator
	srand48(bns->seed);
	m_seqs = m_holes = 8; m_pac = 0x10000;
	bns->anns = (bntann1_t*)calloc(m_seqs, sizeof(bntann1_t));
    assert(bns->anns != NULL);
	bns->ambs = (bntamb1_t*)calloc(m_holes, sizeof(bntamb1_t));
    assert(bns->ambs != NULL);
	pac = (uint8_t*) calloc(m_pac/4, 1);
	if (pac == NULL) { perror("Allocation of pac failed"); exit(EXIT_FAILURE); }
	q = bns->ambs;
	//assert(strlen(prefix) + 4 < 1024);
	strcpy_s(name, PATH_MAX, prefix); strcat_s(name, PATH_MAX, ".pac");
	fp = xopen(name, "wb");
	// read sequences
	while (kseq_read(seq) >= 0) pac = add1(seq, bns, pac, &m_pac, &m_seqs, &m_holes, &q);
	if (!for_only) { // add the reverse complemented sequence
		m_pac = (bns->l_pac * 2 + 3) / 4 * 4;
		pac = (uint8_t*) realloc(pac, m_pac/4);
		if (pac == NULL) { perror("Reallocation of pac failed"); exit(EXIT_FAILURE); }
		memset(pac + (bns->l_pac+3)/4, 0, (m_pac - (bns->l_pac+3)/4*4) / 4);
		for (l = bns->l_pac - 1; l >= 0; --l, ++bns->l_pac)
			_set_pac(pac, bns->l_pac, 3-_get_pac(pac, l));
	}
	ret = bns->l_pac;
	{ // finalize .pac file
		ubyte_t ct;
		err_fwrite(pac, 1, (bns->l_pac>>2) + ((bns->l_pac&3) == 0? 0 : 1), fp);
		// the following codes make the pac file size always (l_pac/4+1+1)
		if (bns->l_pac % 4 == 0) {
			ct = 0;
			err_fwrite(&ct, 1, 1, fp);
		}
		ct = bns->l_pac % 4;
		err_fwrite(&ct, 1, 1, fp);
		// close .pac file
		err_fflush(fp);
		err_fclose(fp);
	}
	bns_dump(bns, prefix);
	bns_destroy(bns);
	kseq_destroy(seq);
	free(pac);
	return ret;
}

int bwa_fa2pac(int argc, char *argv[])
{
	int c, for_only = 0;
	gzFile fp;
	while ((c = getopt(argc, argv, "f")) >= 0) {
		switch (c) {
			case 'f': for_only = 1; break;
		}
	}
	if (argc == optind) {
		fprintf(stderr, "Usage: bwa fa2pac [-f] <in.fasta> [<out.prefix>]\n");
		return 1;
	}
	fp = xzopen(argv[optind], "r");
	bns_fasta2bntseq(fp, (optind+1 < argc)? argv[optind+1] : argv[optind], for_only);
	err_gzclose(fp);
	return 0;
}

int bns_pos2rid(const bntseq_t *bns, int64_t pos_f)
{
	int left, mid, right;
	if (pos_f >= bns->l_pac) return -1;
	left = 0; mid = 0; right = bns->n_seqs;
	while (left < right) { // binary search
		mid = (left + right) >> 1;
		if (pos_f >= bns->anns[mid].offset) {
			if (mid == bns->n_seqs - 1) break;
			if (pos_f < bns->anns[mid+1].offset) break; // bracketed
			left = mid + 1;
		} else right = mid;
	}
	return mid;
}

int bns_intv2rid(const bntseq_t *bns, int64_t rb, int64_t re)
{
	int is_rev, rid_b, rid_e;
	if (rb < bns->l_pac && re > bns->l_pac) return -2;
	assert(rb <= re);
	rid_b = bns_pos2rid(bns, bns_depos(bns, rb, &is_rev));
	rid_e = rb < re? bns_pos2rid(bns, bns_depos(bns, re - 1, &is_rev)) : rid_b;
	return rid_b == rid_e? rid_b : -1;
}

int bns_cnt_ambi(const bntseq_t *bns, int64_t pos_f, int len, int *ref_id)
{
	int left, mid, right, nn;
	if (ref_id) *ref_id = bns_pos2rid(bns, pos_f);
	left = 0; right = bns->n_holes; nn = 0;
	while (left < right) {
		mid = (left + right) >> 1;
		if (pos_f >= bns->ambs[mid].offset + bns->ambs[mid].len) left = mid + 1;
		else if (pos_f + len <= bns->ambs[mid].offset) right = mid;
		else { // overlap
			if (pos_f >= bns->ambs[mid].offset) {
				nn += bns->ambs[mid].offset + bns->ambs[mid].len < pos_f + len?
					bns->ambs[mid].offset + bns->ambs[mid].len - pos_f : len;
			} else {
				nn += bns->ambs[mid].offset + bns->ambs[mid].len < pos_f + len?
					bns->ambs[mid].len : len - (bns->ambs[mid].offset - pos_f);
			}
			break;
		}
	}
	return nn;
}

uint8_t *bns_get_seq(int64_t l_pac, const uint8_t *pac, int64_t beg, int64_t end, int64_t *len)
{
	uint8_t *seq = 0;
	if (end < beg) end ^= beg, beg ^= end, end ^= beg; // if end is smaller, swap
	if (end > l_pac<<1) end = l_pac<<1;
	if (beg < 0) beg = 0;
	if (beg >= l_pac || end <= l_pac) {
		int64_t k, l = 0;
		*len = end - beg;
		seq = (uint8_t*) malloc(end - beg + 64);		
        assert(seq != NULL);
		if (beg >= l_pac) { // reverse strand
			int64_t beg_f = (l_pac<<1) - 1 - end;
			int64_t end_f = (l_pac<<1) - 1 - beg;
			for (k = end_f; k > beg_f; --k) {
				seq[l++] = 3 - _get_pac(pac, k);
			}
		} else { // forward strand
			for (k = beg; k < end; ++k) {
				seq[l++] = _get_pac(pac, k);
			}
		}
	} else *len = 0; // if bridging the forward-reverse boundary, return nothing
	return seq;
}

uint8_t *bns_fetch_seq(const bntseq_t *bns, const uint8_t *pac, int64_t *beg, int64_t mid, int64_t *end, int *rid)
{
	int64_t far_beg, far_end, len;
	int is_rev;
	uint8_t *seq;

	if (*end < *beg) *end ^= *beg, *beg ^= *end, *end ^= *beg; // if end is smaller, swap
	// printf("%ld %ld %ld\n", *beg, mid, *end);
	assert(*beg <= mid && mid < *end);
	
	*rid = bns_pos2rid(bns, bns_depos(bns, mid, &is_rev));
	far_beg = bns->anns[*rid].offset;
	far_end = far_beg + bns->anns[*rid].len;
	if (is_rev) { // flip to the reverse strand
		int64_t tmp = far_beg;
		far_beg = (bns->l_pac<<1) - far_end;
		far_end = (bns->l_pac<<1) - tmp;
	}
	*beg = *beg > far_beg? *beg : far_beg;
	*end = *end < far_end? *end : far_end;

	seq = bns_get_seq(bns->l_pac, pac, *beg, *end, &len);
	
	if (seq == 0 || *end - *beg != len) {
		fprintf(stderr, "[E::%s] begin=%ld, mid=%ld, end=%ld, len=%ld, seq=%p, rid=%d, far_beg=%ld, far_end=%ld\n",
				__func__, (long)*beg, (long)mid, (long)*end, (long)len, seq, *rid, (long)far_beg, (long)far_end);
	}
	assert(seq && *end - *beg == len); // assertion failure should never happen
	return seq;
}
