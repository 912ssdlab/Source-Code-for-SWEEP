#include "ssd.h"
#include "flash.h"
extern int index1; 
extern int index2;
extern int RRcount;
extern char aveber;

Status  find_active_block_lf(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
	unsigned int active_lsb_block;
	unsigned int free_lsb_num=0;
	unsigned int last_write_lsb = 0;
	unsigned int fast_erase, dr_state;
	unsigned int count=0;
	unsigned int temp_block;

	struct plane_info candidate_plane = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane];

	active_lsb_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_lsb_block;
	/*
	free_lsb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_lsb_block].free_lsb_num;
	fast_erase = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_lsb_block].fast_erase;
	dr_state = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_lsb_block].dr_state;
	*/
	for(count=0;count<ssd->parameter->block_plane; count++){
		temp_block = (active_lsb_block+count)%ssd->parameter->block_plane;
		if(candidate_plane.blk_head[temp_block].free_lsb_num <= 0) continue;
		if(candidate_plane.blk_head[temp_block].fast_erase == TRUE) continue;
		//if(candidate_plane.blk_head[temp_block].free_lsb_num != 0) continue;
		if(candidate_plane.blk_head[temp_block].dr_state == DR_STATE_OUTPUT) continue;
		if(candidate_plane.blk_head[temp_block].last_write_lsb+3 > ssd->parameter->page_block) continue;
		break;
		}
	active_lsb_block = temp_block;
	/*
	while((free_lsb_num <= 0)&&(count < ssd->parameter->block_plane)){
		active_lsb_block = (active_lsb_block+1)%ssd->parameter->block_plane;
		free_lsb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_lsb_block].free_lsb_num;
		last_write_lsb = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_lsb_block].last_write_lsb;
		fast_erase = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_lsb_block].fast_erase;
	    dr_state = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_lsb_block].dr_state;
		if (last_write_lsb != (ssd->parameter->page_block-2*free_lsb_num-2)){
			printf("ERROR, the free page number is NOT corresponding to the last write page.\n");
			}
		count++;
		}
	*/
	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_lsb_block = active_lsb_block;
	
	if(count < ssd->parameter->block_plane){
		//printf("find active_lsb_block SUCCESS, %d, %d, %d, %d, %d,free_lsb: %d.\n", channel,chip,die,plane,active_lsb_block,free_lsb_num);
		return SUCCESS_LSB;
		}
	else{
		return find_active_block_csb(ssd, channel, chip, die, plane);
		}
}
Status  find_active_block_csb(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
	unsigned int active_csb_block, temp_block;
	unsigned int free_csb_num=0;
	unsigned int count=0;
	struct plane_info candidate_plane;
	candidate_plane = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane];
	active_csb_block = candidate_plane.active_csb_block;
	//free_msb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_msb_block].free_msb_num;

	for(count=0; count < ssd->parameter->block_plane; count++){
		temp_block = (active_csb_block+count)%ssd->parameter->block_plane;
		if(candidate_plane.blk_head[temp_block].free_csb_num <= 0) continue;
		if(candidate_plane.blk_head[temp_block].fast_erase == TRUE) continue;
		//if(candidate_plane.blk_head[temp_block].free_lsb_num != 0) continue;
		if(candidate_plane.blk_head[temp_block].last_write_lsb < candidate_plane.blk_head[temp_block].last_write_csb) continue;
		if(candidate_plane.blk_head[temp_block].dr_state == DR_STATE_OUTPUT) continue;
		if(candidate_plane.blk_head[temp_block].last_write_csb+3 > ssd->parameter->page_block) continue;
		break;
		}
	active_csb_block = temp_block;
	/*
	while(((free_msb_num <= 0)||(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_msb_block].fast_erase == TRUE))&&(count < ssd->parameter->block_plane)){
		active_msb_block = (active_msb_block+1)%ssd->parameter->block_plane;
		free_msb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_msb_block].free_msb_num;
		count++;
		}
	*/
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_csb_block = active_csb_block;
		
	if(count < ssd->parameter->block_plane){
		//printf("find active_CSB_block SUCCESS, %d.\n", active_csb_block);
		//printf("free_csb_num: %d, last_write_csb: %d.\n", candidate_plane.blk_head[temp_block].free_csb_num, candidate_plane.blk_head[temp_block].last_write_csb);
		return SUCCESS_CSB;
		}
	else{
		return find_active_block_msb(ssd, channel, chip, die, plane);
		}
}
Status  find_active_block_msb(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
	unsigned int active_msb_block, temp_block;
	unsigned int free_msb_num=0;
	unsigned int count=0;
	struct plane_info candidate_plane;
	candidate_plane = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane];
	active_msb_block = candidate_plane.active_msb_block;
	//free_msb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_msb_block].free_msb_num;

	for(count=0; count < ssd->parameter->block_plane; count++){
		temp_block = (active_msb_block+count)%ssd->parameter->block_plane;
		if(candidate_plane.blk_head[temp_block].free_msb_num <= 0) continue;
		if(candidate_plane.blk_head[temp_block].fast_erase == TRUE) continue;
		//if(candidate_plane.blk_head[temp_block].free_lsb_num != 0) continue;
		if(candidate_plane.blk_head[temp_block].last_write_csb < candidate_plane.blk_head[temp_block].last_write_msb) continue;
		if(candidate_plane.blk_head[temp_block].dr_state == DR_STATE_OUTPUT) continue;
		if(candidate_plane.blk_head[temp_block].last_write_msb+3 > ssd->parameter->page_block) continue;
		break;
		}
	active_msb_block = temp_block;
	/*
	while(((free_msb_num <= 0)||(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_msb_block].fast_erase == TRUE))&&(count < ssd->parameter->block_plane)){
		active_msb_block = (active_msb_block+1)%ssd->parameter->block_plane;
		free_msb_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_msb_block].free_msb_num;
		count++;
		}
	*/
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_msb_block = active_msb_block;
		
	if(count < ssd->parameter->block_plane){
		return SUCCESS_MSB;
		}
	else{
		return FAILURE;
		}
}
Status  find_active_block_rd(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
	unsigned int active_block;
	unsigned int free_page_num=0;
	unsigned int count=0;
	unsigned int gc_processing=FALSE;
	
	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	//gc_processing=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase;
	if(ssd->same_block_flag == 1){
	active_block=(active_block+1)%ssd->parameter->block_plane;	
		free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
		//gc_processing=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase;
	}
	//last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	while(((free_page_num==0)||(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count>4000))&&(count<ssd->parameter->block_plane))
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;	
		free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
		//gc_processing=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase;
		count++;
	}
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block=active_block;
	if(count<ssd->parameter->block_plane)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}
Status  find_active_block_dr(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
	unsigned int active_block;
	unsigned int free_page_num=0;
	unsigned int invalid_page_num=0;
	unsigned int dr_state;
	unsigned int count=0;
	
	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	invalid_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].invalid_page_num;
	dr_state=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].dr_state;
	//last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	while((free_page_num==0)&&(count<ssd->parameter->block_plane)&&(invalid_page_num!=0)&&(dr_state==DR_STATE_OUTPUT))
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;	
		free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
		invalid_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].invalid_page_num;
		count++;
	}
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block=active_block;
	if(count<ssd->parameter->block_plane)
	{
		//ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].dr_state=DR_STATE_RECIEVE;
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}
Status write_page(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int active_block,unsigned int *ppn)
{
	int last_write_page=0;
	last_write_page=++(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);	
	if(last_write_page>=(int)(ssd->parameter->page_block))
	{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page=0;
		printf("AAA error! the last write page larger than 64!!\n");
		return ERROR;
	}
		
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--; 
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[last_write_page].written_count++;
	ssd->write_flash_count++;
	//====================================================================================
	if(last_write_page%3 == 0){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_lsb_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
		ssd->free_lsb_count--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_lsb = last_write_page;
		ssd->write_lsb_count++;
		ssd->newest_write_lsb_count++;
		}
	else if(last_write_page%3==2){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_msb_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_msb = last_write_page;
		ssd->write_msb_count++;
		ssd->free_msb_count--;
		ssd->newest_write_msb_count++;
		}
	else{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_csb_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_csb = last_write_page;
		ssd->write_csb_count++;
		ssd->free_csb_count--;
		ssd->newest_write_csb_count++;
		}
	//====================================================================================
	*ppn=find_ppn(ssd,channel,chip,die,plane,active_block,last_write_page);

	return SUCCESS;
}
Status copy_back(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die,struct sub_request * sub)
{
	int old_ppn=-1, new_ppn=-1;
	long long time=0;
	if (ssd->parameter->greed_CB_ad==1)                                               /*����̰��ʹ��copyback�߼�����*/
	{
		old_ppn=-1;
		if (ssd->dram->map->map_entry[sub->lpn].state!=0)                             /*˵������߼�ҳ֮ǰ��д������Ҫʹ��copyback+random input�������ֱ��д��ȥ����*/
		{
			if ((sub->state&ssd->dram->map->map_entry[sub->lpn].state)==ssd->dram->map->map_entry[sub->lpn].state)       
			{
				sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;	
			} 
			else
			{
				sub->next_state_predict_time=ssd->current_time+19*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				ssd->copy_back_count++;
				ssd->read_count++;
				ssd->update_read_count++;
				old_ppn=ssd->dram->map->map_entry[sub->lpn].pn;                       /*��¼ԭ��������ҳ��������copybackʱ���ж��Ƿ�����ͬΪ���ַ����ż��ַ*/
			}															
		} 
		else
		{
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		}
		sub->complete_time=sub->next_state_predict_time;		
		time=sub->complete_time;

		get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);

		if (old_ppn!=-1)                                                              /*������copyback��������Ҫ�ж��Ƿ���������ż��ַ������*/
		{
			new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
			while (old_ppn%2!=new_ppn%2)                                              /*û��������ż��ַ���ƣ���Ҫ��������һҳ*/
			{
				get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
				ssd->program_count--;
				ssd->write_flash_count--;
				ssd->waste_page_count++;
				new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
			}
		}
	} 
	else                                                                              /*����̰����ʹ��copyback�߼�����*/
	{
		if (ssd->dram->map->map_entry[sub->lpn].state!=0)
		{
			if ((sub->state&ssd->dram->map->map_entry[sub->lpn].state)==ssd->dram->map->map_entry[sub->lpn].state)        
			{
				sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
			} 
			else
			{
				old_ppn=ssd->dram->map->map_entry[sub->lpn].pn;                       /*��¼ԭ��������ҳ��������copybackʱ���ж��Ƿ�����ͬΪ���ַ����ż��ַ*/
				get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
				new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
				if (old_ppn%2==new_ppn%2)
				{
					ssd->copy_back_count++;
					sub->next_state_predict_time=ssd->current_time+19*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				} 
				else
				{
					sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(size(ssd->dram->map->map_entry[sub->lpn].state))*ssd->parameter->time_characteristics.tRC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				}
				ssd->read_count++;
				ssd->update_read_count++;
			}
		} 
		else
		{
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
			get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
		}
		sub->complete_time=sub->next_state_predict_time;		
		time=sub->complete_time;
	}
    
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
	ssd->channel_head[channel].next_state_predict_time=time;

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;
	//ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG;
	if (new_ppn%2 == 0)
		{
			ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG_L;
			//printf("LSB programming\n");
		}
	else
		{
			ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG_M;
			//printf("MSB programming\n");
		}
	
	return SUCCESS;
}
struct ssd_info *dynamic_advanced_process(struct ssd_info *ssd,unsigned int channel,unsigned int chip)         
{
	unsigned int die=0,plane=0;
	unsigned int subs_count=0;
	int flag;
	unsigned int gate;                                                                    /*record the max subrequest that can be executed in the same channel. it will be used when channel-level priority order is highest and allocation scheme is full dynamic allocation*/
	unsigned int plane_place;                                                             /*record which plane has sub request in static allocation*/
	struct sub_request *sub=NULL,*p=NULL,*sub0=NULL,*sub1=NULL,*sub2=NULL,*sub3=NULL,*sub0_rw=NULL,*sub1_rw=NULL,*sub2_rw=NULL,*sub3_rw=NULL;
	struct sub_request ** subs=NULL;
	unsigned int max_sub_num=0;
	unsigned int die_token=0,plane_token=0;
	unsigned int * plane_bits=NULL;
	unsigned int interleaver_count=0;
	
	unsigned int mask=0x00000001;
	unsigned int i=0,j=0;
	
	max_sub_num=(ssd->parameter->die_chip)*(ssd->parameter->plane_die);
	gate=max_sub_num;
	subs=(struct sub_request **)malloc(max_sub_num*sizeof(struct sub_request *));
	alloc_assert(subs,"sub_request");
	
	for(i=0;i<max_sub_num;i++)
	{
		subs[i]=NULL;
	}
	
	if((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0)&&(ssd->parameter->ad_priority2==0))
	{
		gate=ssd->real_time_subreq/ssd->parameter->channel_number;

		if(gate==0)
		{
			gate=1;
		}
		else
		{
			if(ssd->real_time_subreq%ssd->parameter->channel_number!=0)
			{
				gate++;
			}
		}
	}

