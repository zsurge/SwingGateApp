/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : LocalData.c
  版 本 号   : 初稿
  作    者   :
  生成日期   : 2020年3月21日
  最近修改   :
  功能描述   : 管理本地数据，对卡号，用户ID为索引的数-
                   据进行增删改查
  函数列表   :
  修改历史   :
  1.日    期   : 2020年3月21日
    作    者   :
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include "LocalData.h"
#include "bsp_spi_flash.h"
#include "easyflash.h"
#include "stdio.h"
#include "tool.h"
#include "malloc.h"
#include "bsp_MB85RC128.h"
#include "deviceInfo.h"
#include "quickSort.h"



#define LOG_TAG    "localData"
#include "elog.h"


/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
// *----------------------------------------------*/


/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/
static uint8_t checkFlashSpace ( uint8_t mode );

static int Bin_Search(HEADINFO_STRU *num,int numsSize,int target);
static int Bin_Search_addr(uint32_t startAddr,int numsSize,int target);

static void swap(uint32_t *x, uint32_t *y);
static int selectPivotMedianOfThree(uint32_t *arr,uint32_t low,uint32_t high);
static void insertSort(uint32_t *arr, uint32_t m, uint32_t n);
static void qSort(void *dataBuff,uint32_t low,uint32_t high);
static void optAccessIndex(uint8_t mode);
static uint8_t checkFlashSpace ( uint8_t mode )
{
	if ( mode == CARD_MODE )
	{
		if ( gRecordIndex.cardNoIndex > MAX_HEAD_RECORD-5 )
		{
			return 1;
		}
	}
	else
	{
		if ( gRecordIndex.delCardNoIndex > MAX_HEAD_DEL_CARDNO-5 )
		{
			return 1;
		}
	}
	
	return 0;
}




int readHead(uint8_t *headBuff,uint8_t mode)
{
	uint8_t i = 0;
	uint8_t multiple = 0;
	uint16_t remainder = 0;
	uint32_t address = 0;
	uint32_t curIndex = 0;
	
	int32_t iTime1, iTime2;
	 
	int ret = 0;
    HEADINFO_STRU targetData,firstData,lastData;


	if ( headBuff == NULL )
	{
		return NO_FIND_HEAD;
	}	

//	if(headBuff[0] == 0x01)
//	{
//	    log_d("card status:del\r\n");
//	    return NO_FIND_HEAD; //已删除卡
//	}

    

	memcpy(targetData.headData.sn,headBuff,sizeof(targetData.headData.sn));

    log_d("want find head.headData.id = %x,sn = %02x,%02x,%02x,%02x\r\n",targetData.headData.id,targetData.headData.sn[0],targetData.headData.sn[1],targetData.headData.sn[2],targetData.headData.sn[3]);
	

    ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);
    

	if ( mode == CARD_MODE )
	{
		address = CARD_NO_HEAD_ADDR;
		curIndex = gRecordIndex.cardNoIndex;
	}
	else if ( mode == CARD_DEL_MODE )
	{
		address = CARD_DEL_HEAD_ADDR;
		curIndex = gRecordIndex.delCardNoIndex;		
	}
	
	multiple = curIndex / HEAD_NUM_SECTOR;
	remainder = curIndex % HEAD_NUM_SECTOR;

    //1.读取单页或者多页最后一页的地址
    address += multiple * HEAD_NUM_SECTOR  * CARD_USER_LEN;

    log_d("addr = %x,multiple = %d,remainder=%d\r\n",address,multiple,remainder);
    

