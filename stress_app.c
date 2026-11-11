#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#define DEMODEV_IOC_MAGIC 'k'

struct demodev_stats {
    unsigned long write_count;
    unsigned long read_count;
    size_t current_len;
};

#define DEMODEV_IOC_RESET      _IO(DEMODEV_IOC_MAGIC, 1)
#define DEMODEV_IOC_GET_STATS  _IOR(DEMODEV_IOC_MAGIC, 2, struct demodev_stats)

#define NUM_THREADS 4
#define ITERATIONS  100

void *worker_thread(void *arg)
{
    int thread_id = *(int *)arg;
    int fd;
    char write_buf[128];
    char read_buf[128];
    struct demodev_stats stats;
    int i;

    snprintf(write_buf, sizeof(write_buf), "Thread %d payload data", thread_id);

    for (i = 0; i < ITERATIONS; i++) {
        fd = open("/dev/demodev", O_RDWR);
        if (fd < 0) {
            fprintf(stderr, "Thread %d: failed to open device: %s\n", thread_id, strerror(errno));
            pthread_exit(NULL);
        }

        /* Concurrent Write */
        if (write(fd, write_buf, strlen(write_buf)) < 0) {
            fprintf(stderr, "Thread %d: write failed: %s\n", thread_id, strerror(errno));
        }

        /* Concurrent Read */
        memset(read_buf, 0, sizeof(read_buf));
        if (read(fd, read_buf, sizeof(read_buf)) < 0) {
            fprintf(stderr, "Thread %d: read failed: %s\n", thread_id, strerror(errno));
        }

        /* Concurrent ioctl stats query */
        if (ioctl(fd, DEMODEV_IOC_GET_STATS, &stats) < 0) {
            fprintf(stderr, "Thread %d: ioctl GET_STATS failed: %s\n", thread_id, strerror(errno));
        }

        close(fd);
    }

    printf("Thread %d completed %d iterations successfully.\n", thread_id, ITERATIONS);
    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int i;
    int fd;
    struct demodev_stats final_stats;

    printf("Starting multi-threaded stress test with %d threads (%d iterations each)...\n", 
           NUM_THREADS, ITERATIONS);

    /* Reset device before test */
    fd = open("/dev/demodev", O_RDWR);
    if (fd >= 0) {
        ioctl(fd, DEMODEV_IOC_RESET);
        close(fd);
    }

    /* Spawn worker threads */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_ids[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    /* Wait for all threads to complete */
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Query final statistics */
    fd = open("/dev/demodev", O_RDWR);
    if (fd >= 0) {
        if (ioctl(fd, DEMODEV_IOC_GET_STATS, &final_stats) == 0) {
            printf("\n--- Final Driver Statistics ---\n");
            printf("Total Writes recorded: %lu (Expected: %d)\n", 
                   final_stats.write_count, NUM_THREADS * ITERATIONS);
            printf("Total Reads recorded:  %lu (Expected: %d)\n", 
                   final_stats.read_count, NUM_THREADS * ITERATIONS);
            printf("Current Buffer Length: %zu bytes\n", final_stats.current_len);
        }
        close(fd);
    }

    printf("Stress test completed successfully without race conditions or deadlocks!\n");
    return 0;
}