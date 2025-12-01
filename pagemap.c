
#define _CRTDBG_MAP_ALLOC
 
#include "pagemap.h"
#include "ssd.h"
#define rank_size 512


void file_assert(int error,char *s)
{
	if(error == 0) return;
	printf("open %s error\n",s);
	getchar();
	exit(-1);
}
void alloc_assert(void *p,char *s)//����
{
	if(p!=NULL) return;
	printf("malloc %s error\n",s);
	getchar();
	exit(-1);
}
void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope)//����
{
	if(time_t <0 || device < 0 || lsn < 0 || size < 0 || ope < 0)
	{
		printf("trace error:%lld %d %d %d %d\n",time_t,device,lsn,size,ope);
		getchar();
		exit(-1);
	}
	if(time_t == 0 && device == 0 && lsn == 0 && size == 0 && ope == 0)
	{
		printf("probable read a blank line\n");
		getchar();
	}
}
struct local *find_location(struct ssd_info *ssd,unsigned int ppn)
{
	struct local *location=NULL;
	unsigned int i=0;
	int pn,ppn_value=ppn;
	int page_plane=0,page_die=0,page_chip=0,page_channel=0;

	pn = ppn;

#ifdef DEBUG
	printf("enter find_location\n");
#endif

	location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(location,"location");
	memset(location,0, sizeof(struct local));

	page_plane=ssd->parameter->page_block*ssd->parameter->block_plane; //number of page per plane
	page_die=page_plane*ssd->parameter->plane_die;                     //number of page per die
	page_chip=page_die*ssd->parameter->die_chip;                       //number of page per chip
	page_channel=page_chip*ssd->parameter->chip_channel[0];            //number of page per channel
	
	location->channel = ppn/page_channel;
	location->chip = (ppn%page_channel)/page_chip;
	location->die = ((ppn%page_channel)%page_chip)/page_die;
	location->plane = (((ppn%page_channel)%page_chip)%page_die)/page_plane;
	location->block = ((((ppn%page_channel)%page_chip)%page_die)%page_plane)/ssd->parameter->page_block;
	location->page = (((((ppn%page_channel)%page_chip)%page_die)%page_plane)%ssd->parameter->page_block)%ssd->parameter->page_block;

	return location;
}
unsigned int find_ppn(struct ssd_info * ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page)
{
	unsigned int ppn=0;
	unsigned int i=0;
	int page_plane=0,page_die=0,page_chip=0;
	int page_channel[100];                  /*��������ŵ���ÿ��channel��page��Ŀ*/

#ifdef DEBUG
	printf("enter find_psn,channel:%d, chip:%d, die:%d, plane:%d, block:%d, page:%d\n",channel,chip,die,plane,block,page);
#endif
    
	page_plane=ssd->parameter->page_block*ssd->parameter->block_plane;
	page_die=page_plane*ssd->parameter->plane_die;
	page_chip=page_die*ssd->parameter->die_chip;
	while(i<ssd->parameter->channel_number)
	{
		page_channel[i]=ssd->parameter->chip_channel[i]*page_chip;
		i++;
	}
	i=0;
	while(i<channel)
	{
		ppn=ppn+page_channel[i];
		i++;
	}
	ppn=ppn+page_chip*chip+page_die*die+page_plane*plane+block*ssd->parameter->page_block+page;
	
	return ppn;
}
int set_entry_state(struct ssd_info *ssd,unsigned int lsn,unsigned int size)
{
	int temp,state,move;
	if(size == 32){
		temp = 0xffffffff;
		}
	else{
		temp=~(0xffffffff<<size);
		}
	move=lsn%ssd->parameter->subpage_page;
	state=temp<<move;

	return state;
}
struct ssd_info *pre_process_page(struct ssd_info *ssd)
{
	int fl=0;
	unsigned int device,lsn,size,ope,lpn,full_page,read_flag;
	unsigned int largest_lsn,sub_size,ppn,add_size=0;
	unsigned int i=0,j,k;
	int map_entry_new,map_entry_old,modify;
	int flag=0;
	char buffer_request[200];
	struct local *location;
	int64_t time;

	printf("\n");
	printf("begin pre_process_page.................\n");

	ssd->tracefile=fopen(ssd->tracefilename,"r");
	if(ssd->tracefile == NULL )      /*��trace�ļ����ж�ȡ����*/
	{
		printf("the trace file can't open3\n");
		return NULL;
	}
	if(ssd->parameter->subpage_page == 32){
		full_page = 0xffffffff;
		}
	else{
		full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
		}
	//full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
	/*��������ssd������߼�������*/
	largest_lsn=(unsigned int )((ssd->parameter->chip_num*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->subpage_page)*(1-ssd->parameter->overprovide));

	while(fgets(buffer_request,200,ssd->tracefile))
	{
		//sscanf(buffer_request,"%lld %d %d %d %d %d",&time,&device,&lsn,&size,&ope,&read_flag);
		sscanf(buffer_request,"%lld %d %d %d %d",&time,&device,&lsn,&size,&ope);
		fl++;
		trace_assert(time,device,lsn,size,ope);                         /*���ԣ���������time��device��lsn��size��ope���Ϸ�ʱ�ͻᴦ��*/
		ssd->total_request_num++;
		if(ope==1)
			ssd->total_read_num++;
		add_size=0;                                                     /*add_size����������Ѿ�Ԥ�����Ĵ�С*/
		if(ope==1)                                                      /*����ֻ�Ƕ������Ԥ��������Ҫ��ǰ����Ӧλ�õ���Ϣ������Ӧ�޸�*/
		{
			//ssd->flag_rd=read_flag;
			while(add_size<size)
			{				
				lsn=lsn%largest_lsn;                                    /*��ֹ��õ�lsn������lsn����*/		
				sub_size=ssd->parameter->subpage_page-(lsn%ssd->parameter->subpage_page);		
				if(add_size+sub_size>=size)                             /*ֻ�е�һ������Ĵ�СС��һ��page�Ĵ�Сʱ�����Ǵ���һ����������һ��pageʱ������������*/
				{		
					sub_size=size-add_size;		
					add_size+=sub_size;		
				}

				if((sub_size>ssd->parameter->subpage_page)||(add_size>size))/*��Ԥ����һ���Ӵ�Сʱ�������С����һ��page�����Ѿ������Ĵ�С����size�ͱ���*/		
				{		
					printf("pre_process sub_size:%d\n",sub_size);		
				}

				lpn=lsn/ssd->parameter->subpage_page;
				if(ssd->dram->map->map_entry[lpn].state==0)                 
				{
				
					ppn=get_ppn_for_pre_process(ssd,lsn);                  
					location=find_location(ssd,ppn);
					ssd->program_count++;	
					ssd->channel_head[location->channel].program_count++;
					ssd->channel_head[location->channel].chip_head[location->chip].program_count++;		
					ssd->dram->map->map_entry[lpn].pn=ppn;	
					ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,lsn,sub_size);   //0001
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=lpn;
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=ssd->dram->map->map_entry[lpn].state;
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page);
					
					free(location);
					location=NULL;
				}//if(ssd->dram->map->map_entry[lpn].state==0)
				else if(ssd->dram->map->map_entry[lpn].state>0)           /*״̬��Ϊ0�����*/
				{
					map_entry_new=set_entry_state(ssd,lsn,sub_size);      /*�õ��µ�״̬������ԭ����״̬���ĵ�һ��״̬*/
					map_entry_old=ssd->dram->map->map_entry[lpn].state;
                    modify=map_entry_new|map_entry_old;
					ppn=ssd->dram->map->map_entry[lpn].pn;
					location=find_location(ssd,ppn);

					ssd->program_count++;	
					ssd->channel_head[location->channel].program_count++;
					ssd->channel_head[location->channel].chip_head[location->chip].program_count++;		
					ssd->dram->map->map_entry[lsn/ssd->parameter->subpage_page].state=modify; 
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=modify;
					ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~modify)&full_page);
					
					free(location);
					location=NULL;
				}//else if(ssd->dram->map->map_entry[lpn].state>0)
				lsn=lsn+sub_size;                                         /*�¸����������ʼλ��*/
				add_size+=sub_size;                                       /*�Ѿ������˵�add_size��С�仯*/
			}//while(add_size<size)
		}//if(ope==1) 
	}	

	printf("\n");
	printf("pre_process is complete!\n");

	fclose(ssd->tracefile);

	for(i=0;i<ssd->parameter->channel_number;i++)
    for(j=0;j<ssd->parameter->die_chip;j++)
	for(k=0;k<ssd->parameter->plane_die;k++)
	{
		fprintf(ssd->outputfile,"chip:%d,die:%d,plane:%d have free page: %d\n",i,j,k,ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);				
		fflush(ssd->outputfile);
	}
	
	return ssd;
}
unsigned int get_ppn_for_pre_process(struct ssd_info *ssd,unsigned int lsn)     
{
	unsigned int channel=0,chip=0,die=0,plane=0; 
	unsigned int ppn,lpn;
	unsigned int active_block;
	unsigned int channel_num=0,chip_num=0,die_num=0,plane_num=0;

#ifdef DEBUG
	printf("enter get_psn_for_pre_process\n");
#endif

	channel_num=ssd->parameter->channel_number;
	chip_num=ssd->parameter->chip_channel[0];
	die_num=ssd->parameter->die_chip;
	plane_num=ssd->parameter->plane_die;
	lpn=lsn/ssd->parameter->subpage_page;

	if (ssd->parameter->allocation_scheme==0)                           /*��̬��ʽ�»�ȡppn*/
	{
		if (ssd->parameter->dynamic_allocation==0)                      /*��ʾȫ��̬��ʽ�£�Ҳ����channel��chip��die��plane��block�ȶ��Ƕ�̬����*/
		{
			channel=ssd->token;
			ssd->token=(ssd->token+1)%ssd->parameter->channel_number;
			chip=ssd->channel_head[channel].token;
			ssd->channel_head[channel].token=(chip+1)%ssd->parameter->chip_channel[0];
			die=ssd->channel_head[channel].chip_head[chip].token;
			ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
			plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
		} 
		else if (ssd->parameter->dynamic_allocation==1)                 /*��ʾ�붯̬��ʽ��channel��̬������package��die��plane��̬����*/                 
		{
			channel=lpn%ssd->parameter->channel_number;
			chip=ssd->channel_head[channel].token;
			ssd->channel_head[channel].token=(chip+1)%ssd->parameter->chip_channel[0];
			die=ssd->channel_head[channel].chip_head[chip].token;
			ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
			plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
		}
	} 
	else if (ssd->parameter->allocation_scheme==1)                       /*��ʾ��̬���䣬ͬʱҲ��0,1,2,3,4,5��6�в�ͬ��̬���䷽ʽ*/
	{
		switch (ssd->parameter->static_allocation)
		{
			
			case 0:         
			{
				channel=(lpn/(plane_num*die_num*chip_num))%channel_num;
				chip=lpn%chip_num;
				die=(lpn/chip_num)%die_num;
				plane=(lpn/(die_num*chip_num))%plane_num;
				break;
			}
		case 1:
			{
				channel=lpn%channel_num;
				chip=(lpn/channel_num)%chip_num;
				die=(lpn/(chip_num*channel_num))%die_num;
				plane=(lpn/(die_num*chip_num*channel_num))%plane_num;
							
				break;
			}
		case 2:
			{
				channel=lpn%channel_num;
				chip=(lpn/(plane_num*channel_num))%chip_num;
				die=(lpn/(plane_num*chip_num*channel_num))%die_num;
				plane=(lpn/channel_num)%plane_num;
				break;
			}
		case 3:
			{
				channel=lpn%channel_num;
				chip=(lpn/(die_num*channel_num))%chip_num;
				die=(lpn/channel_num)%die_num;
				plane=(lpn/(die_num*chip_num*channel_num))%plane_num;
				break;
			}
		case 4:  
			{
				channel=lpn%channel_num;
				chip=(lpn/(plane_num*die_num*channel_num))%chip_num;
				die=(lpn/(plane_num*channel_num))%die_num;
				plane=(lpn/channel_num)%plane_num;
							
				break;
			}
		case 5:   
			{
				channel=lpn%channel_num;
				chip=(lpn/(plane_num*die_num*channel_num))%chip_num;
				die=(lpn/channel_num)%die_num;
				plane=(lpn/(die_num*channel_num))%plane_num;
							
				break;
			}
		default : return 0;
		}
	}
    
	// if(ssd->flag_rd == 1){
	// 	printf("flag_rd:%d\n",ssd->flag_rd);
	// 	if(find_active_block_rd(ssd,channel,chip,die,plane)==FAILURE)
	// 	{
	// 		printf("the read operation is expand the capacity of SSD\n");	
	// 		return 0;
	// 	}
	// }else{
		if(find_active_block(ssd,channel,chip,die,plane)==FAILURE)
		{
			printf("the read operation is expand the capacity of SSD\n");	
			return 0;
		}
	// }
	
	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	if(write_page(ssd,channel,chip,die,plane,active_block,&ppn)==ERROR)
	{
		return 0;
	}

	return ppn;
}
int check_plane(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane){
	unsigned int free_page_num;
	unsigned int cumulate_free_page_num=0;
	unsigned int cumulate_free_lsb_num=0;
	unsigned int i;
	struct plane_info candidate_plane;
	candidate_plane = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane];
	for(i=0;i<ssd->parameter->block_plane;i++){
		if(candidate_plane.blk_head[i].free_lsb_num+candidate_plane.blk_head[i].free_csb_num+candidate_plane.blk_head[i].free_msb_num!=candidate_plane.blk_head[i].free_page_num){
			printf("Meta data Wrong, BLOCK LEVEL!!\n");
			return FALSE;
			}
		cumulate_free_page_num+=candidate_plane.blk_head[i].free_page_num;
		cumulate_free_lsb_num+=candidate_plane.blk_head[i].free_lsb_num;
		}
	if(cumulate_free_page_num!=candidate_plane.free_page){
		printf("Meta data Wrong, PLANE LEVEL!!\n");
		if(cumulate_free_page_num>candidate_plane.free_page){
			printf("The data in plane_info is SMALLER than the blocks.\n");
			}
		else{
			printf("The data in plane_info is GREATER than the blocks.\n");
			}
		return FALSE;
		}
	return TRUE;
}
struct ssd_info *get_ppn_lf(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub)
{
	//printf("Turbo mode page allocation.\n");
	int old_ppn=-1;
	unsigned int ppn,lpn,full_page;
	unsigned int active_block;
	unsigned int block;
	unsigned int page,flag=0,flag1=0;
	unsigned int old_state=0,state=0,copy_subpage=0;
	struct local *location;
	struct direct_erase *direct_erase_node,*new_direct_erase,*direct_fast_erase_node,*new_direct_fast_erase;
	struct gc_operation *gc_node;