//2.读取最后一页第一个卡号和最后一个卡号；

    firstData.headData.id = 0;
    lastData.headData.id = 0;   

    iTime1 = xTaskGetTickCount();   /* 记下开始时间 */
    
    ret = FRAM_Read (FM24V10_1, address, &firstData,CARD_USER_LEN);
    if(ret == 0)
    {
        log_e("read fram error\r\n");
        return NO_FIND_HEAD; 
    }
    
    ret = FRAM_Read (FM24V10_1, address+(remainder-1)* CARD_USER_LEN, &lastData,CARD_USER_LEN); 
    if(ret == 0)
    {
        log_e("read fram error\r\n");
        return NO_FIND_HEAD;
    }  
    log_d("head = %x,last page %x,%x\r\n",targetData.headData.id,firstData.headData.id,lastData.headData.id);

    
    if((targetData.headData.id >= firstData.headData.id) && (targetData.headData.id <= lastData.headData.id))
    {      
    
//        ret = Bin_Search(gSectorBuff,remainder,targetData.headData.id);
        ret = Bin_Search_addr(address,remainder,targetData.headData.id);       

        log_d("1.Bin_Search flash index = %d\r\n",ret);
        
        if(ret != NO_FIND_HEAD)
        {
            iTime2 = xTaskGetTickCount();   /* 记下结束时间 */
            log_d ( "find it，use %d ms\r\n",iTime2 - iTime1);      

            return multiple*HEAD_NUM_SECTOR+ret;
        }
    }    
    
    for(i=0;i<multiple;i++)
    {
        address = CARD_NO_HEAD_ADDR;//从零开始读;
        address += i * HEAD_NUM_SECTOR  * CARD_USER_LEN;
        
        firstData.headData.id = 0;
        lastData.headData.id = 0;   
        
        ret = FRAM_Read (FM24V10_1, address, &firstData,CARD_USER_LEN);
        if(ret == 0)
        {
            log_e("read fram error\r\n");
            return NO_FIND_HEAD; 
        }
        
        ret = FRAM_Read (FM24V10_1, address+(HEAD_NUM_SECTOR-1)* CARD_USER_LEN, &lastData,CARD_USER_LEN); 
        if(ret == 0)
        {
            log_e("read fram error\r\n");
            return NO_FIND_HEAD;
        }  
        log_d("head = %x,last page %x,%x\r\n",targetData.headData.id,firstData.headData.id,lastData.headData.id);

        
        if((targetData.headData.id >= firstData.headData.id) && (targetData.headData.id <= lastData.headData.id))
        {
//            ret = Bin_Search(gSectorBuff,HEAD_NUM_SECTOR,targetData.headData.id);
            
            ret = Bin_Search_addr(address,HEAD_NUM_SECTOR,targetData.headData.id);       
            if(ret != NO_FIND_HEAD)
            {
            	iTime2 = xTaskGetTickCount();	/* 记下结束时间 */
            	log_d ( "find it，use %d ms,index = %d\r\n",iTime2 - iTime1,i*HEAD_NUM_SECTOR+ret);      
                
                return i*HEAD_NUM_SECTOR+ret;
            }
        }
    
    }

	iTime2 = xTaskGetTickCount();	/* 记下结束时间 */
	log_d ( "read all Head，use %d ms\r\n",iTime2 - iTime1 );    

    return NO_FIND_HEAD;

}



void sortHead(HEADINFO_STRU *head,int length)
{
    int left, right, mid;
    HEADINFO_STRU tmp;

    memset(&tmp,0x00,sizeof(tmp));

    log_d("sortHead length = %d\r\n",length);
    
    for (int i = 1; i < length; i++)
    {
        /* 找到数组中第一个无序的数，保存为tmp */
        if (head[i].headData.id < head[i - 1].headData.id)
        {
            tmp = head[i];
        }
        else
        {
            continue;
        }
		/* 找到数组中第一个无序的数，保存为tmp */

		/* 二分查询开始 */
        left = 0;
        right = i - 1;
        while (left <= right)
        {
            mid = (left + right) / 2;
            if (head[mid].headData.id > tmp.headData.id)
            {
                right = mid - 1;
            }
            else
            {
                left = mid + 1;
            }
        }
		/* 二分查询结束,此时a[left]>=a[i],记录下left的值 */

		/* 将有序数组中比要插入的数大的数右移 */
        for (int j = i; j > left; j--)
        {
            head[j] = head[j - 1];
        }
		/* 将有序数组中比要插入的数大的数右移 */

		// 将left位置赋值为要插入的数
        head[left] = tmp;
    }
}

