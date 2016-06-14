#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "vmm.h"

/* 页表 */
PageTableItem fullpageTable[8][8][8];
PageTableItem (*pageTable)[8][8];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;

Ptr_PageTableItem actualMemory[BLOCK_SUM];

char s[2100];
FILE *aux;

node stree[maxn<<2];
int flag[maxn],timer=1;

/* 初始化环境 */
void do_init()
{
	int i, j, k, pro;
	srandom(time(NULL));
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			for (k = 0; k < 8; k++)
			{
				fullpageTable[i][j][k].pageNum = 8*j + k;
				fullpageTable[i][j][k].filled = FALSE;
				fullpageTable[i][j][k].edited = FALSE;
				fullpageTable[i][j][k].count = 0;
				/* 使用随机数设置该页的保护类型 */
				switch (random() % 7)
				{
					case 0:
					{
						fullpageTable[i][j][k].proType = READABLE;
						break;
					}
					case 1:
					{
						fullpageTable[i][j][k].proType = WRITABLE;
						break;
					}
					case 2:
					{
						fullpageTable[i][j][k].proType = EXECUTABLE;
						break;
					}
					case 3:
					{
						fullpageTable[i][j][k].proType = READABLE | WRITABLE;
						break;
					}
					case 4:
					{
						fullpageTable[i][j][k].proType = READABLE | EXECUTABLE;
						break;
					}
					case 5:
					{
						fullpageTable[i][j][k].proType = WRITABLE | EXECUTABLE;
						break;
					}
					case 6:
					{
						fullpageTable[i][j][k].proType = READABLE | WRITABLE | EXECUTABLE;
						break;
					}
					default:
						break;
				}
				/* 设置该页对应的辅存地址 */
				fullpageTable[i][j][k].auxAddr = (i*8 + j*8 + k) * PAGE_SIZE;
			}
		}
	}
	
	pageTable = fullpageTable;
	
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
//		pro = random() % 8;
		i = j / 8; k = j % 8;
		if (random() % 2 == 0)
		{
			do_page_in(&pageTable[0][i][k], j);
			pageTable[0][i][k].blockNum = j;
			pageTable[0][i][k].filled = TRUE;
			blockStatus[j] = TRUE;
		}
		else
			blockStatus[j] = FALSE;
	}
}


/* 响应请求 */
void do_switch()
{
	pageTable = &(fullpageTable[ptr_memAccReq->value]);
}
void do_response()
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum, offAddr;
	unsigned int actAddr;
	static int counter=10;
	if(--counter<=0) {
		div2(); counter=10;
	}

	if(ptr_memAccReq->reqType==REQUEST_SWITCH) 
	{
		do_switch();
		return;
	}	
	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		printf("error1\n");
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	
	/* 计算页号和页内偏移值 */
	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("页号为：%u\t页内偏移为：%u\n", pageNum, offAddr);

	/* 获取对应页表项 */
	ptr_pageTabIt = (*pageTable)[pageNum/8]+pageNum%8;;
	
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_pageTabIt->filled)
	{
		do_page_fault(ptr_pageTabIt);
	}
	
	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
	printf("实地址为：%u\n", actAddr);
	
	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			ptr_pageTabIt->count++;
			aaaaaaccess(ptr_pageTabIt->blockNum);
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				printf("error2\n");
				do_error(ERROR_READ_DENY);
				return;
			}
			/* 读取实存中的内容 */
			printf("读操作成功：值为%02X\n", actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			ptr_pageTabIt->count++;
			aaaaaaccess(ptr_pageTabIt->blockNum);
			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
			{
				printf("error3\n");
				do_error(ERROR_WRITE_DENY);	
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;			
			printf("写操作成功\n");
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			ptr_pageTabIt->count++;
			aaaaaaccess(ptr_pageTabIt->blockNum);
			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
			{
				printf("error4\n");
				do_error(ERROR_EXECUTE_DENY);
				return;
			}			
			printf("执行成功\n");
			break;
		}
		default: //非法请求类型
		{	
			printf("error5\n");
			do_error(ERROR_INVALID_REQUEST);
			return;
		}
	}
}

