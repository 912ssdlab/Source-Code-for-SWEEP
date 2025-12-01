/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName�� ssd.h
Author: Hu Yang		Version: 2.1	Date:2011/12/02
Description: 

History:
<contributor>     <time>        <version>       <desc>                   <e-mail>
Yang Hu	        2009/09/25	      1.0		    Creat SSDsim       yanghu@foxmail.com
                2010/05/01        2.x           Change 
Zhiming Zhu     2011/07/01        2.0           Change               812839842@qq.com
Shuangwu Zhang  2011/11/01        2.1           Change               820876427@qq.com
Chao Ren        2011/07/01        2.0           Change               529517386@qq.com
Hao Luo         2011/01/01        2.0           Change               luohao135680@gmail.com
*****************************************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include "initialize.h"
#include "flash.h"
#include "pagemap.h"

#define MAX_INT64  0x7fffffffffffffffll
#define read_data_max_size 8192  
#define write_data_max_size 512
#define read_data_max 2048   //B,没有
#define MAX_CAPACITY 40
#define full_ratio 0.75
#define full_valid_ratio 0.4
#define BRT_ratio 0.6

int get_old_zwh(struct ssd_info *ssd);
struct ssd_info *simulate(struct ssd_info *);
struct ssd_info *simulate_multiple(struct ssd_info *, int);
int get_requests(struct ssd_info *);
struct ssd_info *buffer_management(struct ssd_info *);
unsigned int lpn2ppn(struct ssd_info * ,unsigned int lsn);
struct ssd_info *distribute(struct ssd_info *);
void trace_output(struct ssd_info* );
void statistic_output(struct ssd_info *);
//void statistic_output_easy(struct ssd_info *ssd, unsigned long simulate_times);
unsigned int size(unsigned int);
unsigned int transfer_size(struct ssd_info *,int,unsigned int,struct request *);
int64_t find_nearest_event(struct ssd_info *);
void free_all_node(struct ssd_info *);
struct ssd_info *make_aged(struct ssd_info *);
struct ssd_info *no_buffer_distribute(struct ssd_info *);
int full_block(struct ssd_info *);
int full_valid(struct ssd_info *ssd);
int full_invalid(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block);
int get_lsn(struct ssd_info *ssd);
int full_valid1(struct ssd_info *ssd);
int full_block_valid(struct ssd_info *ssd);