//return:<0,未找到，>=0//返回查找到的卡号的索引
static int Bin_Search(HEADINFO_STRU *num,int numsSize,int target)
{
	int low = 0,high = numsSize-1,mid;
	
	while(low <= high)
	{
		mid = low + ((high - low) >> 1); //获取中间值

		if(num[mid].headData.id == target)
		{
		    return mid;
		}
		else if(num[mid].headData.id < target)
		{
		    low = mid+1;//mid已经交换过了,low往后移一位
		}
		else
		{
		    high = mid-1; //mid已经交换过了,right往前移一位
		}
	}        
    return NO_FIND_HEAD;
}

static int Bin_Search_addr(uint32_t startAddr,int numsSize,int target)
{
	int low = 0,high = numsSize-1,mid;
	HEADINFO_STRU tmpData;
	uint8_t ret = 0;
	    
	while(low <= high)
	{
		mid = low + ((high - low) >> 1); //获取中间值

        ret = FRAM_Read (FM24V10_1, startAddr+(mid)*CARD_USER_LEN, &tmpData, (1)* CARD_USER_LEN);
      
        if(ret == 0)
        {
            log_e("read fram error\r\n");
            return NO_FIND_HEAD; 
        }  
        
		if(tmpData.headData.id == target)
		{
		    return mid;
		}
		else if(tmpData.headData.id < target)
		{
		    low = mid+1;//mid已经交换过了,low往后移一位
		}
		else
		{
		    high = mid-1; //mid已经交换过了,right往前移一位
		}
	}        
    return NO_FIND_HEAD;
}


uint32_t addCard(uint8_t *head,uint8_t mode)
{
    uint8_t multiple = 0;
	uint16_t remainder = 0;
	uint32_t addr = 0;
	uint8_t ret = 0;
	uint32_t curIndex = 0;

    HEADINFO_STRU tmpCard,rxCard;

    int32_t iTime1, iTime2;

    log_d("head %02x,%02x,%02x,%02x\r\n",head[0],head[1],head[2],head[3]);   

    if(!head)
    {
        return 0;
    }

    memcpy(tmpCard.headData.sn,head,CARD_NO_LEN_BCD);

    
   iTime1 = xTaskGetTickCount();   /* 记下开始时间 */
   //1.先判定当前有多少个卡号;
    ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);

	if ( mode == CARD_MODE )
	{
		addr = CARD_NO_HEAD_ADDR;
		curIndex = gRecordIndex.cardNoIndex;
	}
	else if ( mode == CARD_DEL_MODE )
	{
		addr = CARD_DEL_HEAD_ADDR;
		curIndex = gRecordIndex.delCardNoIndex;		
	}
   
    
    multiple = curIndex / HEAD_NUM_SECTOR;
    remainder = curIndex % HEAD_NUM_SECTOR;

    //索引要从1开始    
    if(multiple==0 && remainder==0)
    {
        //第一条记录
        log_d("<<<<<write first recond>>>>>\r\n");
        
        //写入到存储区域
//        SPI_FLASH_BufferWrite(&tmpCard, addr,1*sizeof(HEADINFO_STRU));   
        
        ret = FRAM_Write ( FM24V10_1, addr, &tmpCard,1* sizeof(HEADINFO_STRU));
        
        if(ret == 0)
        {
            log_e("write fram error\r\n");
            return ret;
        }           
    }
    else 
    {
        //2.追加到最后一页最后一条；
        addr += (multiple * HEAD_NUM_SECTOR + remainder)  * sizeof(HEADINFO_STRU);
//        SPI_FLASH_BufferWrite(&tmpCard, addr,1*sizeof(HEADINFO_STRU));

        ret = FRAM_Write ( FM24V10_1, addr, &tmpCard,1* sizeof(HEADINFO_STRU));
        
        if(ret == 0)
        {
            log_e("write fram error\r\n");
            return ret;
        }           
        

        log_d("add = %d,multiple = %d,remainder=%d\r\n",addr,multiple,remainder);
        

        //3.判断是否写满一页，是的话，排序
//        if((remainder == HEAD_NUM_SECTOR-1) &&(remainder%(HEAD_NUM_SECTOR-1)==0))
//        {  
//            //读一页数据
//            //SPI_FLASH_BufferRead(gSectorBuff, multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU), HEAD_NUM_SECTOR * sizeof(HEADINFO_STRU));            
//            ret = FRAM_Read (FM24V10_1, multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU), gSectorBuff, (HEAD_NUM_SECTOR)* sizeof(HEADINFO_STRU));
//            if(ret == 0)
//            {
//                log_e("read fram error\r\n");
//                return ret;
//            }
//            
//            //排序
//            qSortCard(gSectorBuff,HEAD_NUM_SECTOR);
//            //写回数据
//            //SPI_FLASH_BufferWrite(gSectorBuff, multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU), HEAD_NUM_SECTOR * sizeof(HEADINFO_STRU));            
//            ret = FRAM_Write ( FM24V10_1, multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU), gSectorBuff,HEAD_NUM_SECTOR* sizeof(HEADINFO_STRU));
//            if(ret == 0)
//            {
//                log_e("write fram error\r\n");
//                return ret;
//            } 
//        }  
    }

    ret = FRAM_Read (FM24V10_1, addr, &rxCard, 1*sizeof(HEADINFO_STRU));
    if(ret == 0)
    {
        log_e("read fram error\r\n");
        return ret;
    }

    if(tmpCard.headData.id != rxCard.headData.id)
    {
        return 0;//写不成功
    }   


	if ( mode == CARD_MODE )
	{
        gRecordIndex.cardNoIndex++;
	}
	else if ( mode == CARD_DEL_MODE )
	{
        gRecordIndex.delCardNoIndex++;		
	}
	
	optRecordIndex(&gRecordIndex,WRITE_PRARM);

	iTime2 = xTaskGetTickCount();	/* 记下结束时间 */
	log_d ( "add head成功，耗时: %dms\r\n",iTime2 - iTime1 );

    return gRecordIndex.cardNoIndex;
  
}




