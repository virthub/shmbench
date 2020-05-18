// Vector Lock using shared memory
// The status of each lock is stored in a seperated page

#include "vlock.h"
#include "bl.h"

int vlock_create(int key, int nodes, int locks)
{
    assert(vlock_size(nodes) < PAGE_SIZE);
    return shmget(key, vlock_vec_size(locks), IPC_DIPC | IPC_CREAT);
}


int vlock_find(int key, int locks)
{
    return shmget(key, vlock_vec_size(locks), IPC_DIPC);
}


void *vlock_check(int desc)
{
    return shmat(desc, NULL, 0);
}


void *vlock_init(int desc, int nodes, int locks)
{
    int i;
    void *vec = shmat(desc, NULL, 0);

    memset(vec, 0, vlock_vec_size(locks));
    for (i = 0; i < locks; i++) {
        vlock_t *lock = vlock_get(vec, i);

        lock->total = nodes;
    }
    return vec;
}


void vlock_lock(int id, void *vec, int ent, bool *first, bool *fresh)
{
    int pos = id - 1;
    vlock_t *lock = vlock_get(vec, ent);
    int total = lock->total;
    int *visit = &lock->B[total];

#ifdef SHOW_LOCK
    vlock_t prev;
    memset(&prev, 0, sizeof(vlock_t));
#endif
start:
    lock->B[pos] = 1;
    lock->X = id;
    if (lock->Y != 0) {
        lock->B[pos] = 0;
        while (lock->Y) {
#ifdef SHOW_LOCK
            if (memcmp(lock, &prev, sizeof(vlock_t))) {
                prev = *lock;
                show_banary("lock->Y (first time)", lock, sizeof(vlock_t));
            }
#endif
        }
        goto start;
    }
    lock->Y = id;
    if (lock->X != id) {
        int i;
        
        lock->B[pos] = 0;
        for (i = 0; i < total; i++) {
            while (lock->B[i]) {
#ifdef SHOW_LOCK
                if (memcmp(lock, &prev, sizeof(vlock_t))) {
                    char name[64] = {0};

                    prev = *lock;
                    sprintf(name, "lock->B[%d]", i);
                    show_banary(name, lock, sizeof(vlock_t));
                }
#endif
            }
        }

        if (lock->Y != id) {
            while (lock->Y) {
#ifdef SHOW_LOCK
                if (memcmp(lock, &prev, sizeof(vlock_t))) {
                    prev = *lock;
                    show_banary("lock->Y (second time)", lock, sizeof(vlock_t));
                }
#endif
            }
            goto start;
        }
    }
#ifdef SHOW_LOCK
    show_banary("lock", lock, sizeof(vlock_t));
#endif
    if (first)
        *first = false;
    if (!visit[id]) {
        if (!lock->count && first)
            *first = true;
        lock->count += 1;
        visit[id] = 1;
        if (lock->count == total) {
            memset(visit, 0, sizeof(int) * total);
            lock->count = 0;
        }
        if (fresh)
            *fresh = true;
    } else if (fresh)
        *fresh = false;
}


void vlock_unlock(int id, void *vec, int ent)
{
    vlock_t *lock = vlock_get(vec, ent);

    lock->Y = 0;
    lock->B[id - 1] = 0;
#ifdef SHOW_LOCK
    show_banary("unlock", lock, sizeof(vlock_t));
#endif
}


void vlock_join(int id, void *vec, int ent)
{
    vlock_t *lock = vlock_get(vec, ent);

    vlock_lock(id, vec, ent, NULL, NULL);
    lock->count += 1;
    vlock_unlock(id, vec, ent);
}


void vlock_wait(int id, void *vec, int ent, int total)
{
    bool finish;
    vlock_t *lock = vlock_get(vec, ent);

    do {
        vlock_lock(id, vec, ent, NULL, NULL);
        finish = lock->count == total;
        vlock_unlock(id, vec, ent);
    } while (!finish);
}


void vlock_release(int desc, void *vec)
{
    if (vec && (vec != (vlock_t *)-1))
        shmdt(vec);
    if (desc >= 0)
        shmctl(desc, IPC_RMID, NULL);
}
