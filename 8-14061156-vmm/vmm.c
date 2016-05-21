#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vmm.h"

/* 一级页表 */
MenuItem global[GLOBAL_SUM];
/* 物理二级页表 */
MenuItem act_middle[ACTUAL_MIDDLE_SUM];
/* 物理三级页表 */
EntryItem act_entry[ACTUAL_ENTRY_SUM];
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_virMem,*ptr_virMem_Entry,*ptr_virMem_Middle;
/* 三级页表项使用标识 */
BOOL blockStatus_Entry[ACTUAL_ENTRY_SUM/MENU_SIZE];
/* 二级页表项使用标识*/
BOOL blockStatus_Middle[ACTUAL_MIDDLE_SUM/MENU_SIZE];
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;
/* LRU算法记录表 */
unsigned int LRU_global[GLOBAL_SUM] = {0};
unsigned int LRU_middle[MIDDLE_SUM] = {0};
unsigned int LRU_entry[ENTRY_SUM] = {0};

/* FIFO文件 */
int fifo_req,fifo_res;


/* 初始化环境 */
void do_init()
{
	do_init_entry();
	do_init_middle();
	do_init_global();
	do_init_act();
}

void do_init_act()
{
	int i;
	for(i=0;i<ACTUAL_ENTRY_SUM;i++)
	{
		blockStatus_Entry[i] = FALSE;
	}
	for(i=0;i<ACTUAL_MIDDLE_SUM;i++)
	{
		blockStatus_Middle[i] = FALSE;
	}
	for(i=0;i<BLOCK_SUM;i++)
	{
		blockStatus[i] = FALSE;
	}
}

void do_init_global()
{
	int i;
	for(i=0;i<GLOBAL_SUM;i++)
	{
		global[i].pageNum = i;
		global[i].actNum = 0;
		global[i].filled = FALSE;
		global[i].edited = FALSE;
		/* 设置该页对应的虚存地址 */
		global[i].virAddr = i * MENU_SIZE * PAGEMENU_SIZE/* * 2*/;
	}
}