int delHead(uint8_t *headBuff,uint8_t mode)
{
	int ret = 0;
	uint8_t multiple = 0;
	uint16_t remainder = 0;
    HEADINFO_STRU tmpCard;
    uint32_t addr = CARD_NO_HEAD_ADDR;    
    int num = 0;

    if ( headBuff == NULL )
	{
		return NO_FIND_HEAD;
	}	

    //1.查找要删除的卡号
    ret = readHead(headBuff,CARD_MODE);

    if(ret == NO_FIND_HEAD)
    {
        return NO_FIND_HEAD;
    }

    //2.计算要删除卡号的地址
	multiple = ret / HEAD_NUM_SECTOR;
	remainder = ret % HEAD_NUM_SECTOR;
	
    addr += (multiple * HEAD_NUM_SECTOR + remainder)  * sizeof(HEADINFO_STRU);

    //3.将要删除的卡号置全0    
    tmpCard.headData.id = 0;    
    ret = FRAM_Write ( FM24V10_1, addr, &tmpCard,1* sizeof(HEADINFO_STRU));    
    if(ret == 0)
    {
        log_e("write fram error\r\n");
        return NO_FIND_HEAD;
    }   

    //4.对这一页重新排序
    if(multiple >= 1)
    {
        addr += (multiple-1) * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU);
        num = HEAD_NUM_SECTOR;
    }
    else
    {
        addr = CARD_NO_HEAD_ADDR;
        ClearRecordIndex();
        optRecordIndex(&gRecordIndex,READ_PRARM);        
        num = gRecordIndex.cardNoIndex % HEAD_NUM_SECTOR;
    }
    
//    if(remainder == 0 && multiple >= 1)
//    {     
//        addr = (multiple-1) * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU);
//        num = HEAD_NUM_SECTOR * sizeof(HEADINFO_STRU);
//    }
//    else
//    { 
//        addr = multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU);
//        num = remainder * sizeof(HEADINFO_STRU);    
//    }
    
    //读一页数据
    ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, num* sizeof(HEADINFO_STRU));
    if(ret == 0)
    {
        log_e("read fram error\r\n");
        return ret;
    }

    //排序
    qSortCard(gSectorBuff,num);  

    //写回数据
    ret = FRAM_Write ( FM24V10_1, addr, gSectorBuff,num* sizeof(HEADINFO_STRU));
    if(ret == 0)
    {
        log_e("write fram error\r\n");
        return ret;
    }  
    

    #ifdef DEBUG_PRINT
    for(int i=0;i<num;i++)
    {
        log_d("del card id =%x\r\n",gSectorBuff[i].headData.id);
    }  
    #endif
    

    log_d("qSortCard success\r\n");
    

    
    return 1;
  
}