	if ((ssd->parameter->allocation_scheme==0))                                           /*ȫ��̬���䣬��Ҫ��ssd->subs_w_head��ѡȡ�ȴ������������*/
	{
		if(ssd->parameter->dynamic_allocation==0)
		{
			sub=ssd->subs_w_head;
		}
		else
		{
			sub=ssd->channel_head[channel].subs_w_head;
		}
		
		subs_count=0;
		
		while ((sub!=NULL)&&(subs_count<max_sub_num)&&(subs_count<gate))
		{
			if(sub->current_state==SR_WAIT)								
			{
				if ((sub->update==NULL)||((sub->update!=NULL)&&((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))))    //û����Ҫ��ǰ������ҳ
				{
					subs[subs_count]=sub;
					subs_count++;
				}						
			}
			
			p=sub;
			sub=sub->next_node;	
		}

		if (subs_count==0)                                                               /*û��������Է��񣬷���NULL*/
		{
			for(i=0;i<max_sub_num;i++)
			{
				subs[i]=NULL;
			}
			free(subs);

			subs=NULL;
			free(plane_bits);
			return NULL;
		}
		if(subs_count>=2)
		{
			if (((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))     
			{                                                                        
				get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE_TWO_PLANE); 
			}
			else if (((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE))
			{
				if(subs_count>ssd->parameter->plane_die)
				{	
					for(i=ssd->parameter->plane_die;i<subs_count;i++)
					{
						subs[i]=NULL;
					}
					subs_count=ssd->parameter->plane_die;
				}
				get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,TWO_PLANE);
			}
			else if (((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
			{
				
				if(subs_count>ssd->parameter->die_chip)
				{	
					for(i=ssd->parameter->die_chip;i<subs_count;i++)
					{
						subs[i]=NULL;
					}
					subs_count=ssd->parameter->die_chip;
				}
				get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE);
			}
			else
			{
				for(i=1;i<subs_count;i++)
				{
					subs[i]=NULL;
				}
				subs_count=1;
				get_ppn_for_normal_command(ssd,channel,chip,subs[0]);
			}
			
		}//if(subs_count>=2)
		else if(subs_count==1)     //only one request
		{
			get_ppn_for_normal_command(ssd,channel,chip,subs[0]);
		}
		
	}//if ((ssd->parameter->allocation_scheme==0)) 
	else                                                                                  /*��̬���䷽ʽ��ֻ�������ض���channel��ѡȡ�ȴ������������*/
	{
		                                                                                  /*�ھ�̬���䷽ʽ�У�����channel�ϵ���������ͬһ��die�ϵ���Щplane��ȷ��ʹ��ʲô����*/
		
			sub=ssd->channel_head[channel].subs_w_head;
			plane_bits=(unsigned int * )malloc((ssd->parameter->die_chip)*sizeof(unsigned int));
			alloc_assert(plane_bits,"plane_bits");
			memset(plane_bits,0, (ssd->parameter->die_chip)*sizeof(unsigned int));

			for(i=0;i<ssd->parameter->die_chip;i++)
			{
				plane_bits[i]=0x00000000;
			}
			subs_count=0;
			
			while ((sub!=NULL)&&(subs_count<max_sub_num))
			{
				if(sub->current_state==SR_WAIT)								
				{
					if ((sub->update==NULL)||((sub->update!=NULL)&&((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))))
					{
						if (sub->location->chip==chip)
						{
							plane_place=0x00000001<<(sub->location->plane);
	
							if ((plane_bits[sub->location->die]&plane_place)!=plane_place)      //we have not add sub request to this plane
							{
								subs[sub->location->die*ssd->parameter->plane_die+sub->location->plane]=sub;
								subs_count++;
								plane_bits[sub->location->die]=(plane_bits[sub->location->die]|plane_place);
							}
						}
					}						
				}
				sub=sub->next_node;	
			}//while ((sub!=NULL)&&(subs_count<max_sub_num))

			if (subs_count==0)                                                            /*û��������Է��񣬷���NULL*/
			{
				for(i=0;i<max_sub_num;i++)
				{
					subs[i]=NULL;
				}
				free(subs);
				subs=NULL;
				free(plane_bits);
				return NULL;
			}
			
			flag=0;
			if (ssd->parameter->advanced_commands!=0)
			{
				if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)        /*ȫ���߼��������ʹ��*/
				{
					if (subs_count>1)                                                    /*��1�����Ͽ���ֱ�ӷ����д����*/
					{
						get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,COPY_BACK);
					} 
					else
					{
						for(i=0;i<max_sub_num;i++)
						{
							if(subs[i]!=NULL)
							{
								break;
							}
						}
						get_ppn_for_normal_command(ssd,channel,chip,subs[i]);
					}
				
				}// if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
				else                                                                     /*����ִ��copyback*/
				{
					if (subs_count>1)                                                    /*��1�����Ͽ���ֱ�ӷ����д����*/
					{
						if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
						{
							get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE_TWO_PLANE);
						} 
						else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))
						{
							for(die=0;die<ssd->parameter->die_chip;die++)
							{
								if(plane_bits[die]!=0x00000000)
								{
									for(i=0;i<ssd->parameter->plane_die;i++)
									{
										plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
										ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane_token+1)%ssd->parameter->plane_die;
										mask=0x00000001<<plane_token;
										if((plane_bits[die]&mask)==mask)
										{
											plane_bits[die]=mask;
											break;
										}
									}
									for(i=i+1;i<ssd->parameter->plane_die;i++)
									{
										plane=(plane_token+1)%ssd->parameter->plane_die;
										subs[die*ssd->parameter->plane_die+plane]=NULL;
										subs_count--;
									}
									interleaver_count++;
								}//if(plane_bits[die]!=0x00000000)
							}//for(die=0;die<ssd->parameter->die_chip;die++)
							if(interleaver_count>=2)
							{
								get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE);
							}
							else
							{
								for(i=0;i<max_sub_num;i++)
								{
									if(subs[i]!=NULL)
									{
										break;
									}
								}
								get_ppn_for_normal_command(ssd,channel,chip,subs[i]);	
							}
						}//else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))
						else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
						{
							for(i=0;i<ssd->parameter->die_chip;i++)
							{
								die_token=ssd->channel_head[channel].chip_head[chip].token;
								ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
								if(size(plane_bits[die_token])>1)
								{
									break;
								}
								
							}
							
							if(i<ssd->parameter->die_chip)
							{
								for(die=0;die<ssd->parameter->die_chip;die++)
								{
									if(die!=die_token)
									{
										for(plane=0;plane<ssd->parameter->plane_die;plane++)
										{
											if(subs[die*ssd->parameter->plane_die+plane]!=NULL)
											{
												subs[die*ssd->parameter->plane_die+plane]=NULL;
												subs_count--;
											}
										}
									}
								}
								get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,TWO_PLANE);
							}//if(i<ssd->parameter->die_chip)
							else
							{
								for(i=0;i<ssd->parameter->die_chip;i++)
								{
									die_token=ssd->channel_head[channel].chip_head[chip].token;
									ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
									if(plane_bits[die_token]!=0x00000000)
									{
										for(j=0;j<ssd->parameter->plane_die;j++)
										{
											plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die_token].token;
											ssd->channel_head[channel].chip_head[chip].die_head[die_token].token=(plane_token+1)%ssd->parameter->plane_die;
											if(((plane_bits[die_token])&(0x00000001<<plane_token))!=0x00000000)
											{
												sub=subs[die_token*ssd->parameter->plane_die+plane_token];
												break;
											}
										}
									}
								}//for(i=0;i<ssd->parameter->die_chip;i++)
								get_ppn_for_normal_command(ssd,channel,chip,sub);
							}//else
						}//else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
					}//if (subs_count>1)  
					else
					{
						for(i=0;i<ssd->parameter->die_chip;i++)
						{
							die_token=ssd->channel_head[channel].chip_head[chip].token;
							ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
							if(plane_bits[die_token]!=0x00000000)
							{
								for(j=0;j<ssd->parameter->plane_die;j++)
								{
									plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die_token].token;
									ssd->channel_head[channel].chip_head[chip].die_head[die_token].token=(plane_token+1)%ssd->parameter->plane_die;
									if(((plane_bits[die_token])&(0x00000001<<plane_token))!=0x00000000)
									{
										sub=subs[die_token*ssd->parameter->plane_die+plane_token];
										break;
									}
								}
								if(sub!=NULL)
								{
									break;
								}
							}
						}//for(i=0;i<ssd->parameter->die_chip;i++)
						get_ppn_for_normal_command(ssd,channel,chip,sub);
					}//else
				}
			}//if (ssd->parameter->advanced_commands!=0)
			else
			{
				for(i=0;i<ssd->parameter->die_chip;i++)
				{
					die_token=ssd->channel_head[channel].chip_head[chip].token;
					ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
					if(plane_bits[die_token]!=0x00000000)
					{
						for(j=0;j<ssd->parameter->plane_die;j++)
						{
							plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die_token].token;
							ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane_token+1)%ssd->parameter->plane_die;
							if(((plane_bits[die_token])&(0x00000001<<plane_token))!=0x00000000)
							{
								sub=subs[die_token*ssd->parameter->plane_die+plane_token];
								break;
							}
						}
						if(sub!=NULL)
						{
							break;
						}
					}
				}//for(i=0;i<ssd->parameter->die_chip;i++)
				get_ppn_for_normal_command(ssd,channel,chip,sub);
			}//else
		
	}//else

	for(i=0;i<max_sub_num;i++)
	{
		subs[i]=NULL;
	}
	free(subs);
	subs=NULL;
	free(plane_bits);
	return ssd;
}
Status get_ppn_for_normal_command(struct ssd_info * ssd, unsigned int channel,unsigned int chip, struct sub_request * sub)
{
	unsigned int die=0;
	unsigned int plane=0;
	if(sub==NULL)
	{
		return ERROR;
	}
	
	if (ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)
	{
		die=ssd->channel_head[channel].chip_head[chip].token;
		plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
		get_ppn(ssd,channel,chip,die,plane,sub);
		ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
		ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
		
		compute_serve_time(ssd,channel,chip,die,&sub,1,NORMAL);
		return SUCCESS;
	}
	else
	{
		die=sub->location->die;
		plane=sub->location->plane;
		get_ppn(ssd,channel,chip,die,plane,sub);   
		compute_serve_time(ssd,channel,chip,die,&sub,1,NORMAL);
		return SUCCESS;
	}

}
Status get_ppn_for_advanced_commands(struct ssd_info *ssd,unsigned int channel,unsigned int chip,struct sub_request * * subs ,unsigned int subs_count,unsigned int command)      
{
	unsigned int die=0,plane=0;
	unsigned int die_token=0,plane_token=0;
	struct sub_request * sub=NULL;
	unsigned int i=0,j=0,k=0;
	unsigned int unvalid_subs_count=0;
	unsigned int valid_subs_count=0;
	unsigned int interleave_flag=FALSE;
	unsigned int multi_plane_falg=FALSE;
	unsigned int max_subs_num=0;
	struct sub_request * first_sub_in_chip=NULL;
	struct sub_request * first_sub_in_die=NULL;
	struct sub_request * second_sub_in_die=NULL;
	unsigned int state=SUCCESS;
	unsigned int multi_plane_flag=FALSE;

	max_subs_num=ssd->parameter->die_chip*ssd->parameter->plane_die;
	
	if (ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)                         /*��̬�������*/ 
	{
		if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))                      /*INTERLEAVE_TWO_PLANE�Լ�COPY_BACK�����*/
		{
			for(i=0;i<subs_count;i++)
			{
				die=ssd->channel_head[channel].chip_head[chip].token;
				if(i<ssd->parameter->die_chip)                                         /*Ϊÿ��subs[i]��ȡppn��iС��die_chip*/
				{
					plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
					get_ppn(ssd,channel,chip,die,plane,subs[i]);
					ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
				}
				else                                                                  
				{   
					state=make_level_page(ssd,subs[i%ssd->parameter->die_chip],subs[i]);
					if(state!=SUCCESS)                                                 
					{
						subs[i]=NULL;
						unvalid_subs_count++;
					}
					else
					{
						multi_plane_flag=TRUE;
					}
				}
				ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
			}
			valid_subs_count=subs_count-unvalid_subs_count;
			ssd->interleave_count++;
			if(multi_plane_flag==TRUE)
			{
				ssd->inter_mplane_count++;
				compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE_TWO_PLANE);/*����д������Ĵ���ʱ�䣬��д�������״̬ת��*/		
			}
			else
			{
				compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
			}
			return SUCCESS;
		}//if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
		else if(command==INTERLEAVE)
		{
			for(i=0;(i<subs_count)&&(i<ssd->parameter->die_chip);i++)
			{
				die=ssd->channel_head[channel].chip_head[chip].token;
				plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
				get_ppn(ssd,channel,chip,die,plane,subs[i]);
				ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
				ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
				valid_subs_count++;
			}
			ssd->interleave_count++;
			compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
			return SUCCESS;
		}//else if(command==INTERLEAVE)
		else if(command==TWO_PLANE)
		{
			if(subs_count<2)
			{
				return ERROR;
			}
			die=ssd->channel_head[channel].chip_head[chip].token;
			for(j=0;j<subs_count;j++)
			{
				if(j==1)
				{
					state=find_level_page(ssd,channel,chip,die,subs[0],subs[1]);        /*Ѱ����subs[0]��ppnλ����ͬ��subs[1]��ִ��TWO_PLANE�߼�����*/
					if(state!=SUCCESS)
					{
						get_ppn_for_normal_command(ssd,channel,chip,subs[0]);           /*û�ҵ�����ô�͵���ͨ����������*/
						return FAILURE;
					}
					else
					{
						valid_subs_count=2;
					}
				}
				else if(j>1)
				{
					state=make_level_page(ssd,subs[0],subs[j]);                         /*Ѱ����subs[0]��ppnλ����ͬ��subs[j]��ִ��TWO_PLANE�߼�����*/
					if(state!=SUCCESS)
					{
						for(k=j;k<subs_count;k++)
						{
							subs[k]=NULL;
						}
						subs_count=j;
						break;
					}
					else
					{
						valid_subs_count++;
					}
				}
			}//for(j=0;j<subs_count;j++)
			ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
			ssd->m_plane_prog_count++;
			compute_serve_time(ssd,channel,chip,die,subs,valid_subs_count,TWO_PLANE);
			return SUCCESS;
		}//else if(command==TWO_PLANE)
		else 
		{
			return ERROR;
		}
	}//if (ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)
	else                                                                              /*��̬��������*/
	{
		if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
		{
			for(die=0;die<ssd->parameter->die_chip;die++)
			{
				first_sub_in_die=NULL;
				for(plane=0;plane<ssd->parameter->plane_die;plane++)
				{
					sub=subs[die*ssd->parameter->plane_die+plane];
					if(sub!=NULL)
					{
						if(first_sub_in_die==NULL)
						{
							first_sub_in_die=sub;
							get_ppn(ssd,channel,chip,die,plane,sub);
						}
						else
						{
							state=make_level_page(ssd,first_sub_in_die,sub);
							if(state!=SUCCESS)
							{
								subs[die*ssd->parameter->plane_die+plane]=NULL;
								subs_count--;
								sub=NULL;
							}
							else
							{
								multi_plane_flag=TRUE;
							}
						}
					}
				}
			}
			if(multi_plane_flag==TRUE)
			{
				ssd->inter_mplane_count++;
				compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE_TWO_PLANE);
				return SUCCESS;
			}
			else
			{
				compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
				return SUCCESS;
			}
		}//if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
		else if(command==INTERLEAVE)
		{
			for(die=0;die<ssd->parameter->die_chip;die++)
			{	
				first_sub_in_die=NULL;
				for(plane=0;plane<ssd->parameter->plane_die;plane++)
				{
					sub=subs[die*ssd->parameter->plane_die+plane];
					if(sub!=NULL)
					{
						if(first_sub_in_die==NULL)
						{
							first_sub_in_die=sub;
							get_ppn(ssd,channel,chip,die,plane,sub);
							valid_subs_count++;
						}
						else
						{
							subs[die*ssd->parameter->plane_die+plane]=NULL;
							subs_count--;
							sub=NULL;
						}
					}
				}
			}
			if(valid_subs_count>1)
			{
				ssd->interleave_count++;
			}
			compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);	
		}//else if(command==INTERLEAVE)
		else if(command==TWO_PLANE)
		{
			for(die=0;die<ssd->parameter->die_chip;die++)
			{	
				first_sub_in_die=NULL;
				second_sub_in_die=NULL;
				for(plane=0;plane<ssd->parameter->plane_die;plane++)
				{
					sub=subs[die*ssd->parameter->plane_die+plane];
					if(sub!=NULL)
					{	
						if(first_sub_in_die==NULL)
						{
							first_sub_in_die=sub;
						}
						else if(second_sub_in_die==NULL)
						{
							second_sub_in_die=sub;
							state=find_level_page(ssd,channel,chip,die,first_sub_in_die,second_sub_in_die);
							if(state!=SUCCESS)
							{
								subs[die*ssd->parameter->plane_die+plane]=NULL;
								subs_count--;
								second_sub_in_die=NULL;
								sub=NULL;
							}
							else
							{
								valid_subs_count=2;
							}
						}
						else
						{
							state=make_level_page(ssd,first_sub_in_die,sub);
							if(state!=SUCCESS)
							{
								subs[die*ssd->parameter->plane_die+plane]=NULL;
								subs_count--;
								sub=NULL;
							}
							else
							{
								valid_subs_count++;
							}
						}
					}//if(sub!=NULL)
				}//for(plane=0;plane<ssd->parameter->plane_die;plane++)
				if(second_sub_in_die!=NULL)
				{
					multi_plane_flag=TRUE;
					break;
				}
			}//for(die=0;die<ssd->parameter->die_chip;die++)
			if(multi_plane_flag==TRUE)
			{
				ssd->m_plane_prog_count++;
				compute_serve_time(ssd,channel,chip,die,subs,valid_subs_count,TWO_PLANE);
				return SUCCESS;
			}//if(multi_plane_flag==TRUE)
			else
			{
				i=0;
				sub=NULL;
				while((sub==NULL)&&(i<max_subs_num))
				{
					sub=subs[i];
					i++;
				}
				if(sub!=NULL)
				{
					get_ppn_for_normal_command(ssd,channel,chip,sub);
					return FAILURE;
				}
				else 
				{
					return ERROR;
				}
			}//else
		}//else if(command==TWO_PLANE)
		else
		{
			return ERROR;
		}
	}//elseb ��̬��������
}
Status make_level_page(struct ssd_info * ssd, struct sub_request * sub0,struct sub_request * sub1)
{
	unsigned int i=0,j=0,k=0;
	unsigned int channel=0,chip=0,die=0,plane0=0,plane1=0,block0=0,block1=0,page0=0,page1=0;
	unsigned int active_block0=0,active_block1=0;
	unsigned int old_plane_token=0;
	
	if((sub0==NULL)||(sub1==NULL)||(sub0->location==NULL))
	{
		return ERROR;
	}
	channel=sub0->location->channel;
	chip=sub0->location->chip;
	die=sub0->location->die;
	plane0=sub0->location->plane;
	block0=sub0->location->block;
	page0=sub0->location->page;
	old_plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die].token;

	if(ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)                             
	{
		old_plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
		for(i=0;i<ssd->parameter->plane_die;i++)
		{
			plane1=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
			if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].add_reg_ppn==-1)
			{
				find_active_block(ssd,channel,chip,die,plane1);                               /*��plane1���ҵ���Ծ��*/
				block1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].active_block;

				if(block1==block0)
				{
					page1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].blk_head[block1].last_write_page+1;
					if(page1==page0)
					{
						break;
					}
					else if(page1<page0)
					{
						if (ssd->parameter->greed_MPW_ad==1)                                  /*����̰����ʹ�ø߼�����*/
						{                                                                   
							//make_same_level(ssd,channel,chip,die,plane1,active_block1,page0); /*С��page��ַ�����page��ַ��*/
							make_same_level(ssd,channel,chip,die,plane1,block1,page0);
							break;
						}    
					}
				}//if(block1==block0)
			}
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane1+1)%ssd->parameter->plane_die;
		}//for(i=0;i<ssd->parameter->plane_die;i++)
		if(i<ssd->parameter->plane_die)
		{
			flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page0);          /*������������þ��Ǹ���page1����Ӧ������ҳ�Լ�location����map��*/
			//flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page1);
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane1+1)%ssd->parameter->plane_die;
			return SUCCESS;
		}
		else
		{
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane_token;
			return FAILURE;
		}
	}
	else                                                                                      /*��̬��������*/
	{
		if((sub1->location==NULL)||(sub1->location->channel!=channel)||(sub1->location->chip!=chip)||(sub1->location->die!=die))
		{
			return ERROR;
		}
		plane1=sub1->location->plane;
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].add_reg_ppn==-1)
		{
			find_active_block(ssd,channel,chip,die,plane1);
			block1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].active_block;
			if(block1==block0)
			{
				page1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].blk_head[block1].last_write_page+1;
				if(page1>page0)
				{
					return FAILURE;
				}
				else if(page1<page0)
				{
					if (ssd->parameter->greed_MPW_ad==1)
					{ 
						//make_same_level(ssd,channel,chip,die,plane1,active_block1,page0);    /*С��page��ַ�����page��ַ��*/
                        make_same_level(ssd,channel,chip,die,plane1,block1,page0);
						flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page0);
						//flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page1);
						return SUCCESS;
					}
					else
					{
						return FAILURE;
					}					
				}
				else
				{
					flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page0);
					//flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page1);
					return SUCCESS;
				}
				
			}
			else
			{
				return FAILURE;
			}
			
		}
		else
		{
			return ERROR;
		}
	}
	
}
Status find_level_page(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request *subA,struct sub_request *subB)       
{
	unsigned int i,planeA,planeB,active_blockA,active_blockB,pageA,pageB,aim_page,old_plane;
	struct gc_operation *gc_node;

	old_plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
    
	if (ssd->parameter->allocation_scheme==0)                                                
	{
		planeA=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
		if (planeA%2==0)
		{
			planeB=planeA+1;
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(ssd->channel_head[channel].chip_head[chip].die_head[die].token+2)%ssd->parameter->plane_die;
		} 
		else
		{
			planeA=(planeA+1)%ssd->parameter->plane_die;
			planeB=planeA+1;
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(ssd->channel_head[channel].chip_head[chip].die_head[die].token+3)%ssd->parameter->plane_die;
		}
	} 
	else                                                                                     
	{
		planeA=subA->location->plane;
		planeB=subB->location->plane;
	}
	find_active_block(ssd,channel,chip,die,planeA);                                         
	find_active_block(ssd,channel,chip,die,planeB);
	active_blockA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].active_block;
	active_blockB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].active_block;
	if (active_blockA==active_blockB)
	{
		pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockA].last_write_page+1;      
		pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockB].last_write_page+1;
		if (pageA==pageB)                                                                  
		{
			flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,pageA);
			flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,pageB);
		} 
		else
		{
			if (ssd->parameter->greed_MPW_ad==1)                                             
			{
				if (pageA<pageB)                                                            
				{
					aim_page=pageB;
					make_same_level(ssd,channel,chip,die,planeA,active_blockA,aim_page);     
				}
				else
				{
					aim_page=pageA;
					make_same_level(ssd,channel,chip,die,planeB,active_blockB,aim_page);    
				}
				flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,aim_page);
				flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,aim_page);
			} 
			else                                                                             
			{
				subA=NULL;
				subB=NULL;
				ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
				return FAILURE;
			}
		}
	}
	else
	{   
		pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockA].last_write_page+1;      
		pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockB].last_write_page+1;
		if (pageA<pageB)
		{
			if (ssd->parameter->greed_MPW_ad==1)                                             
			{
				if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockB].page_head[pageB].free_state==PG_SUB)    
				{
					make_same_level(ssd,channel,chip,die,planeA,active_blockB,pageB);
					flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockB,pageB);
					flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,pageB);
				}
				else    
				{
					for (i=0;i<ssd->parameter->block_plane;i++)
					{
						pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].last_write_page+1;
						pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].last_write_page+1;
						if ((pageA<ssd->parameter->page_block)&&(pageB<ssd->parameter->page_block))
						{
							if (pageA<pageB)
							{
								if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].page_head[pageB].free_state==PG_SUB)
								{
									aim_page=pageB;
									make_same_level(ssd,channel,chip,die,planeA,i,aim_page);
									break;
								}
							} 
							else
							{
								if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].page_head[pageA].free_state==PG_SUB)
								{
									aim_page=pageA;
									make_same_level(ssd,channel,chip,die,planeB,i,aim_page);
									break;
								}
							}
						}
					}//for (i=0;i<ssd->parameter->block_plane;i++)
					if (i<ssd->parameter->block_plane)
					{
						flash_page_state_modify(ssd,subA,channel,chip,die,planeA,i,aim_page);
						flash_page_state_modify(ssd,subB,channel,chip,die,planeB,i,aim_page);
					} 
					else
					{
						subA=NULL;
						subB=NULL;
						ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
						return FAILURE;
					}
				}
			}//if (ssd->parameter->greed_MPW_ad==1)  
			else
			{
				subA=NULL;
				subB=NULL;
				ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
				return FAILURE;
			}
		}//if (pageA<pageB)
		else
		{
			if (ssd->parameter->greed_MPW_ad==1)     
			{
				if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockA].page_head[pageA].free_state==PG_SUB)
				{
					make_same_level(ssd,channel,chip,die,planeB,active_blockA,pageA);
					flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,pageA);
					flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockA,pageA);
				}
				else    
				{
					for (i=0;i<ssd->parameter->block_plane;i++)
					{
						pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].last_write_page+1;
						pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].last_write_page+1;
						if ((pageA<ssd->parameter->page_block)&&(pageB<ssd->parameter->page_block))
						{
							if (pageA<pageB)
							{
								if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].page_head[pageB].free_state==PG_SUB)
								{
									aim_page=pageB;
									make_same_level(ssd,channel,chip,die,planeA,i,aim_page);
									break;
								}
							} 
							else
							{
								if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].page_head[pageA].free_state==PG_SUB)
								{
									aim_page=pageA;
									make_same_level(ssd,channel,chip,die,planeB,i,aim_page);
									break;
								}
							}
						}
					}//for (i=0;i<ssd->parameter->block_plane;i++)
					if (i<ssd->parameter->block_plane)
					{
						flash_page_state_modify(ssd,subA,channel,chip,die,planeA,i,aim_page);
						flash_page_state_modify(ssd,subB,channel,chip,die,planeB,i,aim_page);
					} 
					else
					{
						subA=NULL;
						subB=NULL;
						ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
						return FAILURE;
					}
				}
			} //if (ssd->parameter->greed_MPW_ad==1) 
			else
			{
				if ((pageA==pageB)&&(pageA==0))
				{
					
					if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockA].page_head[pageA].free_state==PG_SUB)
					  &&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockA].page_head[pageA].free_state==PG_SUB))
					{
						flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,pageA);
						flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockA,pageA);
					}
					else if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockB].page_head[pageA].free_state==PG_SUB)
						   &&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockB].page_head[pageA].free_state==PG_SUB))
					{
						flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockB,pageA);
						flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,pageA);
					}
					else
					{
						subA=NULL;
						subB=NULL;
						ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
						return FAILURE;
					}
				}
				else
				{
					subA=NULL;
					subB=NULL;
					ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
					return ERROR;
				}
			}
		}
	}

	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
	{
		gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
		alloc_assert(gc_node,"gc_node");
		memset(gc_node,0, sizeof(struct gc_operation));

		gc_node->next_node=NULL;
		gc_node->chip=chip;
		gc_node->die=die;
		gc_node->plane=planeA;
		gc_node->block=0xffffffff;
		gc_node->page=0;
		gc_node->state=GC_WAIT;
		gc_node->priority=GC_UNINTERRUPT;
		gc_node->next_node=ssd->channel_head[channel].gc_command;
		ssd->channel_head[channel].gc_command=gc_node;
		ssd->gc_request++;
	}
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
	{
		gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
		alloc_assert(gc_node,"gc_node");
		memset(gc_node,0, sizeof(struct gc_operation));

		gc_node->next_node=NULL;
		gc_node->chip=chip;
		gc_node->die=die;
		gc_node->plane=planeB;
		gc_node->block=0xffffffff;
		gc_node->page=0;
		gc_node->state=GC_WAIT;
		gc_node->priority=GC_UNINTERRUPT;
		gc_node->next_node=ssd->channel_head[channel].gc_command;
		ssd->channel_head[channel].gc_command=gc_node;
		ssd->gc_request++;
	}

	return SUCCESS;     
}
struct ssd_info *flash_page_state_modify(struct ssd_info *ssd,struct sub_request *sub,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page)
{
	unsigned int ppn,full_page;
	struct local *location;
	struct direct_erase *new_direct_erase,*direct_erase_node;
	
