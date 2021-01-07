/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : LocalData.c
  �� �� ��   : ����
  ��    ��   :
  ��������   : 2020��3��21��
  ����޸�   :
  ��������   : ���������ݣ��Կ��ţ��û�IDΪ��������-
                   �ݽ�����ɾ�Ĳ�
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2020��3��21��
    ��    ��   :
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
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
 * �궨��                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ģ�鼶����                                   *
// *----------------------------------------------*/


/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
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
//	    return NO_FIND_HEAD; //��ɾ����
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

    //1.��ȡ��ҳ���߶�ҳ���һҳ�ĵ�ַ
    address += multiple * HEAD_NUM_SECTOR  * CARD_USER_LEN;

    log_d("addr = %x,multiple = %d,remainder=%d\r\n",address,multiple,remainder);
    

//2.��ȡ���һҳ��һ�����ź����һ�����ţ�

    firstData.headData.id = 0;
    lastData.headData.id = 0;   

    iTime1 = xTaskGetTickCount();   /* ���¿�ʼʱ�� */
    
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
            iTime2 = xTaskGetTickCount();   /* ���½���ʱ�� */
            log_d ( "find it��use %d ms\r\n",iTime2 - iTime1);      

            return multiple*HEAD_NUM_SECTOR+ret;
        }
    }    
    
    for(i=0;i<multiple;i++)
    {
        address = CARD_NO_HEAD_ADDR;//���㿪ʼ��;
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
            	iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
            	log_d ( "find it��use %d ms,index = %d\r\n",iTime2 - iTime1,i*HEAD_NUM_SECTOR+ret);      
                
                return i*HEAD_NUM_SECTOR+ret;
            }
        }
    
    }

	iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_d ( "read all Head��use %d ms\r\n",iTime2 - iTime1 );    

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
        /* �ҵ������е�һ���������������Ϊtmp */
        if (head[i].headData.id < head[i - 1].headData.id)
        {
            tmp = head[i];
        }
        else
        {
            continue;
        }
		/* �ҵ������е�һ���������������Ϊtmp */

		/* ���ֲ�ѯ��ʼ */
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
		/* ���ֲ�ѯ����,��ʱa[left]>=a[i],��¼��left��ֵ */

		/* �����������б�Ҫ���������������� */
        for (int j = i; j > left; j--)
        {
            head[j] = head[j - 1];
        }
		/* �����������б�Ҫ���������������� */

		// ��leftλ�ø�ֵΪҪ�������
        head[left] = tmp;
    }
}

