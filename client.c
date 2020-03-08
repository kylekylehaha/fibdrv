#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

unsigned int diff_in_ns(struct timespec t1, struct timespec t2)
{
    struct timespec result;
    if ((t2.tv_nsec - t1.tv_nsec) < 0) {
        result.tv_sec = t2.tv_sec - t1.tv_sec - 1;
        result.tv_nsec = 1000000000 + t2.tv_nsec - t1.tv_nsec;
    } else {
        result.tv_sec = t2.tv_sec - t1.tv_sec;
        result.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (unsigned int) result.tv_sec * 1000000000 + result.tv_nsec;
}

int main()
{
    FILE *fp = fopen("time.txt", "wb+");
    long long sz;

    char buf[1];
    char write_buf[] = "testing writing";
    int offset = 10; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int i = 0; i <= offset; i++) {
        sz = write(fd, write_buf, strlen(write_buf));
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    }

    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);
        sz = read(fd, buf, 1);
        clock_gettime(CLOCK_REALTIME, &end);
        fprintf(fp, "%d %u\n", i, diff_in_ns(start, end));

        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%lld.\n",
               i, sz);
    }
    /*
        for (int i = offset; i >= 0; i--) {
            lseek(fd, i, SEEK_SET);
            sz = read(fd, buf, 1);
            printf("Reading from " FIB_DEV
                   " at offset %d, returned the sequence "
                   "%lld.\n",
                   i, sz);
        }
    */
    fclose(fp);
    close(fd);
    return 0;
}
