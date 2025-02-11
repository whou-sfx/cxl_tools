/*************************************************************************
@File Name: mmap_io_copy.c
@Desc: 
@Author: Andy-wei.hou
@Mail: wei.hou@scaleflux.com 
@Created Time: 2024年11月17日 星期日 22时45分10秒
@Log: 
************************************************************************/

#include<stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <x86intrin.h>


#define PAGE_SIZE   (8 * 1024* 1024 * 1024ULL)
#define BUF_SIZE  (2*1024*1024)
#define FILLLOOP PAGE_SIZE / (BUF_SIZE * sizeof(int))
unsigned int buf[BUF_SIZE];
int main()
{
    int i = 0, loop = 0; 
    /* DAX mapping requires a 2MiB alignment */
    int fd = open("/dev/dax0.0", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("open() failed");
        return 1;
    } 
    void *dax_addr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (dax_addr == MAP_FAILED) {
        perror("mmap() failed");
        close(fd);
        return 1;

    } 
    printf("map size 0x%llx, buf size 0x%x, base %p\n", PAGE_SIZE, BUF_SIZE, dax_addr);
    for (loop = 0; loop < FILLLOOP; loop++)
    {
        char * dst_addr = dax_addr + loop * sizeof(buf);
	    memset(buf, 0x00, sizeof(buf));

        /** for (i = 0; i < BUF_SIZE; i++) */
        /** { */
        /**     buf[i] = i | loop<< 20 ; */
        /** } */
        /** printf("proc %p, buf[]:0x%08x...\n", dst_addr, buf[0]); */
        /* Write something to the memory */
        memcpy(dst_addr, buf, sizeof(buf));

   	//int result = msync((void *)dst_addr, sizeof(buf), MS_SYNC);
      	//if (result == -1) {
	//	perror("msync failed");
	//}

	for (i = 0; i < sizeof(buf)/64; i++) {
	  _mm_clflushopt((char *)dst_addr + i*64);
	}
    }


    for (i = 0; i < sizeof(buf)/64; i++) {
	    _mm_clflushopt((char *)dax_addr + i*64);
    }

    /*do final read 4k*/
    memcpy(buf, dax_addr, 4096);


    munmap(dax_addr, PAGE_SIZE);
    close(fd);
    return 0;
}