	full_page=~(0xffffffff<<ssd->parameter->subpage_page);
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=page;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num--;
	if(page%2==0){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_lsb_num--;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
		ssd->free_lsb_count--;
		}
	else{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_msb_num--;
		}

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page>=(int)ssd->parameter->page_block)
	{
		printf("BBB error! the last write page larger than 64!!\n");
		while(1){}
	}

	if(ssd->dram->map->map_entry[sub->lpn].state==0)                                          /*this is the first logical page*/
	{
		ssd->dram->map->map_entry[sub->lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[sub->lpn].state=sub->state;
	}
	else                                                                                      /*����߼�ҳ�����˸��£���Ҫ��ԭ����ҳ��ΪʧЧ*/
	{
		ppn=ssd->dram->map->map_entry[sub->lpn].pn;
		location=find_location(ssd,ppn);
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;        //��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;         //��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
		if((location->page)%2==0){
			ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_lsb_num++;
			}
		if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num==ssd->parameter->page_block)    //��block��ȫ��invalid��ҳ������ֱ��ɾ��
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
		ssd->dram->map->map_entry[sub->lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[sub->lpn].state=(ssd->dram->map->map_entry[sub->lpn].state|sub->state);
	}

	sub->ppn=ssd->dram->map->map_entry[sub->lpn].pn;
	sub->location->channel=channel;
	sub->location->chip=chip;
	sub->location->die=die;
	sub->location->plane=plane;
	sub->location->block=block;
	sub->location->page=page;
	
	ssd->program_count++;
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	if(page%2==0){
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
		ssd->free_lsb_count--;
		}
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].lpn=sub->lpn;	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].valid_state=sub->state;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].free_state=((~(sub->state))&full_page);
	ssd->write_flash_count++;

	return ssd;
}
struct ssd_info *make_same_level(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int aim_page)
{
	int i=0,step,page;
	struct direct_erase *new_direct_erase,*direct_erase_node;

	page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page+1;                  /*��Ҫ�����ĵ�ǰ��Ŀ�дҳ��*/
	step=aim_page-page;
	while (i<step)
	{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].valid_state=0;     /*��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0*/
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].free_state=0;      /*��ʾĳһҳʧЧ��ͬʱ���valid��free״̬��Ϊ0*/
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].lpn=0;

		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num++;

		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num--;

		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
		/*
		if((page+i)%2==0){
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_lsb_num++;
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_lsb_num--;
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_lsb_num--;
			}
		else{
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_msb_num--;
			}
		*/
		i++;
	}

	ssd->waste_page_count+=step;

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=aim_page-1;

	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num==ssd->parameter->page_block)    /*��block��ȫ��invalid��ҳ������ֱ��ɾ��*/
	{
		new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
		alloc_assert(new_direct_erase,"new_direct_erase");
		memset(new_direct_erase,0, sizeof(struct direct_erase));

		direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
		if (direct_erase_node==NULL)
		{
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=new_direct_erase;
		} 
		else
		{
			new_direct_erase->next_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=new_direct_erase;
		}
	}

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page>=ssd->parameter->page_block)
		{
		printf("CCC error! the last write page larger than 64!!\n");
		while(1){}
		}

	return ssd;
}
struct ssd_info *compute_serve_time(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request **subs, unsigned int subs_count,unsigned int command)
{
	unsigned int i=0;
	unsigned int max_subs_num=0;
	struct sub_request *sub=NULL,*p=NULL;
	struct sub_request * last_sub=NULL;
	max_subs_num=ssd->parameter->die_chip*ssd->parameter->plane_die;

	if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
	{
		for(i=0;i<max_subs_num;i++)
		{
			if(subs[i]!=NULL)
			{
				last_sub=subs[i];
				subs[i]->current_state=SR_W_TRANSFER;
				subs[i]->current_time=ssd->current_time;
				subs[i]->next_state=SR_COMPLETE;
				subs[i]->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				subs[i]->complete_time=subs[i]->next_state_predict_time;

				delete_from_channel(ssd,channel,subs[i]);
			}
		}
		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=last_sub->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;	
	}
	else if(command==TWO_PLANE)
	{
		for(i=0;i<max_subs_num;i++)
		{
			if(subs[i]!=NULL)
			{
				
				subs[i]->current_state=SR_W_TRANSFER;
				if(last_sub==NULL)
				{
					subs[i]->current_time=ssd->current_time;
				}
				else
				{
					subs[i]->current_time=last_sub->complete_time+ssd->parameter->time_characteristics.tDBSY;
				}
				
				subs[i]->next_state=SR_COMPLETE;
				subs[i]->next_state_predict_time=subs[i]->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				subs[i]->complete_time=subs[i]->next_state_predict_time;
				last_sub=subs[i];

				delete_from_channel(ssd,channel,subs[i]);
			}
		}
		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=last_sub->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
	}
	else if(command==INTERLEAVE)
	{
		for(i=0;i<max_subs_num;i++)
		{
			if(subs[i]!=NULL)
			{
				
				subs[i]->current_state=SR_W_TRANSFER;
				if(last_sub==NULL)
				{
					subs[i]->current_time=ssd->current_time;
				}
				else
				{
					subs[i]->current_time=last_sub->complete_time;
				}
				subs[i]->next_state=SR_COMPLETE;
				subs[i]->next_state_predict_time=subs[i]->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				subs[i]->complete_time=subs[i]->next_state_predict_time;
				last_sub=subs[i];

				delete_from_channel(ssd,channel,subs[i]);
			}
		}
		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=last_sub->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
	}
	else if(command==NORMAL)
	{
		subs[0]->current_state=SR_W_TRANSFER;
		subs[0]->current_time=ssd->current_time;
		subs[0]->next_state=SR_COMPLETE;
		subs[0]->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[0]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		subs[0]->complete_time=subs[0]->next_state_predict_time;

		delete_from_channel(ssd,channel,subs[0]);

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=subs[0]->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;
		//ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
		//**************************************************************
		int new_ppn;
		new_ppn=subs[0]->ppn;
		if (new_ppn%2 == 0)
			{
				ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG_L;
				printf("LSB programming\n");
			}
		else
			{
				ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG_M;
				printf("MSB programming\n");
			}
		//****************************************************************
	}
	else
	{
		return NULL;
	}
	
	return ssd;

}
struct ssd_info *delete_from_channel(struct ssd_info *ssd,unsigned int channel,struct sub_request * sub_req)
{
	struct sub_request *sub,*p;
    
	/******************************************************************
	*��ȫ��̬�������������ssd->subs_w_head��
	*������ȫ��̬�������������ssd->channel_head[channel].subs_w_head��
	*******************************************************************/
	if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))    
	{
		sub=ssd->subs_w_head;
	} 
	else
	{
		sub=ssd->channel_head[channel].subs_w_head;
	}
	p=sub;

	while (sub!=NULL)
	{
		if (sub==sub_req)
		{
			if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))
			{
				if(ssd->parameter->ad_priority2==0)
				{
					ssd->real_time_subreq--;
				}
				
				if (sub==ssd->subs_w_head)                                                     /*������������sub request������ɾ��*/
				{
					if (ssd->subs_w_head!=ssd->subs_w_tail)
					{
						ssd->subs_w_head=sub->next_node;
						sub=ssd->subs_w_head;
						continue;
					} 
					else
					{
						ssd->subs_w_head=NULL;
						ssd->subs_w_tail=NULL;
						p=NULL;
						break;
					}
				}//if (sub==ssd->subs_w_head) 
				else
				{
					if (sub->next_node!=NULL)
					{
						p->next_node=sub->next_node;
						sub=p->next_node;
						continue;
					} 
					else
					{
						ssd->subs_w_tail=p;
						ssd->subs_w_tail->next_node=NULL;
						break;
					}
				}
			}//if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0)) 
			else
			{
				if (sub==ssd->channel_head[channel].subs_w_head)                               /*������������channel������ɾ��*/
				{
					if (ssd->channel_head[channel].subs_w_head!=ssd->channel_head[channel].subs_w_tail)
					{
						ssd->channel_head[channel].subs_w_head=sub->next_node;
						sub=ssd->channel_head[channel].subs_w_head;
						continue;;
					} 
					else
					{
						ssd->channel_head[channel].subs_w_head=NULL;
						ssd->channel_head[channel].subs_w_tail=NULL;
						p=NULL;
						break;
					}
				}//if (sub==ssd->channel_head[channel].subs_w_head)
				else
				{
					if (sub->next_node!=NULL)
					{
						p->next_node=sub->next_node;
						sub=p->next_node;
						continue;
					} 
					else
					{
						ssd->channel_head[channel].subs_w_tail=p;
						ssd->channel_head[channel].subs_w_tail->next_node=NULL;
						break;
					}
				}//else
			}//else
		}//if (sub==sub_req)
		p=sub;
		sub=sub->next_node;
	}//while (sub!=NULL)

	return ssd;
}
struct ssd_info *un_greed_interleave_copyback(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request *sub1,struct sub_request *sub2)
{
	unsigned int old_ppn1,ppn1,old_ppn2,ppn2,greed_flag=0;

	old_ppn1=ssd->dram->map->map_entry[sub1->lpn].pn;
	get_ppn(ssd,channel,chip,die,sub1->location->plane,sub1);                                  /*�ҳ�����ppnһ���Ƿ���������������ͬ��plane��,����ʹ��copyback����*/
	ppn1=sub1->ppn;

	old_ppn2=ssd->dram->map->map_entry[sub2->lpn].pn;
	get_ppn(ssd,channel,chip,die,sub2->location->plane,sub2);                                  /*�ҳ�����ppnһ���Ƿ���������������ͬ��plane��,����ʹ��copyback����*/
	ppn2=sub2->ppn;

	if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2==ppn2%2))
	{
		ssd->copy_back_count++;
		ssd->copy_back_count++;

		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub1->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		sub2->current_state=SR_W_TRANSFER;
		sub2->current_time=sub1->complete_time;
		sub2->next_state=SR_COMPLETE;
		sub2->next_state_predict_time=sub2->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub2->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub2->complete_time=sub2->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub2->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;

		delete_from_channel(ssd,channel,sub1);
		delete_from_channel(ssd,channel,sub2);
	} //if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2==ppn2%2))
	else if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2!=ppn2%2))
	{
		ssd->interleave_count--;
		ssd->copy_back_count++;

		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub1->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;

		delete_from_channel(ssd,channel,sub1);
	}//else if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2!=ppn2%2))
	else if ((old_ppn1%2!=ppn1%2)&&(old_ppn2%2==ppn2%2))
	{
		ssd->interleave_count--;
		ssd->copy_back_count++;

		sub2->current_state=SR_W_TRANSFER;
		sub2->current_time=ssd->current_time;
		sub2->next_state=SR_COMPLETE;
		sub2->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub2->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub2->complete_time=sub2->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub2->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;

		delete_from_channel(ssd,channel,sub2);
	}//else if ((old_ppn1%2!=ppn1%2)&&(old_ppn2%2==ppn2%2))
	else
	{
		ssd->interleave_count--;

		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+2*(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;

		delete_from_channel(ssd,channel,sub1);
	}//else

	return ssd;
}
struct ssd_info *un_greed_copyback(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request *sub1)
{
	unsigned int old_ppn,ppn;

	old_ppn=ssd->dram->map->map_entry[sub1->lpn].pn;
	get_ppn(ssd,channel,chip,die,0,sub1);                                                     /*�ҳ�����ppnһ���Ƿ���������������ͬ��plane��,����ʹ��copyback����*/
	ppn=sub1->ppn;
	
	if (old_ppn%2==ppn%2)
	{
		ssd->copy_back_count++;
		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub1->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
	}//if (old_ppn%2==ppn%2)
	else
	{
		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+2*(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
	}//else

	delete_from_channel(ssd,channel,sub1);

	return ssd;
}
struct sub_request *find_interleave_twoplane_page(struct ssd_info *ssd, struct sub_request *one_page,unsigned int command)
{
	struct sub_request *two_page;

