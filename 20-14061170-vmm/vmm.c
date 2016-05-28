#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vmm.h"

/* 页表 */
PageTableItem pageTable[PAGE_SUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;


/*first pagetable*/
PageTableItem pageTable1[4];
/*second pagetable*/
PageTableItem pageTable2[4][4];
/*third pagetable*/
PageTableItem pageTable3[16][4];
/* \B3\F5ʼ\BB\AF\BB\B7\BE\B3 */
void do_init()
{
	int i, j, l, m, n, k;
	srandom(time(NULL));
	/*initialize the first pagetable*/
	for (i = 0; i < 4; i++)
	{
		pageTable1[i].pageNum = i;
		pageTable1[i].filled = FALSE;
		pageTable1[i].edited = FALSE;
		pageTable1[i].R = 0;
		pageTable1[i].counter = 0;
		pageTable1[i].count = 0;
		/* 使用随机数设置该页的保护类型 */
		switch (random() % 7)
		{
			case 0:
			{
				pageTable1[i].proType = READABLE;
				pageTable1[i].processNum = 0;
				break;
			}
			case 1:
			{
				pageTable1[i].proType = WRITABLE;
				pageTable1[i].processNum = 1;
				break;
			}
			case 2:
			{
				pageTable1[i].proType = EXECUTABLE;
				pageTable1[i].processNum = 2;
				break;
			}
			case 3:
			{
				pageTable1[i].proType = READABLE | WRITABLE;
				pageTable1[i].processNum = 3;
				break;
			}
			case 4:
			{
				pageTable1[i].proType = READABLE | EXECUTABLE;
				pageTable1[i].processNum = 4;
				break;
			}
			case 5:
			{
				pageTable1[i].proType = WRITABLE | EXECUTABLE;
				pageTable1[i].processNum = 5;
				break;
			}
			case 6:
			{
				pageTable1[i].proType = READABLE | WRITABLE | EXECUTABLE;
				pageTable1[i].processNum = 6;
				break;
			}
			default:
				break;
		}
		/* 设置该页对应的辅存地址 */
		/*pageTable[i].auxAddr = i * PAGE_SIZE * 2;*/
	}
	/*initialize the second pagetable*/
	for (l = 0; l < 4; l++)
	{
		for(m = 0; m < 4; m++)
		{
			pageTable2[l][m].pageNum = i;
			pageTable2[l][m].filled = FALSE;
			pageTable2[l][m].edited = FALSE;
			pageTable2[l][m].R = 0;
			pageTable2[l][m].counter = 0;
			pageTable2[l][m].count = 0;
			/* 使用随机数设置该页的保护类型 */
			switch (random() % 7)
			{
				case 0:
				{
					pageTable2[l][m].proType = READABLE;
					pageTable2[l][m].processNum = 0;
					break;
				}
				case 1:
				{
					pageTable2[l][m].proType = WRITABLE;
					pageTable2[l][m].processNum = 1;
					break;
				}
				case 2:
				{
					pageTable2[l][m].proType = EXECUTABLE;
					pageTable2[l][m].processNum = 2;
					break;
				}
				case 3:
				{
					pageTable2[l][m].proType = READABLE | WRITABLE;
					pageTable2[l][m].processNum = 3;
					break;
				}
				case 4:
				{
					pageTable2[l][m].proType = READABLE | EXECUTABLE;
					pageTable2[l][m].processNum = 4;
					break;
				}
				case 5:
				{
					pageTable2[l][m].proType = WRITABLE | EXECUTABLE;
					pageTable2[l][m].processNum = 5;
					break;
				}
				case 6:
				{
					pageTable2[l][m].proType = READABLE | WRITABLE | EXECUTABLE;
					pageTable2[l][m].processNum = 6;
					break;
				}
				default:
					break;
			}
		}
	}
	/*initialize the third pagetable*/
	for (n = 0; n < 16; n++)
	{
		for(k = 0; k < 4; k++)
		{
			pageTable3[n][k].pageNum = i;
			pageTable3[n][k].filled = FALSE;
			pageTable3[n][k].edited = FALSE;
			pageTable3[n][k].R = 0;
			pageTable3[n][k].counter = 0;
			pageTable3[n][k].count = 0;
			/* 使用随机数设置该页的保护类型 */
			switch (random() % 7)
			{
				case 0:
				{
					pageTable3[n][k].proType = READABLE;
					pageTable3[n][k].processNum = 0;
					break;
				}
				case 1:
				{
					pageTable3[n][k].proType = WRITABLE;
					pageTable3[n][k].processNum = 1;
					break;
				}
				case 2:
				{
					pageTable3[n][k].proType = EXECUTABLE;
					pageTable3[n][k].processNum = 2;
					break;
				}
				case 3:
				{
					pageTable3[n][k].proType = READABLE | WRITABLE;
					pageTable3[n][k].processNum = 3;
					break;
				}
				case 4:
				{
					pageTable3[n][k].proType = READABLE | EXECUTABLE;
					pageTable3[n][k].processNum = 4;
					break;
				}
				case 5:
				{
					pageTable3[n][k].proType = WRITABLE | EXECUTABLE;
					pageTable3[n][k].processNum = 5;
					break;
				}
				case 6:
				{
					pageTable3[n][k].proType = READABLE | WRITABLE | EXECUTABLE;
					pageTable3[n][k].processNum = 6;
					break;
				}
				default:
					break;
			}
			pageTable3[n][k].auxAddr = ((n+1)*(k+1)-1) * PAGE_SIZE * 2;
		}
	}
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (random() % 2 == 0)
		{
			switch((j+1)%4)
			{
				case 0:
				{
					do_page_in(&pageTable3[(j+1)/4-1][3], j);
					pageTable3[(j+1)/4-1][3].blockNum = j;
					pageTable3[(j+1)/4-1][3].filled = TRUE;
					blockStatus[j] = TRUE;
				}
				case 1:
				{
					do_page_in(&pageTable3[(j+1)/4-1][0], j);
					pageTable3[(j+1)/4-1][0].blockNum = j;
					pageTable3[(j+1)/4-1][0].filled = TRUE;
					blockStatus[j] = TRUE;
				}
				case 2:
				{
					do_page_in(&pageTable3[(j+1)/4-1][1], j);
					pageTable3[(j+1)/4-1][1].blockNum = j;
					pageTable3[(j+1)/4-1][1].filled = TRUE;
					blockStatus[j] = TRUE;
				}
				case 3:
				{
					do_page_in(&pageTable3[(j+1)/4-1][2], j);
					pageTable3[(j+1)/4-1][2].blockNum = j;
					pageTable3[(j+1)/4-1][2].filled = TRUE;
					blockStatus[j] = TRUE;
				}
			}
		}
		else
			blockStatus[j] = FALSE;
	}
}


