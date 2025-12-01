
#include "ssd.h"
extern int index1 = 0, index2 = 0, index3 = 0, RRcount = 0;
extern float aveber=0;
extern int simuTIMES;


int  main(int argc, char* argv[])
{
	unsigned  int i,j,k;
	struct ssd_info *ssd;

	#ifdef DEBUG
	printf("enter main\n"); 
	#endif

	ssd=(struct ssd_info*)malloc(sizeof(struct ssd_info));
	alloc_assert(ssd,"ssd");
	memset(ssd,0, sizeof(struct ssd_info));
	
	//*****************************************************

	int sTIMES, speed_up=1;
	printf("Read parameters to the main function.\n");
	sscanf(argv[1], "%d", &sTIMES);
	//sscanf(argv[1], "%d", &speed_up);
	sscanf(argv[2], "%s", &(ssd->tracefilename));
	sscanf(argv[3], "%d", &(ssd->block_distributed));
	printf("Running trace file: %s.\n", ssd->tracefilename);
	
	//*****************************************************
	simuTIMES=sTIMES;
	ssd=initiation(ssd);

	//map_RL 不明白干嘛的
	ssd->map_RL = (double*)malloc(sizeof(double) * 9 * 80);
	alloc_assert(ssd->map_RL,"ssd->map_RL");
	//new added
	for(i = 0; i < 9 * 80; i++){
		if (i< 9 * 40 && i%9>3)
			*(ssd->map_RL + i)=-10000;
		else
			*(ssd->map_RL + i) = 0; 
	}	
	i = 0;


	printf("Chip_channel: %d, %d\n", ssd->parameter->chip_channel[0],ssd->parameter->chip_num);

	pre_process_page(ssd);

	full_block(ssd);

	//full_block_valid(ssd);
	
	//get_old_zwh(ssd);
	printf("free_lsb: %d, free_csb: %d, free_msb: %d\n", ssd->free_lsb_count, ssd->free_csb_count, ssd->free_msb_count);
	printf("Total request num: %lld.\n", ssd->total_request_num);

	// for (i=0;i<ssd->parameter->channel_number;i++)
	// {
	// 	for (j=0;j<ssd->parameter->die_chip;j++)
	// 	{
	// 		for (k=0;k<ssd->parameter->plane_die;k++)
	// 		{
	// 			printf("%d,0,%d,%d:  %5d\n",i,j,k,ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);
	// 		}
	// 	}
	// }
	fprintf(ssd->outputfile,"\t\t\t\t\t\t\t\t\tOUTPUT\n");
	fprintf(ssd->outputfile,"****************** TRACE INFO ******************\n");
	//********************************************
	
	//sscanf(argv[3], "%s", &
	if(speed_up<=0){
		printf("Parameter ERROR.\n");
		return 0;
		}
	printf("RUN %d TIMES with %dx SPEED.\n", sTIMES,speed_up);
	if(ssd->parameter->fast_gc==1){
		printf("Fast GC is activated, %.2f:%.2f and %.2f:%.2f.\n",ssd->parameter->fast_gc_thr_1,ssd->parameter->fast_gc_filter_1,ssd->parameter->fast_gc_thr_2,ssd->parameter->fast_gc_filter_2);
		}
	printf("dr_switch: %d, dr_cycle: %d\n",ssd->parameter->dr_switch,ssd->parameter->dr_cycle);
	if(ssd->parameter->dr_switch==1){
		printf("Data Reorganization is activated, dr cycle is %d days.\n", ssd->parameter->dr_cycle);
		}
	printf("GC_hard threshold: %.2f.\n", ssd->parameter->gc_hard_threshold);
	ssd->speed_up = speed_up;
	//*********************************************
	//ssd=simulate(ssd);
       // srand((unsigned int)time(NULL));
	ssd=simulate_multiple(ssd, sTIMES);
	

	statistic_output(ssd);  
	free_all_node(ssd);

	printf("\n");

	time_t t_now;
	struct tm *current_time;

	time(&t_now);
	current_time = localtime(&t_now);

	printf("Finish time: %04d-%02d-%02d %02d:%02d:%02d\n",
		current_time->tm_year + 1900,
		current_time->tm_mon + 1,
		current_time->tm_mday,
		current_time->tm_hour,
		current_time->tm_min,
		current_time->tm_sec);
	
    printf("average response time: %lld\n", (ssd->write_avg + ssd->read_avg) / (ssd->write_request_count + ssd->read_request_count));
	printf("\n");
	printf("the simulation is completed!\n");

	//printf("rr_request:%d\n",ssd->rr_request);
	
	return 1;
/* 	_CrtDumpMemoryLeaks(); */
}


struct ssd_info *simulate(struct ssd_info *ssd)
{
	int flag=1,flag1=0;
	double output_step=0;
	unsigned int a=0,b=0;
	//errno_t err;

	printf("\n");
	printf("begin simulating.......................\n");
	printf("\n");
	printf("\n");
	printf("   ^o^    OK, please wait a moment, and enjoy music and coffee   ^o^    \n");

	ssd->tracefile = fopen(ssd->tracefilename,"r");
	if(ssd->tracefile == NULL)
	{  
		printf("the trace file can't open2\n");
		return NULL;
	}

	fprintf(ssd->outputfile,"      arrive           lsn     size ope     begin time    response time    process time\n");	
	fflush(ssd->outputfile);

	while(flag!=100)      
	{
        
		flag=get_requests(ssd);
		//new added 但是差别不大
		if (flag == 0)
			flag = 100;
		if(flag == 1)
		{   
			//printf("once\n");
			if (ssd->parameter->dram_capacity!=0)
			{
				buffer_management(ssd);  
				distribute(ssd); 
			} 
			else
			{
				no_buffer_distribute(ssd);
			}		
		}

		process(ssd);

		trace_output(ssd);
		if (flag == 0)
			flag = 100;
	}

	fclose(ssd->tracefile);
	return ssd;
}

//这个函数的目的就是为了同时模拟多次，和simulate 没啥区别，加了一个循环。
struct ssd_info *simulate_multiple(struct ssd_info *ssd, int sTIMES)
{
	int flag=1,flag1=0;
	double output_step=0;
	unsigned int a=0,b=0;
	//errno_t err;

	int simulate_times = 0;
	//int sTIMES = 10;
	printf("\n");
	printf("begin simulating.......................\n");
	printf("\n");
	printf("\n");
	printf("   ^o^    OK, please wait a moment, and enjoy music and coffee   ^o^    \n");
	if(ssd->parameter->turbo_mode == 2 || ssd->parameter->turbo_mode == 0){
		ssd->parameter->lsb_first_allocation=0;
		ssd->parameter->dr_switch=0;
		ssd->parameter->fast_gc=0;
		}
	else{
		ssd->parameter->lsb_first_allocation=1;
		//ssd->parameter->dr_switch=ssd->parameter->dr_switch;
		//ssd->parameter->fast_gc=1;
		}
	/************************IMPORTANT FACTOR************************************/
	//ssd->parameter->turbo_mode_factor = 100;
	fprintf(ssd->statisticfile, "requests            time       gc_count          r_lat          w_lat          w_lsb          w_csb          w_msb           f_gc       mov_page      free_lsb     r_req_count    w_req_count    same_pages       req_lsb       req_csb       req_msb       w_req_count_l    same_pages_l       req_lsb_l       req_csb_l       req_msb_l\n");
	while(simulate_times < sTIMES){
		ssd->rr_trace_times++;
		//*********************************************************
		/*
		if(simulate_times<7){
			ssd->parameter->turbo_mode=0;
			ssd->parameter->dr_switch=0;
			ssd->parameter->fast_gc=0;
			}
		else{
			ssd->parameter->turbo_mode=1;
			ssd->parameter->dr_switch=1;
			ssd->parameter->fast_gc=1;
			}
		*/
		//*********************************************************
		ssd->tracefile = fopen(ssd->tracefilename,"r");
		if(ssd->tracefile == NULL){  
			printf("the trace file can't open1\n");
			return NULL;
			}
		fseek(ssd->tracefile,0,SEEK_SET);
		printf("simulating %d times.\n", simulate_times);
		//printf("file point: %ld\n", ftell(ssd->tracefile));
		fprintf(ssd->outputfile,"      arrive           lsn     size ope     begin time    response time    process time\n");	
		fflush(ssd->outputfile);
		ssd->basic_time = ssd->current_time;
		flag = 1;
		//int trace_count = 0;
		//while(flag!=100 && trace_count < 4500000){
		while(flag!=100){
			flag=get_requests(ssd);

			//trace_count++;
			if(flag == 1){   
				//printf("once\n");
				if (ssd->parameter->dram_capacity!=0){
					buffer_management(ssd);  
					distribute(ssd); 
					} 
				else{
					no_buffer_distribute(ssd);
					}		
				}
			process(ssd);
			trace_output(ssd);
			//if(trace_count%100000 == 0){
			//	printf("trace_num: %d\n",trace_count);
			//	}
			if(flag == 0 && ssd->request_queue == NULL)
				flag = 100;

			if(simulate_times && ssd->completed_read_count >= ((int)(ssd->total_read_num/10000))*10000*(simulate_times+1)){
				printf("It should be terminated.\n");
				flag = 100;
				}
			if(ssd->completed_read_count > ((int)(ssd->total_read_num/10000))*10000*(simulate_times+1) ){
				printf("It should be terminated.\n");
				flag = 100;
				}
		
			// if(ssd->completed_request_count > ((int)(ssd->total_request_num/10000))*10000*(simulate_times+1)){  //
			// 	printf("It should be terminated.\n");
			// 	flag = 100;
			// 	}
			}
			
		fclose(ssd->tracefile);
		//statistic_output_easy(ssd, simulate_times);
		//ssd->newest_read_avg = 0;
		//ssd->newest_write_avg = 0;
		//ssd->newest_read_request_count = 0;
		//ssd->newest_write_request_count = 0;
		//ssd->newest_write_lsb_count = 0;
		//ssd->newest_write_msb_count = 0;
		simulate_times++;
		ssd->simulate_times++;
		
		//unsigned int channel;
		//if((ssd->parameter->dr_switch==1)&&(simulate_times)%(ssd->parameter->dr_cycle)==0){
		/*
		int this_day;
		this_day = (int)(ssd->current_time/1000000000/3600/24);
		if((ssd->parameter->dr_switch==1)&&(this_day>ssd->time_day)&&(this_day%ssd->parameter->dr_cycle==0)){
			ssd->time_day = this_day;
			for(channel=0;channel<ssd->parameter->channel_number;channel++){
				dr_for_channel(ssd, channel);
				}
			}
			*/
		}
	return ssd;
}



