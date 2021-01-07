/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : Comm_Task.c
  版 本 号   : 初稿
  作    者   :  
  生成日期   : 2020年2月28日
  最近修改   :
  功能描述   : 跟电梯通讯的任务处理文件
  函数列表   :
  修改历史   :
  1.日    期   : 2020年2月28日
    作    者   :  
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#define LOG_TAG    "CommTask"
#include "elog.h"
#include "bsp_time.h"
#include "Comm_Task.h"
#include "CmdHandle.h"
#include "bsp_usart5.h"
#include "malloc.h"
#include "bsp_uart_fifo.h"



/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
 
#define COMM_TASK_PRIO		(tskIDLE_PRIORITY + 8) 
#define COMM_STK_SIZE 		(configMINIMAL_STACK_SIZE*8)

#define SPACE		        			0x00
#define FINISH		       	 			0x55

#define STEP1   0
#define STEP2   10
#define STEP3   20

#define READ_DEV_STATUS     0x05
#define SET_DEV_PARAM       0x0A
#define READ_DEV_PARAM      0x0B
#define SET_DEV_OPEN        0x06
#define RESET_DEV           0x08
#define RESET_FACTORY       0x09

#define LEFT_OPEN           0x4C
#define RIGHT_OPEN          0x52

typedef struct FROMHOST
{
    uint8_t rxStatus;                   //接收状态
    uint8_t rxCRC;                       //校验值.
//    uint8_t rxLen;                     //需要接收的字节长度
    uint8_t rxCnt;                     //已接收字节数  
    uint8_t rxBuff[32];                 //接收字节
    uint8_t rxPacketState;              //整个数据包接收状态 
}FROMHOST_STRU;

typedef enum
{
    NO_INIT=0x31,
    LEFT_OK,
    LEFT_OPENING,
    RIGHT_OK,
    RIGHT_OPENING,
    CLOSE_OK,
    CLOSE_OPTING
    
}DEV_STATE_INDEX;


typedef enum
{
    STANDBY= 0x31,
    HOST_OPEN,
    INDUCTION_OPEN,
    ANTI_PINCH_OPEN,    
    OBSTRUCT_OPEN,
    KEY_OPEN
}OPEN_MODE_INDEX;

typedef enum
{
    NORMAL=0x30,
    LEFT1_WARING,
    LEFT2_WARING,
    LEFT3_WARING,
    LEFT4_WARING,
    RIGHT1_WARING,
    RIGHT2_WARING,
    RIGHT3_WARING,
    RIGHT4_WARING,
    DETENTION_TIMEOUT
}SENSOR_STATE_INDEX;

typedef enum
{
    HOST_NORMAL=0x31,
    RETROGRADE_ALARM = 0x33,
    FOLLOWING_ALARM,
    KEY_ERROR_ALARM,
    DEV_LOST_CONNECT_ALARM 
}HOST_STATE_INDEX;

typedef enum
{   
   OUTSIDE_TIPS=0,   
   FOLLOWING_TIPS,  
   RESET_TIPS,   
   RETROGRADE_TIPS,
   LINK_OPEN_TIPS,     
   OPENDOOR_TIPS, 
   CLOSEDOOR_TIPS,  
   RETENTION_TIPS,  
   WELCOME_TIPS,
   LOST_CONNECT
}HOST_TIPS_INDEX;

typedef struct DEV_STATE
{
    uint8_t curState;			    //当前状态值
	uint8_t preState;		        //上一次状态值          
}DEV_STATE_STRU;





const char * const dev_info[] =
{
    [NO_INIT] =         "设备未初始化",
    [LEFT_OK] =         "左开门完成",
    [LEFT_OPENING] =    "左开门中",
    [RIGHT_OK] =        "右开门完成",
    [RIGHT_OPENING] =   "右开门中",
    [CLOSE_OK] =        "关门完成",
    [CLOSE_OPTING] =    "关门中",
};

