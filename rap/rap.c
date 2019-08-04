#include "lock.h"
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


void free_lock(rap_lock_t *lock)
{
	shmlock_release(lock->desc, lock->lock);
}


void free_area(rap_areas_t *areas, int n)
{
	rap_area_t *area = &areas->areas[n];

	if (!(areas->flags & RAP_CONT)) {
		shmdt(area->buf);
		shmctl(area->desc, IPC_RMID, NULL);
	}
	free_lock(&area->lock);
}


void free_areas(rap_areas_t *areas)
{
	int i;

	for (i = 0; i < areas->count; i++)
		free_area(areas, i);
	if (areas->flags & RAP_CONT) {
		shmdt(areas->buf);
		shmctl(areas->desc, IPC_RMID, NULL);
	}
	free(areas);
}


void detach_lock(rap_lock_t *lock)
{
	shmdt(lock->lock);
}


void detach_area(rap_areas_t *areas, int n)
{
	rap_area_t *area = &areas->areas[n];

	if (!(areas->flags & RAP_CONT))
		shmdt(area->buf);
	detach_lock(&area->lock);
}


void detach_areas(rap_areas_t *areas)
{
	int i;

	for (i = 0; i < areas->count; i++)
		detach_area(areas, i);
	if (areas->flags & RAP_CONT)
		shmdt(areas->buf);
	free(areas);
}


int alloc_lock(rap_lock_t *lock, int users)
{
	static int key = KEY_LOCK;

	lock->desc = shmlock_create(key, users);
	if (lock->desc < 0) {
		printf("failed to allocate lock\n");
		return -1;
	}
	lock->lock = shmlock_init(lock->desc, users);
	if (lock->lock == (shmlock_t *)-1) {
		printf("failed to allocate lock\n");
		shmlock_release(lock->desc, lock->lock);
		return -1;
	}
	key++;
	return 0;
}


int alloc_area(rap_areas_t *areas, int n, int users)
{
	int size = 1 << n;
	static int key = KEY_AREA;
	rap_area_t *area = &areas->areas[n];

	if (areas->flags & RAP_CONT) {
		area->desc = 0;
		area->buf = &areas->buf[size];
	} else {
		if ((area->desc = shmget(key, size, IPC_DIPC | IPC_CREAT)) < 0) {
			printf("failed to allocate area\n");
			return -1;
		}
		area->buf = shmat(area->desc, NULL, 0);
		if (area->buf == (char *)-1) {
			printf("failed to allocate area\n");
			shmctl(area->desc, IPC_RMID, NULL);
			return -1;
		}
		key++;
	}
	if (alloc_lock(&area->lock, users) < 0) {
		shmdt(area->buf);
		shmctl(area->desc, IPC_RMID, NULL);
	}
	area->size = size;
	return 0;
}


rap_areas_t *alloc_areas(int users, int flags)
{
	int i;
	rap_areas_t *areas = (rap_areas_t *)malloc(sizeof(rap_areas_t));

	if (!areas) {
		printf("failed to allocate areas (no mem)");
		return NULL;
	}
	memset(areas, 0, sizeof(rap_areas_t));
	if (flags & RAP_CONT) {
		if ((areas->desc = shmget(KEY_AREA, (1 << RAP_AREAS), IPC_DIPC | IPC_CREAT)) < 0) {
			free(areas);
			return NULL;
		}
		areas->buf = shmat(areas->desc, NULL, 0);
		if (areas->buf == (char *)-1) {
			shmctl(areas->desc, IPC_RMID, NULL);
			free(areas);
			return NULL;
		}
	}
	areas->flags = flags;
	for (i = 0; i < RAP_AREAS; i++) {
		if (alloc_area(areas, i, users) < 0)
			goto err;
		areas->count += 1;
	}
	return areas;
err:
	free_areas(areas);
	return NULL;
}


int attach_lock(rap_lock_t *lock, int users)
{
	static int key = KEY_LOCK;

	lock->desc = shmlock_find(key, users);
	if (lock->desc < 0) {
		printf("failed to attach lock\n");
		return -1;
	}
	lock->lock = shmlock_check(lock->desc);
	if (lock->lock == (shmlock_t *)-1) {
		printf("failed to  attach lock\n");
		return -1;
	}
	key++;
	return 0;
}