int get_requests(struct ssd_info *ssd)  
{  
	char buffer[200];
	unsigned int lsn=0;
	int device,  size, ope,large_lsn, i = 0,j=0;
	int priority;
	int read_flag;
	struct request *request1;
	int flag = 1;
	long filepoint; 
	uint64_t time_t = 0;
	int64_t nearest_event_time;    

	#ifdef DEBUG
	printf("enter get_requests,  current time:%lld\n",ssd->current_time);
	#endif

	if(feof(ssd->tracefile))
		return 0; 

	filepoint = ftell(ssd->tracefile);	
	fgets(buffer, 200, ssd->tracefile); 
	//sscanf(buffer,"%lld %d %d %d %d %d",&time_t,&device,&lsn,&size,&ope,&read_flag);
	sscanf(buffer,"%lld %d %d %d %d",&time_t,&device,&lsn,&size,&ope);

	while(ssd->simulate_times ){
		//simulate_times_flag=1;
		if(feof(ssd->tracefile)){
			printf("debug\n");
			return 0;
		}
			 
		filepoint = ftell(ssd->tracefile);	
		fgets(buffer, 200, ssd->tracefile); 
		sscanf(buffer,"%lld %d %d %d %d %d",&time_t,&device,&lsn,&size,&ope,&priority);
		//printf("%lld, %d, %d\n", time_t, ope, ssd->simulate_times);
	}

	//printf("%lld\n", time_t);
    //=======================================
    priority = 1;   //ǿ���趨���ȼ�
    time_t = time_t/(ssd->speed_up);
	time_t = ssd->basic_time + time_t;
	//=======================================
	if ((device<0)&&(lsn<0)&&(size<0)&&(ope<0))
	{
		return 100;
	}
	if(size==0){
		size = 1;
		}
	if (lsn<ssd->min_lsn) 
		ssd->min_lsn=lsn;
	if (lsn>ssd->max_lsn)
		ssd->max_lsn=lsn;
	
	large_lsn=(int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
	//printf("largest:%d\n",large_lsn);
	//#exit(0);
	lsn = lsn%large_lsn;

	nearest_event_time=find_nearest_event(ssd);
	if (nearest_event_time==MAX_INT64)
	{
		ssd->current_time=time_t;           
		                                                  
		//if (ssd->request_queue_length>ssd->parameter->queue_length)    //���������еĳ��ȳ����������ļ��������õĳ���                     
		//{
			//printf("error in get request , the queue length is too long\n");
		//}
	}
	else
	{   
		if(nearest_event_time<time_t)
		{
			
			fseek(ssd->tracefile,filepoint,0); 
			if(ssd->current_time<=nearest_event_time)
				ssd->current_time=nearest_event_time;
			return -1;
		}
		else
		{
			if (ssd->request_queue_length>=ssd->parameter->queue_length)
			{
				fseek(ssd->tracefile,filepoint,0);
				ssd->current_time=nearest_event_time;
				return -1;

			} 
			else
			{
				ssd->current_time=time_t;
			}
		}
	}

	if(time_t < 0)
	{
		printf("error!\n");
		while(1){}
	}

	if(feof(ssd->tracefile))
	{
		request1=NULL;
		return 0;
	}

	request1 = (struct request*)malloc(sizeof(struct request));
	alloc_assert(request1,"request");
	memset(request1,0, sizeof(struct request));

	request1->time = time_t;
	request1->lsn = lsn;
	request1->size = size;
	request1->operation = ope;	
	request1->priority = priority;
	//request1->flag = read_flag;
	request1->begin_time = time_t;
	request1->response_time = 0;	
	request1->energy_consumption = 0;	
	request1->next_node = NULL;
	request1->distri_flag = 0;              // indicate whether this request has been distributed already
	request1->subs = NULL;
	request1->need_distr_flag = NULL;
	request1->complete_lsn_count=0;         //record the count of lsn served by buffer
	filepoint = ftell(ssd->tracefile);		// set the file point

	if(ssd->request_queue == NULL)          //The queue is empty
	{
		ssd->request_queue = request1;
		ssd->request_tail = request1;
		ssd->request_queue_length++;
	}
	else
	{			
		(ssd->request_tail)->next_node = request1;	
		ssd->request_tail = request1;			
		ssd->request_queue_length++;
	}
	if(ssd->request_queue_length > ssd->max_queue_depth){
		ssd->max_queue_depth = ssd->request_queue_length;
		}
	/*
	if(ssd->request_queue_length > 10){
		if(ssd->parameter->turbo_mode==1 && ssd->performance_mode==0){
			ssd->parameter->turbo_mode_factor = 100;
			ssd->parameter->turbo_mode_factor_2 = 100;
			//printf("Ajusting to high performance, %d, %d.\n", ssd->parameter->turbo_mode_factor, ssd->parameter->turbo_mode_factor_2);
			ssd->performance_mode = 1;
			}
		}
	else if(ssd->request_queue_length < 5){
		if(ssd->parameter->turbo_mode==1 && ssd->performance_mode==1){
			//float turbo_mode_factor, turbo_mode_factor_2;
			//turbo_mode_factor = (((float)ssd->free_lsb_count)/(ssd->free_lsb_count+ssd->free_csb_count+ssd->free_msb_count))*100;
			//turbo_mode_factor_2 = (((float)(ssd->free_lsb_count+ssd->free_csb_count))/(ssd->free_lsb_count+ssd->free_csb_count+ssd->free_msb_count))*100;
			ssd->parameter->turbo_mode_factor = 34;
			ssd->parameter->turbo_mode_factor_2 = 67;
			ssd->performance_mode = 0;
			}
		}
	*/
	if (request1->operation==1)             //����ƽ�������С 1Ϊ�� 0Ϊд
	{
		ssd->ave_read_size=(ssd->ave_read_size*ssd->read_request_count+request1->size)/(ssd->read_request_count+1);
	} 
	else
	{
		ssd->ave_write_size=(ssd->ave_write_size*ssd->write_request_count+request1->size)/(ssd->write_request_count+1);
	}

	
	filepoint = ftell(ssd->tracefile);	
	fgets(buffer, 200, ssd->tracefile);    //Ѱ����һ������ĵ���ʱ��
	sscanf(buffer,"%lld %d %d %d %d",&time_t,&device,&lsn,&size,&ope);
	ssd->next_request_time=time_t;
	fseek(ssd->tracefile,filepoint,0);

	return 1;
}


struct ssd_info *buffer_management(struct ssd_info *ssd)
{   
	unsigned int j,lsn,lpn,last_lpn,first_lpn,index,complete_flag=0, state,full_page;
	unsigned int flag=0,need_distb_flag,lsn_flag,flag1=1,active_region_flag=0;           
	struct request *new_request;
	struct buffer_group *buffer_node,key;
	unsigned int mask=0,offset1=0,offset2=0;

	#ifdef DEBUG
	printf("enter buffer_management,  current time:%lld\n",ssd->current_time);
	#endif
	ssd->dram->current_time=ssd->current_time;
	full_page=~(0xffffffff<<ssd->parameter->subpage_page);
	
	new_request=ssd->request_tail;
	lsn=new_request->lsn;
	lpn=new_request->lsn/ssd->parameter->subpage_page;
	last_lpn=(new_request->lsn+new_request->size-1)/ssd->parameter->subpage_page;
	first_lpn=new_request->lsn/ssd->parameter->subpage_page;

	new_request->need_distr_flag=(unsigned int*)malloc(sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32+1));
	alloc_assert(new_request->need_distr_flag,"new_request->need_distr_flag");
	memset(new_request->need_distr_flag, 0, sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32+1));
	
	if(new_request->operation==READ) 
	{		
		while(lpn<=last_lpn)      		
		{
			need_distb_flag=full_page;   
			key.group=lpn;
			buffer_node= (struct buffer_group*)avlTreeFind(ssd->dram->buffer, (TREE_NODE *)&key);		// buffer node 

			while((buffer_node!=NULL)&&(lsn<(lpn+1)*ssd->parameter->subpage_page)&&(lsn<=(new_request->lsn+new_request->size-1)))             			
			{             	
				lsn_flag=full_page;
				mask=1 << (lsn%ssd->parameter->subpage_page);
				if(mask> (1 <<31))
				{
					printf("%d",mask);
					printf("%d",ssd->parameter->subpage_page);
					printf("the subpage number is larger than 32!add some cases");
					getchar(); 		   
				}
				else if((buffer_node->stored & mask)==mask)
				{
					flag=1;
					lsn_flag=lsn_flag&(~mask);
				}

				if(flag==1)				
				{	//�����buffer�ڵ㲻��buffer�Ķ��ף���Ҫ������ڵ��ᵽ���ף�ʵ����LRU�㷨�������һ��˫����С�		       		
					if(ssd->dram->buffer->buffer_head!=buffer_node)     
					{		
						if(ssd->dram->buffer->buffer_tail==buffer_node)								
						{			
							buffer_node->LRU_link_pre->LRU_link_next=NULL;					
							ssd->dram->buffer->buffer_tail=buffer_node->LRU_link_pre;							
						}				
						else								
						{				
							buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;				
							buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;								
						}								
						buffer_node->LRU_link_next=ssd->dram->buffer->buffer_head;
						ssd->dram->buffer->buffer_head->LRU_link_pre=buffer_node;
						buffer_node->LRU_link_pre=NULL;			
						ssd->dram->buffer->buffer_head=buffer_node;													
					}						
					ssd->dram->buffer->read_hit++;					
					new_request->complete_lsn_count++;											
				}		
				else if(flag==0)
					{
						ssd->dram->buffer->read_miss_hit++;
					}

				need_distb_flag=need_distb_flag&lsn_flag;
				
				flag=0;		
				lsn++;						
			}	
				
			index=(lpn-first_lpn)/(32/ssd->parameter->subpage_page); 			
			new_request->need_distr_flag[index]=new_request->need_distr_flag[index]|(need_distb_flag<<(((lpn-first_lpn)%(32/ssd->parameter->subpage_page))*ssd->parameter->subpage_page));	
			lpn++;
			
		}
	}  
	else if(new_request->operation==WRITE)
	{
		while(lpn<=last_lpn)           	
		{	
			need_distb_flag=full_page;
			mask=~(0xffffffff<<(ssd->parameter->subpage_page));
			state=mask;

			if(lpn==first_lpn)
			{
				offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-new_request->lsn);
				state=state&(0xffffffff<<offset1);
			}
			if(lpn==last_lpn)
			{
				offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(new_request->lsn+new_request->size));
				state=state&(~(0xffffffff<<offset2));
			}
			
			ssd=insert2buffer(ssd, lpn, state,NULL,new_request);
			lpn++;
		}
	}
	complete_flag = 1;
	for(j=0;j<=(last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32;j++)
	{
		if(new_request->need_distr_flag[j] != 0)
		{
			complete_flag = 0;
		}
	}
	if((complete_flag == 1)&&(new_request->subs==NULL))               
	{
		new_request->begin_time=ssd->current_time;
		new_request->response_time=ssd->current_time+1000;            
	}

	return ssd;
}

unsigned int lpn2ppn(struct ssd_info *ssd,unsigned int lsn)
{
	int lpn, ppn;	
	struct entry *p_map = ssd->dram->map->map_entry;                //��ȡӳ���
#ifdef DEBUG
	printf("enter lpn2ppn,  current time:%lld\n",ssd->current_time);
#endif
	lpn = lsn/ssd->parameter->subpage_page;			//subpage_pageָһ��page�а�������ҳ�������ڲ����ļ��п����趨
	ppn = (p_map[lpn]).pn;                     //�߼�ҳlpn������ҳppn��ӳ���¼��ӳ�����
	return ppn;
}



struct ssd_info *distribute(struct ssd_info *ssd) 
{
	unsigned int start, end, first_lsn,last_lsn,lpn,flag=0,flag_attached=0,full_page;
	unsigned int j, k, sub_size;
	int i=0;
	struct request *req;
	struct sub_request *sub;
	int* complt;

	#ifdef DEBUG
	printf("enter distribute,  current time:%lld\n",ssd->current_time);
	#endif
	full_page=~(0xffffffff<<ssd->parameter->subpage_page);

	req = ssd->request_tail;
	if(req->response_time != 0){
		return ssd;
	}
	if (req->operation==WRITE)
	{
		return ssd;
	}

