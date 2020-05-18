#include "hotspot.h"

void release_wait()
{
    FILE *fp;

    fp = fopen(HOTSPOT_RELEASE, "w");
    fclose(fp);
    while (!access(HOTSPOT_RELEASE, F_OK))
        sleep(1);
}


void hotspot(char *shmbuf, int shmsz, int id, bool random, int rw_ratio)
{
    int i;
    int j;
    int size;
    int start;

    srand((unsigned)time(0) + (unsigned)id);
    for (i = 0; i < HOTSPOT_ROUNDS; i++) {
        start = 0;
        size = shmsz;
        hotspot_log("round=%d\n", i);
        while (size > PAGE_SIZE) {
            int rwr = rand() % (rw_ratio + 1);

            size /= 2;
            if (rand() % 2)
                start += size;
            
            if (random) {
                int pos;
                int total;

                pos = rand() % size;
                total = rand() % size;
                if (!rwr) {
                    for (j = 0; j < total; j++) {
                        shmbuf[start + pos] = rand() & 0xff;
                        pos++;
                        if (pos == size)
                            pos = 0;
                    }
                } else {
                    for (j = 0; j < total; j++) {
                        if (shmbuf[start + pos]);
                        pos++;
                        if (pos == size)
                            pos = 0;
                    }
                }
            } else {
                if (!rwr) {
                    for (j = 0; j < size; j++)
                        shmbuf[start + j] = rand() & 0xff;
                } else {
                    for (j = 0; j < size; j++)
                        if (shmbuf[start + j]);
                }
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
    filp = fopen(HOTSPOT_RESULT, "w");
    fprintf(filp, "%f", result);
    fclose(filp);
}


int start_leader(int id, int total, int shmsz, bool random, int rw_ratio)
{
    int desc;
    int ret = -1;
    char *shmbuf;
    float result;
    struct timeval time_start, time_end;

    hotspot_log("leader: step 0, preparing ...\n");
    desc = shmget(KEY_MAIN, shmsz, IPC_DIPC | IPC_CREAT);
    if (desc < 0)
        goto out;
    shmbuf = shmat(desc, NULL, 0);
    if (shmbuf == (char *)-1)
        goto release;
    memset(shmbuf, 0, shmsz);
    barrier_wait(BARRIER_TEST, total);
    hotspot_log("leader: step 1, testing ...\n");
    gettimeofday(&time_start, NULL);
    hotspot(shmbuf, shmsz, id, random, rw_ratio);
    hotspot_log("leader: step 2, waiting ...\n");
    barrier_wait(BARRIER_END, total);
    gettimeofday(&time_end, NULL);
    hotspot_log("leader: step 3, save result\n");
    result = time_end.tv_sec - time_start.tv_sec + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
    save_result(result);
    hotspot_log("leader: step 4, release\n");
    release_wait();
    shmdt(shmbuf);
    ret = 0;
release:
    shmctl(desc, IPC_RMID, NULL);
out:
    if (ret < 0)
        printf("leader: failed to start\n");
    return ret;
}


int start_member(int id, int total, int shmsz, bool random, int rw_ratio)
{
    int desc;
    float result;
    char *shmbuf;
    struct timeval time_start, time_end;

    hotspot_log("member: step 0, preparing ...\n");
    while ((desc = shmget(KEY_MAIN, shmsz, IPC_DIPC)) < 0)
        sleep(1);
    shmbuf = shmat(desc, NULL, 0);
    if (shmbuf == (char *)-1)
        goto err;
    barrier_wait(BARRIER_TEST, total);
    hotspot_log("member: step 1, testing ...\n");
    gettimeofday(&time_start, NULL);
    hotspot(shmbuf, shmsz, id, random, rw_ratio);
    hotspot_log("member: step 2, waiting ...\n");
    barrier_wait(BARRIER_END, total);
    gettimeofday(&time_end, NULL);
    result = time_end.tv_sec - time_start.tv_sec + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
    hotspot_log("member: step 3, save result\n");
    save_result(result);
    hotspot_log("member: step 4, release\n");
    release_wait();
    return 0;
err:
    printf("member: failed to start\n");
    return -1;
}


int start(int total, int shmsz, bool random, int rw_ratio)
{
    int id;

    if (total <= 0) {
        printf("invalid patameters\n");
        return -1;
    }
    id = barrier_wait(BARRIER_START, total);
    if ((id < 0) || (id >= total)) {
        printf("invalid id\n");
        return -1;
    }
    if (0 == id) {
        if (start_leader(id, total, shmsz, random, rw_ratio))
            return -1;
    } else {
        if (start_member(id, total, shmsz, random, rw_ratio))
            return -1;
    }
    return 0;
}


void usage()
{
    printf("Usage: hotspot [-r] [-n nodes] [-s size] [-w read-to-write-ratio]\n");
    printf("-r: random test\n");
    printf("-n: the number of nodes\n");
    printf("-s: the size of shared-memory\n");
    printf("-w: setting workload in terms of a read to write ratio\n");
}


int main(int argc, char **argv)
{
    char ch;
    int total = 0;
    bool random = false;
    int size = HOTSPOT_SIZE;
    int rw_ratio = HOTSPOT_RW_RATIO;
    
    while ((ch = getopt(argc, argv, "n:w:s:rh")) != -1) {
        switch(ch) {
        case 'n':
            total = atoi(optarg);
            break;
        case 's':
            size = atoi(optarg);
            break;
        case 'r':
            random = true;
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
    return start(total, size, random, rw_ratio);
}
