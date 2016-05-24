#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

#include "vmm.h"



/* ҳ�� */
PageTableItem pageTable[4][SecondPAGE_SUM][PAGE_SUM];
/* ʵ��ռ� */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* ���ļ�ģ�⸨��ռ� */
FILE *ptr_auxMem;
FILE *fp;
/* �����ʹ�ñ�ʶ */
BOOL blockStatus[BLOCK_SUM];
/* �ô����� */
Ptr_MemoryAccessRequest ptr_memAccReq;



/* ��ʼ������ */
void do_init()
{
	unsigned int i, j, k,n;
	srand(time(NULL));
	for(n=0;n<4;n++){
	for (k = 0; k<SecondPAGE_SUM;k++){
		for (i = 0; i < PAGE_SUM; i++)
		{
			pageTable[n][k][i].pageNum = i;
			pageTable[n][k][i].filled = FALSE;
			pageTable[n][k][i].edited = FALSE;
			pageTable[n][k][i].count = 0;
			for(j=0;j<8;j++)
			{
				pageTable[n][k][i].LRUcount[j]=0;
			}
			pageTable[n][k][i].processNum=n;
			pageTable[n][k][i].visited=0;

			/* ʹ����������ø�ҳ�ı������� */
			switch (rand() % 7)
			{
				case 0:
				{
					pageTable[n][k][i].proType = READABLE;
					break;
				}
				case 1:
				{
					pageTable[n][k][i].proType = WRITABLE;
					break;
				}
				case 2:
				{
					pageTable[n][k][i].proType = EXECUTABLE;
					break;
				}
				case 3:
				{
					pageTable[n][k][i].proType = READABLE | WRITABLE;
					break;
				}
				case 4:
				{
					pageTable[n][k][i].proType = READABLE | EXECUTABLE;
					break;
				}
				case 5:
				{
					pageTable[n][k][i].proType = WRITABLE | EXECUTABLE;
					break;
				}
				case 6:
				{
					pageTable[n][k][i].proType = READABLE | WRITABLE | EXECUTABLE;
					break;
				}
				default:
					break;
			}
			/* ���ø�ҳ��Ӧ�ĸ����ַ */
			pageTable[n][k][i].auxAddr =(k*8+ i) * PAGE_SIZE * 2;

		}
	}
	}
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* ���ѡ��һЩ��������ҳ��װ�� */
		if (rand() % 2 == 0)
		{
			do_page_in(&pageTable[j%4][j/8][j%8], j);
			pageTable[j%4][j/8][j%8].blockNum = j;
			pageTable[j%4][j/8][j%8].filled = TRUE;
			blockStatus[j] = TRUE;
		}
		else
			blockStatus[j] = FALSE;
	}
}


/* ��Ӧ���� */
void do_response(int processNum)//��Ӧ�����ʱ��������������ʹ�
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum1, pageNum2,offAddr;
	unsigned int actAddr;
	
	/* ����ַ�Ƿ�Խ�� */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	
	/* ����ҳ�ź�ҳ��ƫ��ֵ */
	pageNum1 = (ptr_memAccReq->virAddr / PAGE_SIZE)/SecondPAGE_SUM;//ҳ��Ŀ¼��
	pageNum2 = (ptr_memAccReq->virAddr / PAGE_SIZE)%SecondPAGE_SUM;//ҳ����
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("���̺ţ�%d\tҳĿ¼��Ϊ��%u\tҳ��Ϊ��%u\tҳ��ƫ��Ϊ��%u\n", processNum+1,pageNum1, pageNum2,offAddr);

	/* ��ȡ��Ӧҳ���� */
	ptr_pageTabIt = &pageTable[processNum][pageNum1][pageNum2];
	
	/* ��������λ�����Ƿ����ȱҳ�ж� */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt,processNum);
	}
	
	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("ʵ��ַΪ��%u\n", actAddr);
	
	/* ���ҳ�����Ȩ�޲�����ô����� */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //������
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->visited=1;
			if (!(ptr_pageTabIt->proType & READABLE)) //ҳ�治�ɶ�
			{
				do_error(ERROR_READ_DENY);
				return;
			}
			/* ��ȡʵ���е����� */
			printf("�������ɹ���ֵΪ%02x\n", actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //д����
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->visited=1;
			if (!(ptr_pageTabIt->proType & WRITABLE)) //ҳ�治��д
			{
				do_error(ERROR_WRITE_DENY);	
				return;
			}
			/* ��ʵ����д����������� */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;			
			printf("д�����ɹ�,д��ֵΪ%02x\n",ptr_memAccReq->value);
			break;
		}
		case REQUEST_EXECUTE: //ִ������
		{
			ptr_pageTabIt->count++;
			ptr_pageTabIt->visited=1;
			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //ҳ�治��ִ��
			{
				do_error(ERROR_EXECUTE_DENY);
				return;
			}			
			printf("ִ�гɹ�\n");
			break;
		}
		default: //�Ƿ���������
		{	
			do_error(ERROR_INVALID_REQUEST);
			return;
		}
	}
}

