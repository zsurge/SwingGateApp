/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : comm.h
  版 本 号   : 初稿
  作    者   : 张舵
  生成日期   : 2019年6月18日
  最近修改   :
  功能描述   : 串口通讯协议解析及处理
  函数列表   :
  修改历史   :
  1.日    期   : 2019年6月18日
    作    者   : 张舵
    修改内容   : 创建文件

******************************************************************************/
#ifndef __CMDHANDLE_H
#define __CMDHANDLE_H

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

#include "errorcode.h"




/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define MQTT_MAX_LEN 1024*2
#define MQTT_TEMP_LEN 300


#define AUTH_MODE_RELIEVECONTROL        1
//IC卡
#define AUTH_MODE_CARD      2
//远程
#define AUTH_MODE_REMOTE    8
//解绑
#define AUTH_MODE_UNBIND    5
//绑定
#define AUTH_MODE_BIND      6
//二维码
#define AUTH_MODE_QR        7

#define QUEUE_BUF_LEN   512


#define CARD_ID_LEN     4



#pragma pack(1)
typedef struct
{
    uint8_t cardID[CARD_ID_LEN];           //需要发送给服务器的数据
                                           //最后一个字节 =0 DISABLE 禁止发送; = 1 ENABLE 允许发送
    uint8_t devID;                          // = 1 1号读卡器；=2 2号读卡器
    uint8_t mode;                           // = 1 读卡
}READER_BUFF_STRU;
#pragma pack()




extern READER_BUFF_STRU gReaderMsg,gReaderRecvMsg;



extern int gMySock;
extern uint8_t gUpdateDevSn; 


//发送消息到服务器
int mqttSendData(uint8_t *payload_out,uint16_t payload_out_len); 

SYSERRORCODE_E exec_proc ( char* cmd_id, uint8_t *msg_buf );

void Proscess(void* data);

#endif