	if (one_page->current_state!=SR_WAIT)
	{
		return NULL;                                                            
	}
	if (((ssd->channel_head[one_page->location->channel].chip_head[one_page->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[one_page->location->channel].chip_head[one_page->location->chip].next_state==CHIP_IDLE)&&
		(ssd->channel_head[one_page->location->channel].chip_head[one_page->location->chip].next_state_predict_time<=ssd->current_time))))
	{
		two_page=one_page->next_node;
		if(command==TWO_PLANE)
		{
			while (two_page!=NULL)
		    {
				if (two_page->current_state!=SR_WAIT)
				{
					two_page=two_page->next_node;
				}
				else if ((one_page->location->chip==two_page->location->chip)&&(one_page->location->die==two_page->location->die)&&(one_page->location->block==two_page->location->block)&&(one_page->location->page==two_page->location->page))
				{
					if (one_page->location->plane!=two_page->location->plane)
					{
						return two_page;                                                       /*�ҵ�����one_page����ִ��two plane������ҳ*/
					}
					else
					{
						two_page=two_page->next_node;
					}
				}
				else
				{
					two_page=two_page->next_node;
				}
		     }//while (two_page!=NULL)
		    if (two_page==NULL)                                                               /*û���ҵ����Ժ�one_pageִ��two_plane������ҳ,��Ҫ��one_page�����һ���ڵ�*/
		    {
				return NULL;
			}
		}//if(command==TWO_PLANE)
		else if(command==INTERLEAVE)
		{
			while (two_page!=NULL)
		    {
				if (two_page->current_state!=SR_WAIT)
				{
					two_page=two_page->next_node;
				}
				else if ((one_page->location->chip==two_page->location->chip)&&(one_page->location->die!=two_page->location->die))
				{
					return two_page;                                                           /*�ҵ�����one_page����ִ��interleave������ҳ*/
				}
				else
				{
					two_page=two_page->next_node;
				}
		     }
		    if (two_page==NULL)                                                                /*û���ҵ����Ժ�one_pageִ��interleave������ҳ,��Ҫ��one_page�����һ���ڵ�*/
		    {
				return NULL;
			}//while (two_page!=NULL)
		}//else if(command==INTERLEAVE)
		
	} 
	{
		return NULL;
	}
}
int find_interleave_twoplane_sub_request(struct ssd_info * ssd, unsigned int channel,struct sub_request * sub_request_one,struct sub_request * sub_request_two,unsigned int command)
{
	sub_request_one=ssd->channel_head[channel].subs_r_head;
	while (sub_request_one!=NULL)
	{
		sub_request_two=find_interleave_twoplane_page(ssd,sub_request_one,command);                /*�ҳ�����������two_plane����interleave��read�����󣬰���λ��������ʱ������*/
		if (sub_request_two==NULL)
		{
			sub_request_one=sub_request_one->next_node;
		}
		else if (sub_request_two!=NULL)                                                            /*�ҵ�����������ִ��two plane������ҳ*/
		{
			break;
		}
	}

	if (sub_request_two!=NULL)
	{
		if (ssd->request_queue!=ssd->request_tail)      
		{                                                                                         /*ȷ��interleave read���������ǵ�һ�������������*/
			if ((ssd->request_queue->lsn-ssd->parameter->subpage_page)<(sub_request_one->lpn*ssd->parameter->subpage_page))  
			{
				if ((ssd->request_queue->lsn+ssd->request_queue->size+ssd->parameter->subpage_page)>(sub_request_one->lpn*ssd->parameter->subpage_page))
				{
				}
				else
				{
					sub_request_two=NULL;
				}
			}
			else
			{
				sub_request_two=NULL;
			}
		}//if (ssd->request_queue!=ssd->request_tail) 
	}//if (sub_request_two!=NULL)

	if(sub_request_two!=NULL)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}

}
















Status allocate_location(struct ssd_info * ssd ,struct sub_request *sub_req)
{
	struct sub_request * update=NULL;
	unsigned int channel_num=0,chip_num=0,die_num=0,plane_num=0;
	struct local *location=NULL;

	channel_num=ssd->parameter->channel_number;
	chip_num=ssd->parameter->chip_channel[0];
	die_num=ssd->parameter->die_chip;
	plane_num=ssd->parameter->plane_die;
    
	
	if (ssd->parameter->allocation_scheme==0)                                         
	{
		if (ssd->dram->map->map_entry[sub_req->lpn].state!=0)    
		{
			if ((sub_req->state&ssd->dram->map->map_entry[sub_req->lpn].state)!=ssd->dram->map->map_entry[sub_req->lpn].state)
			{
				ssd->read_count++;
				ssd->update_read_count++;
				ssd->total_sub_read_count++;
				update=(struct sub_request *)malloc(sizeof(struct sub_request));
				alloc_assert(update,"update");
				memset(update,0, sizeof(struct sub_request));
				if(update==NULL)
				{
					return ERROR;
				}
				update->location=NULL;
				update->next_node=NULL;
				update->next_subs=NULL;
				update->update=NULL;						
				location = find_location(ssd,ssd->dram->map->map_entry[sub_req->lpn].pn);
				update->location=location;
				update->begin_time = ssd->current_time;
				update->current_state = SR_WAIT;
				update->current_time=MAX_INT64;
				update->next_state = SR_R_C_A_TRANSFER;
				update->next_state_predict_time=MAX_INT64;
				update->lpn = sub_req->lpn;
				update->state=((ssd->dram->map->map_entry[sub_req->lpn].state^sub_req->state)&0x7fffffff);
				update->size=size(update->state);
				update->ppn = ssd->dram->map->map_entry[sub_req->lpn].pn;
				update->operation = READ;
				
				if (ssd->channel_head[location->channel].subs_r_tail!=NULL)           
				{
						ssd->channel_head[location->channel].subs_r_tail->next_node=update;
						ssd->channel_head[location->channel].subs_r_tail=update;
				} 
				else
				{
					ssd->channel_head[location->channel].subs_r_tail=update;
					ssd->channel_head[location->channel].subs_r_head=update;
				}
			}
		}
		switch(ssd->parameter->dynamic_allocation)
		{
			case 0:
			{
				sub_req->location->channel=-1;
				sub_req->location->chip=-1;
				sub_req->location->die=-1;
				sub_req->location->plane=-1;
				sub_req->location->block=-1;
				sub_req->location->page=-1;

				if (ssd->subs_w_tail!=NULL)
				{
					ssd->subs_w_tail->next_node=sub_req;
					ssd->subs_w_tail=sub_req;
				} 
				else
				{
					ssd->subs_w_tail=sub_req;
					ssd->subs_w_head=sub_req;
				}

				if (update!=NULL)
				{
					sub_req->update=update;
				}

				break;
			}
			case 1:
			{
				 
				sub_req->location->channel=sub_req->lpn%ssd->parameter->channel_number;
				sub_req->location->chip=-1;
				sub_req->location->die=-1;
				sub_req->location->plane=-1;
				sub_req->location->block=-1;
				sub_req->location->page=-1;

				if (update!=NULL)
				{
					sub_req->update=update;
				}

				break;
			}
			case 2:
			{
				break;
			}
			case 3:
			{
				break;
			}
		}

	}
	else                                                                          
	{   
		switch (ssd->parameter->static_allocation)
		{
			case 0:         //no striping static allocation
			{
				sub_req->location->channel=(sub_req->lpn/(plane_num*die_num*chip_num))%channel_num;
				sub_req->location->chip=sub_req->lpn%chip_num;
				sub_req->location->die=(sub_req->lpn/chip_num)%die_num;
				sub_req->location->plane=(sub_req->lpn/(die_num*chip_num))%plane_num;
				break;
			}
			case 1:
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/channel_num)%chip_num;
				sub_req->location->die=(sub_req->lpn/(chip_num*channel_num))%die_num;
				sub_req->location->plane=(sub_req->lpn/(die_num*chip_num*channel_num))%plane_num;
							
				break;
			}
			case 2:
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/(plane_num*channel_num))%chip_num;
				sub_req->location->die=(sub_req->lpn/(plane_num*chip_num*channel_num))%die_num;
				sub_req->location->plane=(sub_req->lpn/channel_num)%plane_num;
				break;
			}
			case 3:
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/(die_num*channel_num))%chip_num;
				sub_req->location->die=(sub_req->lpn/channel_num)%die_num;
				sub_req->location->plane=(sub_req->lpn/(die_num*chip_num*channel_num))%plane_num;
				break;
			}
			case 4:  
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/(plane_num*die_num*channel_num))%chip_num;
				sub_req->location->die=(sub_req->lpn/(plane_num*channel_num))%die_num;
				sub_req->location->plane=(sub_req->lpn/channel_num)%plane_num;
							
				break;
			}
			case 5:   
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/(plane_num*die_num*channel_num))%chip_num;
				sub_req->location->die=(sub_req->lpn/channel_num)%die_num;
				sub_req->location->plane=(sub_req->lpn/(die_num*channel_num))%plane_num;
							
				break;
			}
			default : return ERROR;
		
		}
		if (ssd->dram->map->map_entry[sub_req->lpn].state!=0)
		{                                                                             
			if ((sub_req->state&ssd->dram->map->map_entry[sub_req->lpn].state)!=ssd->dram->map->map_entry[sub_req->lpn].state)  
			{
				ssd->read_count++;
				ssd->update_read_count++;
				update=(struct sub_request *)malloc(sizeof(struct sub_request));
				alloc_assert(update,"update");
				memset(update,0, sizeof(struct sub_request));
				
				if(update==NULL)
				{
					return ERROR;
				}
				update->location=NULL;
				update->next_node=NULL;
				update->next_subs=NULL;
				update->update=NULL;						
				location = find_location(ssd,ssd->dram->map->map_entry[sub_req->lpn].pn);
				update->location=location;
				update->begin_time = ssd->current_time;
				update->current_state = SR_WAIT;
				update->current_time=MAX_INT64;
				update->next_state = SR_R_C_A_TRANSFER;
				update->next_state_predict_time=MAX_INT64;
				update->lpn = sub_req->lpn;
				update->state=((ssd->dram->map->map_entry[sub_req->lpn].state^sub_req->state)&0x7fffffff);
				update->size=size(update->state);
				update->ppn = ssd->dram->map->map_entry[sub_req->lpn].pn;
				update->operation = READ;
				
				if (ssd->channel_head[location->channel].subs_r_tail!=NULL)
				{
					ssd->channel_head[location->channel].subs_r_tail->next_node=update;
					ssd->channel_head[location->channel].subs_r_tail=update;
				} 
				else
				{
					ssd->channel_head[location->channel].subs_r_tail=update;
					ssd->channel_head[location->channel].subs_r_head=update;
				}
			}

			if (update!=NULL)
			{
				sub_req->update=update;

				sub_req->state=(sub_req->state|update->state);
				sub_req->size=size(sub_req->state);
			}

 		}
	}
	if ((ssd->parameter->allocation_scheme!=0)||(ssd->parameter->dynamic_allocation!=0))
	{
		if (ssd->channel_head[sub_req->location->channel].subs_w_tail!=NULL)
		{
			ssd->channel_head[sub_req->location->channel].subs_w_tail->next_node=sub_req;
			ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
		} 
		else
		{
			ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
			ssd->channel_head[sub_req->location->channel].subs_w_head=sub_req;
		}
	}
	return SUCCESS;					
}	
struct ssd_info * insert2buffer(struct ssd_info *ssd,unsigned int lpn,int state,struct sub_request *sub,struct request *req)      
{
	int write_back_count,flag=0;                                                            
	unsigned int i,lsn,hit_flag,add_flag,sector_count,active_region_flag=0,free_sector=0;
	struct buffer_group *buffer_node=NULL,*pt,*new_node=NULL,key;
	struct sub_request *sub_req=NULL,*update=NULL;
	
	
	unsigned int sub_req_state=0, sub_req_size=0,sub_req_lpn=0;

	#ifdef DEBUG
	printf("enter insert2buffer,  current time:%lld, lpn:%d, state:%d,\n",ssd->current_time,lpn,state);
	#endif

	sector_count=size(state);                                                               
	key.group=lpn;
	buffer_node= (struct buffer_group*)avlTreeFind(ssd->dram->buffer, (TREE_NODE *)&key);  
	if (req->operation == WRITE)
	{
		if(buffer_node==NULL)
		{
			free_sector=ssd->dram->buffer->max_buffer_sector-ssd->dram->buffer->buffer_sector_count;   
			if(free_sector>=sector_count)
			{
				flag=1;    
			}
			if(flag==0)     
			{
				write_back_count=sector_count-free_sector;
				ssd->dram->buffer->write_miss_hit=ssd->dram->buffer->write_miss_hit+write_back_count;
				while(write_back_count>0)
				{
					sub_req=NULL;
					sub_req_state=ssd->dram->buffer->buffer_tail->stored; 
					sub_req_size=size(ssd->dram->buffer->buffer_tail->stored);
					if (ssd->dram->buffer->buffer_tail->dirty_clean > 0)
					{
						sub_req_lpn=ssd->dram->buffer->buffer_tail->group;
						sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE,0);
					}	
					if(req!=NULL)                                             
					{
					}
					else    
					{
						sub_req->next_subs=sub->next_subs;
						sub->next_subs=sub_req;
					}
					
					ssd->dram->buffer->buffer_sector_count=ssd->dram->buffer->buffer_sector_count-sub_req_size;
					pt = ssd->dram->buffer->buffer_tail;
					avlTreeDel(ssd->dram->buffer, (TREE_NODE *) pt);
					if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL){
						ssd->dram->buffer->buffer_head = NULL;
						ssd->dram->buffer->buffer_tail = NULL;
					}else{
						ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
						ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
					}
					pt->LRU_link_next=NULL;
					pt->LRU_link_pre=NULL;
					AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
					pt = NULL;
					write_back_count=write_back_count-sub_req_size;                           
				}
			}
			
		
			new_node=NULL;
			new_node=(struct buffer_group *)malloc(sizeof(struct buffer_group));
			alloc_assert(new_node,"buffer_group_node");
			memset(new_node,0, sizeof(struct buffer_group));
			
