#ifndef VMM_H
#define VMM_H

#ifndef DEBUG
#define DEBUG
#endif
#undef DEBUG

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>

/* 模拟虚存的文件路径 */
#define VIRTUAL_MEMORY "vmm_virMem"
/* 模拟存放页表（三级页表）的虚存的文件路径 */
#define VIRTUAL_MEMORY_ENTRY "vmm_virMem_Entry"
/* 模拟存放二级页表的虚存的文件路径 */
#define VIRTUAL_MEMORY_MIDDLE "vmm_virMem_Middle"
/* 页面大小（字节） */
#define ENTRY_SIZE 8
/* 目录大小（项数） */
#define MENU_SIZE 8
/* 页表项大小（字节） */
#define PAGEITEM_SIZE (sizeof(EntryItem))
/* 页表目录项大小（字节） */
#define PAGEMENU_SIZE (sizeof(MenuItem))
//虚存：
/* 总三级页表数 */
#define ENTRY_SUM 512
/* 虚存空间大小（字节） */
#define VIRTUAL_MEMORY_SIZE (ENTRY_SUM * ENTRY_SIZE)
/* 存放页表（三级页表）的虚存空间大小（字节） */
#define VIRTUAL_MEMORY_ENTRY_SIZE (ENTRY_SUM * PAGEITEM_SIZE)
/* 总二级页表数 */
#define MIDDLE_SUM (ENTRY_SUM / MENU_SIZE)
/* 存放二级页表的虚存空间大小 */
#define VIRTUAL_MEMORY_MIDDLE_SIZE (MIDDLE_SUM * PAGEMENU_SIZE)
//实存：
/* 总物理块（实页）数 */
#define BLOCK_SUM 128
/* 实存空间大小（字节） */
#define ACTUAL_MEMORY_SIZE (BLOCK_SUM * ENTRY_SIZE)
/* 总物理三级页表数 */
#define ACTUAL_ENTRY_SUM 32
/* 总物理二级页表数 */
#define ACTUAL_MIDDLE_SUM 16
/* 总顶级页表数 */
#define GLOBAL_SUM (MIDDLE_SUM / MENU_SIZE)

/* 可读标识位 */
#define READABLE 1
/* 可写标识位 */
#define WRITABLE 2
/* 可执行标识位 */
#define EXECUTABLE 4

/* 数据长度 */
#define REQ_LEN sizeof(MemoryAccessRequest)



/* 定义字节类型 */
#define BYTE unsigned char

typedef enum {
	TRUE = 1, FALSE = 0
} BOOL;



/* 页表项 */
typedef struct
{
	unsigned int pageNum;
	unsigned int pid;	//该页表所属的进程号
	unsigned int blockNum; 	//物理块号
	BOOL filled; //页面装入特征位
	BYTE proType; //页面保护类型
	BOOL edited; //页面修改标识
	unsigned int virAddr; //虚存地址
} EntryItem, *Ptr_EntryItem;

/* 页表目录项 */
typedef struct
{
	unsigned int pageNum;
	unsigned int actNum; //物理序号
	BOOL filled; //页面装入特征位
	BOOL edited; //页面修改标识
	unsigned int virAddr; //虚存地址
} MenuItem, *Ptr_MenuItem;

/* 访存请求类型 */
typedef enum { 
	REQUEST_READ, 
	REQUEST_WRITE, 
	REQUEST_EXECUTE 
} MemoryAccessRequestType;

/* 访存请求 */
typedef struct
{
	unsigned int pid; //发出请求的进程号
	MemoryAccessRequestType reqType; //访存请求类型
	unsigned int virAddr; //虚地址
	BYTE value; //写请求的值
} MemoryAccessRequest, *Ptr_MemoryAccessRequest;


/* 访存错误代码 */
typedef enum {
	ERROR_READ_DENY, //该页不可读
	ERROR_WRITE_DENY, //该页不可写
	ERROR_EXECUTE_DENY, //该页不可执行
	ERROR_INVALID_REQUEST, //非法请求类型
	ERROR_OVER_BOUNDARY, //地址越界
	ERROR_FILE_OPEN_FAILED, //文件打开失败
	ERROR_FILE_CLOSE_FAILED, //文件关闭失败
	ERROR_FILE_SEEK_FAILED, //文件指针定位失败
	ERROR_FILE_READ_FAILED, //文件读取失败
	ERROR_FILE_WRITE_FAILED, //文件写入失败
	ERROR_INCOMPATIBLE_PID //进程号不匹配
} ERROR_CODE;

/* 初始化函数 */
void do_init_global();
void do_init_middle();
void do_init_entry();
void do_init_act();

/* 产生访存请求 */
void do_request();

/* 响应访存请求 */
void do_response();

/* 处理缺页中断 */
void do_entry_fault(Ptr_EntryItem ptr_entryIt);
void do_middle_fault(Ptr_MenuItem ptr_menuIt);
void do_global_fault(Ptr_MenuItem ptr_menuIt);

/* LRU页面替换 */
void do_LRU_entry(Ptr_EntryItem ptr_entryIt);
void do_LRU_middle(Ptr_MenuItem ptr_menuIt);
void do_LRU_global(Ptr_MenuItem ptr_menuIt);

/* 装入页面 */
void do_entry_in(Ptr_EntryItem ptr_pageTabIt, unsigned int blockNum);
void do_middle_in(Ptr_MenuItem ptr_menuIt, unsigned int actNum);
void do_global_in(Ptr_MenuItem ptr_menuIt, unsigned int actNum);

/* 写出页面 */
void do_entry_out(Ptr_EntryItem ptr_pageTabIt);
void do_middle_out(Ptr_MenuItem ptr_menuIt);
void do_global_out(Ptr_MenuItem ptr_menuIt);

/* 错误处理 */
void do_error(ERROR_CODE);

/* 打印页表相关信息 */
void do_print_info();

/* 获取页面保护类型字符串 */
char *get_proType_str(char *, BYTE);


#endif