	unsigned int available_page;
	
	unsigned int i=0,j=0,k=0,l=0,m=0,n=0;
	
#ifdef DEBUG
	printf("enter get_ppn,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif
	if(ssd->parameter->subpage_page == 32){
		full_page = 0xffffffff;
		}
	else{
		full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
		}
	lpn=sub->lpn;

	if(sub->target_page_type==TARGET_LSB){
		available_page = find_active_block_lf(ssd,channel,chip,die,plane);
		}
	else if(sub->target_page_type==TARGET_CSB){
		available_page = find_active_block_csb(ssd, channel, chip, die, plane);
		if(available_page==FAILURE){
			available_page = find_active_block_lf(ssd,channel,chip,die,plane);
			}
		}
	else if(sub->target_page_type==TARGET_MSB){
		available_page = find_active_block_msb(ssd, channel, chip, die, plane);
		if(available_page==FAILURE){
			available_page = find_active_block_lf(ssd,channel,chip,die,plane);
			}
		}
	else{
		printf("WRONG Target page type!!!!\n");
		return ssd;
		}
	if(available_page == SUCCESS_LSB){
		active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_lsb_block;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb += 3;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
		ssd->free_lsb_count--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb;
		sub->allocated_page_type = TARGET_LSB;
		}
	else if(available_page == SUCCESS_CSB){
		active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_csb_block;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb += 3;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
		ssd->free_csb_count--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb;
		sub->allocated_page_type = TARGET_CSB;
		}
	else if(available_page == SUCCESS_MSB){
		active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_msb_block;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb += 3;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->free_msb_count--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb;
		sub->allocated_page_type = TARGET_MSB;
		}
	else{
		printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
		printf("free page number: %d, free lsb page number: %d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page,ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num);
		return ssd;
	}
	ssd->sub_request_all++;
	if(sub->allocated_page_type == sub->target_page_type){
		ssd->sub_request_success++;
		}
	//���ҳ����Ƿ񳬳���Χ
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>=(int)ssd->parameter->page_block){
		printf("ERROR! the last write page larger than the number of pages per block!!\n");
		printf("The active block is %d, ",active_block);
		if(available_page == SUCCESS_LSB){
			printf("targeted LSB page.\n");
			}
		else if(available_page == SUCCESS_CSB){
			printf("targeted CSB page.\n");
			}
		else{
			printf("targeted MSB page.\n");
			}
		for(i=0;i<ssd->parameter->block_plane;i++){
			printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_lsb);
			}
		printf("\n");
		for(i=0;i<ssd->parameter->block_plane;i++){
			printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_csb);
			}
		printf("\n");
		for(i=0;i<ssd->parameter->block_plane;i++){
			printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_msb);
			}
		printf("\n");
		//printf("The page number: %d.\n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);
		while(1){}
		}
	
	//�ɹ�Ѱ�ҵ���Ծ���Լ�����д��Ŀ���page
	block=active_block; 
	page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page; 
	/*
	if(lpn == 40470){
		printf("+++++++++++++lpn:40470, location:%d, %d, %d, %d, %d, %d.\n",channel, chip, die, plane, active_block, page);
		}
	*/
	//�жϸ��߼�ҳ�Ƿ��ж�Ӧ�ľ�����ҳ������о��ҳ���������Ϊ��Чҳ
	if(ssd->dram->map->map_entry[lpn].state == 0){
		if(ssd->dram->map->map_entry[lpn].pn != 0){
			printf("Error in get_ppn_lf()_position_1, pn: %d.\n", ssd->dram->map->map_entry[lpn].pn);
			return NULL;
			}
		ssd->dram->map->map_entry[lpn].pn = find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[lpn].state = sub->state;
		}
	else{
		ppn = ssd->dram->map->map_entry[lpn].pn;
		location = find_location(ssd,ppn);
		if(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn != lpn)
			{
			printf("Error in get_ppn()_position_2\n");
			printf("lpn:%d, ppn:%d, location:%d, %d, %d, %d, %d, %d, ",lpn,ppn,location->channel,location->chip,location->die,location->plane,location->block,location->page);
			printf("the wrong lpn:%d.\n", ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn);
			return NULL;
			}
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;			  /*��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0*/
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;			  /*��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0*/
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
		if((location->page)%3==0){
			ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_lsb_num++;
			}
		/*******************************************************************************************
		*��block��ȫ��invalid��ҳ������ֱ��ɾ�������ڴ���һ���ɲ����Ľڵ㣬����location�µ�plane����
		********************************************************************************************/
		struct blk_info target_block = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block];
		if(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num == ssd->parameter->page_block)	  
			{
			new_direct_erase = (struct direct_erase *)malloc(sizeof(struct direct_erase));
			alloc_assert(new_direct_erase,"new_direct_erase");
			memset(new_direct_erase,0, sizeof(struct direct_erase));
	
			new_direct_erase->block = location->block;
			new_direct_erase->next_node = NULL;
			direct_erase_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
			if(direct_erase_node == NULL){
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
				} 
			else{
				new_direct_erase->next_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node = new_direct_erase;
				}
			}
		/*
		else if((target_block.free_msb_num == ssd->parameter->page_block/2)&&(target_block.invalid_lsb_num == ssd->parameter->page_block/2)){
			//�����ֻ��lsb��д����������lsbҳ������Чҳ
			printf("==>A FAST GC CAN BE TRIGGER.\n");
			new_direct_fast_erase = (struct direct_erase *)malloc(sizeof(struct direct_erase));
			alloc_assert(new_direct_fast_erase,"new_direct_fast_erase");
			memset(new_direct_fast_erase,0, sizeof(struct direct_erase));
	
			new_direct_fast_erase->block = location->block;
			new_direct_fast_erase->next_node = NULL;
			direct_fast_erase_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].fast_erase_node;
			if(direct_fast_erase_node == NULL){
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].fast_erase_node=new_direct_fast_erase;
				} 
			else{
				new_direct_fast_erase->next_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].fast_erase_node;
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].fast_erase_node = new_direct_fast_erase;
				}
			//ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].fast_erase = TRUE;
			printf("====>FAST GC OPT IS INITIALED.\n");
			}
			*/
		free(location);
		location = NULL;
		ssd->dram->map->map_entry[lpn].pn = find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[lpn].state = (ssd->dram->map->map_entry[lpn].state|sub->state);
		}
	
	sub->ppn=ssd->dram->map->map_entry[lpn].pn; 									 /*�޸�sub�������ppn��location�ȱ���*/
	sub->location->channel=channel;
	sub->location->chip=chip;
	sub->location->die=die;
	sub->location->plane=plane;
	sub->location->block=active_block;
	sub->location->page=page;
	
	ssd->program_count++;															/*�޸�ssd��program_count,free_page�ȱ���*/
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	//if(page%2==0){
	//	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
	//	}
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn=lpn;	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].valid_state=sub->state;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].free_state=((~(sub->state))&full_page);
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
	/*
	if(lpn == 40470){
		printf("+++++++++++++lpn:40470, stored lpn:%d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn);
		}
	*/
	ssd->write_flash_count++;
	if(page%3 == 0){
		ssd->write_lsb_count++;
		ssd->newest_write_lsb_count++;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
		//ssd->free_lsb_count--;
		}
	else if(page%3 == 1){
		ssd->write_csb_count++;
		ssd->newest_write_csb_count++;
		}
	else{
		ssd->write_msb_count++;
		ssd->newest_write_msb_count++;
		}
	
	if (ssd->parameter->active_write == 0)											/*���û���������ԣ�ֻ����gc_hard_threshold�������޷��ж�GC����*/
		{																				/*���plane�е�free_page����Ŀ����gc_hard_threshold���趨����ֵ�Ͳ���gc����*/
		if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page < (ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
			{
			//printf("+++++Garbage Collection is NEEDED.\n");
			gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
			alloc_assert(gc_node,"gc_node");
			memset(gc_node,0, sizeof(struct gc_operation));
	
			gc_node->next_node=NULL;
			gc_node->chip=chip;
			gc_node->die=die;
			gc_node->plane=plane;
			gc_node->block=0xffffffff;
			gc_node->page=0;
			gc_node->state=GC_WAIT;
			gc_node->priority=GC_UNINTERRUPT;
			gc_node->next_node=ssd->channel_head[channel].gc_command;
			ssd->channel_head[channel].gc_command=gc_node;
			ssd->gc_request++;
			}
		/*IMPORTANT!!!!!!*/
		/*Fast Collection Function*/
		else if(ssd->parameter->fast_gc == 1){
			if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num < (ssd->parameter->page_block*ssd->parameter->block_plane/2)*ssd->parameter->fast_gc_thr_2){
				//printf("==>A FAST GC CAN BE TRIGGERED, %d,%d,%d,%d,,,%d.\n",channel,chip,die,plane,ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num);
				struct gc_operation *temp_gc_opt;
				int temp_flag=0;
				temp_gc_opt = ssd->channel_head[channel].gc_command;
				while(temp_gc_opt != NULL){
					if(temp_gc_opt->chip==chip&&temp_gc_opt->die==die&&temp_gc_opt->plane==plane){
						temp_flag=1;
						break;
						}
					temp_gc_opt = temp_gc_opt->next_node;
					}
				if(temp_flag!=1){
					gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
					alloc_assert(gc_node,"fast_gc_node");
					memset(gc_node,0, sizeof(struct gc_operation));
			
					gc_node->next_node=NULL;
					gc_node->chip=chip;
					gc_node->die=die;
					gc_node->plane=plane;
					gc_node->block=0xffffffff;
					gc_node->page=0;
					gc_node->state=GC_WAIT;
					gc_node->priority=GC_FAST_UNINTERRUPT_EMERGENCY;
					gc_node->next_node=ssd->channel_head[channel].gc_command;
					ssd->channel_head[channel].gc_command=gc_node;
					ssd->gc_request++;
					}
				}
			else if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num < (ssd->parameter->page_block*ssd->parameter->block_plane/2)*ssd->parameter->fast_gc_thr_1){
				//printf("==>A FAST GC CAN BE TRIGGERED, %d,%d,%d,%d,,,%d.\n",channel,chip,die,plane,ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num);
				struct gc_operation *temp_gc_opt;
				int temp_flag=0;
				temp_gc_opt = ssd->channel_head[channel].gc_command;
				while(temp_gc_opt != NULL){
					if(temp_gc_opt->chip==chip&&temp_gc_opt->die==die&&temp_gc_opt->plane==plane){
						temp_flag=1;
						break;
						}
					temp_gc_opt = temp_gc_opt->next_node;
					}
				if(temp_flag!=1){
					gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
					alloc_assert(gc_node,"fast_gc_node");
					memset(gc_node,0, sizeof(struct gc_operation));
			
					gc_node->next_node=NULL;
					gc_node->chip=chip;
					gc_node->die=die;
					gc_node->plane=plane;
					gc_node->block=0xffffffff;
					gc_node->page=0;
					gc_node->state=GC_WAIT;
					gc_node->priority=GC_FAST_UNINTERRUPT;
					gc_node->next_node=ssd->channel_head[channel].gc_command;
					ssd->channel_head[channel].gc_command=gc_node;
					ssd->gc_request++;
					}
				}
			else if(ssd->request_queue->priority==0){
				//printf("It is IDLE time for garbage collection.\n")
				struct gc_operation *temp_gc_opt;
				int temp_flag=0;
				temp_gc_opt = ssd->channel_head[channel].gc_command;
				/*
				while(temp_gc_opt != NULL){
					//�޸�Ϊÿ��chip�����һ�����գ�����������Ӱ������
					//if(temp_gc_opt->chip==chip&&temp_gc_opt->die==die&&temp_gc_opt->plane==plane){
					if(temp_gc_opt->chip==chip){
						temp_flag=1;
						break;
						}
					temp_gc_opt = temp_gc_opt->next_node;
					}
					*/
				//Ҫ��ÿ�ν���gc��channel������������channel����25%
				unsigned int channel_count,gc_channel_num = 0;
				unsigned int newest_avg_write_lat;
				if(ssd->newest_write_request_count==0){
					newest_avg_write_lat = 0;
					}
				else{
					newest_avg_write_lat = ssd->newest_write_avg/ssd->newest_write_request_count;
					}
				if(newest_avg_write_lat >= 8000000){
				/*
				unsigned int last_ten_write_avg_lat=0;
				unsigned int fgc_i;
				for(fgc_i=0;fgc_i<10;fgc_i++){
					last_ten_write_avg_lat += ssd->last_ten_write_lat[fgc_i];
					}
				last_ten_write_avg_lat = last_ten_write_avg_lat/10;
				if(last_ten_write_avg_lat >= 8000000){
				*/
					temp_flag=1;
					}
				else if(temp_gc_opt == NULL){
					for(channel_count=0;channel_count<ssd->parameter->channel_number;channel_count++){
						if(ssd->channel_head[channel_count].gc_command != NULL){
							gc_channel_num++;
							}
						}
					if(gc_channel_num >= 4){
						temp_flag=1;
						}
					}
				else{
					temp_flag=1;
					}
				if(temp_flag!=1){
					gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
					alloc_assert(gc_node,"fast_gc_node");
					memset(gc_node,0, sizeof(struct gc_operation));
			
					gc_node->next_node=NULL;
					gc_node->chip=chip;
					gc_node->die=die;
					gc_node->plane=plane;
					gc_node->block=0xffffffff;
					gc_node->page=0;
					gc_node->state=GC_WAIT;
					gc_node->priority=GC_FAST_UNINTERRUPT_IDLE;
					gc_node->next_node=ssd->channel_head[channel].gc_command;
					ssd->channel_head[channel].gc_command=gc_node;
					ssd->gc_request++;
					}
				}
			}
		}
	if(check_plane(ssd, channel, chip, die, plane) == FALSE){
		printf("Something Wrong Happened.\n");
		return FAILURE;
		}
	return ssd;
}
unsigned int get_ppn_for_gc_lf(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane) 	
{
	unsigned int ppn;
	unsigned int active_block,block,page;
	int i;
	
	unsigned int available_page;
#ifdef DEBUG
	printf("enter get_ppn_for_gc,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif
	/*
	if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)
	{
		printf("\n\n Error int get_ppn_for_gc().\n");
		return 0xffffffff;
	}
	*/
	//available_page = find_active_block_lf(ssd,channel,chip,die,plane);
	
	available_page = find_active_block_msb(ssd, channel, chip, die, plane);
	if(available_page==FAILURE){
		available_page = find_active_block_lf(ssd,channel,chip,die,plane);
		}
	
	if(available_page == SUCCESS_LSB){
		active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_lsb_block;
		//printf("the last write lsb page: %d.\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb);
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb += 3;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
		ssd->free_lsb_count--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb;
		ssd->write_lsb_count++;
		ssd->newest_write_lsb_count++;
		}
	else if(available_page == SUCCESS_CSB){
		active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_csb_block;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb += 3;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb;
		ssd->write_csb_count++;
		ssd->free_csb_count--;
		ssd->newest_write_csb_count++;
		}
	else if(available_page == SUCCESS_MSB){
		active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_msb_block;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb += 3;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb;
		ssd->write_msb_count++;
		ssd->free_msb_count--;
		ssd->newest_write_msb_count++;
		}
	else{
		printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);	
		while(1){
			}
	}

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>=(int)ssd->parameter->page_block){
		printf("ERROR! the last write page larger than the number of pages per block!! pos 2\n");
		for(i=0;i<ssd->parameter->block_plane;i++){
			printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_lsb);
			}
		printf("\n");
		for(i=0;i<ssd->parameter->block_plane;i++){
			printf("%d, ", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_page);
			}
		printf("\n");
		//printf("The page number: %d.\n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);
		while(1){}
		}

	
	//active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

	//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;	
	//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
	/*
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>ssd->parameter->page_block)
	{
		printf("DDD error! the last write page larger than 64!!\n");
		while(1){}
	}
	*/
	block=active_block; 
	page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page; 

	ppn=find_ppn(ssd,channel,chip,die,plane,block,page);

	ssd->program_count++;
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
	ssd->write_flash_count++;

	return ppn;
}
Status erase_planes(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command)
{
	unsigned int die=0;
	unsigned int plane=0;
	unsigned int block=0;
	struct direct_erase *direct_erase_node=NULL;
	unsigned int block0=0xffffffff;
	unsigned int block1=0;

	if((ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node==NULL)||               
		((command!=INTERLEAVE_TWO_PLANE)&&(command!=INTERLEAVE)&&(command!=TWO_PLANE)&&(command!=NORMAL)))     /*���û�в�������������command���ԣ����ش���*/           
	{
		return ERROR;
	}

	/************************************************************************************************************
	*������������ʱ������Ҫ���Ͳ����������channel��chip���ڴ��������״̬����CHANNEL_TRANSFER��CHIP_ERASE_BUSY
	*��һ״̬��CHANNEL_IDLE��CHIP_IDLE��
	*************************************************************************************************************/
	block1=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node->block;
	
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;	

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

	if(command==INTERLEAVE_TWO_PLANE)                                       /*�߼�����INTERLEAVE_TWO_PLANE�Ĵ���*/
	{
		for(die=0;die<ssd->parameter->die_chip;die++)
		{
			block0=0xffffffff;
			if(die==die1)
			{
				block0=block1;
			}
			for (plane=0;plane<ssd->parameter->plane_die;plane++)
			{
				direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
				if(direct_erase_node!=NULL)
				{
					
					block=direct_erase_node->block; 
					
					if(block0==0xffffffff)
					{
						block0=block;
					}
					else
					{
						if(block!=block0)
						{
							continue;
						}

					}
					ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=direct_erase_node->next_node;
					erase_operation(ssd,channel,chip,die,plane,block);     /*��ʵ�Ĳ��������Ĵ���*/
					free(direct_erase_node);                               
					direct_erase_node=NULL;
					ssd->direct_erase_count++;
				}

			}
		}

		ssd->interleave_mplane_erase_count++;                             /*������һ��interleave two plane erase����,���������������ʱ�䣬�Լ���һ��״̬��ʱ��*/
		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+18*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tWB;       
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time-9*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tBERS;

	}
	else if(command==INTERLEAVE)                                          /*�߼�����INTERLEAVE�Ĵ���*/
	{
		for(die=0;die<ssd->parameter->die_chip;die++)
		{
			for (plane=0;plane<ssd->parameter->plane_die;plane++)
			{
				if(die==die1)
				{
					plane=plane1;
				}
				direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
				if(direct_erase_node!=NULL)
				{
					block=direct_erase_node->block;
					ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=direct_erase_node->next_node;
					erase_operation(ssd,channel,chip,die,plane,block);
					free(direct_erase_node);
					direct_erase_node=NULL;
					ssd->direct_erase_count++;
					break;
				}	
			}
		}
		ssd->interleave_erase_count++;
		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;       
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
	}
	else if(command==TWO_PLANE)                                          /*�߼�����TWO_PLANE�Ĵ���*/
	{

		for(plane=0;plane<ssd->parameter->plane_die;plane++)
		{
			direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node;
			if((direct_erase_node!=NULL))
			{
				block=direct_erase_node->block;
				if(block==block1)
				{
					ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node=direct_erase_node->next_node;
					erase_operation(ssd,channel,chip,die1,plane,block);
					free(direct_erase_node);
					direct_erase_node=NULL;
					ssd->direct_erase_count++;
				}
			}
		}

		ssd->mplane_erase_conut++;
		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;      
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
	}
	else if(command==NORMAL)                                             /*��ͨ����NORMAL�Ĵ���*/
	{
		direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;
		block=direct_erase_node->block;
		if(direct_erase_node->rr_flag==1){
			ssd->rr_erase_count++;
		}
		ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node=direct_erase_node->next_node;
		free(direct_erase_node);
		direct_erase_node=NULL;
		

		erase_operation(ssd,channel,chip,die1,plane1,block);

		ssd->direct_erase_count++;
		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;       								
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tWB+ssd->parameter->time_characteristics.tBERS;	
	}
	else
	{
		return ERROR;
	}

	direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;

	if(((direct_erase_node)!=NULL)&&(direct_erase_node->block==block1))
	{
		return FAILURE; 
	}
	else
	{
		return SUCCESS;
	}
}
Status fast_erase_planes(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command)
{
	unsigned int die=0;
	unsigned int plane=0;
	unsigned int block=0;
	struct direct_erase *direct_erase_node=NULL;
	unsigned int block0=0xffffffff;
	unsigned int block1=-1;
	/*
	if((ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node==NULL)|| (command!=NORMAL))
	{
		return ERROR;
	}
	*/
	/************************************************************************************************************
	*������������ʱ������Ҫ���Ͳ����������channel��chip���ڴ��������״̬����CHANNEL_TRANSFER��CHIP_ERASE_BUSY
	*��һ״̬��CHANNEL_IDLE��CHIP_IDLE��
	*************************************************************************************************************/
	struct plane_info candidate_plane;
	candidate_plane = ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1];
	while(block < ssd->parameter->block_plane)
		{
		if((candidate_plane.blk_head[block].free_msb_num==ssd->parameter->page_block/2)&&(candidate_plane.blk_head[block].invalid_lsb_num==ssd->parameter->page_block/2)&&(candidate_plane.blk_head[block].fast_erase!=TRUE))
			{
			block1 = block;
			break;
			}
		block++;
		}
	//block1=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node->block;
	if(block1==-1){
		return FAILURE;
		}
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;	

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

	
	if(command==NORMAL)                                             /*��ͨ����NORMAL�Ĵ���*/
	{
		//direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node;
		//block=direct_erase_node->block;
		//ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node=direct_erase_node->next_node;
		//free(direct_erase_node);
		//direct_erase_node=NULL;
		candidate_plane.blk_head[block1].fast_erase = TRUE;
		erase_operation(ssd,channel,chip,die1,plane1,block1);

		ssd->fast_gc_count++;
		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;       								
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tWB+ssd->parameter->time_characteristics.tBERS;
		return SUCCESS;
	}
	else
	{
		printf("Wrong Commend for fast gc.\n");
		return ERROR;
	}
	/*
	direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].fast_erase_node;

	if(((direct_erase_node)!=NULL)&&(direct_erase_node->block==block1))
	{
		return FAILURE; 
	}
	else
	{
		return SUCCESS;
	}
	*/
}
int gc_direct_erase(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)     
{
	unsigned int lv_die=0,lv_plane=0;                                                          
	unsigned int interleaver_flag=FALSE,muilt_plane_flag=FALSE;
	unsigned int normal_erase_flag=TRUE;

	struct direct_erase * direct_erase_node1=NULL;
	struct direct_erase * direct_erase_node2=NULL;

	direct_erase_node1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
	if (direct_erase_node1==NULL)
	{
		return FAILURE;
	}
    
	if((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)
	{	
		for(lv_plane=0;lv_plane<ssd->parameter->plane_die;lv_plane++)
		{
			direct_erase_node2=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
			if((lv_plane!=plane)&&(direct_erase_node2!=NULL))
			{
				if((direct_erase_node1->block)==(direct_erase_node2->block))
				{
					muilt_plane_flag=TRUE;
					break;
				}
			}
		}
	}

	if((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)
	{
		for(lv_die=0;lv_die<ssd->parameter->die_chip;lv_die++)
		{
			if(lv_die!=die)
			{
				for(lv_plane=0;lv_plane<ssd->parameter->plane_die;lv_plane++)
				{
					if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node!=NULL)
					{
						interleaver_flag=TRUE;
						break;
					}
				}
			}
			if(interleaver_flag==TRUE)
			{
				break;
			}
		}
	}
    
	if ((muilt_plane_flag==TRUE)&&(interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))     
	{
		if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE_TWO_PLANE)==SUCCESS)
		{
			return SUCCESS;
		}
	} 
	else if ((interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
	{
		if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE)==SUCCESS)
		{
			return SUCCESS;
		}
	}
	else if ((muilt_plane_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
	{
		if(erase_planes(ssd,channel,chip,die,plane,TWO_PLANE)==SUCCESS)
		{
			return SUCCESS;
		}
	}

	if ((normal_erase_flag==TRUE))                              /*����ÿ��plane���п���ֱ��ɾ����block��ֻ�Ե�ǰplane������ͨ��erase����������ֻ��ִ����ͨ����*/
	{
		if (erase_planes(ssd,channel,chip,die,plane,NORMAL)==SUCCESS)
		{
			return SUCCESS;
		} 
		else
		{
			return FAILURE;                                     /*Ŀ���planeû�п���ֱ��ɾ����block����ҪѰ��Ŀ����������ʵʩ��������*/
		}
	}
	return SUCCESS;
}
int gc_direct_fast_erase(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)     
{
	unsigned int lv_die=0,lv_plane=0;                                                           /*Ϊ����������ʹ�õľֲ����� Local variables*/
	unsigned int interleaver_flag=FALSE,muilt_plane_flag=FALSE;
	unsigned int normal_erase_flag=TRUE;

	struct direct_erase * direct_erase_node1=NULL;
	struct direct_erase * direct_erase_node2=NULL;

	/*
	direct_erase_node1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].fast_erase_node;
	if (direct_erase_node1==NULL)
	{
		return FAILURE;
	}
    */
	/********************************************************************************************************
	*���ܴ���TWOPLANE�߼�����ʱ��������Ӧ��channel��chip��die��������ͬ��plane�ҵ�����ִ��TWOPLANE������block
	*����muilt_plane_flagΪTRUE
	*********************************************************************************************************/
	if((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)
	{	
		printf("Advanced commands are NOT supported.\n");
		return FAILURE;
	}

	/***************************************************************************************
	*���ܴ���INTERLEAVE�߼�����ʱ��������Ӧ��channel��chip�ҵ�����ִ��INTERLEAVE������block
	*����interleaver_flagΪTRUE
	****************************************************************************************/
	if((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)
	{
		printf("Advanced commands are NOT supported.\n");
		return FAILURE;
	}
    
	/************************************************************************************************************************
	*A������ȿ���ִ��twoplane������block���п���ִ��interleaver������block����ô��ִ��INTERLEAVE_TWO_PLANE�ĸ߼������������
	*B�����ֻ����ִ��interleaver������block����ô��ִ��INTERLEAVE�߼�����Ĳ�������
	*C�����ֻ����ִ��TWO_PLANE������block����ô��ִ��TWO_PLANE�߼�����Ĳ�������
	*D��û��������Щ�������ô��ֻ�ܹ�ִ����ͨ�Ĳ���������
	*************************************************************************************************************************/
	/*
	if ((muilt_plane_flag==TRUE)&&(interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))     
	{
		if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE_TWO_PLANE)==SUCCESS)
		{
			return SUCCESS;
		}
	} 
	else if ((interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
	{
		if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE)==SUCCESS)
		{
			return SUCCESS;
		}
	}
	else if ((muilt_plane_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
	{
		if(erase_planes(ssd,channel,chip,die,plane,TWO_PLANE)==SUCCESS)
		{
			return SUCCESS;
		}
	}
	*/

	if ((normal_erase_flag==TRUE))                              /*����ÿ��plane���п���ֱ��ɾ����block��ֻ�Ե�ǰplane������ͨ��erase����������ֻ��ִ����ͨ����*/
	{
		if (fast_erase_planes(ssd,channel,chip,die,plane,NORMAL)==SUCCESS)
		{
			return SUCCESS;
		} 
		else
		{
			return FAILURE;                                     /*Ŀ���planeû�п���ֱ��ɾ����block����ҪѰ��Ŀ����������ʵʩ��������*/
		}
	}
	else{
		printf("Wrong command for fast gc.\n");
		return FAILURE;
		}
	//return SUCCESS;
}
int uninterrupt_fast_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int priority)       
{
	//printf("===>Enter uninterrupt_fast_gc.\n");
	unsigned int i=0,invalid_page=0;
	unsigned int block,block1,active_block,transfer_size,free_page,page_move_count=0;                           /*��¼ʧЧҳ���Ŀ��*/
	struct local *  location=NULL;
	unsigned int total_invalid_page_num=0;
	
	block1 = -1;
	block = 0;
	struct plane_info candidate_plane;
	candidate_plane = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane];
	unsigned int invalid_lsb_num = 0;
	while(block < ssd->parameter->block_plane-1){
		//printf("Enter while, %d\n", block);
		if((candidate_plane.blk_head[block].free_msb_num==ssd->parameter->page_block/2)&&(candidate_plane.blk_head[block].fast_erase!=TRUE)){
			if(candidate_plane.blk_head[block].free_lsb_num==0 && candidate_plane.blk_head[block].invalid_lsb_num>invalid_lsb_num){
				block1 = block;
				invalid_lsb_num = candidate_plane.blk_head[block].invalid_lsb_num;
				}
			}
		block++;
		}
	if (block1==-1)
	{
		//printf("====>No block has invalid LSB page??\n");
		return SUCCESS;
	}
	if(priority==GC_FAST_UNINTERRUPT_EMERGENCY){
		if(candidate_plane.blk_head[block1].invalid_lsb_num < ssd->parameter->page_block/2*ssd->parameter->fast_gc_filter_2){
			return SUCCESS;
			}
		}
	else if(priority==GC_FAST_UNINTERRUPT){
		if(candidate_plane.blk_head[block1].invalid_lsb_num < ssd->parameter->page_block/2*ssd->parameter->fast_gc_filter_1){
			return SUCCESS;
			}
		}
	else if(priority==GC_FAST_UNINTERRUPT_IDLE){
		if(candidate_plane.blk_head[block1].invalid_lsb_num < ssd->parameter->page_block/2*ssd->parameter->fast_gc_filter_idle){
			return SUCCESS;
			}
		//printf("FAST_GC_IDLE on %d, %d, %d, %d, %d.\n", channel, chip, die, plane, block1);
		ssd->idle_fast_gc_count++;
		}
	else{
		printf("GC_FAST ERROR. PARAMETERS WRONG.\n");
		return SUCCESS;
		}
	
	free_page=0;
	struct blk_info candidate_block;
	candidate_block = candidate_plane.blk_head[block1];
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block1].fast_erase = TRUE;
	//printf("%d %d %d %d %d is selected, free_msbs:%d, invalid_lsbs:%d.\n",channel,chip,die,plane,block1, candidate_block.free_msb_num, candidate_block.invalid_lsb_num);
	//printf("begin to move pages.\n");
	//printf("<<<Fast GC Candidate, %d, %d, %d, %d, %d.\n",channel,chip,die,plane,block1);
	//printf("Begin to MOVE PAGES.\n");
	for(i=0;i<ssd->parameter->page_block;i++)		                                                     /*������ÿ��page���������Ч���ݵ�page��Ҫ�ƶ��������ط��洢*/		
	{		
		if(candidate_block.page_head[i].valid_state>0) /*��ҳ����Чҳ����Ҫcopyback����*/		
		{	
			//printf("move page %d.\n",i);
			location=(struct local * )malloc(sizeof(struct local ));
			alloc_assert(location,"location");
			memset(location,0, sizeof(struct local));

			location->channel=channel;
			location->chip=chip;
			location->die=die;
			location->plane=plane;
			location->block=block1;
			location->page=i;
			//printf("state of this block: %d.\n",candidate_block.fast_erase);
			move_page(ssd, location, &transfer_size,1);                                                   /*��ʵ��move_page����*/
			page_move_count++;

			free(location);	
			location=NULL;
		}				
	}
	//printf("block: %d, %d, %d, %d, %d going to be erased.\n",channel, chip, die, plane, block1);
	erase_operation(ssd,channel ,chip , die, plane ,block1);	                                              /*ִ����move_page�����󣬾�����ִ��block�Ĳ�������*/
	//printf("ERASE OPT: %d, %d, %d, %d, %d\n",channel,chip,die,plane,block1);
	ssd->channel_head[channel].current_state=CHANNEL_GC;									
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;			

	ssd->fast_gc_count++;
	
	/***************************************************************
	*�ڿ�ִ��COPYBACK�߼������벻��ִ��COPYBACK�߼���������������£�
	*channel�¸�״̬ʱ��ļ��㣬�Լ�chip�¸�״̬ʱ��ļ���
	***************************************************************/
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

		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);					
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;

	}
	//printf("===>Exit uninterrupt_fast_gc Successfully.\n");
	return 1;
}
int uninterrupt_dr(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)       
{
	unsigned int i=0,invalid_page=0;
	unsigned int block,active_block,transfer_size,free_page,page_move_count=0;                           /*��¼ʧЧҳ���Ŀ��*/
	struct local *  location=NULL;
	unsigned int total_invalid_page_num=0;
	/*
	if(find_active_block_dr(ssd,channel,chip,die,plane)!=SUCCESS)
	{
		printf("\n\n Error in uninterrupt_dr(). No active block\n");
		return ERROR;
	}
	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	*/
	invalid_page=0;
	transfer_size=0;
	block=-1;
	for(i=0;i<ssd->parameter->block_plane;i++)                                                           /*�������invalid_page�Ŀ�ţ��Լ�����invalid_page_num*/
	{	
		total_invalid_page_num+=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page)						
		{				
			invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
			block=i;						
		}
	}
	
	if (block==-1)
	{
		printf("No block is selected.\n");
		return 1;
	}
	//��Ŀ��block��dr_state��Ϊ���
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].dr_state=DR_STATE_OUTPUT;

	
	//printf("Block %d with %d invalid pages is selected.\n", block, ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num);
	free_page=0;
	for(i=0;i<ssd->parameter->page_block;i++)		                                                     /*������ÿ��page���������Ч���ݵ�page��Ҫ�ƶ��������ط��洢*/		
	{		
		if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state&PG_SUB)==0x0000000f)
		{
			free_page++;
		}
		/*
		if(free_page!=0)
		{
			//printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t\n",free_page,channel,chip,die,plane);
		}
		*/
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) /*��ҳ����Чҳ����Ҫcopyback����*/		
		{	
			location=(struct local * )malloc(sizeof(struct local ));
			alloc_assert(location,"location");
			memset(location,0, sizeof(struct local));

			location->channel=channel;
			location->chip=chip;
			location->die=die;
			location->plane=plane;
			location->block=block;
			location->page=i;
			if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].dr_state != DR_STATE_OUTPUT){
				printf("The DR_STATE is not settle.\n");
				}
			move_page(ssd, location, &transfer_size,1);                                                   /*��ʵ��move_page����*/
			page_move_count++;

			free(location);	
			location=NULL;
		}				
	}
	erase_operation(ssd,channel ,chip , die,plane ,block);	                                              /*ִ����move_page�����󣬾�����ִ��block�Ĳ�������*/
	/*
	ssd->channel_head[channel].current_state=CHANNEL_GC;									
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;			
	*/
	/***************************************************************
	*�ڿ�ִ��COPYBACK�߼������벻��ִ��COPYBACK�߼���������������£�
	*channel�¸�״̬ʱ��ļ��㣬�Լ�chip�¸�״̬ʱ��ļ���
	***************************************************************/
	/*
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

		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);					
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;

	}
	*/
	return 1;
}
Status find_invalid_block(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane){
	unsigned int block, flag;
	for(block=0;block<ssd->parameter->block_plane;block++){
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num>(ssd->parameter->page_block*ssd->parameter->dr_filter)){
			return TRUE;
			}
		}
	return FALSE;
}
Status dr_for_channel(struct ssd_info *ssd, unsigned int channel)
{
	int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
	unsigned int chip,die,plane,flag_priority=0;
	unsigned int current_state=0, next_state=0;
	long long next_state_predict_time=0;
	struct gc_operation *gc_node=NULL,*gc_p=NULL;

	/*******************************************************************************************
	*����ÿһ��gc_node����ȡgc_node���ڵ�chip�ĵ�ǰ״̬���¸�״̬���¸�״̬��Ԥ��ʱ��
	*�����ǰ״̬�ǿ��У������¸�״̬�ǿ��ж��¸�״̬��Ԥ��ʱ��С�ڵ�ǰʱ�䣬�����ǲ����жϵ�gc
	*��ô��flag_priority��Ϊ1������Ϊ0
	********************************************************************************************/
	//======================================================================
	//�����������ղ����������
	ssd->channel_head[channel].gc_command=NULL;
	gc_node=ssd->channel_head[channel].gc_command;
	//��ѯ����plane��������Чҳ��block�����յ�
	unsigned int plane_flag;
	unsigned int counter;
	for(chip=0;chip<ssd->parameter->chip_channel[0];chip++){
		for(die=0;die<ssd->parameter->die_chip;die++){
			for(plane=0;plane<ssd->parameter->plane_die;plane++){
				plane_flag = find_invalid_block(ssd, channel, chip, die, plane);
				counter=0;
				while(plane_flag==TRUE){
					counter++;
					//printf("There is data should be reorganize in %d, %d, %d, %d.\n",channel, chip, die, plane);
					uninterrupt_dr(ssd, channel, chip, die, plane);
					plane_flag = find_invalid_block(ssd, channel, chip, die, plane);
					}
				printf("SUCCESS. Data in %d, %d, %d, %d is reorganized.\n",channel, chip, die, plane);
				}
			}
    	}
	return SUCCESS;
}
int decide_gc_invoke(struct ssd_info *ssd, unsigned int channel)      
{
	struct sub_request *sub;
	struct local *location;

	if ((ssd->channel_head[channel].subs_r_head==NULL)&&(ssd->channel_head[channel].subs_w_head==NULL))    /*������Ҷ�д�������Ƿ���Ҫռ�����channel�����õĻ�����ִ��GC����*/
	{
		return 1;                                                                        /*��ʾ��ǰʱ�����channelû����������Ҫռ��channel*/
	}
	else
	{
		if (ssd->channel_head[channel].subs_w_head!=NULL)
		{
			return 0;
		}
		else if (ssd->channel_head[channel].subs_r_head!=NULL)
		{
			sub=ssd->channel_head[channel].subs_r_head;
			while (sub!=NULL)
			{
				if (sub->current_state==SR_WAIT)                                         /*����������Ǵ��ڵȴ�״̬���������Ŀ��die����idle������ִ��gc����������0*/
				{
					location=find_location(ssd,sub->ppn);
					if ((ssd->channel_head[location->channel].chip_head[location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[location->channel].chip_head[location->chip].next_state==CHIP_IDLE)&&
						(ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)))
					{
						free(location);
						location=NULL;
						return 0;
					}
					free(location);
					location=NULL;
				}
				else if (sub->next_state==SR_R_DATA_TRANSFER)
				{
					location=find_location(ssd,sub->ppn);
					if (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)
					{
						free(location);
						location=NULL;
						return 0;
					}
					free(location);
					location=NULL;
				}
				sub=sub->next_node;
			}
		}
		return 1;
	}
}
int interrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct gc_operation *gc_node)        
{
	unsigned int i,block,active_block,transfer_size,invalid_page=0;
	struct local *location;

	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	transfer_size=0;
	if (gc_node->block>=ssd->parameter->block_plane)
	{
		for(i=0;i<ssd->parameter->block_plane;i++)
		{			
			if((active_block!=i)&&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))						
			{				
				invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
				block=i;						
			}
		}
		gc_node->block=block;
	}

	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num!=ssd->parameter->page_block)     /*����Ҫִ��copyback����*/
	{
		for (i=gc_node->page;i<ssd->parameter->page_block;i++)
		{
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].page_head[i].valid_state>0)
			{
				location=(struct local * )malloc(sizeof(struct local ));
				alloc_assert(location,"location");
				memset(location,0, sizeof(struct local));

				location->channel=channel;
				location->chip=chip;
				location->die=die;
				location->plane=plane;
				location->block=block;
				location->page=i;
				transfer_size=0;

				move_page( ssd, location, &transfer_size,1);

				free(location);
				location=NULL;

				gc_node->page=i+1;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num++;
				ssd->channel_head[channel].current_state=CHANNEL_C_A_TRANSFER;									
				ssd->channel_head[channel].current_time=ssd->current_time;										
				ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
				ssd->channel_head[channel].chip_head[chip].current_state=CHIP_COPYBACK_BUSY;								
				ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
				ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;		

				if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
				{					
					ssd->channel_head[channel].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC;		
					ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
				} 
				else
				{	
					ssd->channel_head[channel].next_state_predict_time=ssd->current_time+(7+transfer_size*SECTOR)*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(7+transfer_size*SECTOR)*ssd->parameter->time_characteristics.tWC;					
					ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
				}
				return 0;    
			}
		}
	}
	else
	{
		erase_operation(ssd,channel ,chip, die,plane,gc_node->block);	

		ssd->channel_head[channel].current_state=CHANNEL_C_A_TRANSFER;									
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;								
		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;							
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;

		return 1;                                                                      /*��gc������ɣ�����1�����Խ�channel�ϵ�gc����ڵ�ɾ��*/
	}

	printf("there is a problem in interrupt_gc\n");
	return 1;
}












