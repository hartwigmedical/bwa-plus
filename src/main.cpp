// ----------------------------------
#include "main.h"

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "1.0.0"
#endif


// ----------------------------------
uint64_t proc_freq, tprof[LIM_R][LIM_C], prof[LIM_R];
// ----------------------------------

int usage()
{
    fprintf(stderr, "Usage: bwa-plus <command> <arguments>\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  index         create index\n");
    fprintf(stderr, "  mem           alignment\n");
    fprintf(stderr, "  version       print version number\n");
    return 1;
}

int main(int argc, char* argv[])
{
        
    // ---------------------------------    
    uint64_t tim = __rdtsc();
    sleep(1);
    proc_freq = __rdtsc() - tim;

    int ret = -1;
    if (argc < 2) return usage();

    if (strcmp(argv[1], "index") == 0)
    {
         uint64_t tim = __rdtsc();
         ret = bwa_index(argc-1, argv+1);
         fprintf(stderr, "Total time taken: %0.4lf\n", (__rdtsc() - tim)*1.0/proc_freq);
         return ret;
    }
    else if (strcmp(argv[1], "mem") == 0)
    {
        tprof[MEM][0] = __rdtsc();
        kstring_t pg = {0,0,0};
        extern char *bwa_pg;

        fprintf(stderr, "-----------------------------\n");
#if __AVX512BW__
        fprintf(stderr, "Executing in AVX512 mode!!\n");
#elif __AVX2__
        fprintf(stderr, "Executing in AVX2 mode!!\n");
#elif __AVX__
        fprintf(stderr, "Executing in AVX mode!!\n");        
#elif __SSE4_2__
        fprintf(stderr, "Executing in SSE4.2 mode!!\n");
#elif __SSE4_1__
        fprintf(stderr, "Executing in SSE4.1 mode!!\n");        
#endif
        fprintf(stderr, "-----------------------------\n");

        #if SA_COMPRESSION
        fprintf(stderr, "* SA compression enabled with xfactor: %d\n", 0x1 << SA_COMPX);
        #endif
        
        ksprintf(&pg, "@PG\tID:bwa-plus\tPN:bwa-plus\tVN:%s\tCL:%s", PACKAGE_VERSION, argv[0]);

        for (int i = 1; i < argc; ++i) ksprintf(&pg, " %s", argv[i]);
        ksprintf(&pg, "\n");
        bwa_pg = pg.s;
        ret = main_mem(argc-1, argv+1);
        free(bwa_pg);
        
        /** Enable this return to avoid printing of the runtime profiling **/
        //return ret;
    }
    else if (strcmp(argv[1], "version") == 0)
    {
        puts(PACKAGE_VERSION);
        return 0;
    } else {
        fprintf(stderr, "ERROR: unknown command '%s'\n", argv[1]);
        return 1;
    }

    if (ret == 0) {
        fprintf(stderr, "\nImportant parameter settings: \n");
        fprintf(stderr, "\tBATCH_SIZE: %d\n", BATCH_SIZE);
        fprintf(stderr, "\tMAX_SEQ_LEN_REF: %d\n", MAX_SEQ_LEN_REF);
        fprintf(stderr, "\tMAX_SEQ_LEN_QER: %d\n", MAX_SEQ_LEN_QER);
        fprintf(stderr, "\tMAX_SEQ_LEN8: %d\n", MAX_SEQ_LEN8);
        fprintf(stderr, "\tSEEDS_PER_READ: %d\n", SEEDS_PER_READ);
        fprintf(stderr, "\tSIMD_WIDTH8 X: %d\n", SIMD_WIDTH8);
        fprintf(stderr, "\tSIMD_WIDTH16 X: %d\n", SIMD_WIDTH16);
        fprintf(stderr, "\tAVG_SEEDS_PER_READ: %d\n", AVG_SEEDS_PER_READ);
    }
    
    return ret;
}