	if(req != NULL)
	{
		if(req->distri_flag == 0)
		{
			//�������һЩ��������Ҫ����
			if(req->complete_lsn_count != ssd->request_tail->size)
			{		
				first_lsn = req->lsn;				
				last_lsn = first_lsn + req->size;
				complt = req->need_distr_flag;
				start = first_lsn - first_lsn % ssd->parameter->subpage_page;
				end = (last_lsn/ssd->parameter->subpage_page + 1) * ssd->parameter->subpage_page;
				i = (end - start)/32;	

				while(i >= 0)
				{	
					/*************************************************************************************
					*һ��32λ���������ݵ�ÿһλ����һ����ҳ��32/ssd->parameter->subpage_page�ͱ�ʾ�ж���ҳ��
					*�����ÿһҳ��״̬��������� req->need_distr_flag�У�Ҳ����complt�У�ͨ���Ƚ�complt��
					*ÿһ����full_page���Ϳ���֪������һҳ�Ƿ�����ɡ����û���������ͨ��creat_sub_request
					��������������
					*************************************************************************************/
					for(j=0; j<32/ssd->parameter->subpage_page; j++)
					{	
						k = (complt[((end-start)/32-i)] >>(ssd->parameter->subpage_page*j)) & full_page;	
						if (k !=0)
						{
							lpn = start/ssd->parameter->subpage_page+ ((end-start)/32-i)*32/ssd->parameter->subpage_page + j;
							sub_size=transfer_size(ssd,k,lpn,req);    
							if (sub_size==0) 
							{
								continue;
							}
							else
							{
								sub=creat_sub_request(ssd,lpn,sub_size,0,req,req->operation,0);
							}	
						}
					}
					i = i-1;
				}

			}
			else
			{
				req->begin_time=ssd->current_time;
				req->response_time=ssd->current_time+1000;   
			}

		}
	}
	return ssd;
}