int action_for_PM(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block)       
{
	unsigned int i=0,invalid_page=0;
	unsigned int transfer_size,free_page,page_move_count=0;                           /*��¼ʧЧҳ���Ŀ��*/
	struct local *  location=NULL;
	unsigned int total_invalid_page_num=0;
	unsigned char channelChangFlag = 0;
	unsigned char dieChangFlag = 0;
	int64_t channelpre;
	int64_t diepre;
	struct direct_erase *direct_erase_node,*new_direct_erase;
	struct gc_operation *gc_node;

	/*if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)                                           /
	{
		printf("\n\n Error in uninterrupt_gc().\n");
		return ERROR;
	}
	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;*/
	invalid_page=0;
	transfer_size=0;

		
	// (this normal erase)
	//ssd->normal_erase++;
	/*for(i=0;i<ssd->parameter->block_plane;i++)                                                           /
	{	
		total_invalid_page_num+=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].free_page_num > 0){
			continue;
		}
		if((active_block!=i)&&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))						
		{				
			invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
			block=i;						
		}
	}

	if (block==-1)
	{
		return 1;
	}

	//if(invalid_page<5)
	//{
		//printf("\ntoo less invalid page. \t %d\t %d\t%d\t%d\t%d\t%d\t\n",invalid_page,channel,chip,die,plane,block);
	//}
	*/
	//do erase
	// if(ssd->now_erase_flag==1){
	// 	//***which block***
	// 	ssd->erase_flag=0;
	// 	ssd->now_erase_flag=0;

	// 	//ssd->erase_channel=channel;
	// 	//ssd->erase_chip=chip;
	// 	//ssd->erase_die=die;
	// 	//ssd->erase_plane=plane;
	// 	//ssd->erase_block=block;
		
	// 	if(dieChangFlag)
	// 		ssd->channel_head[ssd->erase_channel].chip_head[ssd->erase_chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.tBERS;
	// 	else 
	// 		ssd->channel_head[ssd->erase_channel].chip_head[ssd->erase_chip].next_state_predict_time += ssd->parameter->time_characteristics.tBERS;
	// 	erase_operation(ssd,ssd->erase_channel ,ssd->erase_chip , ssd->erase_die,ssd->erase_plane ,ssd->erase_block);
	// 	for(i=0;i<ssd->parameter->page_block;i++){
	// 		ssd->channel_head[ssd->erase_channel].chip_head[ssd->erase_chip].die_head[ssd->erase_die].plane_head[ssd->erase_plane].blk_head[ssd->erase_block].page_head[i].read_count=0;
	// 	}
	// 	ssd->rr_erase_count++;
	// 	printf("erase_count:%d, rr_erase_count:%d\n",ssd->erase_count,ssd->rr_erase_count);
	// 	return 1;
	// }

	int partial_page_move_count=0;
	int max_read_count=0;
	unsigned int max_pagenum=-1;
	int valid_count=0;
	int erase_flag = 0;
	while(partial_page_move_count<ssd->patial_rr_number){
		max_read_count=-1;
		max_pagenum=-1;
		valid_count=0;
		//从目标块中找到一个达到最大读取次数（阈值）的有效页
		for(i=0;i<ssd->parameter->page_block;i++)		                                                  	
		{		
			if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) {
				//printf("read_count:%d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count);
				if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count>max_read_count){
					//printf("read_count:%d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count);
					max_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count;
					max_pagenum = i;
					
				}
				valid_count+=1;
			}
		}
		//printf("max_pagenum:%d\n",max_pagenum);
		if(max_pagenum==-1&&valid_count!=0){
			printf("error in find max read page,partial_page_move_count:%d, ssd->patial_rr_number:%d, valid_count:%d\n",partial_page_move_count,ssd->patial_rr_number,valid_count);
			while(1){}
		}
		if(valid_count!=0){
			location=(struct local * )malloc(sizeof(struct local ));
			alloc_assert(location,"location");
			memset(location,0, sizeof(struct local));

			location->channel=channel;
			location->chip=chip;
			location->die=die;
			location->plane=plane;
			location->block=block;
			location->page=max_pagenum;
			move_page_for_rr(ssd, location, &transfer_size);
			page_move_count++;
			ssd->rr_page_move_count++;
			ssd->page_move_count++;
			ssd->partial_page_move_count++;

			free(location);	
			location=NULL;
			partial_page_move_count++;
		}else{//have no valid page,break;
			partial_page_move_count=ssd->patial_rr_number;
			//erase_operation(ssd,channel,chip,die,plane,block);
			erase_flag=1;
		}
		
	}
	
	
	
