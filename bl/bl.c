#include "bl.h"
#include "vlock.h"
#include "barrier.h"

void msleep(int msec)
{
    usleep(msec * 1000);
}


void release_wait()
{
    FILE *fp;

    fp = fopen(BL_RELEASE, "w");
    fclose(fp);
    while (!access(BL_RELEASE, F_OK))
        sleep(1);
}


void bl(int id, char *shmbuf, char *lock_vec, int blocks, size_t size, float ep)
{
    int i;
    int j;
    int pos;
    bool fresh = false;
    bool first = false;

    srand((unsigned)time(0) + (unsigned)id);
    pos = rand() % blocks;
    for (i = 0; i < BL_ROUNDS; i++) {
        int start = pos * size;

        bl_log("round=%d (blk=%d/%d)\n", i, pos, blocks);
        vlock_lock(id, lock_vec, pos, &first, &fresh);
        if (first) {
            for (j = 0; j < size; j++)
                shmbuf[start++] = rand() & 0xff;
        } else if (fresh) {
            for (j = 0; j < size; j++)
                if (shmbuf[start++]);
        }
        vlock_unlock(id, lock_vec, pos);
        if ((double)rand() / RAND_MAX < ep)
            pos = (pos + 1) % blocks;
        else
            pos = rand() % blocks;
    }
}


void save_result(float result)
{
    FILE *filp;

#ifdef SHOW_RESULT
    printf("result=%f\n", result);
#endif
    filp = fopen(BL_RESULT, "w");
    fprintf(filp, "%f", result);
    fclose(filp);
}


int start_leader(int id, int total, int blocks, size_t size, float ep)
{
    int desc;
    int ret = -1;
    char *shmbuf;
    float result;
    int desc_lock;
    void *lock_vec;
    size_t shmsz = blocks * size;
    struct timeval time_start, time_end;

    bl_log("leader: step 0, preparing ...\n");
    desc = shmget(KEY_MAIN, shmsz, IPC_DIPC | IPC_CREAT);
    if (desc < 0)
        goto out;
    shmbuf = shmat(desc, NULL, 0);
    if (shmbuf == (char *)-1)
        goto release;
    memset(shmbuf, 0, shmsz);
    msleep(GUARD_TIME);
    desc_lock = vlock_create(KEY_LOCK, total, blocks);
    if (desc_lock < 0)
        goto release_lock;
    lock_vec = vlock_init(desc_lock, total, blocks);
    if (lock_vec == (void *)-1)
        goto release_lock;
    msleep(GUARD_TIME);
    barrier_wait(BARRIER_TEST, total);
    bl_log("leader: step 1, testing ...\n");
    gettimeofday(&time_start, NULL);
    bl(id, shmbuf, lock_vec, blocks, size, ep);
    bl_log("leader: step 2, waiting ...\n");
    barrier_wait(BARRIER_END, total);
    gettimeofday(&time_end, NULL);
    bl_log("leader: step 3, save result\n");
    result = time_end.tv_sec - time_start.tv_sec + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
    save_result(result);
    bl_log("leader: step 4, release\n");
    release_wait();
    shmdt(shmbuf);
    ret = 0;
release_lock:
    vlock_release(desc_lock, lock_vec);
release:
    shmctl(desc, IPC_RMID, NULL);
out:
    if (ret < 0)
        printf("leader: failed to start\n");
    return ret;
}


int start_member(int id, int total, int blocks, size_t size, float ep)
{
    int desc;
    float result;
    char *shmbuf;
    void *lock_vec;
    size_t shmsz = blocks * size;
    struct timeval time_start, time_end;

    bl_log("member: step 0, preparing ...\n");
    while ((desc = shmget(KEY_MAIN, shmsz, IPC_DIPC)) < 0)
        sleep(1);
    shmbuf = shmat(desc, NULL, 0);
    if (shmbuf == (char *)-1)
        goto err;
    msleep(GUARD_TIME);
    while ((desc = vlock_find(KEY_LOCK, blocks)) < 0)
        sleep(1);
    lock_vec = vlock_check(desc);
    if (lock_vec == (void *)-1)
        goto err;
    msleep(GUARD_TIME);
    barrier_wait(BARRIER_TEST, total);
    bl_log("member: step 1, testing ...\n");
    gettimeofday(&time_start, NULL);
    bl(id, shmbuf, lock_vec, blocks, size, ep);
    bl_log("member: step 2, waiting ...\n");
    barrier_wait(BARRIER_END, total);
    gettimeofday(&time_end, NULL);
    bl_log("member: step 3, save result\n");
    result = time_end.tv_sec - time_start.tv_sec + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
    save_result(result);
    bl_log("member: step 4, release\n");
    release_wait();
    return 0;
err:
    printf("member: failed to start\n");
    return -1;
}


int start(int total, int blocks, size_t size, float ep)
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
        if (start_leader(id, total, blocks, size, ep))
            return -1;
    } else {
        if (start_member(id, total, blocks, size, ep))
            return -1;
    }
    return 0;
}


void usage()
{
    printf("Usage: bl [-n nodes] [-b blocks] [-s size] [-e extending-possibility]\n");
    printf("-n: the number of nodes\n");
    printf("-b: the number of blocks\n");
    printf("-s: block size (the size must be 2^N)\n");
    printf("-e: the possibility of extending access to the next page\n");
}


int main(int argc, char **argv)
{
    char ch;
    int total = 0;
    float ep = BL_EP;
    int blocks = BL_BLOCKS;
    size_t size = BL_BLOCK_SIZE;
    
    while ((ch = getopt(argc, argv, "n:b:s:e:h")) != -1) {
        switch(ch) {
        case 'n':
            total = atoi(optarg);
            break;
        case 'b':
            blocks = atoi(optarg);
            if (blocks == 0 || blocks > BLOCK_MAX) {
                usage();
                exit(1);
            }
            break;
        case 's':
            size = atoi(optarg);
            if (size == 0 || size > BLOCK_SIZE || (((PAGE_SIZE % size) != 0) && ((size % PAGE_SIZE) != 0))) {
                usage();
                exit(1);
            }
            break;
        case 'e':
            ep = atof(optarg);
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
    return start(total, blocks, size, ep);
}