/* ����ȱҳ�ж� */
void do_page_fault(Ptr_PageTableItem ptr_pageTabIt,int processNum)
{
	unsigned int i;
	printf("����ȱҳ�жϣ���ʼ���е�ҳ...\n");
	for (i = 0; i < BLOCK_SUM; i++)
	{
		if (!blockStatus[i])
		{
			/* ���������ݣ�д�뵽ʵ�� */
			do_page_in(ptr_pageTabIt, i);
			
			/* ����ҳ������ */
			ptr_pageTabIt->blockNum = i;
			ptr_pageTabIt->filled = TRUE;
			ptr_pageTabIt->edited = FALSE;
			ptr_pageTabIt->count = 0;
			
			blockStatus[i] = TRUE;
			return;
		}
	}
	/* û�п�������飬����ҳ���滻 */
	do_LRU(ptr_pageTabIt,processNum);
}

/* ����LRU�㷨����ҳ���滻 */
void do_LRU(Ptr_PageTableItem ptr_pageTabIt,int processNum)
{
	unsigned int i, j,k,min, page1,page2;
	unsigned int Temp=0;
	printf("û�п�������飬��ʼ����LFUҳ���滻...\n");
	for (k = 0; k < SecondPAGE_SUM; k++){
		for (i = 0, min = 0xFFFFFFFF, page1 = 0,page2=0; i < PAGE_SUM; i++)
		{
			for(j=0;j<8;j++){
				Temp=10*Temp+pageTable[processNum][k][i].LRUcount[j];
			}
			if (Temp < min)
			{
				min = Temp;
				page1 = k;
				page2 = i;
			}
		}
	}
	printf("ѡ���%uҳ�����滻\n", page1*SecondPAGE_SUM+i);
	if (pageTable[processNum][page1][page2].edited)
	{
		/* ҳ���������޸ģ���Ҫд�������� */
		printf("��ҳ�������޸ģ�д��������\n");
		do_page_out(&pageTable[processNum][page1][page2]);
	}
	pageTable[processNum][page1][page2].filled = FALSE;
	for(j=0;j<8;j++)
	{
		pageTable[processNum][page1][page2].LRUcount[j]=0;
	}



	/* ���������ݣ�д�뵽ʵ�� */
	do_page_in(ptr_pageTabIt, pageTable[processNum][page1][page2].blockNum);
	
	/* ����ҳ������ */
	ptr_pageTabIt->blockNum = pageTable[processNum][page1][page2].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	for(j=0;j<8;j++)
	{
		ptr_pageTabIt->LRUcount[j]=0;
	}
	printf("ҳ���滻�ɹ�\n");
}

