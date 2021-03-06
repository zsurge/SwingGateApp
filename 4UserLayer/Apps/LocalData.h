/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : LocalData.h
  版 本 号   : 初稿
  作    者   :  
  生成日期   : 2020年3月21日
  最近修改   :
  功能描述   : LocalData.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2020年3月21日
    作    者   :  
    修改内容   : 创建文件

  FLASH划分
  0-0x200000：参数区域
  0x300000    存放bin文件
  0x400000    存放已存储卡号，已删除卡号，已存储用户ID，已删除用户ID，
  0x500000    存放卡号数据
  0x900000    存放用户数据
  0x1300000   预留

FLASH操作思路：
增：0.先读取已删除索引，若不为零，则把已删除最后一个索引值赋值给卡号索引并写卡号，
      已删除索引值自减，并且执行步骤2；
    1.若删除索引为零，以卡号为索引，存储在FLASH中，做为索引表，每增加一个卡号，索引自增；
    2.卡号关连内容为固定数据长度，存储时，写卡号数据地址+偏移量(索引*固定长度)
删：1.查找卡号，若有，查记录该卡索引值，存储在已删除空间内，且已删除索引值自增,
      若无，则返回未找到；
改：1.查找卡号，确定位置，修改相应的值，并写入到FLASH
查：1.查找卡号，确定位置，并返回相应的值  

******************************************************************************/
#ifndef __LOCALDATA_H__
#define __LOCALDATA_H__

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include "stm32f4xx.h" 
#include "ini.h"


#define CARD_USER_LEN              (4)

////设置卡状态为0，删除卡
#define CARD_DEL                    0
#define CARD_VALID                  1
#define USER_DEL                    CARD_DEL
#define USER_VALID                  CARD_VALID
#define TABLE_HEAD                  0xAA
#define INCREMENT                   1
#define DECLINE                     0

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/


/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

//add 2020.07.14
//读取/查找卡号,-1未找到，其它值为当前存储区域中的索引
int readHead(uint8_t *headBuff,uint8_t mode); 

//对缓冲区的卡号进行排序
void sortHead(HEADINFO_STRU *head,int length);

void qSortCard(HEADINFO_STRU *head,uint32_t length);

void sortLastPageCard(void);

//整页排序
void sortPageCard(void);


//添加卡号
uint32_t addCard(uint8_t *head,uint8_t mode);

//删除卡号
int delHead(uint8_t *headBuff,uint8_t mode);

//添加删除卡号对应的索引位置(想法有问题，暂不实现)
uint8_t addDelHead(int index);

//uint8_t addHeadID(uint8_t *head,uint8_t mode);

//写通行记录
uint8_t writeRecord(uint8_t *buf,int len);

//读通行记录
uint8_t readRecord(uint8_t *buf);

//清除通行记录
void clearRecord(void);

//针对所有数据排序
void manualSortCard(void);


#endif /* __LOCALDATA_H__ */
