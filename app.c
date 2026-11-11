#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define DEMODEV_IOC_MAGIC 'k'

struct demodev_stats {
    unsigned long write_count;
    unsigned long read_count;
    size_t current_len;
};

#define DEMODEV_IOC_RESET      _IO(DEMODEV_IOC_MAGIC, 1)
#define DEMODEV_IOC_GET_STATS  _IOR(DEMODEV_IOC_MAGIC, 2, struct demodev_stats)

int main(void)
{
    int fd;
    char write_buf[] = "Hello from custom userspace C application!";
    char read_buf[1024] = {0};
    struct demodev_stats stats;

    fd = open("/dev/demodev", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }
    printf("Device opened successfully (fd = %d).\n", fd);

    /* Test Write */
    if (write(fd, write_buf, strlen(write_buf)) < 0) {
        perror("Failed to write to device");
        close(fd);
        return 1;
    }
    printf("Successfully wrote: \"%s\"\n", write_buf);

    /* Test Read */
    if (read(fd, read_buf, sizeof(read_buf)) < 0) {
        perror("Failed to read from device");
        close(fd);
        return 1;
    }
    printf("Successfully read back from kernel: \"%s\"\n", read_buf);

    /* Test ioctl GET_STATS */
    if (ioctl(fd, DEMODEV_IOC_GET_STATS, &stats) < 0) {
        perror("ioctl GET_STATS failed");
    } else {
        printf("Driver Stats -> Writes: %lu, Reads: %lu, Current Len: %zu\n",
               stats.write_count, stats.read_count, stats.current_len);
    }

    /* Test ioctl RESET */
    if (ioctl(fd, DEMODEV_IOC_RESET) < 0) {
        perror("ioctl RESET failed");
    } else {
        printf("Buffer successfully reset via ioctl.\n");
    }

    close(fd);
    return 0;
}