void swap(uint32_t *x, uint32_t *y)
{
	int temp = *x;
	*x = *y;
	*y = temp;
}

void insertSort(uint32_t *arr, uint32_t m, uint32_t n)
{
	int i, j;
	int temp; // 用来存放临时的变量
	for(i = m+1; i <= n; i++)
	{
		temp = arr[i];
		for(j = i-1; (j >= m)&&(arr[j] > temp); j--)
		{
			arr[j + 1] = arr[j];
		}
		arr[j + 1] = temp;
	}
}

/*函数作用：取待排序序列中low、mid、high三个位置上数据，选取他们中间的那个数据作为枢轴*/
int selectPivotMedianOfThree(uint32_t *arr,uint32_t low,uint32_t high)
{
	int mid = low + ((high - low) >> 1);//计算数组中间的元素的下标

	//使用三数取中法选择枢轴
	if (arr[mid] > arr[high])//目标: arr[mid] <= arr[high]
	{
		swap(&arr[mid],&arr[high]);
	}

	if (arr[low] > arr[high])//目标: arr[low] <= arr[high]
	{
		swap(&arr[low],&arr[high]);
	}

	if (arr[mid] > arr[low]) //目标: arr[low] >= arr[mid]
	{
		swap(&arr[mid],&arr[low]);
	}

	//此时，arr[mid] <= arr[low] <= arr[high]
	return arr[low];
	//low的位置上保存这三个位置中间的值
	//分割时可以直接使用low位置的元素作为枢轴，而不用改变分割函数了
}

void qSort(void *dataBuff,uint32_t low,uint32_t high)
{
	int first = low;
	int last = high;
 
	int left = low;
	int right = high;
 
	int leftLen = 0;
	int rightLen = 0;

	uint32_t *arr = (uint32_t *)dataBuff;
 
	if (high - low + 1 < 10)
	{
		insertSort(arr,low,high);
		return;
	}
	
	//一次分割
	int key = selectPivotMedianOfThree(arr,low,high);//使用三数取中法选择枢轴
		
	while(low < high)
	{
		while(high > low && arr[high] >= key)
		{
			if (arr[high] == key)//处理相等元素
			{
				swap(&arr[right],&arr[high]);
				right--;
				rightLen++;
			}
			high--;
		}
		arr[low] = arr[high];
		while(high > low && arr[low] <= key)
		{
			if (arr[low] == key)
			{
				swap(&arr[left],&arr[low]);
				left++;
				leftLen++;
			}
			low++;
		}
		arr[high] = arr[low];
	}
	arr[low] = key;
 
	//一次快排结束
	//把与枢轴key相同的元素移到枢轴最终位置周围
	int i = low - 1;
	int j = first;
	while(j < left && arr[i] != key)
	{
		swap(&arr[i],&arr[j]);
		i--;
		j++;
	}
	i = low + 1;
	j = last;
	while(j > right && arr[i] != key)
	{
		swap(&arr[i],&arr[j]);
		i++;
		j--;
	}
	qSort(arr,first,low - 1 - leftLen);
	qSort(arr,low + 1 + rightLen,last);
}


void qSortCard(HEADINFO_STRU *head,uint32_t length)
{
//    qSort(head,0,length-1);
    quickSortNor(head,0, length-1);
}

void sortLastPageCard(void)
{
    uint8_t multiple = 0;
	uint16_t remainder = 0;
	int ret = 0;
	uint32_t addr = CARD_NO_HEAD_ADDR;

    int32_t iTime1, iTime2;
      
    iTime1 = xTaskGetTickCount();   /* 记下开始时间 */
   
   //1.先判定当前有多少个卡号;
    ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);
    
	addr = CARD_NO_HEAD_ADDR;    
    multiple = gRecordIndex.cardNoIndex / HEAD_NUM_SECTOR;
    remainder = gRecordIndex.cardNoIndex % HEAD_NUM_SECTOR;

    if(remainder == 0)
    {
        log_d("SECTOR is zero\r\n");
        return;
    }

    memset(gSectorBuff,0x00,sizeof(gSectorBuff));
    
    //2.计算最后一页地址
    addr += multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU);    

    //3.读取最后一页