void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((readNum = fread(actMem + blockNum * PAGE_SIZE, 
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
	printf("��ҳ�ɹ������̺�%d �����ַ%u-->>�����%u\n", ptr_pageTabIt->processNum,ptr_pageTabIt->auxAddr, blockNum);
}

/* �����滻ҳ�������д�ظ��� */
void do_page_out(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int writeNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((writeNum = fwrite(actMem + ptr_pageTabIt->blockNum * PAGE_SIZE, 
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: writeNum=%u\n", writeNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
	printf("д�سɹ��������%u-->>���̺�%d �����ַ%03X\n", ptr_pageTabIt->auxAddr,ptr_pageTabIt->processNum, ptr_pageTabIt->blockNum);
}

/* ������ */
void do_error(ERROR_CODE code)
{
	switch (code)
	{
		case ERROR_READ_DENY:
		{
			printf("�ô�ʧ�ܣ��õ�ַ���ݲ��ɶ�\n");
			break;
		}
		case ERROR_WRITE_DENY:
		{
			printf("�ô�ʧ�ܣ��õ�ַ���ݲ���д\n");
			break;
		}
		case ERROR_EXECUTE_DENY:
		{
			printf("�ô�ʧ�ܣ��õ�ַ���ݲ���ִ��\n");
			break;
		}		
		case ERROR_INVALID_REQUEST:
		{
			printf("�ô�ʧ�ܣ��Ƿ��ô�����\n");
			break;
		}
		case ERROR_OVER_BOUNDARY:
		{
			printf("�ô�ʧ�ܣ���ַԽ��\n");
			break;
		}
		case ERROR_FILE_OPEN_FAILED:
		{
			printf("ϵͳ���󣺴��ļ�ʧ��\n");
			break;
		}
		case ERROR_FILE_CLOSE_FAILED:
		{
			printf("ϵͳ���󣺹ر��ļ�ʧ��\n");
			break;
		}
		case ERROR_FILE_SEEK_FAILED:
		{
			printf("ϵͳ�����ļ�ָ�붨λʧ��\n");
			break;
		}
		case ERROR_FILE_READ_FAILED:
		{
			printf("ϵͳ���󣺶�ȡ�ļ�ʧ��\n");
			break;
		}
		case ERROR_FILE_WRITE_FAILED:
		{
			printf("ϵͳ����д���ļ�ʧ��\n");
			break;
		}
		default:
		{
			printf("δ֪����û������������\n");
		}
	}
}

/* ��ӡҳ�� */
void do_print_info(int processNum)
{
	unsigned int i, k;
	char str[4];
	printf("ҳ��\t���\tװ��\t�޸�\t����\t����\t����\n");
	for(k = 0; k< SecondPAGE_SUM;k++){
	for (i = 0; i < PAGE_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%s\t%u\t%u\n", k*8+i, pageTable[processNum][k][i].blockNum, pageTable[processNum][k][i].filled, 
			pageTable[processNum][k][i].edited, get_proType_str(str, pageTable[processNum][k][i].proType), 
			pageTable[processNum][k][i].count, pageTable[processNum][k][i].auxAddr);
	}
	}
}

/* ��ȡҳ�汣�������ַ��� */
char *get_proType_str(char *str, BYTE type)
{
	if (type & READABLE)
		str[0] = 'r';
	else
		str[0] = '-';
	if (type & WRITABLE)
		str[1] = 'w';
	else
		str[1] = '-';
	if (type & EXECUTABLE)
		str[2] = 'x';
	else
		str[2] = '-';
	str[3] = '\0';
	return str;
}

int main(int argc, char* argv[])
{
	char c;
	char Request[20]={'\0'};
	char buffer[5]={'\0'};
	char CurrentRequest[20]={'\0'};
	int i,j,k,n,underlinecount;
	int Requestcount=0;
	int virAddr,reqType;
	int value;
	int processNum=1;
	int RequestCount=0;//����ÿ����һ��������͸���ҳ�����λ���޸�LRUcount;
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY1, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}

	do_init();
	do_print_info(1);

	/* ��ѭ����ģ��ô������봦����� */
	while (TRUE)
	{
		virAddr=0;
		reqType=0;
		value=0;
		processNum=0;
		Requestcount++;

		if(!(fp = fopen("MYFIFO","r"))){
			do_error(ERROR_FILE_OPEN_FAILED);
			exit(1);
		}
		for(n=1;n<=Requestcount;n++){
			if(fgets(CurrentRequest,256,fp)==NULL){
				printf("ϵͳ���󣺶����ļ�ʧ��\n");
				break;
			}
		}
		fclose(fp);
		printf("�����ȡ���:%s\n",CurrentRequest);
		underlinecount=0;
		for(j=0;j<strlen(CurrentRequest)-1;j++){
			if(CurrentRequest[j]=='_'){
				underlinecount++;
			}
			else{
				if(underlinecount==0){
					virAddr=10*virAddr+CurrentRequest[j]-'0';
				}
				else if(underlinecount==1){
					reqType=10*reqType+CurrentRequest[j]-'0';
				}
				else if(underlinecount==2){
					value=10*value+CurrentRequest[j]-'0';
				}
				else{
					processNum=10*processNum+CurrentRequest[j]-'0';
				}
			}
		}
		printf("%d %d %d %d",virAddr,reqType,value,processNum);//ģ���ֶ��������
		ptr_memAccReq->virAddr=virAddr;
		ptr_memAccReq->reqType=reqType;
		ptr_memAccReq->value=value;
		ptr_memAccReq->processNum=processNum;
		if(processNum==1){
			if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY1, "r+")))
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				exit(1);
			}
		}
		else if(processNum==2){
			if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY2, "r+")))
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				exit(1);
			}
		}
		else if(processNum==3){			
			if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY3, "r+")))
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				exit(1);
			}
		}
		else if(processNum=4){
			if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY4, "r+")))
			{
				do_error(ERROR_FILE_OPEN_FAILED);
				exit(1);
			}
		}
		do_response(processNum);
		RequestCount++;
		printf("��Y��ӡҳ��������������ӡ...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info(processNum);
		while (c != '\n')
			c = getchar();
		printf("��E��ӡʵ�棬������������ӡ...\n");
		if ((c = getchar()) == 'E' || c == 'e')
		{
			for(i=0;i<ACTUAL_MEMORY_SIZE;i=i+4){
				printf("�����ţ�%d:%c%c%c%c\n",i/4,actMem[i],actMem[i+1],actMem[i+2],actMem[i+3]);
			}
		}
		while (c != '\n')
			c = getchar();
		printf("��R��ӡ���棬������������ӡ...\n");
		if ((c = getchar()) == 'R' || c == 'r')
		{
			for(i=0;i<VIRTUAL_MEMORY_SIZE;i=i+4){
				fread(buffer,sizeof(BYTE),4,ptr_auxMem);
				buffer[4]='\0';
				printf("�����ţ�%d:%s\n",i/4,buffer);
			}
		}
		while (c != '\n')
			c = getchar();
		printf("��X�˳����򣬰�����������...\n");
		if ((c = getchar()) == 'x' || c == 'X')
			break;
		while (c != '\n')
			c = getchar();
		if(RequestCount==100){
			RequestCount=0;
			for(n=0;n<4;n++){
			for(k=0;k<SecondPAGE_SUM;k++){
				for(i=0;i<PAGE_SUM;i++){
					for(j=7;j>0;j++){
						pageTable[n][k][i].LRUcount[j]=pageTable[n][k][i].LRUcount[j-1];
						}
					pageTable[n][k][i].LRUcount[0]=pageTable[n][k][i].visited;
					}
				}
			}
		}
		}

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}