/*
	int partial_page_move_count=0;
	free_page=0;
	for(i=0;i<ssd->parameter->page_block;i++)		                                                  	
	{		
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) 	
		{	
			//printf("vaild!!!\n");
			location=(struct local * )malloc(sizeof(struct local ));
			alloc_assert(location,"location");
			memset(location,0, sizeof(struct local));

			location->channel=channel;
			location->chip=chip;
			location->die=die;
			location->plane=plane;
			location->block=block;
			location->page=i;
			move_page(ssd, location, &transfer_size);
			page_move_count++;
			ssd->rr_page_move_count++;
			ssd->partial_page_move_count++;

			free(location);	
			location=NULL;
			partial_page_move_count++;
			if(partial_page_move_count>=ssd->patial_rr_number)
				break;
		}				
	}*/
	//printf("page_move_count:%d,partial_page_move_count:%d\n",page_move_count,partial_page_move_count);
	
	/*ssd->channel_head[channel].current_state=CHANNEL_GC;									
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;*/			
	
	/***************************************************************
	*�ڿ�ִ��COPYBACK�߼������벻��ִ��COPYBACK�߼���������������£�
	*channel�¸�״̬ʱ��ļ��㣬�Լ�chip�¸�״̬ʱ��ļ���
	***************************************************************/
	//在channel 和对应chip空闲的时候才能执行擦除，以下为判断channel和chip的状态
	if((ssd->channel_head[channel].current_state==CHANNEL_IDLE)||(ssd->channel_head[channel].next_state==CHANNEL_IDLE&&ssd->channel_head[channel].next_state_predict_time<=ssd->current_time)){
		ssd->channel_head[channel].current_state=CHANNEL_GC;									
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
		channelChangFlag = 1;
	}
	
	if((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time))){
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
		diepre = channelpre;//废话
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
		//ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);	
		//ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time;
	}
	//以下代码为再次检查这个块的有效页，有效页在经过前面的页面迁移之后，原块上的页会被置为无效，如果原快上面没有有效页了就可以将原块擦除。	
	valid_count=0;
	for(i=0;i<ssd->parameter->page_block;i++)		                                                  	
		{		
			if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) {
				//printf("read_count:%d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count);
				// if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count>=max_read_count){
				// 	//printf("read_count:%d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count);
				// 	max_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count;
				// 	max_pagenum = i;
					
				// }
				valid_count+=1;
			}
		}
	
	//check erase node
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num==ssd->parameter->page_block||valid_count==0)    
	{
		// ssd->erase_flag=1;
		// ssd->erase_channel=channel;
		// ssd->erase_chip=chip;
		// ssd->erase_die=die;
		// ssd->erase_plane=plane;
		// ssd->erase_block=block;
		// printf("erase_node: %d, %d, %d, %d, %d\n",ssd->erase_channel, ssd->erase_chip, ssd->erase_die, ssd->erase_plane, ssd->erase_block);
		// printf("valid_count:%d\n",valid_count);
		
		if(dieChangFlag)
			ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->current_time + channelpre+ssd->parameter->time_characteristics.tBERS;
		else 
			ssd->channel_head[channel].chip_head[chip].next_state_predict_time += ssd->parameter->time_characteristics.tBERS;
		erase_operation(ssd,channel ,chip , die,plane ,block);
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].read_count = 0;
		for(i=0;i<ssd->parameter->page_block;i++){
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count=0;
		}
		ssd->rr_erase_count++;
	}

	return 1;
}
unsigned int get_ppn_for_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block_old,unsigned int page_old,unsigned int flag)     
{
	unsigned int ppn;
	unsigned int active_block,block,page;
	unsigned int lpn=0;
	struct data_group *hot_node,*r_node,*new_node,*pt;
	unsigned int ope;

	lpn=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block_old].page_head[page_old].lpn;

