#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vmm.h"

Ptr_MemoryAccessRequest ptr_memAccReq;

void usage();
void rand_request();
int wait = 0;

int main(int argc, char* argv[])
{
	int fd;
	BYTE byte;
	ptr_memAccReq = (Ptr_MemoryAccessRequest)malloc(sizeof(MemoryAccessRequest));
	if(argc == 1)
	{
		usage();
		return 0;
	}
	else if(argc == 2)
	{
		if(strcmp(argv[1],"-r")==0)
			rand_request();
		else
		{
			printf("参数错误\n");
			return 1;
		}
	}
	else if(argc == 5 && strcmp(argv[1],"-m")==0 && (strcmp(argv[3],"READ")==0||strcmp(argv[3],"EXECUTE")==0))
	{
		ptr_memAccReq->pid = atoi(argv[2]);
		ptr_memAccReq->reqType = strcmp(argv[3],"READ")==0 ? REQUEST_READ : REQUEST_EXECUTE;
		ptr_memAccReq->virAddr = atoi(argv[4]);
		if(strcmp(argv[3],"READ")==0)
			wait = 1;
	}
	else if(argc == 6 && strcmp(argv[1],"-m")==0 && strcmp(argv[3],"WRITE")==0)
	{
		ptr_memAccReq->pid = atoi(argv[2]);
		ptr_memAccReq->reqType = REQUEST_WRITE;
		ptr_memAccReq->virAddr = atoi(argv[4]);
		ptr_memAccReq->value = argv[5][0];
	}
	else
	{
		printf("参数错误\n");
		return 1;
	}
	
	/* 发送请求 */
	if((fd=open("fifo_req",O_WRONLY))<0)
		printf("request打开FIFO失败");
	if(write(fd,ptr_memAccReq,REQ_LEN)<0)
		printf("request写入FIFO失败");
	close(fd);

	/* 接收数据 */
	if(wait)
	{
		if((fd = open("fifo_res",O_RDONLY))<0)
			printf("打开FIFO失败\n");
		if(read(fd,&byte,sizeof(BYTE))<0)
			printf("读取FIFO失败");
		if(byte!=(BYTE)0xFF)
			printf("读到数据：%c\n", byte);
		else
			printf("读取请求被拒绝\n");
		close(fd);
	}
	return 0;
}

void usage()
{
	printf("Usage: req [MODE] [PID TYPE ADDR VALUE]\n"
		"\tMODE:\t\t-r\t随机生成请求\n"
		"\t\t\t-m\t手动设置请求参数\n"
		"\tPID\t\t进程号，在0-9之间的整数\n"
		"\tTYPE\t\t访问类型，READ、WRITE或EXECUTE\n"
		"\tADDR\t\t虚拟地址，0-4095之间的整数\n"
		"\tVALUE\t\t如果类型是WRITE，此为要写入的值\n");
}

/* 产生随机访存请求 */
void rand_request()
{
		printf("rand");
	/* 随机产生请求地址 */
	ptr_memAccReq->virAddr = random() % VIRTUAL_MEMORY_SIZE;
	/* 随机产生请求进程号 */
	ptr_memAccReq->pid = random() % 10;
	/* 随机产生请求类型 */
	switch (random() % 3)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("产生请求：\n进程号：%u\t类型：读取\t地址：%u\n", ptr_memAccReq->pid, ptr_memAccReq->virAddr);
			wait = 1;
			break;
		}
		case 1: //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = random() % 0xFFu;
			printf("产生请求：\n进程号：%u\t类型：写入\t地址：%u\t值：%02X\n", ptr_memAccReq->pid, 
				ptr_memAccReq->virAddr, ptr_memAccReq->value);
			break;
		}
		case 2:
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("产生请求：\n进程号：%u\t类型：执行\t地址：%u\n", ptr_memAccReq->pid, ptr_memAccReq->virAddr);
			break;
		}
		default:
			break;
	}	
}