/* 处理缺页中断 */
void do_page_fault(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i;
	printf("产生缺页中断，开始进行调页...\n");
	for (i = 0; i < BLOCK_SUM; i++)
	{
		if (!blockStatus[i])
		{
			/* 读辅存内容，写入到实存 */
			do_page_in(ptr_pageTabIt, i);
			
			/* 更新页表内容 */
			ptr_pageTabIt->blockNum = i;
			ptr_pageTabIt->filled = TRUE;
			ptr_pageTabIt->edited = FALSE;
			ptr_pageTabIt->count = 0;
			
			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	do_LFU(ptr_pageTabIt);
}

/* 根据LFU算法进行页面替换 */
void do_LFU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, min, page;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	/*for (i = 0, min = 0xFFFFFFFF, page = 0; i < BLOCK_SUM; i++)
	{
		if (actualMemory[i]->count < min)
		{
			min = actualMemory[i]->count;
			page = i;
		}
	}*/
	page=clear();
	printf("选择第%u页进行替换\n", page);
	if (actualMemory[i]->edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(actualMemory[page]);
	}
	actualMemory[i]->filled = FALSE;
	actualMemory[i]->count = 0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt,page);
		
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum=page;
	ptr_pageTabIt->filled=TRUE;
	ptr_pageTabIt->edited=FALSE;
	ptr_pageTabIt->count=0;
	blockStatus[page]=TRUE;
	printf("页面替换成功\n");
}

/*yemianlaohua*/
void pushup(int pos) {
	node *cn=stree+pos,*lc=&stree[pos<<1],*rc=&stree[pos<<1|1];
	if(cn->l==cn->r) return;
	if((lc->val>>lc->lazy)<(rc->val>>rc->lazy)) {
		cn->minpos=lc->minpos;
		cn->val=lc->val>>lc->lazy;
	}
	else {
		cn->minpos=rc->minpos;
		cn->val=rc->val>>rc->lazy;
	}
}

void pushdown(int pos) {
	node *cn=stree+pos,*lc=&stree[pos<<1],*rc=&stree[pos<<1|1];
	if(cn->l==cn->r) return;
	lc->lazy+=cn->lazy;
	rc->lazy+=cn->lazy;
	cn->lazy=0;
	pushup(pos);
}

void build(int l,int r,int pos) {
	node *cn=&stree[pos];
	cn->l=l; cn->r=r;
	if(l==r) {
		cn->val=0;
		cn->minpos=l;
		cn->lazy=0;
		return;
	}
	build(l,(l+r)>>1,pos<<1);
	build(((l+r)>>1)+1,r,pos<<1|1);
	pushup(pos);
}

void div2() { ++stree[1].lazy; ++timer; }

void update(int x,int pos,int flag) {
	node *cn=&stree[pos];
	int mid=(cn->l+cn->r)>>1;
	if(cn->l==cn->r) {
		if(!flag) cn->val=0;
		else {
			cn->val>>=cn->lazy;
			cn->val|=1LL<<63;
		}
		cn->lazy=0;
		return;
	}
	pushdown(pos);
	update(x,pos<<1|(x>mid),flag);
	pushup(pos);
}

void aaaaaaccess(int x) {
	if(flag[x]==timer) return;
	update(x,1,1);
}

int clear() {
	int ans=stree[1].minpos;
	update(ans,1,0);
	flag[ans]=0;
	return ans;
}


/* 将辅存内容写入实存 */
void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
#endif
		printf("error6\n");
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
		printf("error7\n");
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
	actualMemory[blockNum]=ptr_pageTabIt;
	printf("调页成功：辅存地址%u-->>物理块%u\n", ptr_pageTabIt->auxAddr, blockNum);
}

/* 将被替换页面的内容写回辅存 */
void do_page_out(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int writeNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt, ftell(ptr_auxMem));
#endif
		printf("error8\n");
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
		printf("error9\n");
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
	printf("写回成功：物理块%u-->>辅存地址%03X\n", ptr_pageTabIt->auxAddr, ptr_pageTabIt->blockNum);
}