#ifdef DEBUG
	printf("enter get_ppn_for_gc,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif

	//先找B再找A
	// int result=lRUCacheGet(ssd->h_r_data,lpn);
	// if(result ==-1){
	// 	result=lRUCacheGet(ssd->r_data,lpn);
	// 	if(result ==-1){
	// 		lRUCachePut(ssd->h_r_data,lpn,lpn);
	// 	}
	// }
	
	int result=lRUCacheGet(ssd->r_data,lpn);

	if(result==-1)  //未找到
	{
		lRUCachePut(ssd->r_data,lpn,lpn);
		if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)
		{
			printf("ERROR in finding hot active block!");
			return 0xffffffff;
		}
		active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	}
	else
	{
		if(lRUCacheGet(ssd->w_data,lpn)==-1){
			if(find_hot_active_block(ssd,channel,chip,die,plane,READ)!=SUCCESS)
			{
				printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
				return 0xffffffff;
			}
			active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block;
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block=(active_block+1)%ssd->parameter->block_plane;  //
			if(flag==1){
				ssd->move_gc++;
			}else{
				ssd->move_rr++;
			}
		}
		else{
			if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)
			{
				printf("ERROR in finding hot active block!");
				return 0xffffffff;
			}
			active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
		}

	}

	
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>=(int)ssd->parameter->page_block-1)
	{
		if(result==-1)  //未找到
		{
			lRUCachePut(ssd->r_data,lpn,lpn);
			if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)
			{
				printf("ERROR in finding hot active block!");
				return 0xffffffff;
			}
			active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
		}
		else
		{
			if(lRUCacheGet(ssd->w_data,lpn)==-1){
				if(find_hot_active_block(ssd,channel,chip,die,plane,READ)!=SUCCESS)
				{
					printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
					return 0xffffffff;
				}
				active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block=(active_block+1)%ssd->parameter->block_plane;  //
			}
			else{
				if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)
				{
					printf("ERROR in finding hot active block!");
					return 0xffffffff;
				}
				active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
			}

		}
	}

	//*********************************************
	/*
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb<ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb+3;
		}
	else if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb<ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb+3;
		}
	else{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb+3;
		}
		*/
	//*********************************************

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num == 0){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].full_time=ssd->current_time;
	}

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>(int)ssd->parameter->page_block )
	{
		printf("GC DDD error! the last write page larger than 64!!\n");
		while(1){}
		
	}

	block=active_block;	
	page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;	

	if(page%3==0){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb=page;
		ssd->free_lsb_count--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
		//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->write_lsb_count++;
		ssd->newest_write_lsb_count++;
		}
	else if(page%3==2){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb=page;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
		//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->write_msb_count++;
		ssd->free_msb_count--;
		ssd->newest_write_msb_count++;
		}
	else{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb=page;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
		ssd->write_csb_count++;
		ssd->free_csb_count--;
		ssd->newest_write_csb_count++;
		}

	ppn=find_ppn(ssd,channel,chip,die,plane,block,page);

	ssd->program_count++;
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
	ssd->write_flash_count++;

	return ppn;

}
struct ssd_info *get_ppn(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub)
{
	int old_ppn=-1;
	unsigned int ppn,lpn,full_page;
	unsigned int active_block;
	unsigned int block;
	unsigned int page,flag=0,flag1=0;
	unsigned int old_state=0,state=0,copy_subpage=0;
	struct local *location;
	struct direct_erase *direct_erase_node,*new_direct_erase;
	struct gc_operation *gc_node;
	
	unsigned int lsn;
	unsigned int temp_channel, temp_chip, temp_die,temp_plane, temp_block;

	unsigned int i=0,j=0,k=0,l=0,m=0,n=0;
	float debug = 0;