void trace_output(struct ssd_info* ssd){
	int flag = 1;	
	int64_t start_time, end_time;
	struct request *req, *pre_node;
	struct sub_request *sub, *tmp;

#ifdef DEBUG
	printf("enter trace_output,  current time:%lld\n",ssd->current_time);
#endif
	int debug_0918 = 0;
	pre_node=NULL;
	req = ssd->request_queue;
	start_time = 0;
	end_time = 0;

	if(req == NULL)
		return;

	while(req != NULL)	
	{
		sub = req->subs;
		flag = 1;
		start_time = 0;
		end_time = 0;
		if(req->response_time != 0)
		{
			//printf("Rsponse time != 0?\n");
			// fprintf(ssd->outputfile,"%16lld %10d %6d %2d %16lld %16lld %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, req->response_time-req->time);
		
			if(req->operation==1)
				fprintf(ssd->latencyfile,"%10lld,",req->response_time-req->time);
				fflush(ssd->latencyfile);

			// fprintf(ssd->latencyfile,"%10lld,",req->response_time-req->time);
			// fflush(ssd->latencyfile);

			ssd->completed_request_count++;
			if(req->operation==1)
				ssd->completed_read_count++;
			if(ssd->completed_request_count%10000 == 0){
				printf("completed requests: %d\n", ssd->completed_request_count);
				//statistic_output_easy(ssd, ssd->completed_request_count);
				ssd->newest_read_avg = 0;
				ssd->newest_write_avg = 0;
				ssd->newest_read_request_count = 0;
				ssd->newest_write_request_count = 0;
				ssd->newest_write_lsb_count = 0;
				ssd->newest_write_csb_count = 0;
				ssd->newest_write_msb_count = 0;
				ssd->newest_write_request_completed_with_same_type_pages = 0;
				//***************************************************************************
				int channel;
				int this_day;
				this_day = (int)(ssd->current_time/1000000000/3600/24);
				/*
				if(this_day>ssd->time_day){
					printf("Another Day begin, %d.\n", this_day);
					}
					*/
				if(this_day>ssd->time_day){
					printf("Day %d begin......\n", this_day);
					ssd->time_day = this_day;
					if((ssd->parameter->dr_switch==1)&&(this_day%ssd->parameter->dr_cycle==0)){
						for(channel=0;channel<ssd->parameter->channel_number;channel++){
							dr_for_channel(ssd, channel);
							}
						}
					/*
					if((ssd->parameter->turbo_mode==2)&&(this_day%7==3)){
						printf("Enter turbo-mode.....\n");
						ssd->parameter->lsb_first_allocation = 1;
						ssd->parameter->fast_gc = 1;
						}
					else if(ssd->parameter->turbo_mode==2){
						//printf("Exist turbo-mode....\n");
						ssd->parameter->lsb_first_allocation = 0;
						ssd->parameter->fast_gc = 0;
						}
						*/
					}
				//***************************************************************************
				}
			
			if(debug_0918){
				printf("completed requests: %d\n", ssd->completed_request_count);
				}
			
			if(req->response_time-req->begin_time==0)
			{
				printf("the response time is 0?? \n");
				getchar();
			}

			if (req->operation==READ)
			{
				ssd->read_request_count++;
				ssd->read_avg=ssd->read_avg+(req->response_time-req->time);
				//===========================================
				ssd->newest_read_request_count++;
				ssd->newest_read_avg = ssd->newest_read_avg+(end_time-req->time);
				//===========================================
			} 
			else
			{
				ssd->write_request_count++;
				ssd->write_avg=ssd->write_avg+(req->response_time-req->time);
				//===========================================
				ssd->newest_write_request_count++;
				ssd->newest_write_avg = ssd->newest_write_avg+(end_time-req->time);
				ssd->last_write_lat = end_time-req->time;
				//--------------------------------------------
				// int new_flag = 1;
				// int origin;
				// struct sub_request *next_sub_a;
				// next_sub_a = req->subs;
				// printf("%d\n",req == NULL);
				// origin = next_sub_a->allocated_page_type;
				// next_sub_a = next_sub_a->next_subs;
				// while(next_sub_a!=NULL){
				// 	if(next_sub_a->allocated_page_type != origin){
				// 		new_flag = 0;
				// 		break;
				// 		}
				// 	next_sub_a = next_sub_a->next_subs;
				// 	}
				// if(new_flag==1){
				// 	ssd->newest_write_request_completed_with_same_type_pages++;
				// 	if(origin==1){
				// 		ssd->newest_msb_request_a++;
				// 		}
				// 	else{
				// 		ssd->newest_lsb_request_a++;
				// 		}
				// 	}
				//-------------------------------------------

				//===========================================
			}

			if(pre_node == NULL)
			{
				if(req->next_node == NULL)
				{
					free(req->need_distr_flag);
					req->need_distr_flag=NULL;
					free(req);
					req = NULL;
					ssd->request_queue = NULL;
					ssd->request_tail = NULL;
					ssd->request_queue_length--;
				}
				else
				{
					ssd->request_queue = req->next_node;
					pre_node = req;
					req = req->next_node;
					free(pre_node->need_distr_flag);
					pre_node->need_distr_flag=NULL;
					free((void *)pre_node);
					pre_node = NULL;
					ssd->request_queue_length--;
				}
			}
			else
			{
				if(req->next_node == NULL)
				{
					pre_node->next_node = NULL;
					free(req->need_distr_flag);
					req->need_distr_flag=NULL;
					free(req);
					req = NULL;
					ssd->request_tail = pre_node;
					ssd->request_queue_length--;
				}
				else
				{
					pre_node->next_node = req->next_node;
					free(req->need_distr_flag);
					req->need_distr_flag=NULL;
					free((void *)req);
					req = pre_node->next_node;
					ssd->request_queue_length--;
				}
			}
		}
		else
		{
			//printf("Rsponse time = 0!\n");
			if (ssd->parameter->dram_capacity != 0 )
			{
				while(sub!= NULL&& req->operation == READ)
				{
					//printf("1111");
					if((sub->current_state == SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time)))	// if any sub-request is not completed, the request is not completed
					{
						ssd = insert2buffer(ssd,sub->lpn,sub->state,NULL,req);
					}
					sub = sub->next_subs;
				}
			}
		
			sub = req->subs;
			flag=1;
			while(sub != NULL)
			{
				if(start_time == 0)
					start_time = sub->begin_time;
				if(start_time > sub->begin_time)
					start_time = sub->begin_time;
				if(end_time < sub->complete_time)
					end_time = sub->complete_time;
				if((sub->current_state == SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time)))	// if any sub-request is not completed, the request is not completed
				{
					sub = sub->next_subs;
				}
				else
				{
					flag=0;
					break;
				}
			}
		//大请求在这里完成。
			if (flag == 1)
			{	
				if (switch_on == 1)   //0
				{
				sub = req->subs;
				int move_count = 1;//1  2
				int page_move_count = 0;
				int channelChangFlag = 0;
				int dieChangFlag = 0;
				int channelpre = 0;
				int transfer_size = 0;
				if (ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].freq_count != NULL && ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count >= phase3 )
				{
					
					for (int i = 0 ; i < ssd->parameter->page_block ; i++)
					{
						// if (ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count < erase_phase1)
						// {
							int temp_flag = ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].freq_count[i];
							if ((temp_flag == 7 || temp_flag == 6 || temp_flag == 5 || temp_flag == 4 ) && ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].page_head[i].valid_state > 0)
							{
								struct local *location=(struct local * )malloc(sizeof(struct local ));
								alloc_assert(location,"location");
								memset(location,0, sizeof(struct local));
								location->channel=sub->location->channel;
								location->chip=sub->location->chip;
								location->die=sub->location->die;
								location->plane=sub->location->plane;
								location->block=sub->location->block;
								location->page=i;
								move_page_for_rr(ssd, location, &transfer_size);                                              
								page_move_count++;
								ssd->rr_page_move_count++;
								ssd->page_move_count++;
								free(location);	
								location=NULL;
								move_count--;
								if (temp_flag == 1 || temp_flag == 2 || temp_flag == 4) ssd->third_migrage_count++;
								else if (temp_flag == 6 || temp_flag == 5 || temp_flag == 3) ssd->second_migrate_count++;
								else if (temp_flag == 7) ssd->first_migrate_count++;
							}
							
							if (move_count == 0) break;
						// }
						// else 
						// if (ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count < erase_phase2)
						// {
						// 	int temp_flag = ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].freq_count[i];
							

						// 	if ((temp_flag == 6 || temp_flag == 5 || temp_flag == 3) && ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].page_head[i].valid_state > 0)
						// 	{
						// 		struct local *location=(struct local * )malloc(sizeof(struct local ));
						// 		alloc_assert(location,"location");
						// 		memset(location,0, sizeof(struct local));
						// 		location->channel=sub->location->channel;
						// 		location->chip=sub->location->chip;
						// 		location->die=sub->location->die;
						// 		location->plane=sub->location->plane;
						// 		location->block=sub->location->block;
						// 		location->page=i;
						// 		move_page_for_rr(ssd, location, &transfer_size);                                              
						// 		page_move_count++;
						// 		ssd->rr_page_move_count++;
						// 		ssd->page_move_count++;
						// 		free(location);	
						// 		location=NULL;
						// 		move_count--;
						// 	}
							
						// 	if (temp_flag == 1 || temp_flag == 2 || temp_flag == 4) ssd->third_migrage_count++;
						// 	else if (temp_flag == 6 || temp_flag == 5 || temp_flag == 3) ssd->second_migrate_count++;
						// 	else if (temp_flag == 7) ssd->first_migrate_count++;
						// 	if (move_count == 0) break;
						// }
						// else if (ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count < erase_phase3)
						// {
						// 	int temp_flag = ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].freq_count[i];
						// 	if ((temp_flag == 1 || temp_flag == 2 || temp_flag == 4) && ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].page_head[i].valid_state > 0)
						// 	{
						// 		struct local *location=(struct local * )malloc(sizeof(struct local ));
						// 		alloc_assert(location,"location");
						// 		memset(location,0, sizeof(struct local));
						// 		location->channel=sub->location->channel;
						// 		location->chip=sub->location->chip;
						// 		location->die=sub->location->die;
						// 		location->plane=sub->location->plane;
						// 		location->block=sub->location->block;
						// 		location->page=i;
						// 		move_page(ssd, location, &transfer_size);                                              
						// 		page_move_count++;
						// 		ssd->rr_page_move_count++;
						// 		ssd->page_move_count++;
						// 		free(location);	
						// 		location=NULL;
						// 		move_count--;
						// 	}
						// 	if (temp_flag == 1 || temp_flag == 2 || temp_flag == 4) ssd->third_migrage_count++;
						// 	else if (temp_flag == 6 || temp_flag == 5 || temp_flag == 3) ssd->second_migrate_count++;
						// 	else if (temp_flag == 7) ssd->first_migrate_count++;
						// 	if (move_count == 0) break;
						// }
						
					}
					int channel = sub->location->channel;
					int chip = sub->location->chip;
					int die = sub->location->die;
					int plane = sub->location->plane;
					int block = sub->location->block;
					if((ssd->channel_head[channel].current_state==CHANNEL_IDLE)||(ssd->channel_head[channel].next_state==CHANNEL_IDLE&&ssd->channel_head[channel].next_state_predict_time<=ssd->current_time))
					{
					ssd->channel_head[channel].current_state=CHANNEL_GC;									
					ssd->channel_head[channel].current_time=ssd->current_time;										
					ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
					channelChangFlag = 1;
					}
		
					if((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time)))
					{
						ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
						ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
						ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;
						dieChangFlag = 1;
					}
					if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
					{
						if (ssd->parameter->greed_CB_ad==1)
						{
							ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG);			
							ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
						} 
					} 
					else
					{
						//channelpre 是读一次+写一次的时间
						channelpre = page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
						if(channelChangFlag)
							//这个是加在channel上的
							ssd->channel_head[channel].next_state_predict_time = ssd->current_time+ channelpre;
						else 
							ssd->channel_head[channel].next_state_predict_time += channelpre;
						
						if(dieChangFlag)
							//这个是在加chip上的
							ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->current_time + channelpre;
						else 
							ssd->channel_head[channel].chip_head[chip].next_state_predict_time += channelpre;
					}
					int valid_count=0;
					for(int i = 0;i<ssd->parameter->page_block;i++)		                                                  	
					{		
						if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0)
						{
							valid_count+=1;
						}
					}
					if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num==ssd->parameter->page_block||valid_count==0))			 
					{

						if(dieChangFlag)
							ssd->channel_head[channel].chip_head[chip].next_state_predict_time ==  ssd->current_time+channelpre+ssd->parameter->time_characteristics.tBERS;
						else 
							ssd->channel_head[channel].chip_head[chip].next_state_predict_time += ssd->parameter->time_characteristics.tBERS;
						erase_operation(ssd,channel ,chip , die,plane ,block);
						ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].read_count=0;
						for(int i=0;i<ssd->parameter->page_block;i++)
						{
							ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count=0;
						}
						ssd->rr_erase_count++;	
					}
				}



				}
				
				/**start RL	**/
				// //以下都是与强化学习相关的部分。
				// sub = req->subs;
				// static unsigned int request_count = 0;
				// unsigned long now_page_plane = ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].free_page;
				// unsigned long all_page_plane =  ssd->parameter->page_block*ssd->parameter->block_plane;
				// if(request_count == 0)
				// 	ssd->now_request_time = req->time;
				// else if(request_count == 1){
				// 	ssd->pre_request_time = ssd->now_request_time;
				// 	ssd->now_request_time = req->time;
					
				// } else {
				// 	ssd->pre_pre_request_time = ssd->pre_request_time;
				// 	ssd->pre_request_time = ssd->now_request_time;
				// 	ssd->now_request_time = req->time;
				// }
				// if(request_count!=0)
				// 	ssd->avg_timeinterval = ssd->now_request_time - ssd->pre_request_time;
				// 	//printf("ssd->now_request_time - ssd->pre_request_time:%lld\n",ssd->now_request_time - ssd->pre_request_time);
				// 	ssd->intervalcount++;
				// int early_rr_flag=0;
				// int rr_block=-1;
				// //int req_flag=0;
				
				// int i=0;
				
				
				
					
				// if(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count>=SOFT_BRT){
				// 	//if((req->operation==1&&req->size<=2*ssd->read_size/ssd->read_request_count) || (req->operation==0&&req->size<=2*ssd->write_size/ssd->write_request_count) ){
				// 		early_rr_flag = 1;
				// 		if(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].soft_flag==0){
				// 			ssd->rr_count++;
				// 			ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].soft_flag=1;
				// 		}
							
				// 	//}
				// }
				// //printf("early_rr_flag:%d\n",early_rr_flag);

				
				// if(early_rr_flag==1){//soft threshold

				// 	if(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].rr_plane==-1){
				// 			ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].rr_plane=sub->location->plane;
				// 			ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].rr_block=sub->location->block;
				// 			ssd->rr_request++;
				// 		}
					
				// 	//printf("early_rr_flag:%d\n",early_rr_flag);
				// 	unsigned int now_action = 0;
				// 	unsigned int erase = 0;
				// 	//if(request_count <= 1000)
				// 	request_count++;
				// 	ssd->request_count++;
				// 	if(request_count > 2) {
				// 		double reward = 0;
				// 		long long now_state = 0;
				// 		now_state = ssd->now_request_time - ssd->pre_request_time - 200000;
				// 		//printf("ssd->now_request_time - ssd->pre_request_time:%d\n",ssd->now_request_time - ssd->pre_request_time);
				// 		if(now_state < 0)
				// 			now_state = 1;
				// 		now_state /= 200000;
				// 		if(now_state >= 10)
				// 			now_state = 10;
				// 		else if(now_state == 0)
				// 			now_state = 1;
				// 		//printf("now_state:%d ",now_state);
				// 		if((ssd->now_request_time - ssd->pre_request_time) > 200000)
				// 			now_state += 10;
				// 		//printf("now_state:%d ",now_state);
				// 		if(ssd->pre_action > 2)
				// 			now_state += 20;
				// 		//printf("now_state:%d ",now_state);
				// 		//if((ssd->now_request_time - ssd->pre_request_time) > ssd->avg_timeinterval/ssd->intervalcount)//avg
				// 		if(ssd->erase_flag==1)
				// 			now_state += 40;
				// 		//printf("now_state:%d\n",now_state);
				// 		now_state -= 1;
				// 		if(now_action == 0) {
				// 			unsigned int i = 0;
				// 			double *map_RL = ssd->map_RL + 9 * now_state;
				// 			now_action = 1;
				// 			reward = *map_RL;
				// 			for(i = 1; i < 9; i++){
				// 				if(*(map_RL + i) > reward) {
				// 					reward = *(map_RL + i);
				// 					now_action = i + 1;
				// 					//now_action = i;
				// 				}
				// 			}
				// 		}
				// 		//if(now_action > 2)
				// 		//	now_action = 2;
						
				// 		int rate_of_e = rand() % 100;
				// 		if((request_count <= 1000 && rate_of_e  < 80) || (request_count > 1000 && rate_of_e <= 1)){
				// 				now_action = rand() % 9 + 1;
				// 				while(ssd->erase_flag==0&&now_action>4){
				// 					now_action = rand() % 9 + 1;//random avoiding selecting erase
				// 				}
				// 		}

				// 		unsigned long long avg = 0;
						
				// 		/*if(ssd->write_request_count!=0 && ssd->read_request_count!=0){
				// 			avg = (ssd->write_avg+ ssd->read_avg) / (ssd->write_request_count + ssd->read_request_count);
				// 		}else if(ssd->write_request_count==0){
				// 			avg = ssd->read_avg / ssd->read_request_count;
				// 		}else{
				// 			avg = ssd->write_avg / ssd->write_request_count;
				// 		}*/
				// 		if(req->operation==READ)
				// 			//avg = ssd->read_avg / ssd->read_request_count;
				// 			avg = ssd->read_avg / ssd->read_size;
				// 		else
				// 			//avg = ssd->write_avg / ssd->write_request_count;
				// 			avg = ssd->write_avg / ssd->write_size;
						
				// 		if(avg==0)
				// 			reward = 1;
				// 		else if(end_time - req->time <= (((double)avg) * 0.7 * req->size))
				// 			reward = 1;
				// 		else if(end_time - req->time <= (((double)avg) * 0.9 * req->size))
				// 			reward = 0.5;
				// 		else if(end_time - req->time <= (((double)avg) * 0.99 * req->size))
				// 			reward = 0;
				// 		else 
				// 			reward = -1;
				// 		if(request_count>1000)
				// 		//if(req->size>ssd->read_size/ssd->read_request_count)
				// 		//	reward=1;
				// 		if(1){
				// 			ssd->reward[request_count/1000]+=reward;
				// 			ssd->time[request_count/1000]+=ssd->now_request_time - ssd->pre_request_time;
				// 			ssd->size[request_count/1000]+=req->size;
				// 		}
				// 		if(reward>=0){
				// 			ssd->accu_reward[request_count/1000]+=1;
							
				// 		}
							
				// 		double *pre_position = ssd->map_RL + 9 * ssd->pre_state + ssd->pre_action;
				// 		double *now_position = ssd->map_RL + 9 * now_state + now_action;
				// 		*pre_position = 0.7 * (*pre_position) + 0.3 * (reward + 0.8 * (*now_position));
				// 		ssd->pre_state = now_state;
				// 		ssd->pre_action = now_action;
				// 		if(now_action==1)
				// 			now_action = 1;
				// 		else if(now_action==2)
				// 			now_action = 2;
				// 		else if(now_action==3)
				// 			now_action = 4;
				// 		else if(now_action==4)
				// 			now_action = 8;
				// 		else if(now_action==5){
				// 			now_action = 0;
				// 			erase = 1;
				// 		}
				// 		else if(now_action==6){
				// 			now_action = 1;
				// 			erase = 1;
				// 		}
				// 		else if(now_action==7){
				// 			now_action = 2;
				// 			erase = 1;
				// 		}
				// 		else if(now_action==8){
				// 			now_action = 4;
				// 			erase = 1;
				// 		}
				// 		else if(now_action==9){
				// 			now_action = 8;
				// 			erase = 1;
				// 		}
							
				// 		//else if(now_action==5)
				// 		//	now_action = 16;
				// 		//if(request_count >= 1000)
				// 		fprintf(ssd->statisticfile,"%16lld	%d	%d	%d\n",req->time,now_state+1,now_action,erase);
				// 		fflush(ssd->statisticfile);
				// 		if(now_action >= 0){
				// 			ssd->partial_flag=1;
				// 			int valid_flag=0;
				// 			ssd->patial_rr_number=now_action;
				// 			for(i=0;i<ssd->parameter->page_block;i++){
				// 				if(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].page_head[i].valid_state>0){
				// 					valid_flag=1;
				// 					break;
				// 				}
				// 			}
				// 			if(valid_flag==1){
				// 				ssd->now_erase_flag=erase;
				// 				action_for_PM(ssd, sub->location->channel, sub->location->chip, sub->location->die, sub->location->plane, sub->location->block);
				// 			}
				// 			//ssd->rr_realloc = 0;
				// 			//now_action--;
				// 			//sub = sub->next_subs;
				// 			//if(sub == NULL)
				// 				//sub = req->subs;
				// 		}
				// 		ssd->partial_flag=0;
				// 	}
				// }
				/**end RL **/	
				//fprintf(ssd->outputfile,"%10I64u %10u %6u %2u %16I64u %16I64u %10I64u\n",req->time,req->lsn, req->size, req->operation, start_time, end_time, end_time-req->time);
				// fprintf(ssd->outputfile,"%16lld %10d %6d %2d %16lld %16lld %10lld\n",req->time,req->lsn, req->size, req->operation, start_time, end_time, end_time-req->time);
				if(req->operation==1)
					fprintf(ssd->latencyfile,"%10lld,",end_time-req->time);
					if(end_time-req->time<5000000)
						ssd->zero2fifty++;
					else if(end_time-req->time>5000000 && end_time-req->time<15000000)
						ssd->fifty2onefifty++;
					else if(end_time-req->time>15000000 && end_time-req->time<25000000)
						ssd->onefifty2twofifty++;
					else if(end_time-req->time>25000000 && end_time-req->time<35000000)
						ssd->twofifty2threefifty++;
					else
						ssd->overthreefifty++;
					fflush(ssd->latencyfile);

				// fprintf(ssd->latencyfile,"%10lld,",end_time-req->time);
				// fflush(ssd->latencyfile);
	

				//以下代码是原有的部分
				//------------------------------------------------------------------------------
				ssd->completed_request_count++;
				if(req->operation==1)
					ssd->completed_read_count++;
				if(ssd->completed_request_count%10000 == 0){
					printf("completed requests: %d, max_queue_depth: %d, ", ssd->completed_request_count, ssd->max_queue_depth);
					printf("free_lsb: %d, free_csb: %d, free_msb: %d\n", ssd->free_lsb_count, ssd->free_csb_count, ssd->free_msb_count);
					ssd->max_queue_depth = 0;
					//statistic_output_easy(ssd, ssd->completed_request_count);
					ssd->newest_read_avg = 0;
					ssd->newest_write_avg = 0;
					ssd->newest_write_avg_l = 0;
					ssd->newest_read_request_count = 0;
					ssd->newest_write_request_count = 0;
					ssd->newest_write_lsb_count = 0;
					ssd->newest_write_csb_count = 0;
					ssd->newest_write_msb_count = 0;
					ssd->newest_write_request_completed_with_same_type_pages_l = 0;
					ssd->newest_write_request_num_l= 0;
					ssd->newest_req_with_lsb_l = 0;
					ssd->newest_req_with_csb_l = 0;
					ssd->newest_req_with_msb_l = 0;
					ssd->newest_write_request_completed_with_same_type_pages = 0;
					ssd->newest_req_with_lsb = 0;
					ssd->newest_req_with_csb = 0;
					ssd->newest_req_with_msb = 0;
					//***************************************************************************
					int channel;
					int this_day;
					this_day = (int)(ssd->current_time/1000000000/3600/24);
					if(this_day>ssd->time_day){
						printf("Day %d begin......\n", this_day);
						ssd->time_day = this_day;
						if((ssd->parameter->dr_switch==1)&&(this_day%ssd->parameter->dr_cycle==0)){
							for(channel=0;channel<ssd->parameter->channel_number;channel++){
								dr_for_channel(ssd, channel);
								}
							}
						/*
						if((ssd->parameter->turbo_mode==2)&&(this_day%2==1)){
							printf("Enter turbo-mode.....\n");
							ssd->parameter->lsb_first_allocation = 1;
							ssd->parameter->fast_gc = 1;
							}
						else{
							//printf("Exist turbo-mode....\n");
							ssd->parameter->lsb_first_allocation = 0;
							ssd->parameter->fast_gc = 0;
							}
							*/
						}
					//***************************************************************************
					}
				
				if(debug_0918){
					printf("completed requests: %d\n", ssd->completed_request_count);
					}
				if(end_time-start_time==0)
				{
					printf("the response time is 0?? position 2\n");
					//getchar();
				}
				if (req->operation==READ)
				{
					ssd->read_request_count++;
					ssd->read_avg=ssd->read_avg+(end_time-req->time);
					ssd->read_size+=req->size;
					//=============================================
					ssd->newest_read_request_count++;
					ssd->newest_read_avg = ssd->newest_read_avg+(end_time-req->time);
					//==============================================
				} 
				else
				{
					ssd->write_request_count++;
					ssd->write_avg=ssd->write_avg+(end_time-req->time);
					ssd->write_size+=req->size;
					//=============================================
					ssd->newest_write_request_count++;
					ssd->newest_write_avg = ssd->newest_write_avg+(end_time-req->time);
					ssd->last_write_lat = end_time-req->time;
					ssd->last_ten_write_lat[ssd->write_lat_anchor] = end_time-req->time;
					ssd->write_lat_anchor = (ssd->write_lat_anchor+1)%10;
					
					//--------------------------------------------
					int new_flag = 1;
					int origin, actual_type;
					int num_of_sub = 1;
					struct sub_request *next_sub_a;
					next_sub_a = req->subs;
					origin = next_sub_a->allocated_page_type;
					actual_type = next_sub_a->allocated_page_type;
					next_sub_a = next_sub_a->next_subs;
					while(next_sub_a!=NULL){
						num_of_sub++;
						if(next_sub_a->allocated_page_type > actual_type){
							actual_type = next_sub_a->allocated_page_type;
							}
						if(next_sub_a->allocated_page_type != origin){
							new_flag = 0;
							}
						next_sub_a = next_sub_a->next_subs;
						}
					if(num_of_sub>1){
						ssd->write_request_count_l++;
						ssd->newest_write_request_num_l++;
						ssd->newest_write_avg_l = ssd->newest_write_avg_l+(end_time-req->time);
						ssd->write_avg_l = ssd->write_avg_l+(end_time-req->time);
						}
					if(new_flag==1){
						ssd->newest_write_request_completed_with_same_type_pages++;
						if(num_of_sub>1){
							ssd->newest_write_request_completed_with_same_type_pages_l++;
							}
						if(origin==1){
							ssd->newest_msb_request_a++;
							}
						else if(origin==0){
							ssd->newest_lsb_request_a++;
							}
						else{
							ssd->newest_csb_request_a++;
							}
						}
					if(actual_type==TARGET_LSB){
						ssd->newest_req_with_lsb++;
						if(num_of_sub>1){
							ssd->newest_req_with_lsb_l++;
							}
						}
					else if(actual_type==TARGET_CSB){
						ssd->newest_req_with_csb++;
						if(num_of_sub>1){
							ssd->newest_req_with_csb_l++;
							}
						}
					else{
						ssd->newest_req_with_msb++;
						if(num_of_sub>1){
							ssd->newest_req_with_msb_l++;
							}
						}
					//-------------------------------------------
					
					//==============================================
				}

				while(req->subs!=NULL)
				{
					tmp = req->subs;
					req->subs = tmp->next_subs;
					if (tmp->update!=NULL)
					{
						free(tmp->update->location);
						tmp->update->location=NULL;
						free(tmp->update);
						tmp->update=NULL;
					}
					free(tmp->location);
					tmp->location=NULL;
					free(tmp);
					tmp=NULL;
				}
				
				if(pre_node == NULL)
				{
					if(req->next_node == NULL)
					{
						free(req->need_distr_flag);
						req->need_distr_flag=NULL;
						free(req);
						req = NULL;
						ssd->request_queue = NULL;
						ssd->request_tail = NULL;
						ssd->request_queue_length--;
					}
					else
					{
						ssd->request_queue = req->next_node;
						pre_node = req;
						req = req->next_node;
						free(pre_node->need_distr_flag);
						pre_node->need_distr_flag=NULL;
						free(pre_node);
						pre_node = NULL;
						ssd->request_queue_length--;
					}
				}
				else
				{
					if(req->next_node == NULL)
					{
						pre_node->next_node = NULL;
						free(req->need_distr_flag);
						req->need_distr_flag=NULL;
						free(req);
						req = NULL;
						ssd->request_tail = pre_node;	
						ssd->request_queue_length--;
					}
					else
					{
						pre_node->next_node = req->next_node;
						free(req->need_distr_flag);
						req->need_distr_flag=NULL;
						free(req);
						req = pre_node->next_node;
						ssd->request_queue_length--;
					}

				}
			}
			else
			{	
				pre_node = req;
				req = req->next_node;
			}
		}		
	}
}