/* 错误处理 */
void do_error(ERROR_CODE code)
{
	switch (code)
	{
		case ERROR_READ_DENY:
		{
			printf("访存失败：该地址内容不可读\n");
			break;
		}
		case ERROR_WRITE_DENY:
		{
			printf("访存失败：该地址内容不可写\n");
			break;
		}
		case ERROR_EXECUTE_DENY:
		{
			printf("访存失败：该地址内容不可执行\n");
			break;
		}		
		case ERROR_INVALID_REQUEST:
		{
			printf("访存失败：非法访存请求\n");
			break;
		}
		case ERROR_OVER_BOUNDARY:
		{
			printf("访存失败：地址越界\n");
			break;
		}
		case ERROR_FILE_OPEN_FAILED:
		{
			printf("系统错误：打开文件失败\n");
			break;
		}
		case ERROR_FILE_CLOSE_FAILED:
		{
			printf("系统错误：关闭文件失败\n");
			break;
		}
		case ERROR_FILE_SEEK_FAILED:
		{
			printf("系统错误：文件指针定位失败\n");
			break;
		}
		case ERROR_FILE_READ_FAILED:
		{
			printf("系统错误：读取文件失败\n");
			break;
		}
		case ERROR_FILE_WRITE_FAILED:
		{
			printf("系统错误：写入文件失败\n");
			break;
		}
		default:
		{
			printf("未知错误：没有这个错误代码\n");
		}
	}
}

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k;
	char str[4];
	Ptr_PageTableItem nowpageTable;
	printf("页号\t块号\t装入\t修改\t保护\t计数\t辅存\n");
	for (i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
		{
			nowpageTable = (*pageTable)[i]+j;
			printf("%u\t%u\t%u\t%u\t%s\t%u\t%u\n", i, 
			nowpageTable->blockNum, nowpageTable->filled, 
			nowpageTable->edited, get_proType_str(str, 
			nowpageTable->proType), nowpageTable->count, 
			nowpageTable->auxAddr);
	}
}

/* 打印实存 */
void do_print_actMem()
{
	unsigned int i,j;
	printf("实存:\n");
	for(i = 0; i < 4; i++) printf("地址:\t内容:\t");
	printf("\n");
	for(i = 0; i < 32; i++)
	{
		for(j = 0; j < 4; j++)
		{
			printf("%d\t%c\t",j*32+i,actMem[j*32+i]);
		}		
		printf("\n");
	}
}

/* 打印辅存 */
void do_print_auxMem()
{
	unsigned int i,j;
	aux = fopen(AUXILIARY_MEMORY, "r");
	fscanf(aux, "%s", s); close(aux);
	printf("辅存:\n");
	for (i = 0; i < 4; i++) printf("地址:\t内容:\t");
	printf("\n");
	for (i = 0; i<512; i++)
	{
		for (j = 0; j<4; j++)
		{
			printf("%d\t%c\t",j*64+i,s[j*64+i]);
		}
		printf("\n");
	}
}

/* 获取页面保护类型字符串 */
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

void do_receive(){
	int fifo;
	if ((fifo = open("/tmp/server",O_RDONLY))<0)
		error_sys("open fifo failed");
	read(fifo,ptr_memAccReq,sizeof(MemoryAccessRequest));
	close(fifo);
}
int main(int argc, char* argv[])
{
	char c;
	int i;
	char buffer[256*8];
	int fifo;
	struct stat statbuf;

	build(0,maxn-1,1);
	if (stat("/tmp/server",&statbuf) == 0){
		/* 如果FIFO文件存在,删掉 */
		if (remove("/tmp/server") < 0)
			error_sys("remove failed");
	}

	if (mkfifo("/tmp/server",0666) < 0)
		error_sys("mkfifo failed");

	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		printf("error10\n");
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	for (int j = 0; j < 256*8; j++){
		buffer[j] = 'a'+rand()%26;
	}
	fwrite(buffer,sizeof(char),256*8,ptr_auxMem);

	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
//		do_request();
		do_receive();
		do_response();
		printf("按Y打印页表,按S打印实存，按F打印辅存,按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info();
		else if(c == 's' || c == 'S')
			do_print_actMem();
		else if (c == 'f' || c == 'F')
			do_print_auxMem();
		while (c != '\n')
			c = getchar();
		printf("按X退出程序，按其他键继续...\n");
		if ((c = getchar()) == 'x' || c == 'X')
			break;
		while (c != '\n')
			c = getchar();
		//sleep(5000);
	}

	if (fclose(ptr_auxMem) == EOF)
	{
		printf("error11\n");
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}