			new_node->group=lpn;
			new_node->stored=state;
			new_node->dirty_clean=state;
			new_node->LRU_link_pre = NULL;
			new_node->LRU_link_next=ssd->dram->buffer->buffer_head;
			if(ssd->dram->buffer->buffer_head != NULL){
				ssd->dram->buffer->buffer_head->LRU_link_pre=new_node;
			}else{
				ssd->dram->buffer->buffer_tail = new_node;
			}
			ssd->dram->buffer->buffer_head=new_node;
			new_node->LRU_link_pre=NULL;
			avlTreeAdd(ssd->dram->buffer, (TREE_NODE *) new_node);
			ssd->dram->buffer->buffer_sector_count += sector_count;
		}
		else
		{
			for(i=0;i<ssd->parameter->subpage_page;i++)
			{
				if((state>>i)%2!=0)                                                         
				{
					lsn=lpn*ssd->parameter->subpage_page+i;
					hit_flag=0;
					hit_flag=(buffer_node->stored)&(0x00000001<<i);
					
					if(hit_flag!=0)				                                          
					{	
						active_region_flag=1;                                             

						if(req!=NULL)
						{
							if(ssd->dram->buffer->buffer_head!=buffer_node)     
							{				
								if(ssd->dram->buffer->buffer_tail==buffer_node)
								{				
									ssd->dram->buffer->buffer_tail=buffer_node->LRU_link_pre;
									buffer_node->LRU_link_pre->LRU_link_next=NULL;					
								}				
								else if(buffer_node != ssd->dram->buffer->buffer_head)
								{					
									buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;				
									buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;
								}				
								buffer_node->LRU_link_next=ssd->dram->buffer->buffer_head;	
								ssd->dram->buffer->buffer_head->LRU_link_pre=buffer_node;
								buffer_node->LRU_link_pre=NULL;				
								ssd->dram->buffer->buffer_head=buffer_node;					
							}					
							ssd->dram->buffer->write_hit++;
							req->complete_lsn_count++;                                       				
						}
						else
						{
						}				
					}			
					else                 			
					{	
						ssd->dram->buffer->write_miss_hit++;
						
						if(ssd->dram->buffer->buffer_sector_count>=ssd->dram->buffer->max_buffer_sector)
						{
							if (buffer_node==ssd->dram->buffer->buffer_tail)                 
							{
								pt = ssd->dram->buffer->buffer_tail->LRU_link_pre;
								ssd->dram->buffer->buffer_tail->LRU_link_pre=pt->LRU_link_pre;
								ssd->dram->buffer->buffer_tail->LRU_link_pre->LRU_link_next=ssd->dram->buffer->buffer_tail;
								ssd->dram->buffer->buffer_tail->LRU_link_next=pt;
								pt->LRU_link_next=NULL;
								pt->LRU_link_pre=ssd->dram->buffer->buffer_tail;
								ssd->dram->buffer->buffer_tail=pt;
								
							}
							sub_req=NULL;
							sub_req_state=ssd->dram->buffer->buffer_tail->stored;
							sub_req_size=size(ssd->dram->buffer->buffer_tail->stored);
							if (ssd->dram->buffer->buffer_tail->dirty_clean > 0)
							{
								
								sub_req_lpn=ssd->dram->buffer->buffer_tail->group;
								sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE,0);
							} 
							if(req!=NULL)           
							{
								
							}
							else if(req==NULL)   
							{
								sub_req->next_subs=sub->next_subs;
								sub->next_subs=sub_req;
							}

							ssd->dram->buffer->buffer_sector_count=ssd->dram->buffer->buffer_sector_count-sub_req_size;
							pt = ssd->dram->buffer->buffer_tail;	
							avlTreeDel(ssd->dram->buffer, (TREE_NODE *) pt);
								
							if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL)
							{
								ssd->dram->buffer->buffer_head = NULL;
								ssd->dram->buffer->buffer_tail = NULL;
							}else{
								ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
								ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
							}
							pt->LRU_link_next=NULL;
							pt->LRU_link_pre=NULL;
							AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
							pt = NULL;	
						}                                                    	
						add_flag=0x00000001<<(lsn%ssd->parameter->subpage_page);
						
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
						buffer_node->stored=buffer_node->stored|add_flag;		
						buffer_node->dirty_clean=buffer_node->dirty_clean|add_flag;	
						ssd->dram->buffer->buffer_sector_count++;
					}			
				}
			}
		}
	}
	else if (req->operation == READ)
	{
		if (buffer_node == NULL)
		{
			free_sector=ssd->dram->buffer->max_buffer_sector-ssd->dram->buffer->buffer_sector_count;   
			if(free_sector>=sector_count)
			{
				flag=1;    
			}
			if(flag==0)     
			{
				write_back_count=sector_count-free_sector;
				ssd->dram->buffer->write_miss_hit=ssd->dram->buffer->write_miss_hit+write_back_count;
				while(write_back_count>0)
				{
					sub_req=NULL;
					sub_req_state=ssd->dram->buffer->buffer_tail->stored; 
					sub_req_lpn=ssd->dram->buffer->buffer_tail->group;
					//如果是脏数据，那么该页一定写下去的时候一定是有对应的写请求的
					//也就是说如果没有对应的写请求就不用把该数据写下去。
					if (ssd->dram->buffer->buffer_tail == NULL)
					{
						printf("ssd->dram->buffer->buffer_tail==NULL\n");
						printf("当前缓存的容量:%d\n",ssd->dram->buffer->buffer_sector_count);
					}
					sub_req_size=size(ssd->dram->buffer->buffer_tail->stored);

					if (ssd->dram->buffer->buffer_tail->dirty_clean > 0)
					{
						
						sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE,0);
					}
					if(req!=NULL)                                             
					{

					}
					else    
					{
						sub_req->next_subs=sub->next_subs;
						sub->next_subs=sub_req;
					}
					ssd->dram->buffer->buffer_sector_count=ssd->dram->buffer->buffer_sector_count-sub_req_size;
					pt = ssd->dram->buffer->buffer_tail;
					avlTreeDel(ssd->dram->buffer, (TREE_NODE *) pt);
					if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL){
						ssd->dram->buffer->buffer_head = NULL;
						ssd->dram->buffer->buffer_tail = NULL;
					}else{
						ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
						ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
					}
					pt->LRU_link_next=NULL;
					pt->LRU_link_pre=NULL;
					AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
					pt = NULL;
					write_back_count=write_back_count-sub_req_size;                           
				}
			}
			new_node=NULL;
			new_node=(struct buffer_group *)malloc(sizeof(struct buffer_group));

			alloc_assert(new_node,"buffer_group_node");
			memset(new_node,0, sizeof(struct buffer_group));
			new_node->group=lpn;
			new_node->stored=state;
			new_node->dirty_clean= 0x00000000;
			//此时的是由读产生的buffer节点没有脏数据,所以此时脏的位置为1
			new_node->LRU_link_pre = NULL;
			new_node->LRU_link_next=ssd->dram->buffer->buffer_head;
			if(ssd->dram->buffer->buffer_head != NULL)
			{
				ssd->dram->buffer->buffer_head->LRU_link_pre=new_node;
			}
			else
			{
				ssd->dram->buffer->buffer_tail = new_node;
			}
			ssd->dram->buffer->buffer_head=new_node;
			new_node->LRU_link_pre=NULL;
			avlTreeAdd(ssd->dram->buffer, (TREE_NODE *) new_node);
			ssd->dram->buffer->buffer_sector_count += sector_count;
		}
		else
		{
			for ( i = 0 ; i < ssd->parameter->subpage_page ; i++)
			{
				if ((state >> i) % 2 != 0) // 判断数据是否为1
				{
					lsn=lpn*ssd->parameter->subpage_page+i;
					hit_flag=0;//判断子页是否在buffer中
					hit_flag=(buffer_node->stored)&(0x00000001<<i);
					if (hit_flag == 0)
					{
						if(ssd->dram->buffer->buffer_sector_count>=ssd->dram->buffer->max_buffer_sector)
						{
							if (buffer_node==ssd->dram->buffer->buffer_tail)                 
							{
								pt = ssd->dram->buffer->buffer_tail->LRU_link_pre;
								ssd->dram->buffer->buffer_tail->LRU_link_pre=pt->LRU_link_pre;
								ssd->dram->buffer->buffer_tail->LRU_link_pre->LRU_link_next=ssd->dram->buffer->buffer_tail;
								ssd->dram->buffer->buffer_tail->LRU_link_next=pt;
								pt->LRU_link_next=NULL;
								pt->LRU_link_pre=ssd->dram->buffer->buffer_tail;
								ssd->dram->buffer->buffer_tail=pt;
								
							}
							sub_req=NULL;
							sub_req_state=ssd->dram->buffer->buffer_tail->stored;
							sub_req_size=size(ssd->dram->buffer->buffer_tail->stored);
							if (ssd->dram->buffer->buffer_tail->dirty_clean > 0)
							{
								
								sub_req_lpn=ssd->dram->buffer->buffer_tail->group;
								sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE,0);
							}
							if(req!=NULL)           
							{
								
							}
							else if(req==NULL)   
							{
								sub_req->next_subs=sub->next_subs;
								sub->next_subs=sub_req;
							}
							ssd->dram->buffer->buffer_sector_count=ssd->dram->buffer->buffer_sector_count-sub_req_size;
							pt = ssd->dram->buffer->buffer_tail;	
							avlTreeDel(ssd->dram->buffer, (TREE_NODE *) pt);
							if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL)
							{
								ssd->dram->buffer->buffer_head = NULL;
								ssd->dram->buffer->buffer_tail = NULL;
							}else{
								ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
								ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
							}
							pt->LRU_link_next=NULL;
							pt->LRU_link_pre=NULL;
							AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
							pt = NULL;	
						}

					                                                                
						add_flag=0x00000001<<(lsn%ssd->parameter->subpage_page);
					
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
						buffer_node->stored=buffer_node->stored|add_flag;		
						//buffer_node->dirty_clean=buffer_node->dirty_clean|add_flag;//写命中才会修改
						ssd->dram->buffer->buffer_sector_count++;
					}
				}
			}
		}
	}
	return ssd;
}
Status  find_active_block(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
	unsigned int active_block;
	unsigned int free_page_num=0;
	unsigned int count=0;
	unsigned int gc_processing=FALSE;
	int same_flag=0;
	int id=0;
	
	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].id;
	//gc_processing=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase;
	if(ssd->same_block_flag == 1)
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;	
		free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
		id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].id;
		//gc_processing=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase;
	}
	//last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	//printf("erase_node: %d, %d, %d, %d, %d.\n",ssd->erase_channel, ssd->erase_chip, ssd->erase_die, ssd->erase_plane, ssd->erase_block);
	if(ssd->erase_channel==channel&&ssd->erase_chip==chip&&ssd->erase_die==die&&ssd->erase_plane==plane){
		same_flag=1;
		if(ssd->erase_block==active_block)
			printf("same_flag\n");
	}
	int active_block2 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block2;
	while(((free_page_num==0)||(active_block==ssd->erase_block&&same_flag==1)||(id!=0))&&(count<ssd->parameter->block_plane))
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;	
		free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
		id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].id;
		//gc_processing=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase;
		count++;
	}

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block=active_block;
	if(count<ssd->parameter->block_plane)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

Status get_new_block_with_min_erase_count(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int operation){
	// 统一使用通用变量，替代原有的w/r前缀变量（逻辑更简洁）
	int target_active_block;       // 最终选中的块
	int min_erase_count = 65535; // 记录最小擦除次数（初始设为最大值）
	int free_page_num;             // 块的空闲页数量
	int temp_block;                // 临时遍历用的块索引
	int found = 0;                 // 是否找到符合条件的块标记

	// 根据操作类型获取初始活跃块（保留原有逻辑的起点）
	if (operation == WRITE) {
		temp_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block;
	} else {
		temp_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block;
	}

	// 遍历当前plane下的所有块（总数为block_plane）
	for (int i = 0; i < ssd->parameter->block_plane; i++) {
		// 计算当前遍历的块索引（从初始块开始，循环遍历所有块）
		int current_block = (temp_block + i) % ssd->parameter->block_plane;
		free_page_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[current_block].free_page_num;
		
		// 检查是否满足原循环条件：块为“空块”（free_page_num等于总页数）
		if (free_page_num == ssd->parameter->page_block) {
			// 获取当前块的擦除次数
			int current_erase = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[current_block].erase_count;
			
			// 筛选擦除次数最小的块（首次找到或当前块擦除次数更小）
			if (!found || current_erase < min_erase_count) {
				min_erase_count = current_erase;
				target_active_block = current_block;
				found = 1; // 标记已找到符合条件的块
			}
		}
	}

	// 处理遍历结果（如果找到符合条件的块）
	if (found) {
		// 更新块ID和活跃块指针（根据操作类型区分）
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[target_active_block].id = 1;
		if (operation == WRITE) {
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block = target_active_block;
		} else {
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block = target_active_block;
		}
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count++;
	} else {
		// 未找到符合条件的块（可根据需求添加异常处理，如触发GC等）
		// 此处可保留原逻辑中的debug_count记录或错误处理
	}
}

Status find_hot_active_block(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int operation)
{
	unsigned int r_active_block,w_active_block;
	unsigned int r_free_page_num=0,w_free_page_num=0;
	unsigned int count=0,debug_count=0,count1=0,count2=0;
	unsigned int gc_processing=FALSE;
	unsigned int r_id=0,w_id=0;
	int m=0,n=0,k=0;
	int MAX_WRITE_COUNT= (int)(ssd->parameter->page_block * ssd->write_ratio);
	int MAX_READ_COUNT= (int)((ssd->parameter->page_block * ssd->read_ratio)+1);
	

	static int window=0;
	static int hot_r_num=0;
	if(operation == READ){
		hot_r_num++;
	}
	window++;
	

	//每来10000次热请求调整热读热写比例----根据热读占前一万次all请求的比例调整下一次时间窗口内的块中的读写比例
	if(window >= 10000){
		ssd->current_read_ratio=(float)hot_r_num/window;
		ssd->read_ratio = 0.5*ssd->last_read_ratio + 0.5*ssd->current_read_ratio;
		//printf("%f\n",ssd->read_ratio);
		//while(1){};
		//控制read ratio在2%-10%之间
		if(ssd->read_ratio < 0.02){
			ssd->read_ratio = 0.02;
		}
		if(ssd->read_ratio > 0.1){
			ssd->read_ratio = 0.1;
		}
		ssd->write_ratio = 1.0 - ssd->read_ratio;
		ssd->last_read_ratio = ssd->read_ratio;

		window=0;
		hot_r_num=0;
	}

	w_active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block;
	w_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].free_page_num;
	w_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].id;

	r_active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block;
	r_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].free_page_num;
	r_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].id;
	
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count < MAX_CAPACITY){  //块池没满
		if(operation == WRITE){
			while(((w_free_page_num==0)||(w_id!=1)||(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].write_page_count >= MAX_WRITE_COUNT))&&(count<ssd->parameter->block_plane))   //找有空闲页的热块且写部分没有写满的
			{
				w_active_block=(w_active_block+1)%ssd->parameter->block_plane;
				w_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].free_page_num;
				w_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].id;
				count++;
			}
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block=w_active_block;  //写请求更新的块
		}
		else
		{  //迁热读页找有空闲页的热块且读部分没有迁满的

			while(((r_free_page_num==0)||(r_id!=1)||(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].read_page_count >= MAX_READ_COUNT))&&(count<ssd->parameter->block_plane))  
			{
				r_active_block=(r_active_block+1)%ssd->parameter->block_plane;
				r_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].free_page_num;
				r_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].id;
				count++;
			}  
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block=r_active_block;  //迁移读请求更新的块
		}


		if(count<ssd->parameter->block_plane)
		{
			if(operation == WRITE){
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].write_page_count++;
				// ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].write_page++;
			}else{
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].read_page_count++;
				ssd->hot_block_count_r1++;//读迁已有
			}
			return SUCCESS;
		}
		else //申请
		{
			if(operation == WRITE){ 
				w_active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block;
				w_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].free_page_num;
				w_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].id;
				while((w_free_page_num!=ssd->parameter->page_block)&&(debug_count<ssd->parameter->block_plane))  //找一个新的空块
				{
					w_active_block=(w_active_block+1)%ssd->parameter->block_plane;
					w_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].free_page_num;
					debug_count++;
				}
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].id=1;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block=w_active_block;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count++; //
			}
			else
			{
				r_active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block;
				r_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].free_page_num;
				r_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].id;
				while((r_free_page_num!=ssd->parameter->page_block)&&(debug_count<ssd->parameter->block_plane))  //找一个新的空块
				{
					r_active_block=(r_active_block+1)%ssd->parameter->block_plane;
					r_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].free_page_num;
					debug_count++;
				}
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].id=1;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block=r_active_block;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count++; //
			}
			//get_new_block_with_min_erase_count(ssd,channel,chip,die,plane,operation);

			if(debug_count<ssd->parameter->block_plane)
			{
				if(operation == WRITE){
					ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].write_page_count++;
					// ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].write_page++;
					ssd->hot_block_count_w++; //写申请
				}else{
					ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].read_page_count++;
					ssd->hot_block_count_r2++; //读申请
				}
				return SUCCESS;
			}
			else
			{
				return FAILURE;
			}
		}
	}
	else  //块池满了
	{
		if(operation==WRITE){
			while(((w_free_page_num==0)||(w_id!=1))&&(count1<ssd->parameter->block_plane)){  //找一个有空闲页的热块写
				w_active_block=(w_active_block+1) % ssd->parameter->block_plane;
				w_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].free_page_num;
				w_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].id;
				count1++;
			}
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block=w_active_block; //写指针更新
		}
		else{
			while(((r_free_page_num==0)||(r_id!=1))&&(count1<ssd->parameter->block_plane)){ 
				r_active_block=(r_active_block+1) % ssd->parameter->block_plane;
				r_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].free_page_num;
				r_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].id;
				count1++;
			}
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block=r_active_block; //读指针更新
		}
		
		if(count1<ssd->parameter->block_plane)
		{
			if(operation == WRITE){
				if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].write_page_count < MAX_WRITE_COUNT){
					ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].write_page_count++;
				}else{
					ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].upgrade_page_count++;
				}	
			}else{
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].read_page_count++;
			}
			return SUCCESS;
		}
		else
		{
			//申请不到了，选择将热块链表中，有效页最多,无效页最少。（或者遍历每个热块，找lpn在的块移除热块池，因为这表明他不热了，hot_blk_count--
			//然后我就该再申请一个新的热块，放入热块池里面，再放入新的写请求
			remove_hot_active_block(ssd,channel,chip,die,plane);
			ssd->remove++;

			if(operation == WRITE){
				w_active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block;
				w_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].free_page_num;
				w_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].id;
				while((w_free_page_num!=ssd->parameter->page_block)&&(debug_count<ssd->parameter->block_plane))  //找一个新的空块
				{
					w_active_block=(w_active_block+1)%ssd->parameter->block_plane;
					w_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].free_page_num;
					debug_count++;
				}
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].id=1;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].w_hot_active_block=w_active_block;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count++; //
			}
			else
			{
				r_active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block;
				r_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].free_page_num;
				r_id=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].id;
				while((r_free_page_num!=ssd->parameter->page_block)&&(debug_count<ssd->parameter->block_plane))  //找一个新的空块
				{
					r_active_block=(r_active_block+1)%ssd->parameter->block_plane;
					r_free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].free_page_num;
					debug_count++;
				}
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].id=1;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].r_hot_active_block=r_active_block;
				ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count++; //
			}
			//get_new_block_with_min_erase_count(ssd,channel,chip,die,plane,operation);

			if(debug_count<ssd->parameter->block_plane)
			{
				if(operation == WRITE){
					ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[w_active_block].write_page_count++;
					// ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].write_page++;
					ssd->hot_block_count_w++; //写申请
				}else{
					ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[r_active_block].read_page_count++;
					ssd->hot_block_count_r2++; //读申请
				}
				return SUCCESS;
			}
			else
			{
				return FAILURE;
			}
		}
	}
}


//移除无效页最少的块
Status remove_hot_active_block(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
	unsigned int invalid_num=0,block=0;
	for(int i=0;i < ssd->parameter->block_plane;i++){
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].id ==1){
			if( ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num < invalid_num){
				invalid_num = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
				block=i;
			}
		}
	}
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].id = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count--;
	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].hot_blk_count < 0){
		printf("remove %u,%u,%u,%u,%u",channel,chip,die,plane,block);
		while(1);
	}
	return SUCCESS;
}

struct sub_request * creat_sub_request(struct ssd_info * ssd,unsigned int lpn,int size,unsigned int state,struct request * req,unsigned int operation, unsigned int target_page_type)
{
	struct sub_request* sub=NULL,* sub_r=NULL;
	struct channel_info * p_ch=NULL;
	struct local * loc=NULL;
	unsigned int flag=0;

	sub = (struct sub_request*)malloc(sizeof(struct sub_request));                        /*����һ��������Ľṹ*/
	alloc_assert(sub,"sub_request");
	memset(sub,0, sizeof(struct sub_request));

	if(sub==NULL)
	{
		return NULL;
	}
	sub->location=NULL;
	sub->next_node=NULL;
	sub->next_subs=NULL;
	sub->update=NULL;
	sub->target_page_type = target_page_type;

	if(req!=NULL)
	{
		sub->next_subs = req->subs;
		req->subs = sub;
	}
	

	if (operation == READ)
	{	
		ssd->total_sub_read_count++;
		loc = find_location(ssd,ssd->dram->map->map_entry[lpn].pn);
		sub->location=loc;
		sub->begin_time = ssd->current_time;
		sub->current_state = SR_WAIT;
		sub->current_time=MAX_INT64;
		sub->next_state = SR_R_C_A_TRANSFER;
		sub->next_state_predict_time=MAX_INT64;
		sub->lpn = lpn;
		sub->size=size;                                                               /*��Ҫ�������������������С*/

		p_ch = &ssd->channel_head[loc->channel];	
		sub->ppn = ssd->dram->map->map_entry[lpn].pn;
		ssd->dram->map->map_entry[lpn].read_count++;
		sub->operation = READ;
		sub->state=(ssd->dram->map->map_entry[lpn].state&0x7fffffff);
		sub_r=p_ch->subs_r_head;                                                      /*һ�¼��а���flag�����жϸö�������������Ƿ����������������ͬ�ģ��еĻ������µ�������ֱ�Ӹ�Ϊ���*/
		flag=0;
		while (sub_r!=NULL)
		{
			if (sub_r->ppn==sub->ppn)
			{
				flag=1;
				break;
			}
			sub_r=sub_r->next_node;
		}
		if (flag==0)
		{
			if (p_ch->subs_r_tail!=NULL)
			{
				p_ch->subs_r_tail->next_node=sub;
				p_ch->subs_r_tail=sub;
			} 
			else
			{
				p_ch->subs_r_head=sub;
				p_ch->subs_r_tail=sub;
			}
		}
		else
		{
			ssd->direct_read_count++;
			sub->current_state = SR_R_DATA_TRANSFER;
			sub->current_time=ssd->current_time;
			sub->next_state = SR_COMPLETE;
			sub->next_state_predict_time=ssd->current_time+1000;
			sub->complete_time=ssd->current_time+1000;
		}
	}

	else if(operation == WRITE)
	{ 
		//sub->flag = req->flag;                               
		sub->ppn=0;
		sub->operation = WRITE;
		sub->location=(struct local *)malloc(sizeof(struct local));
		alloc_assert(sub->location,"sub->location");
		memset(sub->location,0, sizeof(struct local));

		sub->current_state=SR_WAIT;
		sub->current_time=ssd->current_time;
		sub->lpn=lpn;
		sub->size=size;
		sub->state=state;
		sub->begin_time=ssd->current_time;
      
		if (allocate_location(ssd ,sub)==ERROR)
		{
			free(sub->location);
			sub->location=NULL;
			free(sub);
			sub=NULL;
			return NULL;
		}
			
	}
	else
	{
		free(sub->location);
		sub->location=NULL;
		free(sub);
		sub=NULL;
		printf("\nERROR ! Unexpected command.\n");
		return NULL;
	}
	