const char * const openMode[] =
{
    [STANDBY] =         "待机",
    [HOST_OPEN] =       "上位机开门",
    [INDUCTION_OPEN] =  "感应开门",
    [ANTI_PINCH_OPEN] = "防夹开门",
    [OBSTRUCT_OPEN] =   "遇阻开门",
    [KEY_OPEN] =        "按键开门",
};


const char * const sensorStateInfo[] =
{
    [NORMAL] =              "正常",
    [LEFT1_WARING] =        "左1红外告警",
    [LEFT2_WARING] =        "左2红外告警",
    [LEFT3_WARING] =        "左3红外告警",
    [LEFT4_WARING] =        "左4红外告警",
    [RIGHT1_WARING] =       "右1红外告警",
    [RIGHT2_WARING] =       "右2红外告警",
    [RIGHT3_WARING] =       "右3红外告警",
    [RIGHT4_WARING] =       "右4红外告警",
    [DETENTION_TIMEOUT] =   "滞留通道超时",
};

const char * const hostStateInfo[] =
{
    [HOST_NORMAL] =            "正常",
    [RETROGRADE_ALARM] =       "逆行告警",
    [FOLLOWING_ALARM] =        "尾随告警",
    [KEY_ERROR_ALARM] =        "右开门完成",
    [DEV_LOST_CONNECT_ALARM] = "右开门中",
};



const char * const alarmInfo[] =
{
    [OUTSIDE_TIPS] =            "请站在通道外识别通行",
    [FOLLOWING_TIPS] =          "请勿尾随，请站在通道外识别通行",
    [RESET_TIPS] =              "请不要推门，设备自动复位中",
    [RETROGRADE_TIPS] =         "禁止逆行，请识别通行" ,
    [LINK_OPEN_TIPS] =          "联动开闸",
    [OPENDOOR_TIPS] =           "开门",
    [CLOSEDOOR_TIPS] =          "关门",
    [RETENTION_TIPS] =          "请不要在通道内停留",
    [WELCOME_TIPS] =            "欢迎光临",
    [LOST_CONNECT] =            "通讯故障"
};

static FROMHOST_STRU rxFromHost;
static char gChannel = 0;
static DEV_STATE_STRU gDevState;

CMD_BUFF_STRU gCmd_buff = {0};
uint8_t gdevParam[11] = {0};

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/
const char *CommTaskName = "vCommTask"; 

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
TaskHandle_t xHandleTaskComm = NULL;  

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/
static void vTaskComm(void *pvParameters);
void deal_rx_Parse(void);
void deal_rx_data(void);
void init_serial_buf(void);

void queryStatusResponse(void);
void setDevParamResponse(void);
void readDevParamResponse(void);
void optDevOpenResponse(char channel);
void resetDevResponse(void);
void resetFactoryResponse(void);


void sendToSpeak(uint8_t *dat);
void writeLog(char *dat);


void CreateCommTask(void)
{
    xTaskCreate((TaskFunction_t )vTaskComm,         
                (const char*    )CommTaskName,       
                (uint16_t       )COMM_STK_SIZE, 
                (void*          )NULL,              
                (UBaseType_t    )COMM_TASK_PRIO,    
                (TaskHandle_t*  )&xHandleTaskComm);
}


static void vTaskComm(void *pvParameters)
{ 
    BaseType_t xReturn = pdTRUE;/* 定义一个创建信息返回值，默认为pdPASS */
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(50); /* 设置最大等待时间为100ms */ 

    const char readDevState[7] = { 0x02,0x00,0x06,0x01,0x05,0x03,0x03 };
    
    CMD_BUFF_STRU *ptCmd = &gCmd_buff;
    
    memset(&gCmd_buff,0x00,sizeof(CMD_BUFF_STRU));   
    

    while(1)
    { 

        xReturn = xQueueReceive( xCmdQueue,    /* 消息队列的句柄 */
                                 (void *)&ptCmd,  /*这里获取的是结构体的地址 */
                                 xMaxBlockTime); /* 设置阻塞时间 */
        if(pdTRUE == xReturn)
        {  
            gChannel = ptCmd->cmd[5];
            //发送指令
            dbh("ptCmd->cmd", ptCmd->cmd, ptCmd->cmd_len);
            RS485_SendBuf(COM6, ptCmd->cmd, ptCmd->cmd_len); 

//            sendToSpeak((uint8_t *)alarmInfo[OPENDOOR_TIPS]);
            
        }
        else
        {
            //发送查询        
            RS485_SendBuf(COM6, (uint8_t *)readDevState, 7); 
        }

        vTaskDelay(50);    
        
        deal_rx_Parse();
        deal_rx_data(); 
        
        xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_1);   
        
    }  
}



