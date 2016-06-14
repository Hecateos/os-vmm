/* 产生访存请求 */
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "vmm.h"
Ptr_MemoryAccessRequest ptr_memAccReq;
void do_request()
{
	int address,request;
	scanf("%d %d",&address,&request);
	/* 随机产生请求地址 */
	ptr_memAccReq->virAddr = address;
	/* 随机产生请求类型 */
	switch (request)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			printf("产生请求：\n地址：%u\t类型：读取\n", ptr_memAccReq->virAddr);
			break;
		}
		case 1: //写请求
		{
			char C_write;
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			scanf(" %c",&C_write);
			ptr_memAccReq->value = C_write;
			printf("产生请求：\n地址：%u\t类型：写入\t值：%02X\n", ptr_memAccReq->virAddr, ptr_memAccReq->value);
			break;
		}
		case 2://zhixing
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			printf("产生请求：\n地址：%u\t类型：执行\n", ptr_memAccReq->virAddr);
			break;
		}
		case 3://change
		{
			ptr_memAccReq->reqType = REQUEST_SWITCH;
			ptr_memAccReq->value = address;
			printf("change");
			break;
		}
		default:
			puts("xxx");
			break;
	}	
}
int main(){
	int fifo;
	MemoryAccessRequest request;
	ptr_memAccReq = &request;
	while (1){	
	/* 在阻塞模式下打开FIFO */
puts("zzzzz");
	if((fifo=open("/tmp/server",O_WRONLY))<0)
		error_sys("open fifo failed");
puts("yyyyy");
	do_request();
	puts("xxxxxxxx");
	write(fifo,ptr_memAccReq,sizeof(MemoryAccessRequest));
	close(fifo);
	}
}