void statistic_output(struct ssd_info *ssd)
{
	unsigned int lpn_count=0,i,j,k,m,p,erase=0,plane_erase=0;
	double gc_energy=0.0;
	extern float aveber ;
#ifdef DEBUG
	printf("enter statistic_output,  current time:%lld\n",ssd->current_time);
#endif

	for(i=0;i<ssd->parameter->channel_number;i++)
	{
		for(j=0;j<ssd->parameter->chip_channel[0];j++)
		{
			for(k=0;k<ssd->parameter->die_chip;k++)
			{
				for(p=0;p<ssd->parameter->plane_die;p++)
				{
					plane_erase=0;
					for(m=0;m<ssd->parameter->block_plane;m++)
					{
						//fprintf(f_read,"%d\n",ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].read_count);
						// fprintf(f_read,"%d\n",ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].read_count);
						if(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].erase_count>0)
						{
							erase=erase+ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].erase_count;
							plane_erase+=ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].erase_count;

						}
					}
				
				}
			}
		}
	}
	
	int page_num = ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num;
	double  valid_page_num = 0;
	double hot_read_pg_count = 0;
	for (int i = 0 ; i < page_num ; i++)
	{
		if (ssd->dram->map->map_entry[i].state > 0)
		{
			valid_page_num++;
			if (ssd->dram->map->map_entry[i].read_count > 6)
			{
				hot_read_pg_count++;
			}
		}
	}


	//fclose(f_read);
	//aveber new added
	aveber = (index1 * 0.002013 + index2 * 0.000607) / (index1 + index2);
	fprintf(ssd->outputfile,"\n");
	fprintf(ssd->outputfile,"\n");
	fprintf(ssd->outputfile,"---------------------------statistic data---------------------------\n");	 
	fprintf(ssd->outputfile,"min lsn: %13d\n",ssd->min_lsn);	
	fprintf(ssd->outputfile,"max lsn: %13d\n",ssd->max_lsn);
	fprintf(ssd->outputfile,"read count: %13d\n",ssd->read_count);	  
	fprintf(ssd->outputfile,"program count: %13d",ssd->program_count);	
	fprintf(ssd->outputfile,"                        include the flash write count leaded by read requests\n");
	fprintf(ssd->outputfile,"the read operation leaded by un-covered update count: %13d\n",ssd->update_read_count);
	fprintf(ssd->outputfile,"erase count: %13d\n",ssd->erase_count);
	fprintf(ssd->outputfile,"direct erase count: %13d\n",ssd->direct_erase_count);
	fprintf(ssd->outputfile,"copy back count: %13d\n",ssd->copy_back_count);
	fprintf(ssd->outputfile,"multi-plane program count: %13d\n",ssd->m_plane_prog_count);
	fprintf(ssd->outputfile,"multi-plane read count: %13d\n",ssd->m_plane_read_count);
	fprintf(ssd->outputfile,"interleave write count: %13d\n",ssd->interleave_count);
	fprintf(ssd->outputfile,"interleave read count: %13d\n",ssd->interleave_read_count);
	fprintf(ssd->outputfile,"interleave two plane and one program count: %13d\n",ssd->inter_mplane_prog_count);
	fprintf(ssd->outputfile,"interleave two plane count: %13d\n",ssd->inter_mplane_count);
	fprintf(ssd->outputfile,"gc copy back count: %13d\n",ssd->gc_copy_back);
	fprintf(ssd->outputfile,"write flash count: %13d\n",ssd->write_flash_count);
	//=================================================================
	fprintf(ssd->outputfile,"write LSB count: %13d\n",ssd->write_lsb_count);
	fprintf(ssd->outputfile,"write MSB count: %13d\n",ssd->write_msb_count);
	//=================================================================
	fprintf(ssd->outputfile,"interleave erase count: %13d\n",ssd->interleave_erase_count);
	fprintf(ssd->outputfile,"multiple plane erase count: %13d\n",ssd->mplane_erase_conut);
	fprintf(ssd->outputfile,"interleave multiple plane erase count: %13d\n",ssd->interleave_mplane_erase_count);
	fprintf(ssd->outputfile,"read request count: %13d\n",ssd->read_request_count);
	fprintf(ssd->outputfile,"write request count: %13d\n",ssd->write_request_count);
	fprintf(ssd->outputfile,"read request average size: %13f\n",ssd->ave_read_size);
	fprintf(ssd->outputfile,"write request average size: %13f\n",ssd->ave_write_size);
	fprintf(ssd->outputfile,"read request average response time: %lld\n",ssd->read_avg/ssd->read_request_count);
	fprintf(ssd->outputfile,"write request average response time: %lld\n",ssd->write_avg/ssd->write_request_count);
	fprintf(ssd->outputfile,"buffer read hits: %13d\n",ssd->dram->buffer->read_hit);
	fprintf(ssd->outputfile,"buffer read miss: %13d\n",ssd->dram->buffer->read_miss_hit);
	fprintf(ssd->outputfile,"buffer write hits: %13d\n",ssd->dram->buffer->write_hit);
	fprintf(ssd->outputfile,"buffer write miss: %13d\n",ssd->dram->buffer->write_miss_hit);
	fprintf(ssd->outputfile,"erase: %13d\n",erase);
	fprintf(ssd->outputfile,"sub_request_all: %13d, sub_request_success: %13d\n", ssd->sub_request_all, ssd->sub_request_success);
	fflush(ssd->outputfile);

	fclose(ssd->outputfile);


	//start
	fprintf(ssd->statisticfile,"\n");
	fprintf(ssd->statisticfile,"\n");
	fprintf(ssd->statisticfile,"---------------------------attention---------------------------\n");
	//fprintf(ssd->statisticfile,"rr_count: %13d\n",ssd->rr_count);
	fprintf(ssd->statisticfile,"evict block:%d\n",ssd->remove);
	fprintf(ssd->statisticfile,"select block: %d\n",ssd->gc_sel);
	fprintf(ssd->statisticfile,"move gc hot read page: %d\n",ssd->move_gc);
	fprintf(ssd->statisticfile,"move rr hot read page: %d\n",ssd->move_rr);
	fprintf(ssd->statisticfile,"find_hot_w_active_block: %13d\n",ssd->hot_block_count_w);//通过写请求来申请了多少次热块=擦+最后有
	fprintf(ssd->statisticfile,"迁移操作到已有的块有: %13d,迁移操作新申请的块： %13d\n",ssd->hot_block_count_r1,ssd->hot_block_count_r2);  //迁找了几次块=已有+申请
	// fprintf(ssd->statisticfile,"id=1填满有: %d\n",ssd->write_1); 
	fprintf(ssd->statisticfile,"rr erase hot: %d, rr erase normal :%d, gc erase hot: %d\n",ssd->rr_erase_1,ssd->rr_erase_0,ssd->gc_erase_1);
	fprintf(ssd->statisticfile,"hot_rate: %f\n",hot_read_pg_count/valid_page_num);
	fprintf(ssd->statisticfile,"read hot page num: %f\n",hot_read_pg_count);
	fprintf(ssd->statisticfile,"valid page num: %f\n",valid_page_num);
	fprintf(ssd->statisticfile,"total erase count: %13d\n",ssd->erase_count);
	fprintf(ssd->statisticfile,"normal erase count: %13d\n",ssd->normal_erase);
	fprintf(ssd->statisticfile,"direct erase count: %13d\n",ssd->direct_erase_count);	
	fprintf(ssd->statisticfile,"rr_erase count: %13d\n",ssd->rr_erase_count);
	fprintf(ssd->statisticfile,"page move count: %d\n",ssd->page_move_count);  //sr,ug,to,apm
	fprintf(ssd->statisticfile,"gc page move count: %d\n",(ssd->page_move_count)-(ssd->rr_page_move_count));
	// fprintf(ssd->statisticfile,"find_active_block: %13d\n",ssd->find_active_block_count);
	// fprintf(ssd->statisticfile,"find_free_block: %13d\n",ssd->find_free_block_count);
	fprintf(ssd->statisticfile,"read request total response time: %llu\n",ssd->read_avg);
	fprintf(ssd->statisticfile,"read request average response time: %llu\n",ssd->read_avg/ssd->read_request_count);  //lld
	fprintf(ssd->statisticfile,"write request total response time: %llu\n",ssd->write_avg);
	fprintf(ssd->statisticfile,"write request average response time: %llu\n",ssd->write_avg/ssd->write_request_count);
	fprintf(ssd->statisticfile,"read request count: %13d\n",ssd->read_request_count);
	fprintf(ssd->statisticfile,"write request count: %13d\n",ssd->write_request_count);
	fprintf(ssd->statisticfile,"average response time: %llu\n",(ssd->read_avg+ssd->write_avg)/(ssd->read_request_count+ssd->write_request_count));
	fprintf(ssd->statisticfile,"total sub_read_count: %lld\n",ssd->total_sub_read_count);
	fprintf(ssd->statisticfile,"read count: %13d\n",ssd->read_count);
	fprintf(ssd->statisticfile,"direct_read_count: %lld\n",ssd->direct_read_count);  //可以直接读的已有的
	fprintf(ssd->statisticfile,"total_read_count: %lld\n",ssd->total_read_count);
	fprintf(ssd->statisticfile,"normal read count: %lld\n",ssd->read_count_fre1);
	fprintf(ssd->statisticfile,"the read operation leaded by un-covered update count: %13d\n",ssd->update_read_count);
	fprintf(ssd->statisticfile,"---------------------------statistic data1---------------------------\n");
	// fprintf(ssd->statisticfile,"page_move_count: %d\n",ssd->page_move_count);
	fprintf(ssd->statisticfile,"min lsn: %13d\n",ssd->min_lsn);	
	fprintf(ssd->statisticfile,"max lsn: %13lld\n",ssd->max_lsn);
	// fprintf(ssd->statisticfile,"wl_page_move_count: %d\n",ssd->wl_page_move_count);	
	// fprintf(ssd->statisticfile,"wl_erase count: %13d\n",ssd->wl_erase_count);
	// fprintf(ssd->statisticfile,"wl_request: %13d\n",ssd->wl_request);
	//fprintf(ssd->statisticfile,"rr_page_move_count: %d\n",ssd->rr_page_move_count);
	fprintf(ssd->statisticfile,"program count: %13d",ssd->program_count);	  
	fprintf(ssd->statisticfile,"                   include the flash write count leaded by read requests\n");

	fprintf(ssd->statisticfile,"rr_page_move_count: %d\n",ssd->rr_page_move_count);	
	// fprintf(ssd->statisticfile,"debug_rr_count: %13d\n",ssd->debug_rr_count);
	fprintf(ssd->statisticfile,"error read count1: %lld\n",ssd->read_count_fre2);
	fprintf(ssd->statisticfile,"error read count2: %lld\n",ssd->read_count_fre3);
	fprintf(ssd->statisticfile,"error read count3: %lld\n",ssd->read_count_fre4);
	fprintf(ssd->statisticfile,"error read count4: %lld\n",ssd->read_count_fre5);
	fprintf(ssd->statisticfile,"error read count5: %lld\n",ssd->read_count_fre6);
	fprintf(ssd->statisticfile,"error read count6: %lld\n",ssd->read_count_fre7);
	fprintf(ssd->statisticfile,"first_migrate_count: %lld\n",ssd->first_migrate_count);
	fprintf(ssd->statisticfile,"second_migrate_count: %lld\n",ssd->second_migrate_count);
	fprintf(ssd->statisticfile,"third_migrate_count: %lld\n",ssd->third_migrage_count);
	fprintf(ssd->statisticfile,"---------------------------statistic data2---------------------------\n");
	// fprintf(ssd->statisticfile,"copy back count: %13d\n",ssd->copy_back_count);
	// fprintf(ssd->statisticfile,"multi-plane program count: %13d\n",ssd->m_plane_prog_count);
	// fprintf(ssd->statisticfile,"multi-plane read count: %13d\n",ssd->m_plane_read_count);
	// fprintf(ssd->statisticfile,"interleave count: %13d\n",ssd->interleave_count);
	// fprintf(ssd->statisticfile,"interleave read count: %13d\n",ssd->interleave_read_count);
	// fprintf(ssd->statisticfile,"interleave two plane and one program count: %13d\n",ssd->inter_mplane_prog_count);
	// fprintf(ssd->statisticfile,"interleave two plane count: %13d\n",ssd->inter_mplane_count);
	// fprintf(ssd->statisticfile,"gc copy back count: %13d\n",ssd->gc_copy_back);
	fprintf(ssd->statisticfile,"write flash count: %13d\n",ssd->write_flash_count);  //lsb+csb+msb
	// fprintf(ssd->statisticfile,"waste page count: %13d\n",ssd->waste_page_count);
	//=================================================================
	// fprintf(ssd->statisticfile,"write LSB count: %13d\n",ssd->write_lsb_count);
	// fprintf(ssd->statisticfile,"write CSB count: %13d\n",ssd->write_csb_count);
	// fprintf(ssd->statisticfile,"write MSB count: %13d\n",ssd->write_msb_count);
	//=================================================================
	// fprintf(ssd->statisticfile,"interleave erase count: %13d\n",ssd->interleave_erase_count);
	// fprintf(ssd->statisticfile,"multiple plane erase count: %13d\n",ssd->mplane_erase_conut);
	// fprintf(ssd->statisticfile,"interleave multiple plane erase count: %13d\n",ssd->interleave_mplane_erase_count);
	fprintf(ssd->statisticfile,"read request average size: %13f\n",ssd->ave_read_size);
	fprintf(ssd->statisticfile,"write request average size: %13f\n",ssd->ave_write_size);

	if(ssd->write_request_count_l==0){
		fprintf(ssd->statisticfile,"large write request average response time: 0\n");
		}
	else{
		fprintf(ssd->statisticfile,"large write request average response time: %lld\n",ssd->write_avg_l/ssd->write_request_count_l);
		}
	fprintf(ssd->statisticfile,"buffer read hits: %13d\n",ssd->dram->buffer->read_hit);
	fprintf(ssd->statisticfile,"buffer read miss: %13d\n",ssd->dram->buffer->read_miss_hit);
	fprintf(ssd->statisticfile,"buffer write hits: %13d\n",ssd->dram->buffer->write_hit);
	fprintf(ssd->statisticfile,"buffer write miss: %13d\n",ssd->dram->buffer->write_miss_hit);
	fprintf(ssd->statisticfile,"erase: %13d\n",erase);

	fprintf(ssd->statisticfile, "RRcount: %13d\n", RRcount);

	fprintf(ssd->statisticfile,"sub_request_all: %13d, sub_request_success: %13d\n", ssd->sub_request_all, ssd->sub_request_success);
	fprintf(ssd->statisticfile, "index1: %13d\n", index1);
	fprintf(ssd->statisticfile, "index2: %13d\n", index2);
	fprintf(ssd->statisticfile, "aveber: %9f\n", aveber);
	fprintf(ssd->statisticfile, "read_error_rate: %f\n", 0.001*ssd->read_error_rate/ssd->error_read_count);
	fprintf(ssd->statisticfile, "max_read_error_rate: %f\n", 0.001*ssd->max_read_error_rate);
	fprintf(ssd->statisticfile, "zero2fifty: %13d\n", ssd->zero2fifty);
	fprintf(ssd->statisticfile, "fifty2onefifty: %13d\n", ssd->fifty2onefifty);
	fprintf(ssd->statisticfile, "onefifty2twofifty: %13d\n", ssd->onefifty2twofifty);
	fprintf(ssd->statisticfile, "twofifty2threefifty: %13d\n", ssd->twofifty2threefifty);
	fprintf(ssd->statisticfile, "overthreefifty: %13d\n", ssd->overthreefifty);
	int temp=0;
	for(temp=0; temp<100; temp++){
		if(ssd->reward[temp]!=0)
			fprintf(ssd->statisticfile, "%13d\n", ssd->reward[temp]);
	}
	fprintf(ssd->statisticfile, "%13d\n", ssd->request_count);
	for(temp=0; temp<100; temp++){
		if(ssd->accu_reward[temp]!=0)
			fprintf(ssd->statisticfile, "%13d\n", ssd->accu_reward[temp]);
	}
	fprintf(ssd->statisticfile, "\n");
	for(temp=0; temp<100; temp++){
		if(ssd->time[temp]!=0)
			fprintf(ssd->statisticfile, "%13d\n", ssd->time[temp]);
	}
	fprintf(ssd->statisticfile, "\n");
	for(temp=0; temp<100; temp++){
		if(ssd->size[temp]!=0)
			fprintf(ssd->statisticfile, "%13d\n", ssd->size[temp]);
	}
	
	fflush(ssd->statisticfile);

	fclose(ssd->statisticfile);
	fprintf(ssd->latencyfile,"\n");
	fflush(ssd->latencyfile);
	fclose(ssd->latencyfile);
}