	return sub;
}
struct sub_request * find_write_sub_request(struct ssd_info * ssd, unsigned int channel)
{
	struct sub_request * sub=NULL,* p=NULL;
	if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))    /*����ȫ�Ķ�̬����*/
	{
		sub=ssd->subs_w_head;
		while(sub!=NULL)        							
		{
			if(sub->current_state==SR_WAIT)								
			{
				if (sub->update!=NULL)                                                      /*�������Ҫ��ǰ������ҳ*/
				{
					if ((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))   //�����µ�ҳ�Ѿ�������
					{
						break;
					}
				} 
				else
				{
					break;
				}						
			}
			p=sub;
			sub=sub->next_node;							
		}

		if (sub==NULL)                                                                      /*���û���ҵ����Է�����������������forѭ��*/
		{
			return NULL;
		}

		if (sub!=ssd->subs_w_head)
		{
			if (sub!=ssd->subs_w_tail)
			{
				p->next_node=sub->next_node;
			}
			else
			{
				ssd->subs_w_tail=p;
				ssd->subs_w_tail->next_node=NULL;
			}
		} 
		else
		{
			if (sub->next_node!=NULL)
			{
				ssd->subs_w_head=sub->next_node;
			} 
			else
			{
				ssd->subs_w_head=NULL;
				ssd->subs_w_tail=NULL;
			}
		}
		sub->next_node=NULL;
		if (ssd->channel_head[channel].subs_w_tail!=NULL)
		{
			ssd->channel_head[channel].subs_w_tail->next_node=sub;
			ssd->channel_head[channel].subs_w_tail=sub;
		} 
		else
		{
			ssd->channel_head[channel].subs_w_tail=sub;
			ssd->channel_head[channel].subs_w_head=sub;
		}
	}
	/**********************************************************
	*����ȫ��̬���䷽ʽ��������ʽ�������Ѿ����䵽�ض���channel��
	*��ֻ��Ҫ��channel���ҳ�׼�������������
	***********************************************************/
	else            
	{
		sub=ssd->channel_head[channel].subs_w_head;
		while(sub!=NULL)        						
		{
			if(sub->current_state==SR_WAIT)								
			{
				if (sub->update!=NULL)    
				{
					if ((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))   //�����µ�ҳ�Ѿ�������
					{
						break;
					}
				} 
				else
				{
					break;
				}						
			}
			p=sub;
			sub=sub->next_node;							
		}

		if (sub==NULL)
		{
			return NULL;
		}
	}
	
	return sub;
}
struct sub_request * find_read_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die)
{
	unsigned int plane=0;
	unsigned int address_ppn=0;
	struct sub_request *sub=NULL,* p=NULL;