void deal_rx_Parse(void)
{
    uint8_t ch = 0;
    uint16_t dataLen = 0;
    
    
    memset(&rxFromHost,0x00,sizeof(FROMHOST_STRU));   
    
    while(RS485_Recv(COM6,&ch,1))
    {       
       switch (rxFromHost.rxStatus)
        {                
            case STEP1:
                if(0x02 == ch) /*接收包头*/
                {
                    rxFromHost.rxBuff[0] = ch;                    
                    rxFromHost.rxCnt = 1;
                    rxFromHost.rxCRC = ch;
                    rxFromHost.rxStatus = STEP2;
                }
                break;
           case STEP2:
                if (0x03 == ch) 
                {
                    rxFromHost.rxStatus = STEP3;
                }
                rxFromHost.rxBuff[rxFromHost.rxCnt++] = ch; //ETX
                rxFromHost.rxCRC ^=  ch;
                break;
          case STEP3:

//                dbh("recv", rxFromHost.rxBuff, rxFromHost.rxCnt);
                
                dataLen = ((uint16_t)rxFromHost.rxBuff[1] << 8) |(uint16_t)(rxFromHost.rxBuff[2]);;

                if(dataLen == rxFromHost.rxCnt && rxFromHost.rxCRC == ch)
                {
//                    log_d("-----------CMD OK-----------\r\n");
                     rxFromHost.rxBuff[rxFromHost.rxCnt++] = ch;
                     rxFromHost.rxPacketState = FINISH;                    
                     rxFromHost.rxStatus = STEP1;
                     break;
                }
                else
                {                
                    log_d("-----------CRC ERR-----------\r\n");
                    rxFromHost.rxPacketState = FINISH;  
                    rxFromHost.rxBuff[rxFromHost.rxCnt++] = ch;
                    rxFromHost.rxStatus = STEP2;
                    rxFromHost.rxCRC ^=  ch;
                }
                break;         
            default:
                if (rxFromHost.rxPacketState == SPACE) 
                {
                    rxFromHost.rxPacketState = FINISH;
                    rxFromHost.rxStatus = STEP1;
                } 
         }
    }  
}

void deal_rx_data(void)
{
    uint8_t buf[256] = {0};

    memset(buf,0x00,sizeof(buf));  
    
    if (FINISH == rxFromHost.rxPacketState)
    {   
        if((((uint16_t)rxFromHost.rxBuff[1] << 8) |(uint16_t)(rxFromHost.rxBuff[2])) == rxFromHost.rxCnt-1)                                   //解析02数据包
        {   
            if(rxFromHost.rxBuff[5] != 0x4f)
            {
                log_d("set dev param error\r\n");
            }   

            dbh("deal_rx_data", rxFromHost.rxBuff, rxFromHost.rxCnt);
            
            switch(rxFromHost.rxBuff[4])
            {
                case READ_DEV_PARAM:
                     if(rxFromHost.rxBuff[2] == 0x11)
                     {
                         memcpy(gdevParam,rxFromHost.rxBuff+5,11);
                     }
                     break;
                default:
                     break;
                
            }
            
        }
        else
        {
            init_serial_buf();
        }
    }
}

void init_serial_buf(void)
{ 
    memset(&rxFromHost,0x00,sizeof(FROMHOST_STRU));
}