/* 响应请求 */
void do_response()
{
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum, offAddr;
	unsigned int actAddr;
	
	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	
	/* 计算页号和页内偏移值 */
	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("页号为：%u\t页内偏移为：%u\n", pageNum, offAddr);

	/* 获取对应页表项 */
	switch((pageNum+1)%4)
	{
		case 0:
		{
			ptr_pageTabIt = &pageTable3[(pageNum+1)/4][3];
		}
		case 1:
		{
			ptr_pageTabIt = &pageTable3[(pageNum+1)/4][0];
		}
		case 2:
		{
			ptr_pageTabIt = &pageTable3[(pageNum+1)/4][1];
		}
		case 3:
		{
			ptr_pageTabIt = &pageTable3[(pageNum+1)/4][2];
		}
	}
	/*ptr_pageTabIt = &pageTable[pageNum];*/
	
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
			if (!(ptr_pageTabIt->proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				return;
			}
			if(ptr_pageTabIt->processNum != ptr_memAccReq->processNum)
			{
				do_error(PROCESS_NUM_NOTMATCH);
				return;
			}
			/* 读取实存中的内容 */
			printf("读操作成功：值为%02X\n", actMem[actAddr]);
			switch((pageNum+1)%4)
			{
				case 0:
				{
					pageTable3[(pageNum+1)/4][3].R = 1;
				}
				case 1:
				{
					pageTable3[(pageNum+1)/4][0].R = 1;
				}
				case 2:
				{
					pageTable3[(pageNum+1)/4][1].R = 1;
				}
				case 3:
				{
					pageTable3[(pageNum+1)/4][2].R = 1;
				}
			}
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);	
				return;
			}
			if(ptr_pageTabIt->processNum != ptr_memAccReq->processNum)
			{
				do_error(PROCESS_NUM_NOTMATCH);
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_pageTabIt->edited = TRUE;			
			printf("写操作成功\n");
			switch((pageNum+1)%4)
			{
				case 0:
				{
					pageTable3[(pageNum+1)/4][3].R = 1;
				}
				case 1:
				{
					pageTable3[(pageNum+1)/4][0].R = 1;
				}
				case 2:
				{
					pageTable3[(pageNum+1)/4][1].R = 1;
				}
				case 3:
				{
					pageTable3[(pageNum+1)/4][2].R = 1;
				}
			}
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			ptr_pageTabIt->count++;
			if (!(ptr_pageTabIt->proType & EXECUTABLE)) //页面不可执行
			{
				do_error(ERROR_EXECUTE_DENY);
				return;
			}
			if(ptr_pageTabIt->processNum != ptr_memAccReq->processNum)
			{
				do_error(PROCESS_NUM_NOTMATCH);
				return;
			}			
			printf("执行成功\n");
			switch((pageNum+1)%4)
			{
				case 0:
				{
					pageTable3[(pageNum+1)/4][3].R = 1;
				}
				case 1:
				{
					pageTable3[(pageNum+1)/4][0].R = 1;
				}
				case 2:
				{
					pageTable3[(pageNum+1)/4][1].R = 1;
				}
				case 3:
				{
					pageTable3[(pageNum+1)/4][2].R = 1;
				}
			}
			break;
		}
		default: //非法请求类型
		{	
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
			ptr_pageTabIt->R = 0;
			ptr_pageTabIt->counter = 0;
			
			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	/*do_LFU(ptr_pageTabIt);*/
	do_PAA(ptr_pageTabIt);
}