	struct data_group *hot_node;
	unsigned int ope;

#ifdef DEBUG
	printf("enter get_ppn,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif
	if(ssd->parameter->subpage_page == 32){
		full_page = 0xffffffff;
		}
	else{
		full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
		}
	lpn=sub->lpn;
	ope=sub->operation;
    
	/*************************************************************************************
	*���ú���find_active_block��channel��chip��die��plane�ҵ���Ծblock
	*�����޸����channel��chip��die��plane��active_block�µ�last_write_page��free_page_num
	**************************************************************************************/
	int result=lRUCacheGet(ssd->w_data,sub->lpn);
	if(result == -1)  //未找到
	{
		if(find_active_block(ssd,channel,chip,die,plane)==FAILURE)
		{
			printf("ERROR in finding hot active block!");
			return ssd;
		}
		active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	}
	else
	{
		if(find_hot_active_block(ssd,channel,chip,die,plane,WRITE)==FAILURE)  //写的时候找的
		{
			printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
			return ssd;
		}
		active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block;
	}

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>=(int)ssd->parameter->page_block-1)
	{
		
		if(result==-1){
			find_active_block(ssd,channel,chip,die,plane);
			active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
		}
		else
		{
			find_hot_active_block(ssd,channel,chip,die,plane,WRITE);
			active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block;
		}
	}
	
	//active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num == 0){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].full_time=ssd->current_time;
	}

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>=(int)ssd->parameter->page_block)
	{
		printf("error! the last write page larger than the number of pages per block!!\n");
		while(1){}
	}

	block=active_block;	
	page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;	
	if(page%3==0){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb=page;
		ssd->free_lsb_count--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
		//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->write_lsb_count++;
		ssd->newest_write_lsb_count++;
		//**********************
		sub->allocated_page_type = TARGET_LSB;
		//**********************
		}
	else if(page%3==2){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb=page;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
		//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->write_msb_count++;
		ssd->free_msb_count--;
		ssd->newest_write_msb_count++;
		//**********************
		sub->allocated_page_type = TARGET_MSB;
		//**********************
		}
	else{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb=page;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
		ssd->write_csb_count++;
		ssd->free_csb_count--;
		ssd->newest_write_csb_count++;
		sub->allocated_page_type = TARGET_CSB;
		}

	if(ssd->dram->map->map_entry[lpn].state==0)                                       /*this is the first logical page*/
	{
		if(ssd->dram->map->map_entry[lpn].pn!=0)
		{
			printf("Error in get_ppn()\n");
		}
		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[lpn].state=sub->state;
	}
	else                                                                            /*����߼�ҳ�����˸��£���Ҫ��ԭ����ҳ��ΪʧЧ*/
	{
		ppn=ssd->dram->map->map_entry[lpn].pn;
		location=find_location(ssd,ppn);
		if(	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn!=lpn)
		{
			printf("\nError in get_ppn()\n");
		}

		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;             /*��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0*/
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;              /*��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0*/
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
		if((location->page)%3==0){
			ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_lsb_num++;
			}
		/*******************************************************************************************
		*��block��ȫ��invalid��ҳ������ֱ��ɾ�������ڴ���һ���ɲ����Ľڵ㣬����location�µ�plane����
		********************************************************************************************/
		if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num==ssd->parameter->page_block)    
		{
			new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
            alloc_assert(new_direct_erase,"new_direct_erase");
			memset(new_direct_erase,0, sizeof(struct direct_erase));

			new_direct_erase->block=location->block;
			new_direct_erase->next_node=NULL;
			direct_erase_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
			if (direct_erase_node==NULL)
			{
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
			} 
			else
			{
				new_direct_erase->next_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
			}
		}

		free(location);
		location=NULL;
		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[lpn].state=(ssd->dram->map->map_entry[lpn].state|sub->state);
	}

	
	sub->ppn=ssd->dram->map->map_entry[lpn].pn;                                      /*�޸�sub�������ppn��location�ȱ���*/
	sub->location->channel=channel;
	sub->location->chip=chip;
	sub->location->die=die;
	sub->location->plane=plane;
	sub->location->block=active_block;
	sub->location->page=page;

	ssd->program_count++;                                                           /*�޸�ssd��program_count,free_page�ȱ���*/
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn=lpn;	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].valid_state=sub->state;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].free_state=((~(sub->state))&full_page);
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
	ssd->write_flash_count++;

	if (ssd->parameter->active_write==0)                                            /*���û���������ԣ�ֻ����gc_hard_threshold�������޷��ж�GC����*/
	{                                                                               /*���plane�е�free_page����Ŀ����gc_hard_threshold���趨����ֵ�Ͳ���gc����*/
		if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
		{
			gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
			alloc_assert(gc_node,"gc_node");
			memset(gc_node,0, sizeof(struct gc_operation));

			gc_node->next_node=NULL;
			gc_node->chip=chip;
			gc_node->die=die;
			gc_node->plane=plane;
			gc_node->block=0xffffffff;
			gc_node->page=0;
			gc_node->state=GC_WAIT;
			gc_node->priority=GC_UNINTERRUPT;
			gc_node->wl_flag=0;
			gc_node->next_node=ssd->channel_head[channel].gc_command;
			ssd->channel_head[channel].gc_command=gc_node;
			ssd->gc_request++;
		}
		
		/***Reset BET****/
		/*if(ssd->table_count >= ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num){
				ssd->table_count = 0;
				ssd->total_erase_count = 0;
				ssd->bet_num = 0;
				for(i = 0 ; i<ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num; i++){
						ssd->BET[i] = 0;
				}
			}
		*/
		//debug = ssd->total_erase_count/ssd->table_count;
		//printf("debug3::wl_value:%f, total_erase_count:%f, table_count:%f, wl_page_move:%d\n", debug, ssd->total_erase_count, ssd->table_count, ssd->wl_page_move_count);
		/***Generate WL request through gc operation when threshold is over the present value***/
		/* if((ssd->table_count != 0) && (ssd->total_erase_count/ssd->table_count >= WL_THRESHOLD) && ((ssd->wl_request+ssd->table_count) < (ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)))
		{
			while(ssd->BET[ssd->bet_num]==1){
				ssd->bet_num= (ssd->bet_num+1)%(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num);
			}

				gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
				alloc_assert(gc_node,"gc_node");
				memset(gc_node,0, sizeof(struct gc_operation));

				gc_node->next_node=NULL;
				gc_node->chip=(ssd->bet_num%(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num))/(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip);
				gc_node->die=(ssd->bet_num%(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)%(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip))/(ssd->parameter->block_plane*ssd->parameter->plane_die);
				gc_node->plane=(ssd->bet_num%(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)%(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip)%(ssd->parameter->block_plane*ssd->parameter->plane_die))/(ssd->parameter->block_plane);
				gc_node->block=(ssd->bet_num%(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)%(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip)%(ssd->parameter->block_plane*ssd->parameter->plane_die)%ssd->parameter->block_plane);
				gc_node->page=0;
				gc_node->state=GC_WAIT;
				gc_node->priority=GC_UNINTERRUPT;        //test
				gc_node->wl_flag = 1;
				temp_channel = ssd->bet_num/(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num);
				gc_node->next_node=ssd->channel_head[temp_channel].gc_command;
				ssd->channel_head[temp_channel].gc_command=gc_node;
				ssd->wl_request++;
				ssd->bet_num= (ssd->bet_num+1)%(ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num);
				
		} */
	} 
/* 	if(check_plane(ssd, channel, chip, die, plane) == FALSE){
		printf("Something Wrong Happened.\n");
		return FAILURE;
		} */
	return ssd;
}

Status erase_operation(struct ssd_info * ssd,unsigned int channel ,unsigned int chip ,unsigned int die ,unsigned int plane ,unsigned int block)
{
	unsigned int flag, i;
	flag = 0;
	int bet_temp;
	for(i=0;i<ssd->parameter->page_block;i++){
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state!=0){
			printf("valid_page:%d\n",i);
			flag = 1;
			}
		}
	if(flag==1){
		printf("Erasing a block with valid data: %d, %d, %d, %d, %d.\n",channel, chip, die, plane, block);
		printf("erase_node: %d, %d, %d, %d, %d.\n",ssd->erase_channel, ssd->erase_chip, ssd->erase_die, ssd->erase_plane, ssd->erase_block);
		while(1);
		return FAILURE;
		}

	unsigned int origin_free_page_num, origin_free_lsb_num, origin_free_csb_num, origin_free_msb_num;
	origin_free_lsb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_lsb_num;
	origin_free_csb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_csb_num;
	origin_free_msb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_msb_num;
	/* if(origin_free_lsb_num+origin_free_csb_num+origin_free_msb_num!=origin_free_page_num){
		printf("WRONG METADATA in BLOCK LEVEL.(erase_operation)\n");
		} */
	//printf("ERASE_OPERATION: %d, %d, %d, %d, %d.\n",channel,chip,die,plane,block);
	//unsigned int i=0;

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].id == 1){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count--;//
	}
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count < 0){
		printf("erase %u,%u,%u,%u,%u",channel,chip,die,plane,block);
		while(1);
	}
	//清空该块
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num=ssd->parameter->page_block;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=-1;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].id=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].read_page_count=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].write_page_count=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].upgrade_page_count=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].full_time=0;
	//*****************************************************
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_lsb=-3;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_csb=-2;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_msb=-1;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_lsb_num=(int)(ssd->parameter->page_block/3);
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_csb_num=(int)(ssd->parameter->page_block/3);
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_msb_num=(int)(ssd->parameter->page_block/3);
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_lsb_num=0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase=FALSE;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].dr_state=DR_STATE_NULL;
	//*****************************************************
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].read_count = 0;  //111
	for (i=0;i<ssd->parameter->page_block;i++)
	{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state=PG_SUB;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state=0;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].lpn=-1;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].read_count = 0;  //111
	}
	ssd->erase_count++;
	ssd->channel_head[channel].erase_count++;			
	ssd->channel_head[channel].chip_head[chip].erase_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page+=ssd->parameter->page_block;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page-=origin_free_page_num;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num+=(ssd->parameter->page_block/3);
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num-=origin_free_lsb_num;
	ssd->free_lsb_count+=(ssd->parameter->page_block/3);
	ssd->free_csb_count+=(ssd->parameter->page_block/3);
	ssd->free_msb_count+=(ssd->parameter->page_block/3);
	ssd->free_lsb_count-=origin_free_lsb_num;
	ssd->free_csb_count-=origin_free_csb_num;
	ssd->free_msb_count-=origin_free_msb_num;
	
	bet_temp = (channel*ssd->parameter->chip_num/ssd->parameter->channel_number*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane+chip*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane+die*ssd->parameter->plane_die*ssd->parameter->block_plane+plane*ssd->parameter->block_plane+block);
	
	ssd->total_erase_count++;

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].rr_plane==plane&&ssd->channel_head[channel].chip_head[chip].die_head[die].rr_block==block){
		ssd->channel_head[channel].chip_head[chip].die_head[die].rr_plane=-1;
		ssd->channel_head[channel].chip_head[chip].die_head[die].rr_block=-1;
		ssd->rr_request--;
	}
	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].soft_flag=0;
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count1 != NULL)
	{
		free(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count1);
	}
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count2 != NULL)
	{
		free(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count2);
	}
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count3 != NULL)
	{

		free(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count3);
	}
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].migrate_page_num != NULL)
	{

		free(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].migrate_page_num);
	}
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count != NULL)
	{
		free(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count);

	}
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count = NULL;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].migrate_page_num = NULL;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_migrate_record = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].migrate_block_ppn = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count1 = NULL;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count2 = NULL;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count3 = NULL;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_record1 = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_record2 = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_record3 = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].Is_migrate = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_flag1 = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_flag21 = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_flag22 = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_flag23 = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_flag31 = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_flag32 = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_flag33 = 0;
	// if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].Is_migrate == 1)
	// {
	// 	//printf("当前擦除的地址 %d %d %d %d %d",channel,chip,die,plane,block);
	// 	int temp_ppn = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].migrate_block_ppn;
	// 	struct local *location = find_location(ssd,temp_ppn);
	// 	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].Is_be_choosen =0;
	// 	free(location);
	// 	location =NULL;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].migrate_page_num = NULL;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_migrate_record = 0;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].migrate_block_ppn = 0;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count1 = NULL;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count2 = NULL;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].freq_count3 = NULL;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_record1 = 0;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_record2 = 0;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_record3 = 0;
	// 	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].Is_migrate = 0;
	// }

	/*if(ssd->BET[bet_temp] == 0){
				ssd->BET[bet_temp] = 1;
				ssd->table_count++;
				//ssd->wl_erase_count++;
				//printf("debug::wl_value:%d, total_erase_count:%d, table_count:%d\n", ssd->total_erase_count/ssd->table_count, ssd->total_erase_count, ssd->table_count);
	}*/

	/* 
	if(ssd->UBT[bet_temp] == 1){
				ssd->UBT[bet_temp] = 0;
				ssd->rr_erase_count++;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].read_count = 0;
				printf("debug_rr: rr_erase_count:%d, erase_count:%d\n", ssd->rr_erase_count, ssd->erase_count);
				//printf("debug::wl_value:%d, total_erase_count:%d, table_count:%d\n", ssd->total_erase_count/ssd->table_count, ssd->total_erase_count, ssd->table_count);
	} */

	return SUCCESS;

}
Status move_page(struct ssd_info * ssd, struct local *location, unsigned int * transfer_size, unsigned int flag)//1forgc,0forRR
{
	struct local *new_location=NULL;
	unsigned int free_state=0,valid_state=0;
	unsigned int lpn=0,old_ppn=0,ppn=0;
	//printf("state of this block: %d\n",ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].fast_erase);

	lpn=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn;
	valid_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state;
	free_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state;
	old_ppn=find_ppn(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page);      /*��¼�����Ч�ƶ�ҳ��ppn���Ա�map���߶���ӳ���ϵ�е�ppn������ɾ�������Ӳ���*/
	ppn=get_ppn_for_gc(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page,flag);                /*�ҳ�����ppnһ�����ڷ���gc������plane��,����ʹ��copyback������Ϊgc������ȡppn*/

	new_location=find_location(ssd,ppn);                                                                   /*�����»�õ�ppn��ȡnew_location*/
	//printf("MOVE PAGE, lpn:%d, old_ppn:%d, new_ppn:%d, FROM %d,%d,%d,%d,%d,%d TO %d,%d,%d,%d,%d,%d.\n",lpn,old_ppn,ppn,location->channel,location->chip,location->die,location->plane,location->block,location->page,new_location->channel,new_location->chip,new_location->die,new_location->plane,new_location->block,new_location->page);
	/*
	if(new_location->channel==location->channel && new_location->chip==location->chip && new_location->die==location->die && new_location->plane==location->plane && new_location->block==location->block){
		printf("MOVE PAGE WRONG!!! Page is moved to the same block.\n");
		return FAILURE;
		}
	*/
	while(new_location->block == location->block){
		ssd->same_block_flag = 1;
		ppn=get_ppn_for_gc(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page,flag);                /*�ҳ�����ppnһ�����ڷ���gc������plane��,����ʹ��copyback������Ϊgc������ȡppn*/
		new_location=find_location(ssd,ppn);  
		ssd->same_block_flag = 0;
	}

	if(new_location->block == location->block){
		printf("Data is moved to the same block!!!!!!\n");
		while(1){};
		return FAILURE;
	}

	if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
	{
		printf("Wrong COMMANDS.\n");
		return FAILURE;
		if (ssd->parameter->greed_CB_ad==1)                                                                /*̰����ʹ�ø߼�����*/
		{
			ssd->copy_back_count++;
			ssd->gc_copy_back++;
			while (old_ppn%2!=ppn%2)
			{
				ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=0;
				ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=0;
				ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=0;
				ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].invalid_page_num++;
				
				free(new_location);
				new_location=NULL;

				ppn=get_ppn_for_gc(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page,flag);    /*�ҳ�����ppnһ�����ڷ���gc������plane�У�����������ż��ַ����,����ʹ��copyback����*/
				ssd->program_count--;
				ssd->write_flash_count--;
				ssd->waste_page_count++;
			}
			if(new_location==NULL)
			{
				new_location=find_location(ssd,ppn);
			}
			
			ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
			ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
			ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;
		} 
		else
		{
			if (old_ppn%2!=ppn%2)
			{
				(* transfer_size)+=size(valid_state);
			}
			else
			{

				ssd->copy_back_count++;
				ssd->gc_copy_back++;
			}
		}	
	} 
	else
	{
		(* transfer_size)+=size(valid_state);
	}

	//更新新页状态
	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;

	//将旧页置为无效
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
	if((location->page)%3==0){
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_lsb_num++;
		}
	
	//修改映射表
	if (old_ppn==ssd->dram->map->map_entry[lpn].pn)                                                     
	{
		ssd->dram->map->map_entry[lpn].pn=ppn;
	}
	ssd->moved_page_count++;
	
	free(new_location);
	new_location=NULL;

	return SUCCESS;
}