void queryStatusResponse(void)
{    
    gDevState.curState = rxFromHost.rxBuff[5];
    
    switch(rxFromHost.rxBuff[5])
    {
        case NO_INIT:
            writeLog((char *)dev_info[NO_INIT]);    
//            log_d(" 1  未初始化\r\n");
            
            break;
        case LEFT_OK:
            writeLog((char *)dev_info[LEFT_OK]);
//            log_d(" 1 左开门OK\r\n");
            break;
        case LEFT_OPENING:
            writeLog((char *)dev_info[LEFT_OPENING]);
//            log_d("  1  左开门中。。。\r\n");
            break;
        case RIGHT_OK:
            writeLog((char *)dev_info[RIGHT_OK]);
//            log_d("  1 右开门OK\r\n");
            
            break;
        case RIGHT_OPENING:
            writeLog((char *)dev_info[RIGHT_OPENING]);
            
//            log_d(" 1 右开门中。。。\r\n");
            break;
        case CLOSE_OK:
            writeLog((char *)dev_info[CLOSE_OK]);             
//            log_d(" 1  关门OK\r\n");
            break;     
        case CLOSE_OPTING:
            writeLog((char *)dev_info[CLOSE_OPTING]);
            break;              
    }

    if(gDevState.curState != gDevState.preState)
    {
        gDevState.preState = gDevState.curState;
        if(gDevState.curState == CLOSE_OPTING)
        {            
            sendToSpeak((uint8_t *)alarmInfo[CLOSEDOOR_TIPS]);
        }
    }

    switch(rxFromHost.rxBuff[6])
    {
        case STANDBY:
            writeLog((char *)openMode[STANDBY]);
            
//            log_d(" 2 待机\r\n");
            break;
        case HOST_OPEN:
            writeLog((char *)openMode[HOST_OPEN]);
//            log_d(" 2 上位机开门\r\n");
            
            break;
        case INDUCTION_OPEN:
            writeLog((char *)openMode[INDUCTION_OPEN]);
//            log_d(" 2 感应开门\r\n");
            
            break;
        case ANTI_PINCH_OPEN:
            writeLog((char *)openMode[ANTI_PINCH_OPEN]);
//            log_d(" 2 防夹开门\r\n");
            
            break;
        case OBSTRUCT_OPEN:
            writeLog((char *)openMode[OBSTRUCT_OPEN]);
//            log_d(" 2 遇阻开门\r\n");
            
            break;
        case KEY_OPEN:
            writeLog((char *)openMode[KEY_OPEN]);    
//            log_d(" 2 按键开门\r\n");
            
            break;
    }


    switch(rxFromHost.rxBuff[7])
    {
        case NORMAL:
            writeLog((char *)sensorStateInfo[NORMAL]);
//            log_d(" 3 sensor normal\r\n");
            break;
        case LEFT1_WARING:
            writeLog((char *)sensorStateInfo[LEFT1_WARING]);
            
            log_d(" 3 sensor LEFT1_WARING\r\n");
            break;
        case LEFT2_WARING:
            writeLog((char *)sensorStateInfo[LEFT2_WARING]);
            
            log_d(" 3 sensor LEFT2_WARING\r\n");
            break;
        case LEFT3_WARING:
            writeLog((char *)sensorStateInfo[LEFT3_WARING]);
            
            log_d(" 3 sensor LEFT3_WARING\r\n");
            break;
        case LEFT4_WARING:
            writeLog((char *)sensorStateInfo[LEFT4_WARING]);
            
            log_d(" 3 sensor LEFT4_WARING\r\n");
            break;
        case RIGHT1_WARING:
            writeLog((char *)sensorStateInfo[RIGHT1_WARING]);    
            
            log_d(" 3 sensor RIGHT1_WARING\r\n");
            break;     
        case RIGHT2_WARING:
            writeLog((char *)sensorStateInfo[RIGHT2_WARING]); 
            
            log_d(" 3 sensor RIGHT2_WARING\r\n");
            break;     
        case RIGHT3_WARING:
            writeLog((char *)sensorStateInfo[RIGHT3_WARING]);
            
            log_d(" 3 sensor RIGHT3_WARING\r\n");
            break;
        case RIGHT4_WARING:
            writeLog((char *)sensorStateInfo[RIGHT4_WARING]);    
            
            log_d(" 3 sensor RIGHT4_WARING\r\n");
            break;     
        case DETENTION_TIMEOUT:
            writeLog((char *)sensorStateInfo[DETENTION_TIMEOUT]); 
            log_d(" 3 sensor DETENTION_TIMEOUT %d\r\n",gPlayTimer);
    
            if(!gPlayTimer)
            {
                gPlayTimer = 18000;
                sendToSpeak((uint8_t *)alarmInfo[RETENTION_TIPS]);   
            }
            break;              
    }    

//    gAbnormalState.curState =  rxFromHost.rxBuff[8];
    switch(rxFromHost.rxBuff[8])
    {
        case HOST_NORMAL: 
            writeLog((char *)hostStateInfo[HOST_NORMAL]);     
//            log_d(" 4----正常----\r\n");
            break;
        case RETROGRADE_ALARM:
            writeLog((char *)hostStateInfo[RETROGRADE_ALARM]);  
            log_d(" 4----逆行----\r\n");  

            if(!gRetrogradeTimer)
            {
                gRetrogradeTimer = 18000;
                sendToSpeak((uint8_t *)alarmInfo[RETROGRADE_TIPS]);   
            }
            
            break;
        case FOLLOWING_ALARM:
            writeLog((char *)hostStateInfo[FOLLOWING_ALARM]);
            if(!gFollowTimer)
            {
                gFollowTimer = 18000;
                sendToSpeak((uint8_t *)alarmInfo[FOLLOWING_TIPS]);   
            }            
            
            log_d(" 4----尾随----\r\n");
            break;
        case KEY_ERROR_ALARM:
            writeLog((char *)hostStateInfo[KEY_ERROR_ALARM]); 
            log_d(" 4----按键错误----\r\n");
            break;
        case DEV_LOST_CONNECT_ALARM:
            writeLog((char *)hostStateInfo[DEV_LOST_CONNECT_ALARM]);
            
            log_d(" 4----驱动器通信异常----\r\n");
            break;          
    }    
}