//return:<0,δ�ҵ���>=0//���ز��ҵ��Ŀ��ŵ�����
static int Bin_Search(HEADINFO_STRU *num,int numsSize,int target)
{
	int low = 0,high = numsSize-1,mid;
	
	while(low <= high)
	{
		mid = low + ((high - low) >> 1); //��ȡ�м�ֵ

		if(num[mid].headData.id == target)
		{
		    return mid;
		}
		else if(num[mid].headData.id < target)
		{
		    low = mid+1;//mid�Ѿ���������,low������һλ
		}
		else
		{
		    high = mid-1; //mid�Ѿ���������,right��ǰ��һλ
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
		mid = low + ((high - low) >> 1); //��ȡ�м�ֵ

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
		    low = mid+1;//mid�Ѿ���������,low������һλ
		}
		else
		{
		    high = mid-1; //mid�Ѿ���������,right��ǰ��һλ
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

    
   iTime1 = xTaskGetTickCount();   /* ���¿�ʼʱ�� */
   //1.���ж���ǰ�ж��ٸ�����;
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

    //����Ҫ��1��ʼ    
    if(multiple==0 && remainder==0)
    {
        //��һ����¼
        log_d("<<<<<write first recond>>>>>\r\n");
        
        //д�뵽�洢����
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
        //2.׷�ӵ����һҳ���һ����
        addr += (multiple * HEAD_NUM_SECTOR + remainder)  * sizeof(HEADINFO_STRU);
//        SPI_FLASH_BufferWrite(&tmpCard, addr,1*sizeof(HEADINFO_STRU));

        ret = FRAM_Write ( FM24V10_1, addr, &tmpCard,1* sizeof(HEADINFO_STRU));
        
        if(ret == 0)
        {
            log_e("write fram error\r\n");
            return ret;
        }           
        

        log_d("add = %d,multiple = %d,remainder=%d\r\n",addr,multiple,remainder);
        

        //3.�ж��Ƿ�д��һҳ���ǵĻ�������
//        if((remainder == HEAD_NUM_SECTOR-1) &&(remainder%(HEAD_NUM_SECTOR-1)==0))
//        {  
//            //��һҳ����
//            //SPI_FLASH_BufferRead(gSectorBuff, multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU), HEAD_NUM_SECTOR * sizeof(HEADINFO_STRU));            
//            ret = FRAM_Read (FM24V10_1, multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU), gSectorBuff, (HEAD_NUM_SECTOR)* sizeof(HEADINFO_STRU));
//            if(ret == 0)
//            {
//                log_e("read fram error\r\n");
//                return ret;
//            }
//            
//            //����
//            qSortCard(gSectorBuff,HEAD_NUM_SECTOR);
//            //д������
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
        return 0;//д���ɹ�
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

	iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_d ( "add head�ɹ�����ʱ: %dms\r\n",iTime2 - iTime1 );

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

    //1.����Ҫɾ���Ŀ���
    ret = readHead(headBuff,CARD_MODE);

    if(ret == NO_FIND_HEAD)
    {
        return NO_FIND_HEAD;
    }

    //2.����Ҫɾ�����ŵĵ�ַ
	multiple = ret / HEAD_NUM_SECTOR;
	remainder = ret % HEAD_NUM_SECTOR;
	
    addr += (multiple * HEAD_NUM_SECTOR + remainder)  * sizeof(HEADINFO_STRU);

    //3.��Ҫɾ���Ŀ�����ȫ0    
    tmpCard.headData.id = 0;    
    ret = FRAM_Write ( FM24V10_1, addr, &tmpCard,1* sizeof(HEADINFO_STRU));    
    if(ret == 0)
    {
        log_e("write fram error\r\n");
        return NO_FIND_HEAD;
    }   

    //4.����һҳ��������
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
    
    //��һҳ����
    ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, num* sizeof(HEADINFO_STRU));
    if(ret == 0)
    {
        log_e("read fram error\r\n");
        return ret;
    }

    //����
    qSortCard(gSectorBuff,num);  

    //д������
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
	int temp; // ���������ʱ�ı���
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

/*�������ã�ȡ������������low��mid��high����λ�������ݣ�ѡȡ�����м���Ǹ�������Ϊ����*/
int selectPivotMedianOfThree(uint32_t *arr,uint32_t low,uint32_t high)
{
	int mid = low + ((high - low) >> 1);//���������м��Ԫ�ص��±�

	//ʹ������ȡ�з�ѡ������
	if (arr[mid] > arr[high])//Ŀ��: arr[mid] <= arr[high]
	{
		swap(&arr[mid],&arr[high]);
	}

	if (arr[low] > arr[high])//Ŀ��: arr[low] <= arr[high]
	{
		swap(&arr[low],&arr[high]);
	}

	if (arr[mid] > arr[low]) //Ŀ��: arr[low] >= arr[mid]
	{
		swap(&arr[mid],&arr[low]);
	}

	//��ʱ��arr[mid] <= arr[low] <= arr[high]
	return arr[low];
	//low��λ���ϱ���������λ���м��ֵ
	//�ָ�ʱ����ֱ��ʹ��lowλ�õ�Ԫ����Ϊ���ᣬ�����øı�ָ����
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
	
	//һ�ηָ�
	int key = selectPivotMedianOfThree(arr,low,high);//ʹ������ȡ�з�ѡ������
		
	while(low < high)
	{
		while(high > low && arr[high] >= key)
		{
			if (arr[high] == key)//�������Ԫ��
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
 
	//һ�ο��Ž���
	//��������key��ͬ��Ԫ���Ƶ���������λ����Χ
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
      
    iTime1 = xTaskGetTickCount();   /* ���¿�ʼʱ�� */
   
   //1.���ж���ǰ�ж��ٸ�����;
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
    
    //2.�������һҳ��ַ
    addr += multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU);    

    //3.��ȡ���һҳ
//    SPI_FLASH_BufferRead(gSectorBuff, addr, (remainder)* sizeof(HEADINFO_STRU));
    
    ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, (remainder)* sizeof(HEADINFO_STRU));
    if(ret == 0)
    {
        log_e("read fram error\r\n");
        return ;
    }    
    
    //5.����        
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


	iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_d ( "sort last page card success��use time: %dms\r\n",iTime2 - iTime1 );  
}


void sortPageCard(void)
{
    uint8_t multiple = 0;
	int ret = 0;
	
	uint32_t addr = CARD_NO_HEAD_ADDR;

    int32_t iTime1, iTime2;
    
    iTime1 = xTaskGetTickCount();   /* ���¿�ʼʱ�� */
   
   //1.���ж���ǰ�ж��ٸ�����;
    ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);
    
	addr = CARD_NO_HEAD_ADDR;    
    multiple = gRecordIndex.cardNoIndex / HEAD_NUM_SECTOR;

    if(multiple<1)
    {
        return;//����Ӧ�ò�̫���ܣ���Ϊֻ�д���1024��ʱ�򣬲Żᴥ���������
    }

    memset(gSectorBuff,0x00,sizeof(gSectorBuff));
    
    //2.���㵱ǰҳ��ַ
    addr += (multiple-1) * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU);

    //3.����ǰҳ
//    SPI_FLASH_BufferRead(gSectorBuff, addr ,HEAD_NUM_SECTOR * sizeof(HEADINFO_STRU));       

    ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, HEAD_NUM_SECTOR* sizeof(HEADINFO_STRU));
    if(ret == 0)
    {
        log_e("read fram error\r\n");
        return ;
    } 
    
    //����
    qSortCard(gSectorBuff,HEAD_NUM_SECTOR);

#ifdef DEBUG_PRINT
    for(int i=0;i<HEAD_NUM_SECTOR;i++)
    {
        log_d("sort page id the %d =%x\r\n",i,gSectorBuff[i].headData.id);
    }  
#endif  

    //д������
//    SPI_FLASH_BufferWrite(gSectorBuff, addr, HEAD_NUM_SECTOR * sizeof(HEADINFO_STRU));  
    
    ret = FRAM_Write ( FM24V10_1, addr, gSectorBuff,HEAD_NUM_SECTOR* sizeof(HEADINFO_STRU));
    
    if(ret == 0)
    {
        log_e("write fram error\r\n");
        return ;
    }  


	iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_d ( "sort FULL page card success��use time: %dms\r\n",iTime2 - iTime1 );  

	
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
       
   //1.���ж���ǰ�ж��ٸ�����;
    ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);
    
	addr = CARD_NO_HEAD_ADDR;    
    multiple = gRecordIndex.cardNoIndex / HEAD_NUM_SECTOR;
    remainder = gRecordIndex.cardNoIndex % HEAD_NUM_SECTOR;

    memset(gSectorBuff,0x00,sizeof(gSectorBuff));
    iTime1 = xTaskGetTickCount();   /* ���¿�ʼʱ�� */
    if(remainder != 0)
    {
        //2.�������һҳ��ַ
        addr += multiple * HEAD_NUM_SECTOR  * sizeof(HEADINFO_STRU);    
        
        //3.��ȡ���һҳ
        ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, remainder* sizeof(HEADINFO_STRU));
        if(ret == 0)
        {
            log_e("read fram error\r\n");
            return ;
        } 
        
        //5.����        
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
        addr = CARD_NO_HEAD_ADDR;//���㿪ʼ��;
        addr += i * HEAD_NUM_SECTOR  * CARD_USER_LEN; 
        memset(gSectorBuff,0x00,sizeof(gSectorBuff));
        
        //3.����ǰҳ      
        ret = FRAM_Read (FM24V10_1, addr, gSectorBuff, HEAD_NUM_SECTOR* sizeof(HEADINFO_STRU));
        if(ret == 0)
        {
            log_e("read fram error\r\n");
            return ;
        }         
        //����
        qSortCard(gSectorBuff,HEAD_NUM_SECTOR); 
        //д������
        ret = FRAM_Write ( FM24V10_1, addr, gSectorBuff,HEAD_NUM_SECTOR* sizeof(HEADINFO_STRU));        
        if(ret == 0)
        {
            log_e("write fram error\r\n");
            return ;
        }        
     }

	iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_e( "sort all card success��use time: %dms\r\n",iTime2 - iTime1 );  	
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


//дͨ�м�¼
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
        return ret;//дFLASHʧ��
    }
    
    memset(tmpBuf,0x00,sizeof(tmpBuf));
    bsp_sf_ReadBuffer(tmpBuf,addr,len);
    log_d("writeRecord = %s\r\n",tmpBuf);
    
    if(memcmp(tmpBuf,buf,len))
    {        
        return 0;//дFLASHʧ��
    }
    
    optAccessIndex(INCREMENT);
    return ret;
}

//��ͨ�м�¼
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


//���ͨ�м�¼
void clearRecord(void)
{
    eraseDataSector();
}



