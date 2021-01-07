/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : pub_options.h
  �� �� ��   : ����
  ��    ��   :  
  ��������   : 2020��2��25��
  ����޸�   :
  ��������   : FreeRTOS���¼���֪ͨ�ȹ��������Ķ���
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2020��2��25��
    ��    ��   :  
    �޸�����   : �����ļ�

******************************************************************************/
#ifndef __PUB_OPTIONS_H__
#define __PUB_OPTIONS_H__

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "semphr.h"
#include "event_groups.h"

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#define  QUEUE_LEN    10     /* ���еĳ��ȣ����ɰ������ٸ���Ϣ */
#define  CARD_QUEUE_LEN    20     /* ���еĳ��ȣ����ɰ������ٸ���Ϣ */

#define READER1         1           
#define READER2         2

#define READMODE        1 
#define SORT_CARD_MODE 2
#define REMOTE_OPEN_MODE 3
#define DEL_CARD_MODE 4
#define MANUAL_SORT     (10)


    
//�¼���־
#define TASK_BIT_0	 (1 << 0)
#define TASK_BIT_1	 (1 << 1)
#define TASK_BIT_2	 (1 << 2)
#define TASK_BIT_3	 (1 << 3)
#define TASK_BIT_4	 (1 << 4)
#define TASK_BIT_5	 (1 << 5)



    
    
#define TASK_BIT_ALL ( TASK_BIT_0 | TASK_BIT_1 | TASK_BIT_2 )
    
//#define TASK_BIT_ALL ( TASK_BIT_1  |TASK_BIT_3|TASK_BIT_4|TASK_BIT_5)

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

#define MAX_CMD_LEN
 typedef struct CMD_BUFF
 {
    uint8_t cmd_len; 
    uint8_t cmd[32];
 }CMD_BUFF_STRU;

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/ 
//�¼����
extern EventGroupHandle_t xCreatedEventGroup;
extern SemaphoreHandle_t gxMutex;
extern QueueHandle_t xCmdQueue; 
extern QueueHandle_t xCardIDQueue; 

extern QueueHandle_t xComm4Queue;

extern CMD_BUFF_STRU gCmd_buff,gCmd4_buff;

extern uint8_t gdevParam[11];
extern uint8_t gdev4Param[11];


/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/



#endif /* __PUB_OPTIONS_H__ */

