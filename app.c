#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/demodev"

int main() {
    int fd;
    char write_buf[] = "Hello from custom userspace C application!";
    char read_buf[100];
    ssize_t ret;

    // 1. Open the character device file for writing/reading
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return errno;
    }
    printf("Device opened successfully (fd = %d).\n", fd);

    // 2. Write data down into the kernel driver
    ret = write(fd, write_buf, strlen(write_buf));
    if (ret < 0) {
        perror("Failed to write to device");
        close(fd);
        return errno;
    }
    printf("Successfully wrote: \"%s\"\n", write_buf);

    // Close the file descriptor to reset the driver's read position
    close(fd);

    // 3. Reopen the device to read the data back out
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to reopen device for reading");
        return errno;
    }

    memset(read_buf, 0, sizeof(read_buf));
    ret = read(fd, read_buf, sizeof(read_buf) - 1);
    if (ret < 0) {
        perror("Failed to read from device");
        close(fd);
        return errno;
    }

    printf("Successfully read back from kernel: \"%s\"\n", read_buf);

    // 4. Close the device file descriptor
    close(fd);
    return 0;
}