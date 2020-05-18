#include "barrier.h"
#include "rap.h"

void release_wait()
{
    FILE *fp;

    fp = fopen(RAP_RELEASE, "w");
    fclose(fp);
    while (!access(RAP_RELEASE, F_OK))
        sleep(1);
}


void free_pages(rap_pages_t *pages)
{
    shmdt(pages->buf);
    shmctl(pages->desc, IPC_RMID, NULL);
    free(pages);
}


rap_pages_t *alloc_pages(int total)
{
    rap_pages_t *pages = (rap_pages_t *)malloc(sizeof(rap_pages_t));

    if (!pages) {
        printf("failed to allocate pages");
        return NULL;
    }
    memset(pages, 0, sizeof(rap_pages_t));
    if ((pages->desc = shmget(KEY_MAIN, RAP_PAGES * RAP_PAGE_SIZE, IPC_DIPC | IPC_CREAT)) < 0) {
        free(pages);
        return NULL;
    }
    pages->buf = shmat(pages->desc, NULL, 0);
    if (pages->buf == (char *)-1) {
        shmctl(pages->desc, IPC_RMID, NULL);
        free(pages);
        return NULL;
    }
    return pages;
}


void random_walk(int id, rap_pages_t *pages, int nr_pages, int rw_ratio)
{
    int i;
    int j;

    srand((unsigned)time(0) + (unsigned)id);
    for (i = 0; i < RAP_ROUNDS; i++) {
        int pg = rand() % nr_pages;
        int len = rand() % RAP_PAGE_SIZE;
        int start = rand() % RAP_PAGE_SIZE;
        char *ptr = &pages->buf[pg * RAP_PAGE_SIZE];

        if (rand() % (rw_ratio + 1)) {
            for (j = 0; j < len; j++) {
                if (ptr[start++]);
                if (start == RAP_PAGE_SIZE)
                    start = 0;
            }
        } else {
            for (j = 0; j < len; j++) {
                ptr[start++] = rand() & 0xff;
                if (start == RAP_PAGE_SIZE)
                    start = 0;
            }
        }
    }
}


void save_result(float result)
{
    FILE *filp;

#ifdef SHOW_RESULT
    printf("result=%f\n", result);
#endif
    filp = fopen(RAP_RESULT, "w");
    fprintf(filp, "%f", result);
    fclose(filp);
}


int start_test(int total, int nr_pages, int rw_ratio)
{
    int id;
    float result;
    rap_pages_t *pages;
    struct timeval time_start, time_end;

    rap_log("step 0, preparing ...\n");
    pages = alloc_pages(total);
    if (!pages)
        goto out;
    id = barrier_wait(BARRIER_START, total);
    rap_log("step 1, testing ...\n");
    gettimeofday(&time_start, NULL);
    random_walk(id, pages, nr_pages, rw_ratio);
    rap_log("step 2, waiting ...\n");
    barrier_wait(BARRIER_END, total);
    gettimeofday(&time_end, NULL);
    rap_log("step 3, save result\n");
    result = time_end.tv_sec - time_start.tv_sec + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
    save_result(result);
    rap_log("step 4, release\n");
    release_wait();
    free_pages(pages);
    return 0;
out:
    printf("failed to start\n");
    return -1;
}


void usage()
{
    printf("Usage: rap [-n nodes] [-p pages] [-w read-to-write-ratio]\n");
    printf("-n: the number of nodes\n");
    printf("-p: the number of 4KB pages for accessing\n");
    printf("-w: setting workload in terms of a read to write ratio\n");
}


int main(int argc, char **argv)
{
    char ch;
    int total = 0;
    int nr_pages = RAP_PAGES;
    int rw_ratio = RAP_RW_RATIO;

    while ((ch = getopt(argc, argv, "n:p:w:h")) != -1) {
        switch(ch) {
        case 'n':
            total = atoi(optarg);
            break;
        case 'p':
            nr_pages = atoi(optarg);
            break;
        case 'w':
            rw_ratio = atoi(optarg);
            break;
        default:
            usage();
            exit(1);
        }
    }
    if (!total) {
        usage();
        exit(1);
    }
    return start_test(total, nr_pages, rw_ratio);
}