	for(plane=0;plane<ssd->parameter->plane_die;plane++)
	{
		address_ppn=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].add_reg_ppn;
		if(address_ppn!=-1)
		{
			sub=ssd->channel_head[channel].subs_r_head;
			if(sub->ppn==address_ppn)
			{
				if(sub->next_node==NULL)
				{
					ssd->channel_head[channel].subs_r_head=NULL;
					ssd->channel_head[channel].subs_r_tail=NULL;
				}
				ssd->channel_head[channel].subs_r_head=sub->next_node;
			}
			while((sub->ppn!=address_ppn)&&(sub->next_node!=NULL))
			{
				if(sub->next_node->ppn==address_ppn)
				{
					p=sub->next_node;
					if(p->next_node==NULL)
					{
						sub->next_node=NULL;
						ssd->channel_head[channel].subs_r_tail=sub;
					}
					else
					{
						sub->next_node=p->next_node;
					}
					sub=p;
					break;
				}
				sub=sub->next_node;
			}
			if(sub->ppn==address_ppn)
			{
				sub->next_node=NULL;
				return sub;
			}
			else 
			{
				printf("Error! Can't find the sub request.");
			}
		}
	}
	return NULL;
}
Status go_one_step(struct ssd_info * ssd, struct sub_request * sub1,struct sub_request *sub2, unsigned int aim_state,unsigned int command) 
{
	unsigned int i = 0, j = 0, k = 0, m = 0;
	float l = 0;
	long long RRtime = 0;
	long long time=0;

	struct sub_request * sub=NULL ; 
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
	struct local * location=NULL;
	if(sub1==NULL)
	{
		return ERROR;
	}
	if(command==NORMAL)
	{
		sub=sub1;
		location=sub1->location;
		switch(aim_state)						
		{	
			case SR_R_READ:
			{   
				ssd->total_read_count++;
				sub->current_time=ssd->current_time;
				sub->current_state=SR_R_READ;
				sub->next_state=SR_R_DATA_TRANSFER;

				//这条被注释掉了，在RR中
				//sub->next_state_predict_time=ssd->current_time+ssd->parameter->time_characteristics.tR;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_READ_BUSY;
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_DATA_TRANSFER;
				//这条被注释掉了，再RR中，不理解
				//ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+ssd->parameter->time_characteristics.tR;
				

				//newadded  RR，这些值都是通过多项式拟合算出来的,这里是和读错误相关
				int temp_erase_count=0;
				temp_erase_count=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].erase_count;
				ssd->error_read_count++;
				ssd->temp_read_error_rate=ssd->read_error_rate;

				//根据SSD的擦除次数和读取次数估算ssd的读取错误率，根据公式读次数的单位为k
				if (temp_erase_count> 4000) {
					//sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R_L1;
					//ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R_L1;
					//index1++;

					if(temp_erase_count>4000&&temp_erase_count<=5000){
						ssd->read_error_rate+=1.163+0.415*((double)(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].read_count))/1000;
					}else if(temp_erase_count>5000&&temp_erase_count<=6000){
						ssd->read_error_rate+=1.116+0.459*((double)(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].read_count))/1000;
					}else if(temp_erase_count>6000&&temp_erase_count<=7000){
						ssd->read_error_rate+=1.328+0.451*((double)(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].read_count))/1000;
					}else if(temp_erase_count>7000){
						ssd->read_error_rate+=2.219+0.370*((double)(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].read_count))/1000;
					}
							
							
				}
				else {
					//ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R_L2;
					//sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R_L2;
					//index2++;
					//newadded  RR		
					if(temp_erase_count<=1000){
						ssd->read_error_rate+=0.557+0.129*((double)(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].read_count))/1000;
					}else if(temp_erase_count>1000&&temp_erase_count<=2000){
						ssd->read_error_rate+=0.811+0.175*((double)(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].read_count))/1000;
					}else if(temp_erase_count>2000&&temp_erase_count<=3000){
						ssd->read_error_rate+=1.073+0.252*((double)(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].read_count))/1000;
					}else if(temp_erase_count>3000&&temp_erase_count<=4000){
						ssd->read_error_rate+=1.193+0.339*((double)(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].read_count))/1000;
					}
							
				}
				// newadded  RR
				// 根据不同的读取错误率计算本次读取操作所需要的时间（纠错时间）x
				if(ssd->read_error_rate-ssd->temp_read_error_rate<5){
					sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R;//t_R是指从闪存中读取的时间
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R;
					ssd->read_count_fre1++;
				}else if(ssd->read_error_rate-ssd->temp_read_error_rate>=5 && ssd->read_error_rate-ssd->temp_read_error_rate<6){
					ssd->read_count_fre2++;
					sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*2+24000;
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*2+24000;
					ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count++;
				}else if(ssd->read_error_rate-ssd->temp_read_error_rate>=6 && ssd->read_error_rate-ssd->temp_read_error_rate<8){
					ssd->read_count_fre3++;
					sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*3+24000*3;
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*3+24000*3;
					ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+2;
				}else if(ssd->read_error_rate-ssd->temp_read_error_rate>=8 && ssd->read_error_rate-ssd->temp_read_error_rate<9){
					ssd->read_count_fre4++;
					sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*4+24000*6;
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*4+24000*6;
					ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+3;
				}else if(ssd->read_error_rate-ssd->temp_read_error_rate>=9 && ssd->read_error_rate-ssd->temp_read_error_rate<10){
					ssd->read_count_fre5++;
					sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*5+24000*10;
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*5+24000*10;
					ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+4;
				}else if(ssd->read_error_rate-ssd->temp_read_error_rate>=10 && ssd->read_error_rate-ssd->temp_read_error_rate<12){
					ssd->read_count_fre6++;
					sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*6+24000*15;
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*6+24000*15;
					ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+5;
				}else if(ssd->read_error_rate-ssd->temp_read_error_rate>=12){
					ssd->read_count_fre7++;
				//else if(ssd->read_error_rate-ssd->temp_read_error_rate>=12 && ssd->read_error_rate-ssd->temp_read_error_rate<13){
					sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*7+24000*21;
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*7+24000*21;
					ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+6;
				}	
				// if(ssd->read_error_rate-ssd->temp_read_error_rate<5){
				// 	sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R;//t_R是指从闪存中读取的时间
				// 	ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R;
				// 	ssd->read_count_fre1++;
				// }else if(ssd->read_error_rate-ssd->temp_read_error_rate>=5 && ssd->read_error_rate-ssd->temp_read_error_rate<6){
				// 	ssd->read_count_fre2++;
				// 	sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*2+24000;
				// 	ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*2+24000;
				// 	ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count++;
				// }else if(ssd->read_error_rate-ssd->temp_read_error_rate>=6 && ssd->read_error_rate-ssd->temp_read_error_rate<8){
				// 	ssd->read_count_fre3++;
				// 	sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*3+24000*3;
				// 	ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*3+24000*3;
				// 	ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+2;
				// }else if(ssd->read_error_rate-ssd->temp_read_error_rate>=8 && ssd->read_error_rate-ssd->temp_read_error_rate<9){
				// 	ssd->read_count_fre4++;
				// 	sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*4+24000*6;
				// 	ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*4+24000*6;
				// 	ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+3;
				// }else if(ssd->read_error_rate-ssd->temp_read_error_rate>=9 && ssd->read_error_rate-ssd->temp_read_error_rate<10){
				// 	ssd->read_count_fre5++;
				// 	sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*5+24000*10;
				// 	ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*5+24000*10;
				// 	ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+4;
				// }else if(ssd->read_error_rate-ssd->temp_read_error_rate>=10 && ssd->read_error_rate-ssd->temp_read_error_rate<12){
				// 	ssd->read_count_fre6++;
				// 	sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*6+24000*15;
				// 	ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*6+24000*15;
				// 	ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+5;
				// }else if(ssd->read_error_rate-ssd->temp_read_error_rate>=12){
				// 	ssd->read_count_fre7++;
				// //else if(ssd->read_error_rate-ssd->temp_read_error_rate>=12 && ssd->read_error_rate-ssd->temp_read_error_rate<13){
				// 	sub->next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*7+24000*21;
				// 	ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time = ssd->current_time + ssd->parameter->time_characteristics.t_R*7+24000*21;
				// 	ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count+6;
				// }
				if(ssd->read_error_rate-ssd->temp_read_error_rate>ssd->max_read_error_rate){
					ssd->max_read_error_rate=ssd->read_error_rate-ssd->temp_read_error_rate;
				}
				break;
			}
			case SR_R_C_A_TRANSFER:
			{   
				/*******************************************************************************************************
				*Ŀ��״̬�������ַ����ʱ��sub����һ��״̬����SR_R_READ
				*���״̬��channel��chip�йأ�����Ҫ�޸�channel��chip��״̬�ֱ�ΪCHANNEL_C_A_TRANSFER��CHIP_C_A_TRANSFER
				*��һ״̬�ֱ�ΪCHANNEL_IDLE��CHIP_READ_BUSY
				*******************************************************************************************************/
				sub->current_time=ssd->current_time;									
				sub->current_state=SR_R_C_A_TRANSFER;									
				sub->next_state=SR_R_READ;									
				sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;									
				sub->begin_time=ssd->current_time;

				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=sub->ppn;
				ssd->read_count++;

				ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
				ssd->channel_head[location->channel].current_time=ssd->current_time;										
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
				ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;
				
				break;
			
			}
			case SR_R_DATA_TRANSFER:
			{   
				/**************************************************************************************************************
				*Ŀ��״̬�����ݴ���ʱ��sub����һ��״̬�������״̬SR_COMPLETE
				*���״̬�Ĵ���Ҳ��channel��chip�йأ�����channel��chip�ĵ�ǰ״̬��ΪCHANNEL_DATA_TRANSFER��CHIP_DATA_TRANSFER
				*��һ��״̬�ֱ�ΪCHANNEL_IDLE��CHIP_IDLE��
				***************************************************************************************************************/
				sub->current_time=ssd->current_time;					
				sub->current_state=SR_R_DATA_TRANSFER;		
				sub->next_state=SR_COMPLETE;				
				sub->next_state_predict_time=ssd->current_time+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub->complete_time=sub->next_state_predict_time;

				ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
				ssd->channel_head[location->channel].current_time=ssd->current_time;		
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
				ssd->channel_head[location->channel].next_state_predict_time=sub->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;

				break;
			}
			case SR_W_TRANSFER:
			{
				sub->current_time=ssd->current_time;
				sub->current_state=SR_W_TRANSFER;
				sub->next_state=SR_COMPLETE;
				sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				//sub->complete_time=sub->next_state_predict_time;		
				time=sub->next_state_predict_time;
				
				ssd->channel_head[location->channel].current_state=CHANNEL_TRANSFER;										
				ssd->channel_head[location->channel].current_time=ssd->current_time;										
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;										
				ssd->channel_head[location->channel].next_state_predict_time=time;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_WRITE_BUSY;										
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;									
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;
				//***************************************************************************
				//ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG;
				if (sub->ppn%3 == 0){
					//printf("LSB\n");
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG_L;
					}
				else if(sub->ppn%3 == 2){
					//printf("MSB\n");
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG_M;
					}
				else{
					ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG_C;
					}
				sub->next_state_predict_time=ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time;
				sub->complete_time=sub->next_state_predict_time;
				//***************************************************************************
				break;
			}
			default :  return ERROR;
			
		}//switch(aim_state)	
	}//if(command==NORMAL)
	else if(command==TWO_PLANE)
	{   
		/**********************************************************************************************
		*�߼�����TWO_PLANE�Ĵ����������TWO_PLANE�߼������Ƕ�������ĸ߼�����
		*״̬ת������ͨ����һ������ͬ������SR_R_C_A_TRANSFERʱ����ʱ���Ǵ��еģ���Ϊ����һ��ͨ��channel
		*����SR_R_DATA_TRANSFERҲ�ǹ���һ��ͨ��
		**********************************************************************************************/
		if((sub1==NULL)||(sub2==NULL))
		{
			return ERROR;
		}
		sub_twoplane_one=sub1;
		sub_twoplane_two=sub2;
		location=sub1->location;
		
		switch(aim_state)						
		{	
			case SR_R_C_A_TRANSFER:
			{
				sub_twoplane_one->current_time=ssd->current_time;									
				sub_twoplane_one->current_state=SR_R_C_A_TRANSFER;									
				sub_twoplane_one->next_state=SR_R_READ;									
				sub_twoplane_one->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;									
				sub_twoplane_one->begin_time=ssd->current_time;

				ssd->channel_head[sub_twoplane_one->location->channel].chip_head[sub_twoplane_one->location->chip].die_head[sub_twoplane_one->location->die].plane_head[sub_twoplane_one->location->plane].add_reg_ppn=sub_twoplane_one->ppn;
				ssd->read_count++;

				sub_twoplane_two->current_time=ssd->current_time;									
				sub_twoplane_two->current_state=SR_R_C_A_TRANSFER;									
				sub_twoplane_two->next_state=SR_R_READ;									
				sub_twoplane_two->next_state_predict_time=sub_twoplane_one->next_state_predict_time;									
				sub_twoplane_two->begin_time=ssd->current_time;

				ssd->channel_head[sub_twoplane_two->location->channel].chip_head[sub_twoplane_two->location->chip].die_head[sub_twoplane_two->location->die].plane_head[sub_twoplane_two->location->plane].add_reg_ppn=sub_twoplane_two->ppn;
				ssd->read_count++;
				ssd->m_plane_read_count++;

				ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
				ssd->channel_head[location->channel].current_time=ssd->current_time;										
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
				ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

				
				break;
			}
			case SR_R_DATA_TRANSFER:
			{
				sub_twoplane_one->current_time=ssd->current_time;					
				sub_twoplane_one->current_state=SR_R_DATA_TRANSFER;		
				sub_twoplane_one->next_state=SR_COMPLETE;				
				sub_twoplane_one->next_state_predict_time=ssd->current_time+(sub_twoplane_one->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub_twoplane_one->complete_time=sub_twoplane_one->next_state_predict_time;
				
				sub_twoplane_two->current_time=sub_twoplane_one->next_state_predict_time;					
				sub_twoplane_two->current_state=SR_R_DATA_TRANSFER;		
				sub_twoplane_two->next_state=SR_COMPLETE;				
				sub_twoplane_two->next_state_predict_time=sub_twoplane_two->current_time+(sub_twoplane_two->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub_twoplane_two->complete_time=sub_twoplane_two->next_state_predict_time;
				
				ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
				ssd->channel_head[location->channel].current_time=ssd->current_time;		
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
				ssd->channel_head[location->channel].next_state_predict_time=sub_twoplane_one->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub_twoplane_one->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;
			
				break;
			}
			default :  return ERROR;
		}//switch(aim_state)	
	}//else if(command==TWO_PLANE)
	else if(command==INTERLEAVE)
	{
		if((sub1==NULL)||(sub2==NULL))
		{
			return ERROR;
		}
		sub_interleave_one=sub1;
		sub_interleave_two=sub2;
		location=sub1->location;
		
		switch(aim_state)						
		{	
			case SR_R_C_A_TRANSFER:
			{
				sub_interleave_one->current_time=ssd->current_time;									
				sub_interleave_one->current_state=SR_R_C_A_TRANSFER;									
				sub_interleave_one->next_state=SR_R_READ;									
				sub_interleave_one->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;									
				sub_interleave_one->begin_time=ssd->current_time;

				ssd->channel_head[sub_interleave_one->location->channel].chip_head[sub_interleave_one->location->chip].die_head[sub_interleave_one->location->die].plane_head[sub_interleave_one->location->plane].add_reg_ppn=sub_interleave_one->ppn;
				ssd->read_count++;

				sub_interleave_two->current_time=ssd->current_time;									
				sub_interleave_two->current_state=SR_R_C_A_TRANSFER;									
				sub_interleave_two->next_state=SR_R_READ;									
				sub_interleave_two->next_state_predict_time=sub_interleave_one->next_state_predict_time;									
				sub_interleave_two->begin_time=ssd->current_time;

				ssd->channel_head[sub_interleave_two->location->channel].chip_head[sub_interleave_two->location->chip].die_head[sub_interleave_two->location->die].plane_head[sub_interleave_two->location->plane].add_reg_ppn=sub_interleave_two->ppn;
				ssd->read_count++;
				ssd->interleave_read_count++;

				ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
				ssd->channel_head[location->channel].current_time=ssd->current_time;										
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
				ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;
				
				break;
						
			}
			case SR_R_DATA_TRANSFER:
			{
				sub_interleave_one->current_time=ssd->current_time;					
				sub_interleave_one->current_state=SR_R_DATA_TRANSFER;		
				sub_interleave_one->next_state=SR_COMPLETE;				
				sub_interleave_one->next_state_predict_time=ssd->current_time+(sub_interleave_one->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub_interleave_one->complete_time=sub_interleave_one->next_state_predict_time;
				
				sub_interleave_two->current_time=sub_interleave_one->next_state_predict_time;					
				sub_interleave_two->current_state=SR_R_DATA_TRANSFER;		
				sub_interleave_two->next_state=SR_COMPLETE;				
				sub_interleave_two->next_state_predict_time=sub_interleave_two->current_time+(sub_interleave_two->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub_interleave_two->complete_time=sub_interleave_two->next_state_predict_time;

				ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
				ssd->channel_head[location->channel].current_time=ssd->current_time;		
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
				ssd->channel_head[location->channel].next_state_predict_time=sub_interleave_two->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub_interleave_two->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;
				
				break;
			}
			default :  return ERROR;	
		}//switch(aim_state)				
	}//else if(command==INTERLEAVE)
	else
	{
		printf("\nERROR: Unexpected command !\n" );
		return ERROR;
	}

	return SUCCESS;
}
Status static_write(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub)
{
	long long time=0;
	if (ssd->dram->map->map_entry[sub->lpn].state!=0)                                    /*˵������߼�ҳ֮ǰ��д������Ҫʹ���ȶ���������д��ȥ������ֱ��д��ȥ����*/
	{
		if ((sub->state&ssd->dram->map->map_entry[sub->lpn].state)==ssd->dram->map->map_entry[sub->lpn].state)   /*���Ը���*/
		{
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		} 
		else
		{
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(size((ssd->dram->map->map_entry[sub->lpn].state^sub->state)))*ssd->parameter->time_characteristics.tRC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
			ssd->read_count++;
			ssd->update_read_count++;
		}
	} 
	else
	{
		sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
	}
	sub->complete_time=sub->next_state_predict_time;		
	time=sub->complete_time;

	get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
	ssd->channel_head[channel].next_state_predict_time=time;

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;
	//**************************************************************
	int new_ppn;
	new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
	//ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG;
	if (new_ppn%2 == 0)
		{
			ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG_L;
			//printf("LSB programming\n");
		}
	else
		{
			ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG_M;
			//printf("MSB programming\n");
		}
	//****************************************************************
	return SUCCESS;
}
Status services_2_r_data_trans(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag)
{
	int chip=0;
	unsigned int die=0,plane=0,address_ppn=0,die1=0;
	struct sub_request * sub=NULL, * p=NULL,*sub1=NULL;
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;

	// RR new added
	struct gc_operation *gc_node;
	struct local *  location=NULL;
	unsigned int block,active_block,transfer_size = 0,free_page,page_move_count=0;//gc
	unsigned int i=0;
	int block_plane=0,block_die=0,block_chip=0,block_channel=0;
	int wl_channel,wl_chip,wl_die,wl_plane,wl_block;
	unsigned int ppn;
	unsigned int hot_page_count=0;
	unsigned int valid_page_count=0;
	int count=0;
	unsigned char channelChangFlag = 0;
	unsigned char dieChangFlag = 0;
	int64_t channelpre;
	int64_t diepre;
	struct direct_erase *direct_erase_node,*new_direct_erase;
	int cnt=0;
	int id=0;
	unsigned int free_page1=0;
	unsigned int invalid_page1=0;
	unsigned int read_count1=0;


	for(chip=0;chip<ssd->channel_head[channel].chip;chip++)           			    
	{				       		      
			if((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_WAIT)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_DATA_TRANSFER)&&
				(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time)))					       					
			{
				for(die=0;die<ssd->parameter->die_chip;die++)
				{
					sub=find_read_sub_request(ssd,channel,chip,die);                   
					if(sub!=NULL)
					{
						break;
					}
				}

				if(sub==NULL)
				{
					continue;
				}
				
				if(((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)==AD_TWOPLANE_READ)||((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
				{
					if ((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)==AD_TWOPLANE_READ)   
					{
						sub_twoplane_one=sub;
						sub_twoplane_two=NULL;                                                     
						ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[sub->location->plane].add_reg_ppn=-1;
						sub_twoplane_two=find_read_sub_request(ssd,channel,chip,die);               /*����ͬ��channel,chip,die��Ѱ������һ����������*/
						
						if (sub_twoplane_two==NULL)
						{
							go_one_step(ssd, sub_twoplane_one,NULL, SR_R_DATA_TRANSFER,NORMAL);   
							*change_current_time_flag=0;   
							*channel_busy_flag=1;

						}
						else
						{
							go_one_step(ssd, sub_twoplane_one,sub_twoplane_two, SR_R_DATA_TRANSFER,TWO_PLANE);
							*change_current_time_flag=0;  
							*channel_busy_flag=1;

						}
					} 
					else if ((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)     
					{
						sub_interleave_one=sub;
						sub_interleave_two=NULL;
						ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[sub->location->plane].add_reg_ppn=-1;
						
						for(die1=0;die1<ssd->parameter->die_chip;die1++)
						{	
							if(die1!=die)
							{
								sub_interleave_two=find_read_sub_request(ssd,channel,chip,die1);    /*����ͬ��channel��chhip��ͬ��die����������һ����������*/
								if(sub_interleave_two!=NULL)
								{
									break;
								}
							}
						}	
						if (sub_interleave_two==NULL)
						{
							go_one_step(ssd, sub_interleave_one,NULL, SR_R_DATA_TRANSFER,NORMAL);

							*change_current_time_flag=0;  
							*channel_busy_flag=1;

						}
						else
						{
							go_one_step(ssd, sub_twoplane_one,sub_interleave_two, SR_R_DATA_TRANSFER,INTERLEAVE);
												
							*change_current_time_flag=0;   
							*channel_busy_flag=1;
							
						}
					}
				}
				else                                                                                 
				{
					unsigned int  temp_lpn;
					go_one_step(ssd, sub,NULL, SR_R_DATA_TRANSFER,NORMAL);
					*change_current_time_flag=0;  
					*channel_busy_flag=1;
					int erase_flag = 0;
					ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count++;
					ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].page_head[sub->location->page].read_count++;
				
	
				
					
					if(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count >= BRT)
					{
						if(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].id==1)
						{
							ssd->rr_erase_1++;
							// FILE *temp2=fopen("./statisticfile/rr_erase1_info.txt","a");
		                    // fprintf(temp2,"free page= %u,invalid page= %u,read page count= %u,write page count= %u,read count= %u\n",ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].free_page_num,ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].invalid_page_num,ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_page_count,ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].write_page_count,ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count);
							// fclose(temp2);
						}
						else
						{
							ssd->rr_erase_0++;
						}
						// if (ssd->rr_trace_flag == 0)
						// {
						// 	FILE *temp = fopen("begain_rr_trace_times.txt","w");
						// 	fprintf(temp,"%d",ssd->rr_trace_times);
						// 	fclose(temp);
						// 	ssd->rr_trace_flag == 1;
						// }

						id=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].id;
						free_page1=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].free_page_num;
						invalid_page1=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].invalid_page_num;
						read_count1=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count;

						for(i=0;i<ssd->parameter->page_block;i++)		                                                    	
						{		
							if(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].page_head[i].valid_state>0) 
							{	
								location=(struct local * )malloc(sizeof(struct local ));
								alloc_assert(location,"location");
								memset(location,0, sizeof(struct local));
								location->channel=sub->location->channel;
								location->chip=sub->location->chip;
								location->die=sub->location->die;
								location->plane=sub->location->plane;
								location->block=sub->location->block;
								location->page=i;
								move_page(ssd, location, &transfer_size, 0);                                              
								page_move_count++;
								cnt++;
								ssd->rr_page_move_count++;
								ssd->page_move_count++;
								free(location);	
								location=NULL;
							}				
						}
							
						// ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].free_page_num=0;
						
						// ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].last_write_page=ssd->parameter->page_block;
						
						// FILE *tmp1 = fopen("./statisticfile/rr_info.txt","a");
						// fprintf(tmp1,"id= %d, valid page= %d, free page= %u, invalid page= %u,read count= %u\n",id,cnt,free_page1,invalid_page1,read_count1);
						// fclose(tmp1);
						cnt=0;

						erase_operation(ssd,sub->location->channel ,sub->location->chip , sub->location->die,sub->location->plane ,sub->location->block);
						ssd->rr_erase_count++;
						ssd->debug_rr_count++;


						/*new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
						alloc_assert(new_direct_erase,"new_direct_erase");
						memset(new_direct_erase,0, sizeof(struct direct_erase));

						new_direct_erase->block=sub->location->block;
						new_direct_erase->rr_flag=1;
						new_direct_erase->next_node=NULL;
						direct_erase_node=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].erase_node;
						if (direct_erase_node==NULL)
						{
							ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].erase_node=new_direct_erase;
						} 
						else
						{
							new_direct_erase->next_node=ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].erase_node;
							ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].erase_node=new_direct_erase;
						}*/
						
						if((ssd->channel_head[sub->location->channel].current_state==CHANNEL_IDLE)||(ssd->channel_head[sub->location->channel].next_state==CHANNEL_IDLE&&ssd->channel_head[sub->location->channel].next_state_predict_time<=ssd->current_time)){
							ssd->channel_head[sub->location->channel].current_state=CHANNEL_GC;									
							ssd->channel_head[sub->location->channel].current_time=ssd->current_time;										
							ssd->channel_head[sub->location->channel].next_state=CHANNEL_IDLE;	
							channelChangFlag = 1;
						}
						if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time))){
							ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state=CHIP_ERASE_BUSY;								
							ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_time=ssd->current_time;						
							ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state=CHIP_IDLE;
							dieChangFlag = 1;
						}
						
						
						if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
						{
							if (ssd->parameter->greed_CB_ad==1)
							{
								ssd->channel_head[sub->location->channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG);			
								ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time=ssd->channel_head[sub->location->channel].next_state_predict_time+ssd->parameter->time_characteristics.tBERS;
							} 
						} 
						else
						{
							channelpre = page_move_count*(7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);
							diepre = channelpre;
							if(channelChangFlag)
								ssd->channel_head[sub->location->channel].next_state_predict_time = ssd->current_time+ channelpre;
							else 
								ssd->channel_head[sub->location->channel].next_state_predict_time += channelpre;
							
							if(dieChangFlag)
								ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time = ssd->current_time + channelpre + ssd->parameter->time_characteristics.tBERS;
							else 
								ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time = ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time + channelpre + ssd->parameter->time_characteristics.tBERS;
						}

						//clean
						// ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].read_count=0;
						// for(i=0;i<ssd->parameter->page_block;i++){
						// 	ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].die_head[sub->location->die].plane_head[sub->location->plane].blk_head[sub->location->block].page_head[i].read_count=0;
						// }
					}
				
						
				}
				break;
			}		
			
		if(*channel_busy_flag==1)
		{
			break;
		}
	}		
	return SUCCESS;
}
int delete_w_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub )
{
	struct sub_request * p=NULL;
	if (sub==ssd->channel_head[channel].subs_w_head)                                   /*������������channel������ɾ��*/
	{
		if (ssd->channel_head[channel].subs_w_head!=ssd->channel_head[channel].subs_w_tail)
		{
			ssd->channel_head[channel].subs_w_head=sub->next_node;
		} 
		else
		{
			ssd->channel_head[channel].subs_w_head=NULL;
			ssd->channel_head[channel].subs_w_tail=NULL;
		}
	}
	else
	{
		p=ssd->channel_head[channel].subs_w_head;
		while(p->next_node !=sub)
		{
			p=p->next_node;
		}

		if (sub->next_node!=NULL)
		{
			p->next_node=sub->next_node;
		} 
		else
		{
			p->next_node=NULL;
			ssd->channel_head[channel].subs_w_tail=p;
		}
	}
	
	return SUCCESS;	
}
int services_2_r_wait(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag)
{
	unsigned int plane=0,address_ppn=0;
	struct sub_request * sub=NULL, * p=NULL;
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;

	//RR newadded
	int bet_temp;
	struct gc_operation *gc_node;
	struct local *  location=NULL;
	unsigned int block,active_block,transfer_size,free_page,page_move_count=0;//gc
	unsigned int i=0;
	int block_plane=0,block_die=0,block_chip=0,block_channel=0;
	int wl_channel,wl_chip,wl_die,wl_plane,wl_block;
	unsigned int ppn;
	unsigned int hot_page_count=0;
	unsigned int valid_page_count=0;
	int count=0;
	unsigned char channelChangFlag = 0;
	unsigned char dieChangFlag = 0;
	int64_t channelpre;
	int64_t diepre;
	struct direct_erase *direct_erase_node,*new_direct_erase;
	struct data_group *hot_node,*new_node,*pt;
	//struct gc_operation *gc_node;
	
	sub=ssd->channel_head[channel].subs_r_head;

	
	if ((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)==AD_TWOPLANE_READ)         /*to find whether there are two sub request can be served by two plane operation*/
	{
		sub_twoplane_one=NULL;
		sub_twoplane_two=NULL;                                                         
		                                                                                /*Ѱ����ִ��two_plane��������������*/
		find_interleave_twoplane_sub_request(ssd,channel,sub_twoplane_one,sub_twoplane_two,TWO_PLANE);

		if (sub_twoplane_two!=NULL)                                                     /*����ִ��two plane read ����*/
		{
			go_one_step(ssd, sub_twoplane_one,sub_twoplane_two, SR_R_C_A_TRANSFER,TWO_PLANE);
						
			*change_current_time_flag=0;
			*channel_busy_flag=1;                                                       /*�Ѿ�ռ����������ڵ����ߣ�����ִ��die�����ݵĻش�*/
		} 
		else if((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)       /*û����������������page��������û��interleave read����ʱ��ֻ��ִ�е���page�Ķ�*/
		{
			while(sub!=NULL)                                                            /*if there are read requests in queue, send one of them to target die*/			
			{		
				if(sub->current_state==SR_WAIT)									
				{	                                                                    /*ע���¸�����ж�������services_2_r_data_trans���ж������Ĳ�ͬ
																						*/
					if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
						(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
					{	
						go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER,NORMAL);
									
						*change_current_time_flag=0;
						*channel_busy_flag=1;                                           /*�Ѿ�ռ����������ڵ����ߣ�����ִ��die�����ݵĻش�*/
						break;										
					}	
					else
					{
						                                                                /*��Ϊdie��busy���µ�����*/
					}
				}						
				sub=sub->next_node;								
			}
		}
	} 
	if ((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)               /*to find whether there are two sub request can be served by INTERLEAVE operation*/
	{
		sub_interleave_one=NULL;
		sub_interleave_two=NULL;
		find_interleave_twoplane_sub_request(ssd,channel,sub_interleave_one,sub_interleave_two,INTERLEAVE);
		
		if (sub_interleave_two!=NULL)                                                  /*����ִ��interleave read ����*/
		{

			go_one_step(ssd, sub_interleave_one,sub_interleave_two, SR_R_C_A_TRANSFER,INTERLEAVE);
						
			*change_current_time_flag=0;
			*channel_busy_flag=1;                                                      /*�Ѿ�ռ����������ڵ����ߣ�����ִ��die�����ݵĻش�*/
		} 
		else                                                                           /*û����������������page��ֻ��ִ�е���page�Ķ�*/
		{
			while(sub!=NULL)                                                           /*if there are read requests in queue, send one of them to target die*/			
			{		
				if(sub->current_state==SR_WAIT)									
				{	
					if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
						(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
					{	

						go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER,NORMAL);
									
						*change_current_time_flag=0;
						*channel_busy_flag=1;                                          /*�Ѿ�ռ����������ڵ����ߣ�����ִ��die�����ݵĻش�*/
						break;										
					}	
					else
					{
						                                                               /*��Ϊdie��busy���µ�����*/
					}
				}						
				sub=sub->next_node;								
			}
		}
	}

	/*******************************
	*ssd����ִ��ִ�и߼�����������
	*******************************/
	// 以下是不支持高级命令的情况
	if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)!=AD_TWOPLANE_READ))
	{
		while(sub!=NULL)                                                               /*if there are read requests in queue, send one of them to target chip*/			
		{		
			if(sub->current_state==SR_WAIT)									
			{	                                                                       
				if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
					(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
				{	
					
					int result=lRUCacheGet(ssd->r_data,sub->lpn);
					if(result == -1){
						lRUCachePut(ssd->r_data,sub->lpn,sub->lpn);
					}

					go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER,NORMAL);
							
					
					
					*change_current_time_flag=0;
					*channel_busy_flag=1;                                              /*�Ѿ�ռ����������ڵ����ߣ�����ִ��die�����ݵĻش�*/
					break;										
				}	
				else
				{
					                                                                   /*��Ϊdie��busy���µ�����*/
				}
			}						
			sub=sub->next_node;								
		}
	}

	return SUCCESS;
}
Status  find_active_block_rr(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,int valid_num)
{
	int same_flag1 = 0, same_flag2 = 0; 
	unsigned int active_block;
	unsigned int free_page_num=0;
	unsigned int count1=0,count2=0;
	//unsigned int gc_processing=FALSE;
	unsigned int active_block1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	unsigned int active_block2=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block2;
	int min_read_count;
	int min_erase_count ;
	//unsigned int temp_active_block = 0;
	//int read_count1 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].read_count;
	//int read_count2 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].read_count;
	unsigned int free_page_num1 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].free_page_num;
	//int  last_write1 =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].last_write_page;
	//int last_write2 =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].last_write_page;
	unsigned int free_page_num2 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].free_page_num;
	//free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	//gc_processing=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase;
	// if(ssd->same_block_flag == 1)
	// {
	// 	active_block=(active_block+1)%ssd->parameter->block_plane;	
	// 	free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	// 	//gc_processing=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase;
	// }
	//last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	//printf("erase_node: %d, %d, %d, %d, %d.\n",ssd->erase_channel, ssd->erase_chip, ssd->erase_die, ssd->erase_plane, ssd->erase_block);
	if(ssd->erase_channel==channel&&ssd->erase_chip==chip&&ssd->erase_die==die&&ssd->erase_plane==plane)
	{
		same_flag2=1;
		if(ssd->erase_block==active_block2)
			printf("same_flag2\n");
	}
	if(ssd->erase_channel==channel&&ssd->erase_chip==chip&&ssd->erase_die==die&&ssd->erase_plane==plane)
	{
		same_flag1=1;
		if(ssd->erase_block==active_block1)
			printf("same_flag1\n");
	}
	int min_block;
	if(ssd->same_block_flag2 == 1)
	{
		min_block = (active_block2+1)% ssd->parameter->block_plane;
		if (min_block == active_block1) min_block = (active_block2+1)% ssd->parameter->block_plane;
		min_erase_count =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].erase_count;
		min_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].read_count;
		for (int i = 1 ; i < ssd->parameter->block_plane ; i++)
		{
			active_block = (active_block2+i)%ssd->parameter->block_plane;
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num <= 0) continue;
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count < min_erase_count || (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count == min_erase_count && ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].read_count < min_read_count) || min_block == active_block1 || ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].free_page_num <= 0 || min_block == active_block2)
			// if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count < min_erase_count || min_block == active_block1 || ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].free_page_num <= 0 || min_block == active_block2)
			{
				min_erase_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count;
				min_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].read_count;
				min_block = active_block;
			}

		}
		//printf("\n更新前的active block2: %d, 更新后的 active block2: %d\n",active_block2,min_block);
		active_block2 = min_block;
		free_page_num2 =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].free_page_num;
		//printf("\n当前的freepagenum :%d", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].free_page_num);
		//printf ("\n当前的lastpage：%d", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].last_write_page);
		//last_write2 =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].last_write_page;
		
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block2 = active_block2;
		
	}
	
	if (ssd->same_block_flag == 1)
	{
		min_block = (active_block1+1)% ssd->parameter->block_plane;
		if (min_block == active_block2)  min_block =(active_block1+1)% ssd->parameter->block_plane;
		min_erase_count =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].erase_count;
		min_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].read_count;
		for (int i = 1 ; i < ssd->parameter->block_plane ; i++)
		{
			active_block = (active_block1+i)% ssd->parameter->block_plane;
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num <=  0) continue;
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count < min_erase_count || (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count == min_erase_count && ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].read_count < min_read_count)|| min_block == active_block2 || ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].free_page_num <= 0 || min_block == active_block1)
			// if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count < min_erase_count || min_block == active_block1 || ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].free_page_num <= 0 || min_block == active_block2)
			{
				min_erase_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count;
				min_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].read_count;
				min_block = active_block;
			}


		}
		active_block1 = min_block;
		free_page_num1 =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].free_page_num;
		//last_write1 =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].last_write_page;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block = active_block1;
	}
	
	min_block = (active_block2+1)%ssd->parameter->block_plane;
	if (min_block == active_block1) min_block = (active_block1+2)% ssd->parameter->block_plane;
	if (free_page_num2 <= 0 ||(active_block2==ssd->erase_block&&same_flag2==1))//如果活跃块写满了，就找一个新的
	{
		// printf("\n触发找新block2\n");
		int block_count = 0;
		min_erase_count =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].erase_count;
		min_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].read_count;
		for (int i = 1 ; i < ssd->parameter->block_plane ; i++)
		{
			
			active_block = (active_block2+i)%ssd->parameter->block_plane;
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num <= 0) continue;
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count < min_erase_count || (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count == min_erase_count && ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].read_count < min_read_count)|| min_block == active_block1 || ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].free_page_num <= 0||(min_block==ssd->erase_block&&same_flag2==1))
			// if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count < min_erase_count || min_block == active_block1 || ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].free_page_num <= 0 || min_block == active_block1||(min_block==ssd->erase_block&&same_flag2==1))
			{
				min_erase_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count;
				min_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].read_count;
				min_block = active_block;
			}
			count1++;
		}
		// printf("orignin_block2 : %d , new_block2 : %d", ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block2,min_block);
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block2 = min_block;
		active_block2 =min_block;
		// printf("\n新block2的freepagenum = %d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block2].free_page_num);
		
	}
	min_block = (active_block1+1)% ssd->parameter->block_plane;
	if (min_block == active_block2) min_block = (active_block1+1)% ssd->parameter->block_plane;
	// if(ssd->erase_channel==channel&&ssd->erase_chip==chip&&ssd->erase_die==die&&ssd->erase_plane==plane){
	// 	same_flag2=1;
	// 	if(ssd->erase_block==active_block1) printf("same_flag\n");
	// }
	if (free_page_num1 <= 0 ||(active_block1==ssd->erase_block&&same_flag1==1))
	{
		//printf("\n触发找新block1\n");
		int block_count = 0;
		min_erase_count =  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].erase_count;
		min_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].read_count;
		for (int i = 1 ; i < ssd->parameter->block_plane ; i++)
		{
			active_block = (active_block1+i)%ssd->parameter->block_plane;
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num <= 0) continue;
			if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count < min_erase_count || (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count == min_erase_count && ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].read_count < min_read_count) || min_block == active_block2||ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].free_page_num <= 0 ||(min_block==ssd->erase_block&&same_flag1==1))
			// if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count < min_erase_count || min_block == active_block1 || ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[min_block].free_page_num <= 0 || min_block == active_block2 || (min_block==ssd->erase_block&&same_flag1==1))
			{
				min_erase_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count;
				min_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].read_count;
				min_block = active_block;
			}
		}

		active_block1 = min_block;
		// printf("\n新block1的freepagenum = %d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block1].free_page_num);
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block = min_block;
	} 
	// printf("\n当前active_block1的free_page_num = %d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block].free_page_num);
	// printf("\n当前active_block2的free_page_num = %d\n",ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block2].free_page_num);
	
	// if (read_count2 >= erase_phase1)
	// {
	// 	if (read_count2 < erase_phase1) ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block
	
	// }
	// //last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	// while(((active_block == active_block2)||(free_page_num==0)||(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].erase_count>4000))&&(count<ssd->parameter->block_plane))
	// {
	// 	active_block=(active_block+1)%ssd->parameter->block_plane;	
	// 	free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	// 	//gc_processing=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].fast_erase;
	// 	count++;
	// }
	// // int min_activeblock = active_block;
	// for (int i = 0 ; i< ssd->parameter->block_plane ; i++)
	// {
	// 	temp_active_block = (active_block+i)%ssd->parameter->block_plane;
	// 	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[temp_active_block].Is_be_choosen == 1) continue;
	// 	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[temp_active_block].Is_migrate == 1) continue;
	// 	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[temp_active_block].free_page_num != ssd->parameter->page_block) continue;
	// 	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[temp_active_block].free_page_num == 0) continue;
	// 	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[temp_active_block].erase_count < min_erase_count)
	// 	{
	// 		min_activeblock = temp_active_block;
	// 		min_read_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[temp_active_block].read_count;
	// 		min_erase_count = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[temp_active_block].erase_count;
	// 	}
	// 
	// if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num == ssd->parameter->page_block)
	// {
	// 	ssd->find_free_block_count++;
	// 	printf("\n找的空块\n");

	// }
	// else
	// {
	// 	ssd->find_active_block_count++;
	// 	printf("找的非空块");
	// }
	// ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block=active_block;
	if(count1<ssd->parameter->block_plane && count2 <ssd->parameter->block_plane)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}
