/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : pub_options.h
  版 本 号   : 初稿
  作    者   :  
  生成日期   : 2020年2月25日
  最近修改   :
  功能描述   : FreeRTOS中事件及通知等公共参数的定义
  函数列表   :
  修改历史   :
  1.日    期   : 2020年2月25日
    作    者   :  
    修改内容   : 创建文件

******************************************************************************/
#ifndef __PUB_OPTIONS_H__
#define __PUB_OPTIONS_H__

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "semphr.h"
#include "event_groups.h"

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define  QUEUE_LEN    10     /* 队列的长度，最大可包含多少个消息 */
#define  CARD_QUEUE_LEN    20     /* 队列的长度，最大可包含多少个消息 */

#define READER1         1           
#define READER2         2

#define READMODE        1 
#define SORT_CARD_MODE 2
#define REMOTE_OPEN_MODE 3
#define DEL_CARD_MODE 4
#define MANUAL_SORT     (10)


    
//事件标志
#define TASK_BIT_0	 (1 << 0)
#define TASK_BIT_1	 (1 << 1)
#define TASK_BIT_2	 (1 << 2)
#define TASK_BIT_3	 (1 << 3)
#define TASK_BIT_4	 (1 << 4)
#define TASK_BIT_5	 (1 << 5)



    
    
#define TASK_BIT_ALL ( TASK_BIT_0 | TASK_BIT_1 | TASK_BIT_2 )
    
//#define TASK_BIT_ALL ( TASK_BIT_1  |TASK_BIT_3|TASK_BIT_4|TASK_BIT_5)

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

#define MAX_CMD_LEN
 typedef struct CMD_BUFF
 {
    uint8_t cmd_len; 
    uint8_t cmd[32];
 }CMD_BUFF_STRU;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/ 
//事件句柄
extern EventGroupHandle_t xCreatedEventGroup;
extern SemaphoreHandle_t gxMutex;
extern QueueHandle_t xCmdQueue; 
extern QueueHandle_t xCardIDQueue; 

extern QueueHandle_t xComm4Queue;

extern CMD_BUFF_STRU gCmd_buff,gCmd4_buff;

extern uint8_t gdevParam[11];
extern uint8_t gdev4Param[11];


/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/



#endif /* __PUB_OPTIONS_H__ */