/* 根据LFU算法进行页面替换 */
/*void do_LFU(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int i, min, page;
	printf("没有空闲物理块，开始进行LFU页面替换...\n");
	for (i = 0, min = 0xFFFFFFFF, page = 0; i < PAGE_SUM; i++)
	{
		if (pageTable[i].count < min)
		{
			min = pageTable[i].count;
			page = i;
		}
	}
	printf("选择第%u页进行替换\n", page);
	if (pageTable[page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		/*printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable[page]);
	}
	pageTable[page].filled = FALSE;
	pageTable[page].count = 0;


	/* 读辅存内容，写入到实存 */
	/*do_page_in(ptr_pageTabIt, pageTable[page].blockNum);
	
	/* 更新页表内容 */
	/*ptr_pageTabIt->blockNum = pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	printf("页面替换成功\n");
}*/
/* 根据PAA算法进行页面替换 */
void do_PAA(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int j, l, page1, page2, cout=0;
	printf("没有空闲物理块，开始进行PAA页面替换...\n");
	cout = pageTable3[0][0].counter;
	for(j = 0, page1 = 0, page2 = 0; j < 16; j++)
	{
		for(l = 1; l < 4; l++)
		{
			if(cout < pageTable3[j][l].counter)
			{
				cout = pageTable3[j][l].counter;
				page1 = j;
				page2 = l;
			}	
		}
	}
	printf("选择第%u页进行替换\n", (page1+1)*(page2+1));
	if (pageTable3[page1][page2].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("该页内容有修改，写回至辅存\n");
		do_page_out(&pageTable3[page1][page2]);
	}
	pageTable3[page1][page2].filled = FALSE;
	pageTable3[page1][page2].count = 0;
	pageTable3[page1][page2].R = 0;
	pageTable3[page1][page2].counter = 0;


	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, pageTable3[page1][page2].blockNum);
	
	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = pageTable3[page1][page2].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	ptr_pageTabIt->R = 0;
	ptr_pageTabIt->counter = 0;
	printf("页面替换成功\n");
}

/*update the counter*/
void do_update_counter()
{
	unsigned i;
	for(i = 0; i < PAGE_SUM; i++)
	{
		switch((i+1)%4)
		{
			case 0:
			{
				pageTable3[(i+1)/4][3].counter=(pageTable3[(i+1)/4][3].R<<31)|(pageTable3[(i+1)/4][3].counter>>1);
			}
			case 1:
			{
				pageTable3[(i+1)/4][0].counter=(pageTable3[(i+1)/4][0].R<<31)|(pageTable3[(i+1)/4][0].counter>>1);
			}
			case 2:
			{
				pageTable3[(i+1)/4][2].counter=(pageTable3[(i+1)/4][2].R<<31)|(pageTable3[(i+1)/4][2].counter>>1);
			}
			case 3:
			{
				pageTable3[(i+1)/4][1].counter=(pageTable3[(i+1)/4][1].R<<31)|(pageTable3[(i+1)/4][1].counter>>1);
			}
		}
	}
}