void do_init_middle()
{
	int i;
	unsigned int writeNum;
	MenuItem middle[MIDDLE_SUM];
	for(i=0;i<MIDDLE_SUM;i++)
	{
		middle[i].pageNum = i;
		middle[i].actNum = 0;
		middle[i].filled = FALSE;
		middle[i].edited = FALSE;
		/* 设置该页对应的虚存地址 */
		middle[i].virAddr = i * MENU_SIZE * PAGEITEM_SIZE/* * 2*/;
	}
	/* 把二级页表全部存入虚存 */
	if ((writeNum = fwrite(middle, 
		PAGEMENU_SIZE, MIDDLE_SUM, ptr_virMem_Middle)) < MIDDLE_SUM)
	{
#ifdef DEBUG
		printf("把二级页表存入虚存出错");
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
}

void do_init_entry()
{
	int i,a;
	unsigned int writeNum;
	EntryItem pageTable[ENTRY_SUM];
	srandom(time(NULL));
	for (i = 0; i < ENTRY_SUM; i++)
	{
		pageTable[i].pageNum = i;
		pageTable[i].blockNum = 0;
		pageTable[i].filled = FALSE;
		pageTable[i].edited = FALSE;
		/* 使用随机数设置该页的保护类型 */
		switch (a=random() % 7)
		{
			case 0:
			{
				pageTable[i].proType = READABLE;
				break;
			}
			case 1:
			{
				pageTable[i].proType = WRITABLE;
				break;
			}
			case 2:
			{
				pageTable[i].proType = EXECUTABLE;
				break;
			}
			case 3:
			{
				pageTable[i].proType = READABLE | WRITABLE;
				break;
			}
			case 4:
			{
				pageTable[i].proType = READABLE | EXECUTABLE;
				break;
			}
			case 5:
			{
				pageTable[i].proType = WRITABLE | EXECUTABLE;
				break;
			}
			case 6:
			{
				pageTable[i].proType = READABLE | WRITABLE | EXECUTABLE;
				break;
			}
			default:
				break;
		}
		/* 使用随机数设置该页所属的进程号 */
		pageTable[i].pid = random() % 10;
		/* 设置该页对应的虚存地址 */
		pageTable[i].virAddr = i * ENTRY_SIZE/* * 2*/;
	}
	/* 把三级页表全部存入虚存 */
	if ((writeNum = fwrite(pageTable, 
		PAGEITEM_SIZE, ENTRY_SUM, ptr_virMem_Entry)) < ENTRY_SUM)
	{
#ifdef DEBUG
		printf("把三级页表存入虚存出错");
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
}

void send_res(BYTE i)
{
	/* 发送给request */
	if((fifo_res=open("fifo_res",O_WRONLY))<0)
		printf("打开FIFO失败");
	if(write(fifo_res,&i,sizeof(BYTE))<0)
		printf("写入FIFO失败");
	close(fifo_res);
}
/* 响应请求 */
void do_response()
{
	Ptr_MenuItem ptr_globalIt,ptr_middleIt;
	Ptr_EntryItem ptr_entryIt;
	unsigned int globalNum, middleNum, entryNum, offAddr;
	unsigned int actAddr, actNum;
	int i;
	
	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}
	
	/* 计算页号和页内偏移值 */
	globalNum = ptr_memAccReq->virAddr / (ENTRY_SIZE * MIDDLE_SUM);
	middleNum = (ptr_memAccReq->virAddr % (ENTRY_SIZE * MIDDLE_SUM))/ (ENTRY_SIZE * ENTRY_SIZE);
	entryNum = (ptr_memAccReq->virAddr % (ENTRY_SIZE * ENTRY_SIZE))/ ENTRY_SIZE;
	offAddr = ptr_memAccReq->virAddr % ENTRY_SIZE;
	printf("一级页号为：%u\t，二级页号为：%u，\n三级页号为：%u\t，页内偏移为：%u\n", globalNum, middleNum, entryNum, offAddr);

	/* 获取对应一级页表项 */
	ptr_globalIt = &global[globalNum];
	
	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_globalIt->filled)
	{
		do_global_fault(ptr_globalIt);
	}

	/* 调整LRU记录表 */
	LRU_global[globalNum] = 0;
	for(i=0;i<GLOBAL_SUM;i++)
	{
		if(i!=globalNum)
			LRU_global[i]++;
	}	
	/* 获取对应二级页表项 */
	actNum = ptr_globalIt->actNum*MENU_SIZE + middleNum;
	printf("二级页表序号为：%u\n", actNum);
	ptr_middleIt = &act_middle[actNum];

	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_middleIt->filled)
	{
		do_middle_fault(ptr_middleIt);
		ptr_globalIt->edited = TRUE;
	}

	/* 调整LRU记录表 */
	LRU_middle[actNum] = 0;
	for(i=0;i<MIDDLE_SUM;i++)
	{
		if(i!=actNum)
			LRU_middle[i]++;
	}

	/* 获取对应三级页表项 */
	actNum = ptr_middleIt->actNum*MENU_SIZE + entryNum;
	printf("三级页表序号为：%u\n", actNum);
	ptr_entryIt = &act_entry[actNum];

	/* 根据特征位决定是否产生缺页中断 */
	if (!ptr_entryIt->filled)
	{
		do_entry_fault(ptr_entryIt);
		ptr_middleIt->edited = TRUE;
	}

	/* 调整LRU记录表 */
	LRU_entry[actNum] = 0;
	for(i=0;i<ENTRY_SUM;i++)
	{
		if(i!=actNum)
			LRU_entry[i]++;
	}

	/* 获取物理地址 */
	actAddr = ptr_entryIt->blockNum * ENTRY_SIZE + offAddr;
	printf("物理地址为：%u\n", actAddr);

	/* 检查是否本进程访问 */
	if(ptr_memAccReq->pid!=ptr_entryIt->pid)
	{
		if(ptr_memAccReq->reqType==REQUEST_READ)
			send_res((BYTE)0xFF);
		do_error(ERROR_INCOMPATIBLE_PID);
		return;
	}
	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			if (!(ptr_entryIt->proType & READABLE)) //页面不可读
			{
				send_res((BYTE)0xFF);
				do_error(ERROR_READ_DENY);
				return;
			}
			/* 读取实存中的内容 */
			printf("读操作成功：值为%02X\n", actMem[actAddr]);
			send_res(actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			if (!(ptr_entryIt->proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);	
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			ptr_entryIt->edited = TRUE;			
			printf("写操作成功\n");
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			if (!(ptr_entryIt->proType & EXECUTABLE)) //页面不可执行
			{
				do_error(ERROR_EXECUTE_DENY);
				return;
			}			
			printf("执行成功\n");
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
void do_entry_fault(Ptr_EntryItem ptr_entryIt)
{
	unsigned int i;
	printf("三级页表产生缺页中断，开始进行调页...\n");
	for (i = 0; i < BLOCK_SUM; i++)
	{
		if (!blockStatus[i])
		{
			/* 读虚存内容，写入到实存 */
			do_entry_in(ptr_entryIt, i);
			
			/* 更新页表内容 */
			ptr_entryIt->blockNum = i;
			ptr_entryIt->filled = TRUE;
			ptr_entryIt->edited = FALSE;
			
			blockStatus[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	do_LRU_entry(ptr_entryIt);
}

void do_middle_fault(Ptr_MenuItem ptr_menuIt)
{
	unsigned int i;
	printf("二级页表产生缺页中断，开始进行调页...\n");
	for (i = 0; i < ACTUAL_ENTRY_SUM/MENU_SIZE; i++)
	{
		if (!blockStatus_Entry[i])
		{
			/* 读虚存内容，写入到实存 */
			do_middle_in(ptr_menuIt, i);
			
			/* 更新页表内容 */
			ptr_menuIt->actNum = i;
			ptr_menuIt->filled = TRUE;
			ptr_menuIt->edited = FALSE;
			
			blockStatus_Entry[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	do_LRU_middle(ptr_menuIt);
}

void do_global_fault(Ptr_MenuItem ptr_menuIt)
{
	unsigned int i;
	printf("一级页表产生缺页中断，开始进行调页...\n");
	for (i = 0; i < ACTUAL_MIDDLE_SUM/MENU_SIZE; i++)
	{
		if (!blockStatus_Middle[i])
		{
			/* 读虚存内容，写入到实存 */
			do_global_in(ptr_menuIt, i);
			
			/* 更新页表内容 */
			ptr_menuIt->actNum = i;
			ptr_menuIt->filled = TRUE;
			ptr_menuIt->edited = FALSE;
			
			blockStatus_Middle[i] = TRUE;
			return;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	do_LRU_global(ptr_menuIt);
}

/* 根据LRU算法进行页面替换 */
void do_LRU_entry(Ptr_EntryItem ptr_entryIt)
{
	int i,max,page;
	printf("没有空闲物理块，开始进行LRU页面替换...\n");
	for(i=0,max=0;i<ACTUAL_ENTRY_SUM;i++)
	{
		if(act_entry[i].filled)
			if(LRU_entry[act_entry[i].pageNum]>max)
			{
				max = LRU_entry[act_entry[i].pageNum];
				page = i;
			}
	}
	printf("选择第%u页进行替换\n", page);
	if(act_entry[page].edited)
	{
		printf("该页内容有修改，写回至辅存\n");
		do_entry_out(&act_entry[page]);
		act_entry[page].edited = FALSE;
	}
	LRU_entry[act_entry[page].pageNum] = 0;
	act_entry[page].filled = FALSE;
	do_entry_in(ptr_entryIt, act_entry[page].blockNum);
	ptr_entryIt->blockNum = act_entry[page].blockNum;
	ptr_entryIt->filled = TRUE;
	ptr_entryIt->edited = FALSE;
}

void do_LRU_middle(Ptr_MenuItem ptr_menuIt)
{
	int i,max,page;
	printf("没有空闲物理块，开始进行LRU页面替换...\n");
	for(i=0,max=0;i<ACTUAL_MIDDLE_SUM;i++)
	{
		if(act_middle[i].filled)
			if(LRU_middle[act_middle[i].pageNum]>max)
			{
				max = LRU_middle[act_middle[i].pageNum];
				page = i;
			}
	}
	printf("选择第%u页进行替换\n", page);
	if(act_middle[page].edited)
	{
		printf("该页内容有修改，写回至辅存\n");
		do_middle_out(&act_middle[page]);
		act_middle[page].edited = FALSE;
	}
	LRU_middle[act_middle[page].pageNum] = 0;
	act_middle[page].filled = FALSE;
	do_middle_in(ptr_menuIt, act_middle[page].actNum);
	ptr_menuIt->actNum = act_middle[page].actNum;
	ptr_menuIt->filled = TRUE;
	ptr_menuIt->edited = FALSE;
}

void do_LRU_global(Ptr_MenuItem ptr_menuIt)
{
	int i,max,page;
	printf("没有空闲物理块，开始进行LRU页面替换...\n");
	for(i=0,max=0;i<GLOBAL_SUM;i++)
	{
		if(global[i].filled)
			if(LRU_global[global[i].pageNum]>max)
			{
				max = LRU_global[global[i].pageNum];
				page = i;
			}
	}
	printf("选择第%u页进行替换\n", page);
	if(global[page].edited)
	{
		printf("该页内容有修改，写回至辅存\n");
		do_global_out(&global[page]);
		global[page].edited = FALSE;
	}
	LRU_global[global[page].pageNum] = 0;
	global[page].filled = FALSE;
	do_global_in(ptr_menuIt, global[page].actNum);
	ptr_menuIt->actNum = global[page].actNum;
	ptr_menuIt->filled = TRUE;
	ptr_menuIt->edited = FALSE;
}

/* 将虚存内容写入实存 */
void do_entry_in(Ptr_EntryItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	if (fseek(ptr_virMem, ptr_pageTabIt->virAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_pageTabIt->virAddr, ftell(ptr_virMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((readNum = fread(actMem + blockNum/* * ENTRY_SIZE*/, 
		sizeof(BYTE), ENTRY_SIZE, ptr_virMem)) < ENTRY_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_pageTabIt->virAddr, ftell(ptr_virMem));
		printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_virMem), ferror(ptr_virMem));
#endif
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
	printf("调页成功：辅存地址%u-->>物理块%u\n", ptr_pageTabIt->virAddr, blockNum);
}

/* 将虚存中的三级页表写入实存 */
void do_middle_in(Ptr_MenuItem ptr_menuIt, unsigned int actNum)
{
	unsigned int readNum;
	if (fseek(ptr_virMem_Entry, ptr_menuIt->virAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_menuIt->virAddr, ftell(ptr_virMem_Entry));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((readNum = fread(act_entry + actNum * MENU_SIZE/* * PAGEITEM_SIZE*/, 
		PAGEITEM_SIZE, MENU_SIZE, ptr_virMem_Entry)) < MENU_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_menuIt->virAddr, ftell(ptr_virMem_Entry));
		printf("DEBUG: actNum=%u\treadNum=%u\n", actNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_virMem_Entry), ferror(ptr_virMem_Entry));
#endif
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
	printf("调页成功：虚存地址%u-->>实存三级页表序号%u\n", ptr_menuIt->virAddr, actNum * MENU_SIZE);
}

/* 将虚存中的二级页表写入实存 */
void do_global_in(Ptr_MenuItem ptr_menuIt, unsigned int actNum)
{
	unsigned int readNum;
	if (fseek(ptr_virMem_Middle, ptr_menuIt->virAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_menuIt->virAddr, ftell(ptr_virMem_Middle));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((readNum = fread(act_middle + actNum * MENU_SIZE /** PAGEMENU_SIZE*/, 
		PAGEMENU_SIZE, MENU_SIZE, ptr_virMem_Middle)) < MENU_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_menuIt->virAddr, ftell(ptr_virMem_Middle));
		printf("DEBUG: actNum=%u\treadNum=%u\n", actNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_virMem_Middle), ferror(ptr_virMem_Middle));
#endif
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
	printf("调页成功：虚存地址%u-->>实存二级页表序号%u\n", ptr_menuIt->virAddr, actNum * MENU_SIZE);
}

/* 将被替换页面的内容写回辅存 */
void do_entry_out(Ptr_EntryItem ptr_pageTabIt)
{
	unsigned int writeNum;
	if (fseek(ptr_virMem, ptr_pageTabIt->virAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_pageTabIt->virAddr, ftell(ptr_virMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((writeNum = fwrite(actMem + ptr_pageTabIt->blockNum * ENTRY_SIZE, 
		sizeof(BYTE), ENTRY_SIZE, ptr_virMem)) < ENTRY_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_pageTabIt->virAddr, ftell(ptr_virMem));
		printf("DEBUG: writeNum=%u\n", writeNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_virMem), ferror(ptr_virMem));
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
	printf("写回成功：物理块%u-->>虚存地址%03X\n", ptr_pageTabIt->blockNum, ptr_pageTabIt->virAddr);
}

/* 将被替换的二级页表的内容写回辅存 */
void do_middle_out(Ptr_MenuItem ptr_menuIt)
{
	unsigned int writeNum;
	if (fseek(ptr_virMem_Entry, ptr_menuIt->virAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_menuIt->virAddr, ftell(ptr_virMem_Entry));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((writeNum = fwrite(act_entry + ptr_menuIt->actNum * MENU_SIZE * PAGEITEM_SIZE, 
		PAGEITEM_SIZE, MENU_SIZE, ptr_virMem_Entry)) < MENU_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_menuIt->virAddr, ftell(ptr_virMem_Entry));
		printf("DEBUG: writeNum=%u\n", writeNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_virMem_Entry), ferror(ptr_virMem_Entry));
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
	printf("写回成功：实存三级页表号%u-->>虚存地址%03X\n", ptr_menuIt->actNum, ptr_menuIt->virAddr);
}

/* 将被替换的一级页表的内容写回辅存 */
void do_global_out(Ptr_MenuItem ptr_menuIt)
{
	unsigned int writeNum;
	if (fseek(ptr_virMem_Middle, ptr_menuIt->virAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_menuIt->virAddr, ftell(ptr_virMem_Middle));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((writeNum = fwrite(act_middle + ptr_menuIt->actNum * MENU_SIZE * PAGEMENU_SIZE, 
		PAGEMENU_SIZE, MENU_SIZE, ptr_virMem_Middle)) < MENU_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: virAddr=%u\tftell=%u\n", ptr_menuIt->virAddr, ftell(ptr_virMem_Middle));
		printf("DEBUG: writeNum=%u\n", writeNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_virMem_Middle), ferror(ptr_virMem_Middle));
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
	printf("写回成功：实存二级页表号%u-->>虚存地址%03X\n", ptr_menuIt->actNum, ptr_menuIt->virAddr);
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
		case ERROR_INCOMPATIBLE_PID:
		{
			printf("系统错误：试图访问其它进程的页面\n");
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
	printf("第一级页表：\n页号\t实序号\t装入\t修改\t虚存\n");
	for (i = 0; i < GLOBAL_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%u\n", global[i].pageNum, global[i].actNum, global[i].filled, 
			global[i].edited, global[i].virAddr);
	}
	printf("第二级页表：\n页号\t实序号\t装入\t修改\t虚存\n");
	for (i = 0; i < ACTUAL_MIDDLE_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%u\n", act_middle[i].pageNum, act_middle[i].actNum, act_middle[i].filled, 
			act_middle[i].edited, act_middle[i].virAddr);
	}
	printf("第三级页表：\n页号\t进程号\t块号\t装入\t修改\t保护\t虚存\n");
	for (i = 0; i < ACTUAL_ENTRY_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%u\t%s\t%u\n", act_entry[i].pageNum, act_entry[i].pid, act_entry[i].blockNum, act_entry[i].filled, 
			act_entry[i].edited, get_proType_str(str, act_entry[i].proType), act_entry[i].virAddr);
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
	if (!(ptr_virMem = fopen(VIRTUAL_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	if (!(ptr_virMem_Entry = fopen(VIRTUAL_MEMORY_ENTRY, "w+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	if (!(ptr_virMem_Middle = fopen(VIRTUAL_MEMORY_MIDDLE, "w+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}
	
	do_init();
	ptr_memAccReq = (Ptr_MemoryAccessRequest)malloc(sizeof(MemoryAccessRequest));
	if(access("fifo_req",F_OK)==0)
	{//如果fifo_res存在，那么删去它
		if(remove("fifo_req")<0)
			printf("删除FIFO失败\n");
	}
	if(mkfifo("fifo_req",0666)<0)
		printf("创建FIFO失败\n");
	if(access("fifo_res",F_OK)==0)
	{//如果fifo_res存在，那么删去它
		if(remove("fifo_res")<0)
			printf("删除FIFO失败\n");
	}
	if(mkfifo("fifo_res",0666)<0)
		printf("创建FIFO失败\n");
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
	while (TRUE)
	{
		bzero(ptr_memAccReq,REQ_LEN);
		if((fifo_req = open("fifo_req",O_RDONLY))<0)
			printf("vmm打开FIFO失败");
		if(read(fifo_req,ptr_memAccReq,REQ_LEN)<0)
			printf("vmm读取FIFO失败");
		do_response();
		printf("按Y打印页表，按其他键不打印...\n");
		if ((c = getchar()) == 'y' || c == 'Y')
			do_print_info();
		while (c != '\n')
			c = getchar();
		/*printf("按X退出程序，按其他键继续...\n");
		if ((c = getchar()) == 'x' || c == 'X')
			break;
		while (c != '\n')
			c = getchar();*/
		//sleep(5000);
	}

	if (fclose(ptr_virMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}
	close(fifo_req);
	return (0);
}