//    SPI_FLASH_BufferRead(gSectorBuff, addr, (remainder)* sizeof(HEADINFO_STRU));
    
    ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, (remainder)* sizeof(HEADINFO_STRU));
    if(ret == 0)
    {
        log_e("read fram error\r\n");
        return ;
    }    
    
    //5.排序        
    qSortCard(gSectorBuff,remainder);

    #ifdef DEBUG_PRINT
    for(int i=0;i<remainder;i++)
    {
        log_d("add = %d,id =%x\r\n",addr,gSectorBuff[i].headData.id);
    }   
    #endif

    ret = FRAM_Write ( FM24V10_1, addr, gSectorBuff,(remainder)* sizeof(HEADINFO_STRU));
    
    if(ret == 0)
    {
        log_e("write fram error\r\n");
        return ;
    }      


	iTime2 = xTaskGetTickCount();	/* 记下结束时间 */
	log_d ( "sort last page card success，use time: %dms\r\n",iTime2 - iTime1 );  
}


void sortPageCard(void)
{
    uint8_t multiple = 0;
	int ret = 0;
	
	uint32_t addr = CARD_NO_HEAD_ADDR;

    int32_t iTime1, iTime2;
    
    iTime1 = xTaskGetTickCount();   /* 记下开始时间 */
   
   //1.先判定当前有多少个卡号;
    ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);
    
	addr = CARD_NO_HEAD_ADDR;    
    multiple = gRecordIndex.cardNoIndex / HEAD_NUM_SECTOR;

    if(multiple<1)
    {
        return;//这里应该不太可能，因为只有大于1024的时候，才会触发这个条件
    }

    memset(gSectorBuff,0x00,sizeof(gSectorBuff));
    
    //2.计算当前页地址
    addr += (multiple-1) * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU);

    //3.读当前页
//    SPI_FLASH_BufferRead(gSectorBuff, addr ,HEAD_NUM_SECTOR * sizeof(HEADINFO_STRU));       

    ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, HEAD_NUM_SECTOR* sizeof(HEADINFO_STRU));
    if(ret == 0)
    {
        log_e("read fram error\r\n");
        return ;
    } 
    
    //排序
    qSortCard(gSectorBuff,HEAD_NUM_SECTOR);

#ifdef DEBUG_PRINT
    for(int i=0;i<HEAD_NUM_SECTOR;i++)
    {
        log_d("sort page id the %d =%x\r\n",i,gSectorBuff[i].headData.id);
    }  
#endif  

    //写回数据
//    SPI_FLASH_BufferWrite(gSectorBuff, addr, HEAD_NUM_SECTOR * sizeof(HEADINFO_STRU));  
    
    ret = FRAM_Write ( FM24V10_1, addr, gSectorBuff,HEAD_NUM_SECTOR* sizeof(HEADINFO_STRU));
    
    if(ret == 0)
    {
        log_e("write fram error\r\n");
        return ;
    }  


	iTime2 = xTaskGetTickCount();	/* 记下结束时间 */
	log_d ( "sort FULL page card success，use time: %dms\r\n",iTime2 - iTime1 );  

	
    log_d("qSortpageCard success\r\n");
}


