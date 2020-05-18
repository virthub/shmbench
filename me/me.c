#include "me.h"
#include "lock.h"
#include "barrier.h"

void release_wait()
{
    FILE *fp;

    fp = fopen(ME_RELEASE, "w");
    fclose(fp);
    while (!access(ME_RELEASE, F_OK))
        sleep(1);
}


void me(char *shmbuf, int shmsz, int id, shmlock_t *lock)
{
    int i;
    int j;
    int k;
    int nr_pages = shmsz / PAGE_SIZE;

    srand((unsigned)time(0) + (unsigned)id);
    for (i = 0; i < ME_ROUNDS; i++) {
        int count = rand() % nr_pages;

        me_log("round=%d\n", i);
        shmlock_lock(id, lock);
        for (j = 0; j < count; j++) {
            int n = (rand() % nr_pages) * PAGE_SIZE;

            for (k = 0; k < PAGE_SIZE; k++) {
                shmbuf[n] = rand() & 0xff;
                n++;
            }
        }
        shmlock_unlock(id, lock);
    }
}


void save_result(float result)
{
    FILE *filp;

#ifdef SHOW_RESULT
    printf("result=%f\n", result);
#endif
    filp = fopen(ME_RESULT, "w");
    fprintf(filp, "%f", result);
    fclose(filp);
}


int start_leader(int id, int total, int shmsz)
{
    int ret = -1;
    char *shmbuf;
    float result;
    int desc, desc_lock;
    shmlock_t *lock = NULL;
    struct timeval time_start, time_end;

    me_log("leader: step 0, preparing ...\n");
    desc = shmget(KEY_MAIN, shmsz, IPC_DIPC | IPC_CREAT);
    if (desc < 0)
        goto out;
    shmbuf = shmat(desc, NULL, 0);
    if (shmbuf == (char *)-1)
        goto release;
    memset(shmbuf, 0, shmsz);
    desc_lock = shmlock_create(KEY_LOCK, total);
    if (desc_lock < 0)
        goto release;
    lock = shmlock_init(desc_lock, total);
    if (lock == (shmlock_t *)-1)
        goto release_lock;
    barrier_wait(BARRIER_TEST, total);
    me_log("leader: step 1, testing ...\n");
    gettimeofday(&time_start, NULL);
    me(shmbuf, shmsz, id, lock);
    me_log("leader: step 2, waiting ...\n");
    barrier_wait(BARRIER_END, total);
    gettimeofday(&time_end, NULL);
    me_log("leader: step 3, save result\n");
    result = time_end.tv_sec - time_start.tv_sec + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
    save_result(result);
    me_log("leader: step 4, release\n");
    release_wait();
    shmdt(shmbuf);
    ret = 0;
release_lock:
    shmlock_release(desc_lock, lock);
release:
    shmctl(desc, IPC_RMID, NULL);
out:
    if (ret < 0)
        printf("leader: failed to start\n");
    return ret;
}


int start_member(int id, int total, int shmsz)
{
    float result;
    char *shmbuf;
    shmlock_t *lock;
    int desc, desc_lock;
    struct timeval time_start, time_end;

    me_log("member: step 0, preparing ...\n");
    while ((desc = shmget(KEY_MAIN, shmsz, IPC_DIPC)) < 0)
        sleep(1);
    shmbuf = shmat(desc, NULL, 0);
    if (shmbuf == (char *)-1)
        goto err;
    while ((desc_lock = shmlock_find(KEY_LOCK, total)) < 0)
        sleep(1);
    lock = shmlock_check(desc_lock);
    if (lock == (shmlock_t *)-1)
        goto err;
    barrier_wait(BARRIER_TEST, total);
    me_log("member: step 1, testing ...\n");
    gettimeofday(&time_start, NULL);
    me(shmbuf, shmsz, id, lock);
    me_log("member: step 2, waiting ...\n");
    barrier_wait(BARRIER_END, total);
    gettimeofday(&time_end, NULL);
    me_log("member: step 3, save result\n");
    result = time_end.tv_sec - time_start.tv_sec + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
    save_result(result);
    me_log("member: step 4, release\n");
    release_wait();
    return 0;
err:
    printf("member: failed to start\n");
    return -1;
}


int start(int total, int shmsz)
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
        if (start_leader(id, total, shmsz))
            return -1;
    } else {
        if (start_member(id, total, shmsz))
            return -1;
    }
    return 0;
}


void usage()
{
    printf("Usage: me [-n nodes] [-s size]\n");
    printf("-n: the number of nodes\n");
    printf("-s: the size of shared-memory\n");
}


int main(int argc, char **argv)
{
    char ch;
    int size = 0;
    int total = 0;

    while ((ch = getopt(argc, argv, "n:s:h")) != -1) {
        switch(ch) {
        case 'n':
            total = atoi(optarg);
            break;
        case 's':
            size = atoi(optarg);
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
    return start(total, size);
}
