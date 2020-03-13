#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "bigN.h"

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

static void bigN_print(bigN buf)
{
    int i = part_num - 1;
    while ((i >= 0) && (buf.part[i] == 0))
        i--;
    if (i < 0) {
        printf("0");
        return;
    }
    printf("%lld", buf.part[i--]);
    while (i >= 0) {
        printf("%08lld", buf.part[i]);
        i--;
    }
}

int main()
{
    bigN output;

    long long sz;

    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */

    // FILE *fp1 = fopen("user_space.txt", "wb+");
    // FILE *fp2 = fopen("kernel_space.txt", "wb+");
    // FILE *fp3 = fopen("fib_fd.txt", "wb+");
    // FILE *fp4 = fopen("fib_fd_clz.txt", "wb+");

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
        sz = read(fd, &output, sizeof(bigN));
        clock_gettime(CLOCK_REALTIME, &end);

        output.user_t = diff_in_ns(start, end);

        printf("Reading to " FIB_DEV ", offset : %d, returned the sequence ",
               i);
        bigN_print(output);
        printf("\n");

        // fprintf(fp1, "%d %u\n", i, output.user_t);
        // fprintf(fp2, "%d %u\n", i, output.kernel_t);
        // fprintf(fp4, "%d %u\n", i, output.user_t);
    }

    // fclose(fp4);
    // fclose(fp3);
    // fclose(fp1);
    // fclose(fp2);
    close(fd);
    return 0;
}