void statistic_output_easy(struct ssd_info *ssd, unsigned long completed_requests_num){
	unsigned int lpn_count=0,i,j,k,m,erase=0,plane_erase=0;
	double gc_energy=0.0;
#ifdef DEBUG
	fprintf(ssd->debugfile,"enter statistic_output,  current time:%lld\n",ssd->current_time);
	//printf("enter statistic_output,  current time:%lld\n",ssd->current_time);
#endif
	unsigned long read_avg_lat, write_avg_lat, write_avg_lat_l;
	if(ssd->newest_read_request_count==0){
		read_avg_lat=0;
		}
	else{
		read_avg_lat=ssd->newest_read_avg/ssd->newest_read_request_count;
		}
	if(ssd->newest_write_request_count==0){
		write_avg_lat=0;
		}
	else{
		write_avg_lat=ssd->newest_write_avg/ssd->newest_write_request_count;
		}
	if(ssd->newest_write_request_num_l==0){
		write_avg_lat_l=0;
		}
	else{
		write_avg_lat_l = ssd->newest_write_avg_l/ssd->newest_write_request_num_l;
		}
	fprintf(ssd->statisticfile, "%lld, %16lld, %13d, %13lld, %13lld, %13d, %13d, %13d, ", completed_requests_num, ssd->current_time, ssd->erase_count, read_avg_lat, write_avg_lat,ssd->newest_write_lsb_count,ssd->newest_write_csb_count,ssd->newest_write_msb_count);
	fprintf(ssd->statisticfile, "%13d, %13d, %13d, %13d, %13d, ", ssd->fast_gc_count, ssd->moved_page_count, ssd->free_lsb_count, ssd->newest_read_request_count, ssd->newest_write_request_count);
	fprintf(ssd->statisticfile, "%13d, %13d, %13d, %13d, ", ssd->newest_write_request_completed_with_same_type_pages, ssd->newest_req_with_lsb, ssd->newest_req_with_csb, ssd->newest_req_with_msb);
	fprintf(ssd->statisticfile, "% 13d, %13d, %13d, %13d, %13d, %13d\n", ssd->newest_write_request_num_l, ssd->newest_write_request_completed_with_same_type_pages_l, ssd->newest_req_with_lsb_l, ssd->newest_req_with_csb_l, ssd->newest_req_with_msb_l, write_avg_lat_l);
	
	//fprintf(ssd->statisticfile, "%13d, %13d, %13d\n", ssd->newest_write_request_completed_with_same_type_pages, ssd->newest_write_lsb_count, ssd->newest_write_msb_count);
	//fprintf(ssd->statisticfile,"\n\n");
	fflush(ssd->statisticfile);
}