void manualSortCard(void)
{
    uint8_t multiple = 0;
	uint16_t remainder = 0;
	int ret = 0;

	int i = 0;
	uint32_t addr = CARD_NO_HEAD_ADDR;

    int32_t iTime1, iTime2;   
       
   //1.先判定当前有多少个卡号;
    ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);
    
	addr = CARD_NO_HEAD_ADDR;    
    multiple = gRecordIndex.cardNoIndex / HEAD_NUM_SECTOR;
    remainder = gRecordIndex.cardNoIndex % HEAD_NUM_SECTOR;

    memset(gSectorBuff,0x00,sizeof(gSectorBuff));
    iTime1 = xTaskGetTickCount();   /* 记下开始时间 */
    if(remainder != 0)
    {
        //2.计算最后一页地址
        addr += multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU);    
        
        //3.读取最后一页
        ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, remainder* sizeof(HEADINFO_STRU));
        if(ret == 0)
        {
            log_e("read fram error\r\n");
            return ;
        } 
        
        //5.排序        
        qSortCard(gSectorBuff,remainder);   
        
        ret = FRAM_Write ( FM24V10_1, addr, gSectorBuff,remainder* sizeof(HEADINFO_STRU));        
        if(ret == 0)
        {
            log_e("write fram error\r\n");
            return ;
        }          
    }    

    
    for(i=0;i<multiple;i++)
    {
        addr = CARD_NO_HEAD_ADDR;//从零开始读;
        addr += i * HEAD_NUM_SECTOR  * CARD_USER_LEN; 
        memset(gSectorBuff,0x00,sizeof(gSectorBuff));
        
        //3.读当前页      
        ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, HEAD_NUM_SECTOR* sizeof(HEADINFO_STRU));
        if(ret == 0)
        {
            log_e("read fram error\r\n");
            return ;
        }         
        //排序
        qSortCard(gSectorBuff,HEAD_NUM_SECTOR); 
        //写回数据
        ret = FRAM_Write ( FM24V10_1, addr, gSectorBuff,HEAD_NUM_SECTOR* sizeof(HEADINFO_STRU));        
        if(ret == 0)
        {
            log_e("write fram error\r\n");
            return ;
        }        
     }

	iTime2 = xTaskGetTickCount();	/* 记下结束时间 */
	log_e( "sort all card success，use time: %dms\r\n",iTime2 - iTime1 );  	
}


static void optAccessIndex(uint8_t mode)
{
    ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);
    
    if(mode == INCREMENT)
    {
            gRecordIndex.accessRecoIndex++;
    }
    else if(mode == DECLINE && gRecordIndex.accessRecoIndex > 0)
    {        
        gRecordIndex.accessRecoIndex--;
    }   
    else
    {
        return;
    }

    optRecordIndex(&gRecordIndex,WRITE_PRARM);
}


//写通行记录
uint8_t writeRecord(uint8_t *buf,int len)
{
    int ret = 0;
    int addr = ACCESS_RECORD_ADDR;
    uint8_t tmpBuf[RECORD_MAX_LEN] = {0};

    ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);

    addr += gRecordIndex.accessRecoIndex * RECORD_MAX_LEN;
    
    log_d("writeRecord addr = %d,len = %d,buff = %s\r\n",addr,len,buf);   

    ret = bsp_sf_WriteBuffer(buf,addr,len);

    log_d("bsp_sf_WriteBuffer ret = %d\r\n",ret);

    if(!ret)
    {
        return ret;//写FLASH失败
    }
    
    memset(tmpBuf,0x00,sizeof(tmpBuf));
    bsp_sf_ReadBuffer(tmpBuf,addr,len);
    log_d("writeRecord = %s\r\n",tmpBuf);
    
    if(memcmp(tmpBuf,buf,len))
    {        
        return 0;//写FLASH失败
    }
    
    optAccessIndex(INCREMENT);
    return ret;
}

//读通行记录
uint8_t readRecord(uint8_t *buf)
{
    int addr = ACCESS_RECORD_ADDR;
    uint8_t rxBuf[RECORD_MAX_LEN] = {0};
    uint8_t len = 0;
    optAccessIndex(DECLINE);

    addr += gRecordIndex.accessRecoIndex * RECORD_MAX_LEN;
    
    memset(rxBuf,0x00,sizeof(rxBuf));    
    bsp_sf_ReadBuffer(rxBuf,addr,RECORD_MAX_LEN);

    log_d("readRecord = %s\r\n",rxBuf);

    if(strlen((const char*)rxBuf)>= RECORD_REAL_LEN)
    {
        len = RECORD_REAL_LEN;
    }
    else
    {
        len = 0;
    }


    log_d("readRecord rxBuf len = %d\r\n",strlen((const char*)rxBuf));
    
    memcpy(buf,rxBuf,RECORD_REAL_LEN);
    
    return len;
}


//清除通行记录
void clearRecord(void)
{
    eraseDataSector();
}



