#include "read_index_ele.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "safe_mem_lib.h"
#include "safe_str_lib.h"
#ifdef __cplusplus
}
#endif

indexEle::indexEle()
{
    idx = (bwaidx_fm_t*) calloc(1, sizeof(bwaidx_fm_t));
    assert(idx != NULL);
}

indexEle::~indexEle()
{
    if (idx == 0) return;
    if (idx->mem == 0)
    {
        if (idx->bns) bns_destroy(idx->bns);
        if (idx->pac) free(idx->pac);
    } else {
        free(idx->bns->anns); free(idx->bns);
        if (!idx->is_shm) free(idx->mem);
    }
    free(idx);
}

void indexEle::bwa_idx_load_ele(const char *hint, int which)
{
    char *prefix;
    int l_hint = strlen(hint);
    prefix = (char *) malloc(l_hint + 3 + 4 + 1);
    assert(prefix != NULL);
    strcpy_s(prefix, l_hint + 3 + 4 + 1, hint);

    fprintf(stderr, "* Index prefix: %s\n", prefix);
    
    // idx = (bwaidx_fm_t*) calloc(1, sizeof(bwaidx_fm_t));
    if (which & BWA_IDX_BNS) {
        int i, c;
        idx->bns = bns_restore(prefix);
        if (idx->bns == 0) {
            printf("Error!! : [%s] bns is NULL!!\n", __func__);
            exit(EXIT_FAILURE);
        }
        for (i = c = 0; i < idx->bns->n_seqs; ++i)
            if (idx->bns->anns[i].is_alt) ++c;
        
        fprintf(stderr, "* Read %d ALT contigs\n", c);
        
        if (which & BWA_IDX_PAC)
        {
            idx->pac = (uint8_t*) calloc(idx->bns->l_pac/4+1, 1);
            assert(idx->pac != NULL);
            err_fread_noeof(idx->pac, 1, idx->bns->l_pac/4+1, idx->bns->fp_pac); // concatenated 2-bit encoded sequence
            err_fclose(idx->bns->fp_pac);
            idx->bns->fp_pac = 0;
        }
    }
    free(prefix);
}

#include <sys/file.h>
char* indexEle::bwa_idx_infer_prefix(const char *hint)
{
    char *prefix;
    int l_hint;
    FILE *fp;
    l_hint = strlen(hint);
    prefix = (char *) malloc(l_hint + 3 + 4 + 1);
    assert(prefix != NULL);
    strcpy_s(prefix, l_hint + 3 + 4 + 1, hint);
    strcpy_s(prefix + l_hint, 8, ".64.bwt");
    if ((fp = fopen(prefix, "rb")) != 0)
    {
        fclose(fp);
        prefix[l_hint + 3] = 0;
        return prefix;
    } else {
        strcpy_s(prefix + l_hint, 8, ".bwt");
        if ((fp = fopen(prefix, "rb")) == 0)
        {
            free(prefix);
            return 0;
        } else {
            //flock(fileno(fp), 1);
            //flock(fileno(fp), 1);  // Unlock the file
            fclose(fp);
            prefix[l_hint] = 0;
            return prefix;
        }
    }
}

#if TEST
//int main(int argc, char* argv[])
//{
//  printf("Testing read_index_ele...\n");
//  indexEle *bwaEle = new indexEle();
//  
//  bwaEle->bwa_idx_load_ele("/projects/PCL-GBB/wasim/read_and_ref_data_1/hgaa.fa",
//                          BWA_IDX_ALL);
//
//  delete bwaEle;
//}
#endif