int attach_area(rap_areas_t *areas, int n, int users)
{
	int size = 1 << n;
	static int key = KEY_AREA;
	rap_area_t *area = &areas->areas[n];

	if (areas->flags & RAP_CONT) {
		area->desc = 0;
		area->buf = &areas->buf[size];
	} else {
		while ((area->desc = shmget(key, size, IPC_DIPC)) < 0)
			sleep(1);
		area->buf = shmat(area->desc, NULL, 0);
		if (area->buf == (char *)-1) {
			printf("Failed to attach area\n");
			return -1;
		}
		key++;
	}
	if (attach_lock(&area->lock, users) < 0) {
		printf("Failed to attach area\n");
		shmdt(area->buf);
		return -1;
	}
	area->size = size;
	return 0;
}


rap_areas_t *attach_areas(int users, int flags)
{
	int i;
	rap_areas_t *areas = (rap_areas_t *)malloc(sizeof(rap_areas_t));

	if (!areas) {
		printf("failed to allocate areas\n");
		return NULL;
	}
	if (flags & RAP_CONT) {
		while ((areas->desc = shmget(KEY_AREA, (1 << RAP_AREAS), IPC_DIPC)) < 0)
            sleep(1);
		areas->buf = shmat(areas->desc, NULL, 0);
		if (areas->buf == (char *)-1) {
			free(areas);
			return NULL;
		}
	}
	areas->flags = flags;
	for (i = 0; i < RAP_AREAS; i++) {
		if (attach_area(areas, i, users) < 0)
			goto err;
		areas->count += 1;
	}
	return areas;
err:
	printf("failed to attach areas\n");
	detach_areas(areas);
	return NULL;
}


void rap(int id, rap_areas_t *areas)
{
	int i;
	int j;
	rap_area_t *area;

	srand((int)time(0));
	for (i = 0; i < RAP_ROUNDS; i++) {
		area = &areas->areas[rand() % RAP_AREAS];
		shmlock_lock(id, area->lock.lock);
		if (rand() % 4) {
			for (j = 0; j < area->size; j++)
				if (area->buf[j])
                    continue;
		} else { // 1/4 write operations
			for (j = 0; j < area->size; j++)
				area->buf[j] = rand() % 256;
		}
		shmlock_unlock(id, area->lock.lock);
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


int start_leader(int id, int total, int flags)
{
	int ret = -1;
	float result;
	rap_areas_t *areas = NULL;
	struct timeval time_start, time_end;

    rap_log("leader: step 0, preparing ...\n");
    areas = alloc_areas(total, flags);
    if (!areas)
        goto out;
	barrier_wait(BARRIER_TEST, total);
    rap_log("leader: step 1, testing ...\n");
    gettimeofday(&time_start, NULL);
	rap(id, areas);
    rap_log("leader: step 2, waiting ...\n");
    barrier_wait(BARRIER_END, total);
    gettimeofday(&time_end, NULL);
    rap_log("leader: step 3, save result\n");
	result = time_end.tv_sec - time_start.tv_sec + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
	save_result(result);
    rap_log("leader: step 4, release\n");
	release_wait();
	ret = 0;
    free_areas(areas);
out:
	if (ret < 0)
		printf("leader: failed to start\n");
	return ret;
}


int start_member(int id, int total, int flags)
{
	float result;
	rap_areas_t *areas;
	struct timeval time_start, time_end;

    rap_log("member: step 0, preparing ...\n");
	areas = attach_areas(total, flags);
	if (!areas)
		goto out;
	barrier_wait(BARRIER_TEST, total);
    rap_log("member: step 1, testing ...\n");
	gettimeofday(&time_start, NULL);
	rap(id, areas);
    rap_log("member: step 2, waiting ...\n");
    barrier_wait(BARRIER_END, total);
    gettimeofday(&time_end, NULL);
    rap_log("member: step 3, save result\n");
	result = time_end.tv_sec - time_start.tv_sec + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
	save_result(result);
    rap_log("member: step 4, release\n");
	release_wait();
	detach_areas(areas);
	return 0;
out:
	printf("member: failed to start\n");
	return -1;
}


int start(int total, int flags)
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
		if (start_leader(id, total, flags))
			return -1;
	} else {
		if (start_member(id, total, flags))
			return -1;
	}
	return 0;
}


void usage()
{
	printf("Usage: rap [-c] [-n nodes]\n");
	printf("-c: using contiguous areas\n");
	printf("-n: the number of nodes\n");
}


int main(int argc, char **argv)
{
	char ch;
	int total = 0;
	int flags = 0;

	if ((argc < 3) || (argc > 4)) {
		usage();
		exit(1);
	}
	while ((ch = getopt(argc, argv, "n:ch")) != -1) {
		switch(ch) {
		case 'c':
			flags = RAP_CONT;
			break;
		case 'n':
			total = atoi(optarg);
			break;
		default:
			usage();
			exit(1);
		}
	}
	return start(total, flags);
}
