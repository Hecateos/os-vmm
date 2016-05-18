#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "vmm.h"

#define FIFO "myFIFO"
int main()
{
	unsigned long addr;
	unsigned char value;
	char s[100];
	srandom(time(NULL));
	FILE *fp;
	int number;
	int pid;
	
	while(1){
		pid=random()%2;
		if((fp=fopen(FIFO,"w"))==NULL){
			printf("write FIFO wrong");
			return 0;
		}
		/* ������������ַ */
		//ptr_memAccReq->virAddr = random() % VIRTUAL_MEMORY_SIZE;
		scanf("%s",s);
	    addr=random() % VIRTUAL_MEMORY_SIZE;		
		/* ��������������� */
		switch (random() % 3)
		{
			case 0: //������
			{
				//ptr_memAccReq->reqType = REQUEST_READ;
				number=0;
				printf("��������\n��ַ��%lu\t���ͣ���ȡ pid:%d\n", addr,pid);
				fprintf(fp,"%d %lu %d",number,addr,pid);
				break;
			}
			case 1: //д����
			{
				number=1;
				//ptr_memAccReq->reqType = REQUEST_WRITE;
				/* ���������д���ֵ */
				value = random() % 0xFFu;
				fprintf(fp,"%d %lu %02X %d",number,addr,value,pid);
				printf("��������\n��ַ��%lu\t���ͣ�д��\tֵ��%02X pid:%d\n",addr, value,pid);
				break;
			}
			case 2:
			{
				number=2;
				//ptr_memAccReq->reqType = REQUEST_EXECUTE;
				fprintf(fp,"%d %lu %d",number,addr,pid);
				printf("��������\n��ַ��%lu\t���ͣ�ִ�� pid:%d\n", addr,pid);
				break;
			}
			default:
				break;
		}	
		fclose(fp);
		sleep(5);
	}
	return 0;
}