struct ssd_info *process(struct ssd_info *ssd)   
{
	int old_ppn=-1,flag_die=-1; 
	unsigned int i,chan,random_num,j,k;   
	unsigned int flag=0,new_write=0,chg_cur_time_flag=1,flag2=0,flag_gc=0;       
	int64_t time, channel_time=MAX_INT64;
	struct sub_request *sub;          

#ifdef DEBUG
	printf("enter process,  current time:%lld\n",ssd->current_time);
#endif
	for(i=0;i<ssd->parameter->channel_number;i++)
	{          
		if((ssd->channel_head[i].subs_r_head==NULL)&&(ssd->channel_head[i].subs_w_head==NULL)&&(ssd->subs_w_head==NULL))
		{
			flag=1;
		}
		else
		{
			flag=0;
			break;
		}
	}
	if(flag==1)
	{
		ssd->flag=1;                                                                
		if (ssd->gc_request>0)                                                            /*SSD����gc����������*/
		{
			gc(ssd,0,1);                                                                  /*���gcҪ������channel�����������*/
		}
		//new add RR 相关，优先做GC 再做RR
		else if(switch_on == 1&&ssd->rr_request>0&&ssd->completed_request_count-ssd->last_requests > 100000)
		{
			for(i=0;i<ssd->parameter->channel_number;i++){
				for(j=0;j<ssd->parameter->chip_channel[0];j++){
					for(k=0;k<ssd->parameter->die_chip;k++){
						if(ssd->channel_head[i].chip_head[j].die_head[k].rr_plane!=-1){
							printf("done one idle RR, completed requests:%d\n",ssd->completed_request_count);
							ssd->idle_flag=1;
							ssd->patial_rr_number=ssd->parameter->page_block;
							///  new added function
							action_for_PM(ssd, i, j, k, ssd->channel_head[i].chip_head[j].die_head[k].rr_plane, ssd->channel_head[i].chip_head[j].die_head[k].rr_block);
							ssd->idle_flag=0;
							ssd->last_requests=ssd->completed_request_count;
							// break;
							return ssd;
						}
					}
					// if(ssd->idle_flag==1)
					// 	break;
				}
				// if(ssd->idle_flag==1)
				// 	break;
			}
			
		}
		return ssd;
	}
	else
	{
		ssd->flag=0;
	}
		
	time = ssd->current_time;
	services_2_r_cmd_trans_and_complete(ssd);                                           

	random_num=ssd->program_count%ssd->parameter->channel_number;                        

	
	for(chan=0;chan<ssd->parameter->channel_number;chan++)	     
	{
		i=(random_num+chan)%ssd->parameter->channel_number;
		flag=0;
		flag_gc=0;                                                                      
		if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))		
		{   
			if (ssd->gc_request>0)                                                     
			{
				if (ssd->channel_head[i].gc_command!=NULL)
				{
					flag_gc=gc(ssd,i,0);                                                
				}
				if (flag_gc==1)                                                         
				{
					continue;
				}
			}

			sub=ssd->channel_head[i].subs_r_head;                                        /*�ȴ���������*/
			services_2_r_wait(ssd,i,&flag,&chg_cur_time_flag);                           /*�������ڵȴ�״̬�Ķ�������*/
		
			if((flag==0)&&(ssd->channel_head[i].subs_r_head!=NULL))                      /*if there are no new read request and data is ready in some dies, send these data to controller and response this request*/		
			{		     
				services_2_r_data_trans(ssd,i,&flag,&chg_cur_time_flag);                    
						
			}
			if(flag==0)                                                                  /*if there are no read request to take channel, we can serve write requests*/ 		
			{	
				services_2_write(ssd,i,&flag,&chg_cur_time_flag);
				
			}	
		}	
	}

	return ssd;
}
Status services_2_write(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag)
{
	int j=0,chip=0;
	unsigned int k=0;
	unsigned int  old_ppn=0,new_ppn=0;
	unsigned int chip_token=0,die_token=0,plane_token=0,address_ppn=0;
	unsigned int  die=0,plane=0;
	long long time=0;
	struct sub_request * sub=NULL, * p=NULL;
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
	struct data_group *hot_node,*new_node,*pt;
    

	if((ssd->channel_head[channel].subs_w_head!=NULL)||(ssd->subs_w_head!=NULL))      
	{
		if (ssd->parameter->allocation_scheme==0)                                       /*��̬����*/
		{
			for(j=0;j<ssd->channel_head[channel].chip;j++)					
			{		
				if((ssd->channel_head[channel].subs_w_head==NULL)&&(ssd->subs_w_head==NULL)) 
				{
					break;
				}
				
				chip_token=ssd->channel_head[channel].token;                            /*����*/
				if (*channel_busy_flag==0)
				{
					if((ssd->channel_head[channel].chip_head[chip_token].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip_token].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip_token].next_state_predict_time<=ssd->current_time)))				
					{
							if((ssd->channel_head[channel].subs_w_head==NULL)&&(ssd->subs_w_head==NULL)) 
							{
								break;
							}
							die_token=ssd->channel_head[channel].chip_head[chip_token].token;	
							if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))       //can't use advanced commands
							{
								sub=find_write_sub_request(ssd,channel);
								if(sub==NULL)
								{
									break;
								}

								int result=lRUCacheGet(ssd->w_data,sub->lpn);
								if(result == -1){
									lRUCachePut(ssd->w_data,sub->lpn,sub->lpn);
								}

								
								if(sub->current_state==SR_WAIT)
								{
									plane_token=ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token;

									get_ppn(ssd,channel,chip_token,die_token,plane_token,sub);

									ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token=(ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token+1)%ssd->parameter->plane_die;

									*change_current_time_flag=0;

									if(ssd->parameter->ad_priority2==0)
									{
										ssd->real_time_subreq--;
									}
									go_one_step(ssd,sub,NULL,SR_W_TRANSFER,NORMAL);       
									delete_w_sub_request(ssd,channel,sub);               
									*channel_busy_flag=1;
					
									ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
									ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
									break;
								}
							} 
							else                                                          /*use advanced commands*/
							{
								if (dynamic_advanced_process(ssd,channel,chip_token)==NULL)
								{
									*channel_busy_flag=0;
								}
								else
								{
									*channel_busy_flag=1;                                 /*ִ����һ�����󣬴��������ݣ�ռ�������ߣ���Ҫ��������һ��channel*/
                                    ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
                                    ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
									break;
								}
							}	
								
						ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
					}
				}
								
				ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
			}
		} 
		else if(ssd->parameter->allocation_scheme==1)                                     /*��̬����*/
		{
			for(chip=0;chip<ssd->channel_head[channel].chip;chip++)					
			{	
				if((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time)))				
				{		
					if(ssd->channel_head[channel].subs_w_head==NULL)
					{
						break;
					}
					if (*channel_busy_flag==0)
					{
							                                                            
							if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))     /*��ִ�и߼�����*/
							{
								for(die=0;die<ssd->channel_head[channel].chip_head[chip].die_num;die++)				
								{	
									if(ssd->channel_head[channel].subs_w_head==NULL)
									{
										break;
									}
									sub=ssd->channel_head[channel].subs_w_head;
									while (sub!=NULL)
									{
										if ((sub->current_state==SR_WAIT)&&(sub->location->channel==channel)&&(sub->location->chip==chip)&&(sub->location->die==die))      /*����������ǵ�ǰdie������*/
										{
											break;
										}
										sub=sub->next_node;
									}
									if (sub==NULL)
									{
										continue;
									}

									if(sub->current_state==SR_WAIT)
									{
										sub->current_time=ssd->current_time;
										sub->current_state=SR_W_TRANSFER;
										sub->next_state=SR_COMPLETE;

										if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
										{
											copy_back(ssd, channel,chip, die,sub);      /*�������ִ��copyback�߼������ô���ú���copy_back(ssd, channel,chip, die,sub)����д������*/
											*change_current_time_flag=0;
										} 
										else
										{
											static_write(ssd, channel,chip, die,sub);   /*����ִ��copyback�߼������ô����static_write(ssd, channel,chip, die,sub)����������д������*/ 
											*change_current_time_flag=0;
										}
										
										delete_w_sub_request(ssd,channel,sub);
										*channel_busy_flag=1;
										break;
									}
								}
							} 
							else                                                        /*���ܴ����߼�����*/
							{
								if (dynamic_advanced_process(ssd,channel,chip)==NULL)
								{
									*channel_busy_flag=0;
								}
								else
								{
									*channel_busy_flag=1;                               /*ִ����һ�����󣬴��������ݣ�ռ�������ߣ���Ҫ��������һ��channel*/
									break;
								}
							}	
						
					}
				}		
			}
		}			
	}
	return SUCCESS;	
}
Status services_2_r_cmd_trans_and_complete(struct ssd_info * ssd)
{
	unsigned int i=0;
	struct sub_request * sub=NULL, * p=NULL;
	for(i=0;i<ssd->parameter->channel_number;i++)                                       /*���ѭ����������Ҫchannel��ʱ��(�������Ѿ�����chip��chip��ready��Ϊbusy)�������������ʱ�������channel�Ķ�����ȡ��*/
	{
		sub=ssd->channel_head[i].subs_r_head;

		while(sub!=NULL)
		{
			if(sub->current_state==SR_R_C_A_TRANSFER)                                  /*���������ϣ�����Ӧ��die��Ϊbusy��ͬʱ�޸�sub��״̬; �������ר�Ŵ����������ɵ�ǰ״̬Ϊ�������Ϊdie��ʼbusy��die��ʼbusy����ҪchannelΪ�գ����Ե����г�*/
			{
				if(sub->next_state_predict_time<=ssd->current_time)
				{
					go_one_step(ssd, sub,NULL, SR_R_READ,NORMAL);                      /*״̬���䴦������*/

				}
			}
			else if((sub->current_state==SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time)))					
			{			
				if(sub!=ssd->channel_head[i].subs_r_head)                             /*if the request is completed, we delete it from read queue */							
				{		
					p->next_node=sub->next_node;						
				}			
				else					
				{	
					if (ssd->channel_head[i].subs_r_head!=ssd->channel_head[i].subs_r_tail)
					{
						ssd->channel_head[i].subs_r_head=sub->next_node;
					} 
					else
					{
						ssd->channel_head[i].subs_r_head=NULL;
						ssd->channel_head[i].subs_r_tail=NULL;
					}							
				}			
			}
			p=sub;
			sub=sub->next_node;
		}
	}
	
	return SUCCESS;
}