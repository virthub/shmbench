#include "lock.h"
#include "me.h"

int shmlock_create(int key, int total)
{
    return shmget(key, shmlock_size(total), IPC_DIPC | IPC_CREAT);
}


int shmlock_find(int key, int total)
{
    return shmget(key, shmlock_size(total), IPC_DIPC);
}


shmlock_t *shmlock_check(int desc)
{
    return shmat(desc, NULL, 0);
}


shmlock_t *shmlock_init(int desc, int total)
{
    shmlock_t *lock = shmat(desc, NULL, 0);

    memset(lock, 0, shmlock_size(total));
    lock->total = total;
    return lock;
}


void shmlock_lock(int id, shmlock_t *lock)
{
    int pos = id - 1;
#ifdef SHOW_LOCK
    shmlock_t prev;
    memset(&prev, 0, sizeof(shmlock_t));
#endif
start:
    lock->B[pos] = 1;
    lock->X = id;
    if (lock->Y != 0) {
        lock->B[pos] = 0;
        while (lock->Y) {
#ifdef SHOW_LOCK
            if (memcmp(lock, &prev, sizeof(shmlock_t))) {
                prev = *lock;
                show_banary("lock->Y (first time)", lock, sizeof(shmlock_t));
            }
#endif
        }
        goto start;
    }
    lock->Y = id;
    if (lock->X != id) {
        int i;
        int total = lock->total;

        lock->B[pos] = 0;
        for (i = 0; i < total; i++) {
            while (lock->B[i]) {
#ifdef SHOW_LOCK
                if (memcmp(lock, &prev, sizeof(shmlock_t))) {
                    char name[64] = {0};

                    prev = *lock;
                    sprintf(name, "lock->B[%d]", i);
                    show_banary(name, lock, sizeof(shmlock_t));
                }
#endif
            }
        }

        if (lock->Y != id) {
            while (lock->Y) {
#ifdef SHOW_LOCK
                if (memcmp(lock, &prev, sizeof(shmlock_t))) {
                    prev = *lock;
                    show_banary("lock->Y (second time)", lock, sizeof(shmlock_t));
                }
#endif
            }
            goto start;
        }
    }
#ifdef SHOW_LOCK
    show_banary("lock", lock, sizeof(shmlock_t));
#endif
}


void shmlock_unlock(int id, shmlock_t *lock)
{
    lock->Y = 0;
    lock->B[id - 1] = 0;
#ifdef SHOW_LOCK
    show_banary("unlock", lock, sizeof(shmlock_t));
#endif
}


void shmlock_join(int id, shmlock_t *lock)
{
    shmlock_lock(id, lock);
    lock->count += 1;
    shmlock_unlock(id, lock);
}


void shmlock_wait(int id, shmlock_t *lock, int total)
{
    bool finish;

    do {
        shmlock_lock(id, lock);
        finish = lock->count == total;
        shmlock_unlock(id, lock);
    } while (!finish);
}


void shmlock_release(int desc, shmlock_t *lock)
{
    if (lock && (lock != (shmlock_t *)-1))
        shmdt(lock);
    if (desc >= 0)
        shmctl(desc, IPC_RMID, NULL);
}