/* 将辅存内容写入实存 */
void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%lu\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((readNum = fread(actMem + blockNum * PAGE_SIZE, 
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%lu\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
	printf("调页成功：辅存地址%lu-->>物理块%u\n", ptr_pageTabIt->auxAddr, blockNum);
}

/* 将被替换页面的内容写回辅存 */
void do_page_out(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int writeNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%lu\tftell=%u\n", ptr_pageTabIt, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((writeNum = fwrite(actMem + ptr_pageTabIt->blockNum * PAGE_SIZE, 
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%lu\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: writeNum=%u\n", writeNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
	printf("写回成功：物理块%lu-->>辅存地址%03X\n", ptr_pageTabIt->auxAddr, ptr_pageTabIt->blockNum);
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
		case PROCESS_NUM_NOTMATCH:
		{
			printf("系统错误：processnumber does not match");
		}
		default:
		{
			printf("未知错误：没有这个错误代码\n");
		}
	}
}

/* 产生访存请求 */
void do_request()
{
	/* 随机产生请求地址 */
	ptr_memAccReq->virAddr = random() % VIRTUAL_MEMORY_SIZE;
	/* 随机产生请求类型 */
	switch (random() % 3)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			ptr_memAccReq->processNum = 0;
			printf("产生请求：\n地址：%lu\t类型：读取\tprocessnumber:%u\n", ptr_memAccReq->virAddr,ptr_memAccReq->processNum);
			break;
		}
		case 1: //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			ptr_memAccReq->processNum = 1;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = random() % 0xFFu;
			printf("产生请求：\n地址：%lu\t类型：写入\t值：%02X\tprocessnumber:%u\n", ptr_memAccReq->virAddr, ptr_memAccReq->value,ptr_memAccReq->processNum);
			break;
		}
		case 2:
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			ptr_memAccReq->processNum = 2;
			printf("产生请求：\n地址：%lu\t类型：执行\tprocessnumber:%u\n", ptr_memAccReq->virAddr,ptr_memAccReq->processNum);
			break;
		}
		default:
			break;
	}	
}

/* 打印页表 */
void do_print_info()
{
	unsigned int i, j, k;
	char str[4];
	printf("页号\t块号\tvisit\tcounter\t装入\t修改\t保护\t计数\t辅存\n");
	for (i = 0; i < PAGE_SUM; i++)
	{
		switch((i+1)%4)
		{
			case 0:
			{
				printf("%u\t%u\t%u\t%u\t%u\t%u\t%s\t%u\t%lu\n", i, pageTable3[(i+1)/4][3].blockNum, pageTable3[(i+1)/4][3].R, 					pageTable3[(i+1)/4][3].counter, pageTable3[(i+1)/4][3].filled, 
					pageTable3[(i+1)/4][3].edited, get_proType_str(str, pageTable3[(i+1)/4][3].proType), 
					pageTable3[(i+1)/4][3].count, pageTable3[(i+1)/4][3].auxAddr);				
			}
			case 1:
			{
				printf("%u\t%u\t%u\t%u\t%u\t%u\t%s\t%u\t%lu\n", i, pageTable3[(i+1)/4][0].blockNum, pageTable3[(i+1)/4][0].R, 					pageTable3[(i+1)/4][0].counter, pageTable3[(i+1)/4][3].filled, 
					pageTable3[(i+1)/4][0].edited, get_proType_str(str, pageTable3[(i+1)/4][0].proType), 
					pageTable3[(i+1)/4][0].count, pageTable3[(i+1)/4][0].auxAddr);
			}
			case 2:
			{
				printf("%u\t%u\t%u\t%u\t%u\t%u\t%s\t%u\t%lu\n", i, pageTable3[(i+1)/4][1].blockNum, pageTable3[(i+1)/4][1].R, 					pageTable3[(i+1)/4][1].counter, pageTable3[(i+1)/4][3].filled, 
					pageTable3[(i+1)/4][1].edited, get_proType_str(str, pageTable3[(i+1)/4][1].proType), 
					pageTable3[(i+1)/4][1].count, pageTable3[(i+1)/4][1].auxAddr);
			}
			case 3:
			{
				printf("%u\t%u\t%u\t%u\t%u\t%u\t%s\t%u\t%lu\n", i, pageTable3[(i+1)/4][2].blockNum, pageTable3[(i+1)/4][2].R, 					pageTable3[(i+1)/4][2].counter, pageTable3[(i+1)/4][3].filled, 
					pageTable3[(i+1)/4][2].edited, get_proType_str(str, pageTable3[(i+1)/4][2].proType), 
					pageTable3[(i+1)/4][2].count, pageTable3[(i+1)/4][2].auxAddr);
			}
		}
		/*printf("%u\t%u\t%u\t%u\t%s\t%u\t%u\n", i, pageTable[i].blockNum, pageTable[i].filled, 
			pageTable[i].edited, get_proType_str(str, pageTable[i].proType), 
			pageTable[i].count, pageTable[i].auxAddr);*/
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

int main(int argc, char* argv[])
{
	char c;
	int i;
	time_t timep, timerc;
	time(&timep);
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	
	do_init();
	do_print_info();
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		do_request();
		do_response();
		time(&timerc);
		if(timerc-timep >= Time)
		{
			do_update_counter;
		}
		printf("按Y打印页表，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info();
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
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	return (0);
}