unsigned int size(unsigned int stored)
{
	unsigned int i,total=0,mask=0x80000000;

	#ifdef DEBUG
	printf("enter size\n");
	#endif
	for(i=1;i<=32;i++)
	{
		if(stored & mask) total++;
		stored<<=1;
	}
	#ifdef DEBUG
	    printf("leave size\n");
    #endif
    return total;
}


unsigned int transfer_size(struct ssd_info *ssd,int need_distribute,unsigned int lpn,struct request *req)
{
	unsigned int first_lpn,last_lpn,state,trans_size;
	unsigned int mask=0,offset1=0,offset2=0;

	first_lpn=req->lsn/ssd->parameter->subpage_page;
	last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;

	mask=~(0xffffffff<<(ssd->parameter->subpage_page));
	state=mask;
	if(lpn==first_lpn)
	{
		offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-req->lsn);
		state=state&(0xffffffff<<offset1);
	}
	if(lpn==last_lpn)
	{
		offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(req->lsn+req->size));
		state=state&(~(0xffffffff<<offset2));
	}

	trans_size=size(state&need_distribute);

	return trans_size;
}



int64_t find_nearest_event(struct ssd_info *ssd) 
{
	unsigned int i,j;
	int64_t time=MAX_INT64;
	int64_t time1=MAX_INT64;
	int64_t time2=MAX_INT64;

	for (i=0;i<ssd->parameter->channel_number;i++)
	{
		if (ssd->channel_head[i].next_state==CHANNEL_IDLE)
			if(time1>ssd->channel_head[i].next_state_predict_time)
				if (ssd->channel_head[i].next_state_predict_time>ssd->current_time)    
					time1=ssd->channel_head[i].next_state_predict_time;
		for (j=0;j<ssd->parameter->chip_channel[i];j++)
		{
			if ((ssd->channel_head[i].chip_head[j].next_state==CHIP_IDLE)||(ssd->channel_head[i].chip_head[j].next_state==CHIP_DATA_TRANSFER))
				if(time2>ssd->channel_head[i].chip_head[j].next_state_predict_time)
					if (ssd->channel_head[i].chip_head[j].next_state_predict_time>ssd->current_time)    
						time2=ssd->channel_head[i].chip_head[j].next_state_predict_time;	
		}   
	} 

	/*****************************************************************************************************
	 *timeΪ���� A.��һ״̬ΪCHANNEL_IDLE����һ״̬Ԥ��ʱ�����ssd��ǰʱ���CHANNEL����һ״̬Ԥ��ʱ��
	 *           B.��һ״̬ΪCHIP_IDLE����һ״̬Ԥ��ʱ�����ssd��ǰʱ���DIE����һ״̬Ԥ��ʱ��
	 *		     C.��һ״̬ΪCHIP_DATA_TRANSFER����һ״̬Ԥ��ʱ�����ssd��ǰʱ���DIE����һ״̬Ԥ��ʱ��
	 *CHIP_DATA_TRANSFER��׼����״̬�������Ѵӽ��ʴ�����register����һ״̬�Ǵ�register����buffer�е���Сֵ 
	 *ע����ܶ�û������Ҫ���time����ʱtime����0x7fffffffffffffff ��
	*****************************************************************************************************/
	time=(time1>time2)?time2:time1;
	return time;
}


void free_all_node(struct ssd_info *ssd)
{
	unsigned int i,j,k,l,n;
	struct buffer_group *pt=NULL;
	struct direct_erase * erase_node=NULL;
	for (i=0;i<ssd->parameter->channel_number;i++)
	{
		for (j=0;j<ssd->parameter->chip_channel[0];j++)
		{
			for (k=0;k<ssd->parameter->die_chip;k++)
			{
				for (l=0;l<ssd->parameter->plane_die;l++)
				{
					for (n=0;n<ssd->parameter->block_plane;n++)
					{
						free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head);
						ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head=NULL;
					}
					free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head);
					ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head=NULL;
					while(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node!=NULL)
					{
						erase_node=ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
						ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node=erase_node->next_node;
						free(erase_node);
						erase_node=NULL;
					}
				}
				
				free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head);
				ssd->channel_head[i].chip_head[j].die_head[k].plane_head=NULL;
			}
			free(ssd->channel_head[i].chip_head[j].die_head);
			ssd->channel_head[i].chip_head[j].die_head=NULL;
		}
		free(ssd->channel_head[i].chip_head);
		ssd->channel_head[i].chip_head=NULL;
	}
	free(ssd->channel_head);
	ssd->channel_head=NULL;

	avlTreeDestroy( ssd->dram->buffer);
	ssd->dram->buffer=NULL;
	
	free(ssd->dram->map->map_entry);
	ssd->dram->map->map_entry=NULL;
	free(ssd->dram->map);
	ssd->dram->map=NULL;
	free(ssd->dram);
	ssd->dram=NULL;
	free(ssd->parameter);
	ssd->parameter=NULL;

	free(ssd);
	ssd=NULL;
}



struct ssd_info *make_aged(struct ssd_info *ssd)
{
	unsigned int i,j,k,l,m,n,ppn;
	int threshould,flag=0;
    
	if (ssd->parameter->aged==1)
	{
		//threshold��ʾһ��plane���ж���ҳ��Ҫ��ǰ��ΪʧЧ
		threshould=(int)(ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->aged_ratio);  
		for (i=0;i<ssd->parameter->channel_number;i++)
			for (j=0;j<ssd->parameter->chip_channel[i];j++)
				for (k=0;k<ssd->parameter->die_chip;k++)
					for (l=0;l<ssd->parameter->plane_die;l++)
					{  
						flag=0;
						for (m=0;m<ssd->parameter->block_plane;m++)
						{  
							if (flag>=threshould)
							{
								break;
							}
							for (n=0;n<(ssd->parameter->page_block*ssd->parameter->aged_ratio+1);n++)
							{  
								ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].valid_state=0;        //��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0
								ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].free_state=0;         //��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0
								ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].lpn=0;  //��valid_state free_state lpn����Ϊ0��ʾҳʧЧ������ʱ�������⣬����lpn=0��������Чҳ
								ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_page_num--;
								ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].invalid_page_num++;
								ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_page++;
								ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page--;
								flag++;
								if(n%3==0){
									ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_lsb=n;
									ssd->free_lsb_count--;
									ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_lsb_num--;
									//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
									ssd->write_lsb_count++;
									ssd->newest_write_lsb_count++;
									ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_lsb_num--;
									}
								else if(n%3==2){
									ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_msb=n;
									ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_msb_num--;
									//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
									ssd->write_msb_count++;
									ssd->free_msb_count--;
									ssd->newest_write_msb_count++;
									}
								else{
									ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_csb=n;
									ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_csb_num--;
									ssd->write_csb_count++;
									ssd->free_csb_count--;
									ssd->newest_write_csb_count++;
									}
								ppn=find_ppn(ssd,i,j,k,l,m,n);
							
							}
						} 
					}	 
	}  
	else
	{
		return ssd;
	}

	return ssd;
}