struct gc_blk{
	int blk_no;
	int invalid_num;
	unsigned int rc;
	int64_t full_time;  //写满的时间
	int64_t n;
};


void swap(struct gc_blk *blk1,struct gc_blk *blk2)
{
	struct gc_blk tmp;
	tmp.blk_no=blk1->blk_no;
	tmp.invalid_num=blk1->invalid_num;
	tmp.rc=blk1->rc;
	tmp.full_time=blk1->full_time;
	tmp.n=blk1->n;

	blk1->blk_no=blk2->blk_no;
	blk1->invalid_num=blk2->invalid_num;
	blk1->rc=blk2->rc;
	blk1->full_time=blk2->full_time;
	blk1->n=blk2->n;

	blk2->blk_no=tmp.blk_no;
	blk2->invalid_num=tmp.invalid_num;
	blk2->rc=tmp.rc;
	blk2->full_time=tmp.full_time;
	blk2->n=tmp.n;
}


void heapify(struct gc_blk arr[], int n, int i) {
    int smallest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    // 找到左右孩子中较小的
    if (left < n && arr[left].invalid_num < arr[smallest].invalid_num) {
        smallest = left;
    }

    if (right < n && arr[right].invalid_num < arr[smallest].invalid_num) {
        smallest = right;
    }

    // 如果最小值不是根节点，进行交换，并且继续对子树进行调整
    if (smallest != i) {
        swap(&arr[i], &arr[smallest]);
        heapify(arr, n, smallest);
    }
}

void insert2heap(struct gc_blk heap[],int *heapsize,struct gc_blk element)
{
	int i;
	if(*heapsize < 5){
		heap[*heapsize]=element;
		(*heapsize)++;
		for(i=*heapsize/2-1;i>=0;i--){
			heapify(heap,*heapsize,i);
		}
	}else if(element.invalid_num>heap[0].invalid_num){
		heap[0]=element;
		heapify(heap,*heapsize,0);
	}
}

void heapSort(struct gc_blk arr[], int n) {
    // 逐次移除最小元素，将其与最后一个元素交换，然后重新调整堆
    for (int i = n - 1; i > 0; i--) {
        // 交换当前堆顶和最后一个元素
        swap(&arr[0], &arr[i]);

        // 重新调整堆的大小，排除已排序的元素
        heapify(arr, i, 0);
    }
}

int uninterrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct gc_operation *gc_node)       
{
	
	unsigned int i=0,invalid_page=0;
	unsigned int block,active_block,transfer_size,free_page,page_move_count=0;                           /*��¼ʧЧҳ���Ŀ��*/
	struct local *  location=NULL;
	unsigned int total_invalid_page_num=0;
	int id=0;
	static int windows = 200;
	static int64_t t_GC=0;  //平均gc的间隔时间
	static int64_t gc_timelist[200];
	static int64_t last_gc_time=0;
	static unsigned int index=0;

	 if(last_gc_time == 0){
        last_gc_time = ssd->current_time;
    } else{
        if(index < windows){
            gc_timelist[index] = ssd->current_time-last_gc_time;
            index++;
            last_gc_time = ssd->current_time;
        }else{
            unsigned int sum = 0;
            for(int i = 0;i<windows;i++){
                sum+=gc_timelist[i];
            }
            t_GC = sum/windows;  //
            index = 0;
        }
    }

	if(find_active_block(ssd,channel,chip,die,plane)!=SUCCESS)                                           /*��ȡ��Ծ��*/
	{
		printf("\n\n Error in uninterrupt_gc().\n");
		return ERROR;
	}
	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	invalid_page=0;
	transfer_size=0;
	block=-1;

	if(gc_node->wl_flag == 2){
		block = gc_node->block;
	}else if(gc_node->wl_flag == 1){
		block = gc_node->block;
		ssd->wl_erase_count++;
	}else{
		ssd->normal_erase++;
		//normal
		// for(i=0;i<ssd->parameter->block_plane;i++)                                                           
		// {	
		// 	total_invalid_page_num+=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
		// 	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].free_page_num > 0){
		// 		continue;
		// 	}
		// 	if((active_block!=i)&&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))						
		// 	{				
		// 		invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
		// 		block=i;						
		// 	}
		// }

		struct gc_blk rank[rank_size];
		struct gc_blk top[10];
		int rank_pointer=0;
		int heap_size=0;
		double t_remain=0;  //当前到下次做RR的间隔时间
		if(t_GC >0){
			for(int k=0;k<ssd->parameter->block_plane;k++){
				if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[k].free_page_num > 0 || ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[k].read_count==0){
					continue;
				}
				rank[rank_pointer].blk_no=k;
				rank[rank_pointer].invalid_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[k].invalid_page_num;
				rank[rank_pointer].rc=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[k].read_count;
				rank[rank_pointer].full_time=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[k].full_time;
				t_remain=(double)(((BRT - rank[rank_pointer].rc)/rank[rank_pointer].rc)*(ssd->current_time - rank[rank_pointer].full_time));
				rank[rank_pointer].n=t_remain/t_GC;
				if(rank[rank_pointer].n==0){
					rank[rank_pointer].n=9999;
				}
				rank_pointer++;
			}
			for(int i=0;i<rank_pointer;i++){
				insert2heap(top,&heap_size,rank[i]);   //将rank数组进行topc插top数组(heap)
			}
			heapSort(top,heap_size); //sort top array with invalid page this only random top-c
			// for (int i = 0; i < 5; i++) {
    		// 	printf("top[%d].n: %d,invalid num: %d\n", i, top[i].n,top[i].invalid_num);
			// }

			int hit_flag=0;
			for(i=0;i<heap_size;i++){
				//printf("i: %d, top[i].n: %d\n", i, top[i].n);
				if(top[i].n < i+1){  //说明RR会先于GC前要提前它,i表示gc的排名从1开始，n应该大于0
					hit_flag=1;
					//printf("Condition met at i: %d, hit_flag set to 1. Exiting loop.\n", i);
					break;
				}
			}			

			if(hit_flag==1){
				block =top[i].blk_no;
				ssd->gc_sel++;
			}else{
				for(i=0;i<ssd->parameter->block_plane;i++)                                                           
				{	
					total_invalid_page_num+=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
					if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].free_page_num > 0){
						continue;
					}
					if((active_block!=i)&&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))						
					{				
						invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
						block=i;						
					}
				}
			}
		}
		else{
			for(i=0;i<ssd->parameter->block_plane;i++)                                                           
			{	
				total_invalid_page_num+=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
				if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].free_page_num > 0){
					continue;
				}
				if((active_block!=i)&&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num>invalid_page))						
				{				
					invalid_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
					block=i;						
				}
			}

		}

	}



	if (block==-1)
	{
		return 1;
	}


	//if(invalid_page<5)
	//{
		//printf("\ntoo less invalid page. \t %d\t %d\t%d\t%d\t%d\t%d\t\n",invalid_page,channel,chip,die,plane,block);
	//}
	//***********************************************
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].fast_erase = TRUE;
	//***********************************************
	free_page=0;
	for(i=0;i<ssd->parameter->page_block;i++)		                                                     /*������ÿ��page���������Ч���ݵ�page��Ҫ�ƶ��������ط��洢*/		
	{		
		if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state&PG_SUB)==0x0000000f)
		{
			free_page++;
		}
		if(free_page!=0)
		{
			//printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t\n",free_page,channel,chip,die,plane);
		}
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state>0) //move page		
		{	
			location=(struct local * )malloc(sizeof(struct local ));
			alloc_assert(location,"location");
			memset(location,0, sizeof(struct local));

			location->channel=channel;
			location->chip=chip;
			location->die=die;
			location->plane=plane;
			location->block=block;
			location->page=i;
			move_page(ssd, location, &transfer_size,1);                                                   /*��ʵ��move_page����*/
			page_move_count++;
			ssd->page_move_count++;
			/*if(gc_node->wl_flag == 0){
				ssd->page_move_count++;
			}else if(gc_node->wl_flag == 1){
				ssd->wl_page_move_count++;
			}*/

			free(location);	
			location=NULL;
		}				
	}
	

	erase_operation(ssd,channel ,chip , die,plane ,block);	                                              /*ִ����move_page�����󣬾�����ִ��block�Ĳ�������*/

	ssd->channel_head[channel].current_state=CHANNEL_GC;									
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;			
	
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

		ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
		//printf("transfer_size:%f\n",transfer_size);	
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;

	}

	return 1;
}






