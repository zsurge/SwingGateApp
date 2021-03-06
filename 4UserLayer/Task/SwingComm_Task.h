/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : SwingComm_Task.h
  版 本 号   : 初稿
  作    者   :  
  生成日期   : 2021年1月6日
  最近修改   :
  功能描述   : SwingComm_Task.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2021年1月6日
    作    者   :  
    修改内容   : 创建文件

******************************************************************************/
#ifndef __SWINGCOMM_TASK_H__
#define __SWINGCOMM_TASK_H__

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include "pub_options.h"


/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
extern TaskHandle_t xHandleTaskSwingComm;      //跟主机通讯
/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/
void CreateSwingCommTask(void);





#endif /* __SWINGCOMM_TASK_H__ */