void setDevParamResponse(void)
{
    if(rxFromHost.rxBuff[5] != 0x4f)
    {
        log_d("set dev param error\r\n");
    }
    
}
void readDevParamResponse(void)
{
    if(rxFromHost.rxBuff[2] == 0x11)
    {
        memcpy(gdevParam,rxFromHost.rxBuff+5,11);
    }
}
void optDevOpenResponse(char channel)
{

    if(channel == 0x4c)
    {
        if(rxFromHost.rxBuff[5] != 0x4f)
        {
            log_d("left open error\r\n");
        }
        else
        {
            //打包数据
            //发送到MQTT
        }
    }
    else if(channel == 0x52)
    {
        if(rxFromHost.rxBuff[5] != 0x4f)
        {
            log_d("right open error\r\n");
        }
        else
        {
            //打包数据
            //发送到MQTT
        }
    }
    else
    {
        log_d("param error\r\n");
    }

    
}
void resetDevResponse(void)
{
    if(rxFromHost.rxBuff[5] != 0x4f)
    {
        log_d("reset error\r\n");
    }
}
void resetFactoryResponse(void)
{
    if(rxFromHost.rxBuff[5] != 0x4f)
    {
        log_d("reset Factory error\r\n");
    }
}


//语音内容增加通信协议头等
void sendToSpeak(uint8_t *dat)   
{
    //协议格式:7E Len 3C DATA CRC 
    static uint8_t addr[128] = {0};
    uint8_t crc=0;
    uint8_t i = 0;
    uint8_t len;

    memset(addr,0x00,sizeof(addr));

    addr[0] = 0x7E;
    addr[1] = strlen((const char*)dat) + 2;
    addr[2] = 0x3c;

    memcpy(addr+3,dat,strlen((const char*)dat));

    len = strlen((const char*)addr);

    for(i=0; i<len; i++)crc^= addr[i];

    addr[len] = crc;

//    dbh("sendToSpeak", addr, len+1);
    RS485_SendBuf(COM4,addr,len+1);        
}



void writeLog(char *dat) 
{
    
}