int get_old_zwh(struct ssd_info *ssd){
	int cn_id, cp_id, di_id, pl_id;
	printf("Enter get_old_zwh.\n");
	for(cn_id=0;cn_id<ssd->parameter->channel_number;cn_id++){
		//printf("channel %d\n", cn_id);
		for(cp_id=0;cp_id<ssd->parameter->chip_channel[0];cp_id++){
			//printf("chip %d\n", cp_id);
			for(di_id=0;di_id<ssd->parameter->die_chip;di_id++){
				//printf("die %d\n", di_id);
				for(pl_id=0;pl_id<ssd->parameter->plane_die;pl_id++){
					//printf("channel %d, chip %d, die %d, plane %d: ", cn_id, cp_id, di_id, pl_id);
					int active_block, ppn, lpn;
					struct local *location;
					lpn = 0;
					while(ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].free_page > (ssd->parameter->page_block*ssd->parameter->block_plane)*0.3){
						//if(cn_id==0&&cp_id==2&&di_id==0&&pl_id==0){
						//	printf("cummulating....\n");
						//	}
						if(find_active_block(ssd,cn_id,cp_id,di_id,pl_id)==FAILURE)
							{
								printf("Wrong in get_old_zwh, find_active_block\n");	
								return 0;
							}
						active_block=ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].active_block;
						if(write_page(ssd,cn_id,cp_id,di_id,pl_id,active_block,&ppn)==ERROR)
							{
								return 0;
							}
						location=find_location(ssd,ppn);
						ssd->program_count++;
						ssd->channel_head[cn_id].program_count++;
						ssd->channel_head[cn_id].chip_head[cp_id].program_count++;	
						ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[active_block].page_head[location->page].lpn=0;
						ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[active_block].page_head[location->page].valid_state=0;
						ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[active_block].page_head[location->page].free_state=0;
						ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[active_block].invalid_page_num++;
						free(location);
						location=NULL;
						}
					//printf("%d\n", ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].free_page);
					}
				}
			}
		}
	printf("Exit get_old_zwh.\n");
}


struct ssd_info *no_buffer_distribute(struct ssd_info *ssd)
{
	unsigned int lsn,lpn,last_lpn,first_lpn,complete_flag=0, state;
	unsigned int flag=0,flag1=1,active_region_flag=0;           //to indicate the lsn is hitted or not
	struct request *req=NULL;
	struct sub_request *sub=NULL,*sub_r=NULL,*update=NULL;
	struct local *loc=NULL;
	struct channel_info *p_ch=NULL;
	int i;
	
	unsigned int mask=0; 
	unsigned int offset1=0, offset2=0;
	unsigned int sub_size=0;
	unsigned int sub_state=0;

	ssd->dram->current_time=ssd->current_time;
	req=ssd->request_tail;       
	lsn=req->lsn;
	lpn=req->lsn/ssd->parameter->subpage_page;
	last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;
	first_lpn=req->lsn/ssd->parameter->subpage_page;

	if(req->operation==READ)        
	{		
		while(lpn<=last_lpn) 		
		{
			sub_state=(ssd->dram->map->map_entry[lpn].state&0x7fffffff);
			sub_size=size(sub_state);
			sub=creat_sub_request(ssd,lpn,sub_size,sub_state,req,req->operation,0);
			lpn++;
		}
	}
	else if(req->operation==WRITE)
	{
		int target_page_type;
		int random_num;
		random_num = rand()%100;
		if(random_num<ssd->parameter->turbo_mode_factor){
			target_page_type = TARGET_LSB;
			}
		else if(random_num<ssd->parameter->turbo_mode_factor_2){
			target_page_type = TARGET_CSB;
			}
		else{
			target_page_type = TARGET_MSB;
			}
		while(lpn<=last_lpn)     	
		{	
			if(ssd->parameter->subpage_page == 32){
				mask = 0xffffffff;
				}
			else{
				mask=~(0xffffffff<<(ssd->parameter->subpage_page));
				}
			state=mask;
			//printf("initial state: %x\n", state);
			if(lpn==first_lpn)
			{
				offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-req->lsn);
				//printf("offset1: %d, ", offset1);
				state=state&(0xffffffff<<offset1);
				//printf("state: %x\n", state);
			}
			if(lpn==last_lpn)
			{
				offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(req->lsn+req->size));
				//printf("offset2: %d, ", offset2);
				if(offset2 != 32){
					state=state&(~(0xffffffff<<offset2));
					}
				//printf("state: %x\n", state);
			}
			//printf("state: %x, ", state);
			sub_size=size(state);
			//printf("sub_size: %d\n", sub_size);
			sub=creat_sub_request(ssd,lpn,sub_size,state,req,req->operation,target_page_type);
			lpn++;
		}
	}

	return ssd;
}


int full_block(struct ssd_info *ssd)
{
	int cn_id, cp_id, di_id, pl_id,bl_id,cnt;
	printf("Enter full_block......\n");
	for(cn_id=0;cn_id<ssd->parameter->channel_number;cn_id++){
		for(cp_id=0;cp_id<ssd->parameter->chip_channel[0];cp_id++){  //2
			for(di_id=0;di_id<ssd->parameter->die_chip;di_id++){
				for(pl_id=0;pl_id<ssd->parameter->plane_die;pl_id++){
					for(bl_id=0;bl_id < full_ratio * ssd->parameter->block_plane;bl_id++){   //给多少块填数据
						if(ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].free_page_num!=ssd->parameter->page_block)
						{
							continue;
						}
						for(int i = 0;i < 0.85 * ssd->parameter->page_block ; i++)   //每块填多少无效页
						{
							full_invalid(ssd,cn_id,cp_id,di_id,pl_id,bl_id);
						}
					}
				}
			}
		}
	}
	get_lsn(ssd);
    full_valid(ssd);   //给多少块填数据
	printf("\nfull_block is complete!\n");
}


int full_valid(struct ssd_info *ssd)
{
	unsigned int full_page,sub_size,lpn,ppn,add_size=0;
	unsigned int largest_lsn,max_lpn;
	unsigned long long max_lsn;
	int cn_id, cp_id, di_id, pl_id,bl_id;

	largest_lsn=(unsigned int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
	max_lsn=ssd->max_lsn % largest_lsn;
	full_page = ~(0xffffffff<<(ssd->parameter->subpage_page));
	sub_size = ssd->parameter->subpage_page;  //16

	for(cn_id=0;cn_id<ssd->parameter->channel_number;cn_id++){
			for(cp_id=0;cp_id<ssd->parameter->chip_channel[0];cp_id++){  //2
				for(di_id=0;di_id<ssd->parameter->die_chip;di_id++){
					for(pl_id=0;pl_id<ssd->parameter->plane_die;pl_id++){
						for(bl_id=0;bl_id < full_ratio * ssd->parameter->block_plane;bl_id++){

							if(ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].free_page_num == (int)(0.15 * ssd->parameter->page_block) && 
							ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].invalid_page_num == (int)(0.85 * ssd->parameter->page_block)+1)
							{
								for(int i = 0;i < ssd->parameter->page_block ; i++)
								{
									//while(1){};
									if(ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].page_head[i].free_state == PG_SUB)  //把填完无效页的块剩余空闲页全填成有效
									{
 										lpn=max_lsn/ssd->parameter->subpage_page;
										max_lsn+=sub_size;
										ppn=find_ppn(ssd,cn_id,cp_id,di_id,pl_id,bl_id,i);
										ssd->dram->map->map_entry[lpn].pn=ppn;	
										ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,max_lsn,sub_size); 
										ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].page_head[i].lpn=lpn;
										ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].page_head[i].valid_state=ssd->dram->map->map_entry[lpn].state;
										ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].page_head[i].free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page); 
										ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].page_head[i].written_count++;
										ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].free_page_num--; 
										ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].free_page--;
									}
								}
							}
						}
					}
				}
			}
	}

	return 1;
}


int full_invalid(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block)
{
	int ppn,lpn=0;
	struct local *location;
	if(write_page(ssd,channel,chip,die,plane,block,&ppn)==ERROR)
	{
		return 0;
	}
	location=find_location(ssd,ppn);
	ssd->program_count++;
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[location->page].lpn=0;  //置为无效
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[location->page].valid_state=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[location->page].free_state=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num++;
	free(location);
	location=NULL;	
}

int get_lsn(struct ssd_info *ssd)
{
	char buffer_request[200];
	unsigned int device,size,ope,lpn,large_lsn,lsn=0;
	int64_t time = 0;

	ssd->tracefile=fopen(ssd->tracefilename,"r");
	if(ssd->tracefile == NULL )      
	{
		printf("the trace file can't open123.\n");
		return 0;
	}
  
	while(fgets(buffer_request,200,ssd->tracefile))
	{
		sscanf(buffer_request,"%lld %d %d %d %d",&time,&device,&lsn,&size,&ope);
		trace_assert(time,device,lsn,size,ope);   
		large_lsn=(int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
		lsn=lsn%large_lsn;
		if (lsn<ssd->min_lsn){
			ssd->min_lsn=lsn;
		} 
		if (lsn>ssd->max_lsn){
			ssd->max_lsn=lsn;
		}        

	}
	return 1;
}

int full_block_valid(struct ssd_info *ssd)
{
	int cn_id, cp_id, di_id, pl_id,bl_id,cnt;
	get_lsn(ssd);
	printf("Enter full_block......\n");

	full_valid1(ssd);

	printf("\nfull_block is complete!\n");
}


int full_valid1(struct ssd_info *ssd)
{
	unsigned int full_page,sub_size,lpn,ppn,add_size=0;
	unsigned int largest_lsn,max_lpn;
	unsigned long long max_lsn;
	int cn_id, cp_id, di_id, pl_id,bl_id;

	largest_lsn=(unsigned int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
	max_lsn=ssd->max_lsn % largest_lsn;
	full_page = ~(0xffffffff<<(ssd->parameter->subpage_page));
	sub_size = ssd->parameter->subpage_page;  //16

	for(cn_id=0;cn_id<ssd->parameter->channel_number;cn_id++){
			for(cp_id=0;cp_id<ssd->parameter->chip_channel[0];cp_id++){  //2
				for(di_id=0;di_id<ssd->parameter->die_chip;di_id++){
					for(pl_id=0;pl_id<ssd->parameter->plane_die;pl_id++){
						for(bl_id=0;bl_id < full_ratio * ssd->parameter->block_plane;bl_id++){
							if(ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].free_page_num!=ssd->parameter->page_block)
							{
								continue;
							}
							for(int i = 0;i < (full_valid_ratio * ssd->parameter->page_block)-1 ; i++)  //每块填多少有效页
							{
								lpn=max_lsn/ssd->parameter->subpage_page;
								max_lsn+=sub_size;
								ppn=find_ppn(ssd,cn_id,cp_id,di_id,pl_id,bl_id,i);
								ssd->dram->map->map_entry[lpn].pn=ppn;	
								ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,max_lsn,sub_size); 
								ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].page_head[i].lpn=lpn;
								ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].page_head[i].valid_state=ssd->dram->map->map_entry[lpn].state;
								ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].page_head[i].free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page); 
								ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].page_head[i].written_count++;
								ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[bl_id].free_page_num--; 
								ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].free_page--;
							}
						}
					}
				}
			}
	}

	return 1;
}