int delete_gc_node(struct ssd_info *ssd, unsigned int channel,struct gc_operation *gc_node)
{
	struct gc_operation *gc_pre=NULL;
	if(gc_node==NULL)                                                                  
	{
		return ERROR;
	}

	if (gc_node==ssd->channel_head[channel].gc_command)
	{
		ssd->channel_head[channel].gc_command=gc_node->next_node;
	}
	else
	{
		gc_pre=ssd->channel_head[channel].gc_command;
		while (gc_pre->next_node!=NULL)
		{
			if (gc_pre->next_node==gc_node)
			{
				gc_pre->next_node=gc_node->next_node;
				break;
			}
			gc_pre=gc_pre->next_node;
		}
	}
	free(gc_node);
	gc_node=NULL;
	ssd->gc_request--;
	return SUCCESS;
}
Status gc_for_channel(struct ssd_info *ssd, unsigned int channel)
{
	int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
	unsigned int chip,die,plane,flag_priority=0;
	unsigned int current_state=0, next_state=0;
	long long next_state_predict_time=0;
	struct gc_operation *gc_node=NULL,*gc_p=NULL;
	gc_node=ssd->channel_head[channel].gc_command;
	while (gc_node!=NULL)
	{
		current_state=ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
		next_state=ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
		next_state_predict_time=ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
		if((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time)))
		{
			if (gc_node->priority==GC_UNINTERRUPT)                                     /*���gc�����ǲ����жϵģ����ȷ������gc����*/
			{
				flag_priority=1;
				break;
			}
		}
		gc_node=gc_node->next_node;
	}
	if (flag_priority!=1)                                                              /*û���ҵ������жϵ�gc��������ִ�ж��׵�gc����*/
	{
		gc_node=ssd->channel_head[channel].gc_command;
		while (gc_node!=NULL)
		{
			current_state=ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
			next_state=ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
			next_state_predict_time=ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
			if((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time)))   
			{
				break;
			}
			gc_node=gc_node->next_node;
		}

	}
	if(gc_node==NULL)
	{
		return FAILURE;
	}

	chip=gc_node->chip;
	die=gc_node->die;
	plane=gc_node->plane;

	if (gc_node->priority==GC_UNINTERRUPT)
	{
		flag_direct_erase=gc_direct_erase(ssd,channel,chip,die,plane);
		if (flag_direct_erase!=SUCCESS)
		{
			flag_gc=uninterrupt_gc(ssd,channel,chip,die,plane,gc_node);                         /*��һ��������gc�������ʱ���Ѿ�����һ���飬������һ��������flash�ռ䣩������1����channel����Ӧ��gc��������ڵ�ɾ��*/
			if (flag_gc==1)
			{
				delete_gc_node(ssd,channel,gc_node);
			}
		}
		else
		{
			delete_gc_node(ssd,channel,gc_node);
		}
		return SUCCESS;
	}
	else if(gc_node->priority==GC_FAST_UNINTERRUPT || gc_node->priority==GC_FAST_UNINTERRUPT_EMERGENCY || gc_node->priority==GC_FAST_UNINTERRUPT_IDLE){
		//printf("===>GC_FAST on %d,%d,%d,%d begin.\n",channel,chip,die,plane);
		flag_direct_erase=gc_direct_fast_erase(ssd,channel,chip,die,plane);
		if (flag_direct_erase!=SUCCESS)
		{
			/*
			printf("Something Weird happened.\n");
			return FAILURE;
			*/
			//printf("NO BLOCK CAN BE ERASED DIRECTLY.\n");
			flag_gc=uninterrupt_fast_gc(ssd,channel,chip,die,plane,gc_node->priority);
			if (flag_gc==1)
			{
				delete_gc_node(ssd,channel,gc_node);
			}
		}
		else
		{
			//printf("THERE IS BLOCK CAN BE ERASED DIRECTLY.\n");
			delete_gc_node(ssd,channel,gc_node);
		}
		//printf("===>GC_FAST on %d,%d,%d,%d successed.\n",channel,chip,die,plane);
		return SUCCESS;
		}
	else        
	{
		flag_invoke_gc=decide_gc_invoke(ssd,channel);                                  /*�ж��Ƿ�����������Ҫchannel���������������Ҫ���channel����ô���gc�����ͱ��ж���*/

		if (flag_invoke_gc==1)
		{
			flag_direct_erase=gc_direct_erase(ssd,channel,chip,die,plane);
			if (flag_direct_erase==-1)
			{
				flag_gc=interrupt_gc(ssd,channel,chip,die,plane,gc_node);             /*��һ��������gc�������ʱ���Ѿ�����һ���飬������һ��������flash�ռ䣩������1����channel����Ӧ��gc��������ڵ�ɾ��*/
				if (flag_gc==1)
				{
					delete_gc_node(ssd,channel,gc_node);
				}
			}
			else if (flag_direct_erase==1)
			{
				delete_gc_node(ssd,channel,gc_node);
			}
			return SUCCESS;
		} 
		else
		{
			return FAILURE;
		}		
	}
}
Status move_page_for_rr(struct ssd_info * ssd, struct local *location, unsigned int * transfer_size)
{
	struct local *new_location=NULL;
	unsigned int free_state=0,valid_state=0;
	unsigned int lpn=0,old_ppn=0,ppn=0;
	//printf("state of this block: %d\n",ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].fast_erase);
	lpn=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn;
	valid_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state;
	free_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state;
	old_ppn=find_ppn(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page);      
	if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].Is_migrate == 0)
	{
		int invalid_page_num = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num;
		int free_page_num = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].free_page_num;
		int valid_page_num = ssd->parameter->page_block - invalid_page_num - free_page_num;
		//printf("\n当前有效页的数量:%d\n",valid_page_num);
		ppn=get_ppn_for_rr(ssd,location->channel,location->chip,location->die,location->plane,valid_page_num); 
		new_location=find_location(ssd,ppn); 
		if(new_location->block == location->block)
		{
			if (new_location->block == ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].active_block)
			{
				ssd->same_block_flag = 1;
				//printf("\nnew_block1 : %d , old_block1 : %d\n",new_location->block, location->block);
			}
			else if (new_location->block == ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].active_block2)
			{
				ssd->same_block_flag2 = 1;
				//printf("\nnew_block2 : %d , old_block2 : %d\n",new_location->block, location->block);
			}
			//printf("\n找到重复的块\n");
			//exit(0);
			ppn=get_ppn_for_rr(ssd,location->channel,location->chip,location->die,location->plane,valid_page_num);
			free(new_location);
			new_location = NULL;    
			new_location=find_location(ssd,ppn);
			//printf("\n更新后的new_block2 : %d , old_block2 : %d\n",ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].active_block2, location->block);
			//printf("\n更新后的new_block1 : %d , old_block1 : %d\n",ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].active_block, location->block);  
			ssd->same_block_flag = 0;
			ssd->same_block_flag2 = 0;
		}
		if(new_location->block == location->block)
		{
			printf("Data is moved to the same block!!!!!!\n");
			//exit(0);
			return FAILURE;
		}
		// ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].Is_migrate=1;
		// ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].migrate_block_ppn = ppn;
		// ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].Is_be_choosen=1;
	}
	// else
	// {
	// 	//这里的ppn是为了找到目标块，而不是目标页
	// 	int invalid_page_num = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num;
	// 	int free_page_num = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].free_page_num;
	// 	int valid_page_num = ssd->parameter->page_block - invalid_page_num - free_page_num+1;
	// 	ppn = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].migrate_block_ppn;
	// 	new_location=find_location(ssd,ppn);
	// 	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].last_write_page++;	
	// 	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].free_page_num--;
	// 	if (valid_page_num > ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].free_page_num)
	// 	{
	// 		printf("\n当前块的有效页数量:%d\n",valid_page_num );
	// 		printf("\n当前迁移块的容量%d\n", ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].free_page_num);
	// 		exit(0);
	// 	}

	// 	if(ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].last_write_page
	// 	> ssd->parameter->page_block)
	// 	{
	// 		printf("rrr error! the last write page larger than 64!!\n");
	// 		while(1){}
			
	// 	}	
	// 	int page=ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].last_write_page;	
	// 	if(page%3==0)
	// 	{
	// 		ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].last_write_lsb=page;
	// 		ssd->free_lsb_count--;
	// 		ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].free_lsb_num--;
	// 		//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
	// 		ssd->write_lsb_count++;
	// 		ssd->newest_write_lsb_count++;
	// 	}
	// 	else if(page%3==2)
	// 	{
	// 		ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].last_write_msb=page;
	// 		ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].free_msb_num--;
	// 		//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
	// 		ssd->write_msb_count++;
	// 		ssd->free_msb_count--;
	// 		ssd->newest_write_msb_count++;
	// 	}
	// 	else
	// 	{
	// 		ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].last_write_csb=page;
	// 		ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].free_csb_num--;
	// 		ssd->write_csb_count++;
	// 		ssd->free_csb_count--;
	// 		ssd->newest_write_csb_count++;
	// 	}
	// 	ssd->program_count++;
	// 	ssd->channel_head[new_location->channel].program_count++;
	// 	ssd->channel_head[new_location->channel].chip_head[new_location->chip].program_count++;
	// 	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].free_page--;
	// 	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[page].written_count++;
	// 	ssd->write_flash_count++;
	// 	new_location->page = page;
	// 	ppn = find_ppn(ssd,new_location->channel,new_location->chip,new_location->die,new_location->plane,new_location->block,page);
	// }

	if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
	{
		printf("Wrong COMMANDS.\n");
		return FAILURE;
		if (ssd->parameter->greed_CB_ad==1)                                                               
		{
			ssd->copy_back_count++;
			ssd->gc_copy_back++;
			while (old_ppn%2!=ppn%2)
			{
				ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=0;
				ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=0;
				ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=0;
				ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].invalid_page_num++;
				free(new_location);
				new_location=NULL;
				ppn=get_ppn_for_gc(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page,0);    /*�ҳ�����ppnһ�����ڷ���gc������plane�У�����������ż��ַ����,����ʹ��copyback����*/
				ssd->program_count--;
				ssd->write_flash_count--;
				ssd->waste_page_count++;
			}
			if(new_location==NULL)
			{
				new_location=find_location(ssd,ppn);
			}
			
			ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
			ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
			ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;
		} 
		else
		{
			if (old_ppn%2!=ppn%2)
			{
				(* transfer_size)+=size(valid_state);
			}
			else
			{

				ssd->copy_back_count++;
				ssd->gc_copy_back++;
			}
		}	
	} 
	else
	{
		(* transfer_size)+=size(valid_state);
	}
	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
	ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;

	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;
	ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
	if((location->page)%3==0)
	{
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_lsb_num++;
	}
	if (old_ppn==ssd->dram->map->map_entry[lpn].pn)                                                     
	{
		ssd->dram->map->map_entry[lpn].pn=ppn;
	}
	ssd->moved_page_count++;
	free(new_location);
	new_location=NULL;
	return SUCCESS;
}
unsigned int get_ppn_for_rr(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,int valid_num)  
{
	if(ssd->parameter->turbo_mode != 0)
	{
		return get_ppn_for_gc_lf(ssd, channel, chip, die, plane);
	}
	unsigned int ppn;
	unsigned int active_block2,block,page,active_block1,active_block;
	#ifdef DEBUG
		printf("enter get_ppn_for_gc,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
	#endif
	if(find_active_block_rr(ssd,channel,chip,die,plane,valid_num)!=SUCCESS)
	{
		printf("\n\n Error int get_ppn_for_rr().\n");
		return 0xffffffff;
	}

	active_block2=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block2;
	active_block1= ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].read_count < erase_phase1)
	{
		active_block = active_block2;
	}
	else if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].read_count >= erase_phase1 &&ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].read_count < erase_phase1 )
	{
		active_block = active_block1;
	}
	else if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].read_count >= erase_phase1 &&ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].read_count >= erase_phase1)
	{
		int erase_count1 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].erase_count;
		int erase_count2 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].erase_count;
		if (erase_count1 > erase_count2) active_block = active_block2;
		else if (erase_count1 < erase_count2) active_block = active_block1;
		else active_block = active_block2;
	}
	//printf("\n当前active_block的lastpage = %d \n", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page >= (int)ssd->parameter->page_block-1)
	{
		//printf("\n执行第二次find\n");
		
		if(find_active_block_rr(ssd,channel,chip,die,plane,valid_num)!=SUCCESS)
		{
			printf("\n\n Error int get_ppn_for_rr().\n");
			return 0xffffffff;
		}
		active_block2=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block2;
		active_block1= ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].read_count < erase_phase1)
		{
			active_block = active_block2;
		}
		else if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].read_count >= erase_phase1 &&ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].read_count < erase_phase1 )
		{
			active_block = active_block1;
		}
		else if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].read_count >= erase_phase1 &&ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].read_count >= erase_phase1)
		{
			int erase_count1 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].erase_count;
			int erase_count2 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].erase_count;
			if (erase_count1 > erase_count2) active_block = active_block2;
			else if (erase_count1 < erase_count2) active_block = active_block1;
			else active_block = active_block2;
		}	
	}

	//*********************************************
	/*
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb<ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb+3;
		}
	else if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb<ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb+3;
		}
	else{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb+3;
		}
		*/
	//*********************************************

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>(int)ssd->parameter->page_block)
	{
		int page_count = 0;
		printf("当前plane下的空闲页数:%d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page);
		for(int i = 0 ; i < ssd->parameter->block_plane ; i++)
		{
			
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].last_write_page < ssd->parameter->page_block-1)
			{
				page_count++;
			}
		}
		printf("当前plane下的空闲块的数量为:%d\n",page_count);
		printf("RR DDD error! the last write page larger than 64!!\n");
		while(1){}
		
	}

	block=active_block;	
	page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;	

	if(page%3==0){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb=page;
		ssd->free_lsb_count--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
		//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->write_lsb_count++;
		ssd->newest_write_lsb_count++;
		}
	else if(page%3==2){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb=page;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
		//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
		ssd->write_msb_count++;
		ssd->free_msb_count--;
		ssd->newest_write_msb_count++;
		}
	else{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb=page;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
		ssd->write_csb_count++;
		ssd->free_csb_count--;
		ssd->newest_write_csb_count++;
		}

	ppn=find_ppn(ssd,channel,chip,die,plane,block,page);

	ssd->program_count++;
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
	ssd->write_flash_count++;
	return ppn;
}
unsigned int gc(struct ssd_info *ssd,unsigned int channel, unsigned int flag)
{
	unsigned int i;
	int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
	unsigned int flag_priority=0;
	struct gc_operation *gc_node=NULL,*gc_p=NULL;

	if (flag==1)                                                                       /*����ssd����IDEL�����*/
	{
		for (i=0;i<ssd->parameter->channel_number;i++)
		{
			flag_priority=0;
			flag_direct_erase=1;
			flag_gc=1;
			flag_invoke_gc=1;
			gc_node=NULL;
			gc_p=NULL;
			if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))
			{
				channel=i;
				if (ssd->channel_head[channel].gc_command!=NULL)
				{
					gc_for_channel(ssd, channel);
				}
			}
		}
		return SUCCESS;

	} 
	else                                                                               /*ֻ�����ĳ���ض���channel��chip��die����gc����Ĳ���(ֻ���Ŀ��die�����ж������ǲ���idle��*/
	{
		if ((ssd->parameter->allocation_scheme==1)||((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==1)))
		{
			if ((ssd->channel_head[channel].subs_r_head!=NULL)||(ssd->channel_head[channel].subs_w_head!=NULL))    /*�������������ȷ�������*/
			{
				return 0;
			}
		}

		gc_for_channel(ssd,channel);
		return SUCCESS;
	}
}