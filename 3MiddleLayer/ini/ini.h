/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : ini.h
  �� �� ��   : ����
  ��    ��   : �Ŷ�
  ��������   : 2019��5��28��
  ����޸�   :
  ��������   : �Բ������в���
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2019��5��28��
    ��    ��   : �Ŷ�
    �޸�����   : �����ļ�

******************************************************************************/

#ifndef __INI_H
#define __INI_H

#include "easyflash.h"
#include "version.h"
#include "string.h"
#include "deviceInfo.h"
#include "tool.h"
#include "eth_cfg.h"
#include "cJSON.h"
#include "errorcode.h"

#include "malloc.h"
#include "bsp_MB85RC128.h"
#include "bsp_ds1302.h"

#define CARD_NO_LEN_ASC     8       //����ASC�볤�ȣ�״̬(1)+����(3)
#define CARD_NO_LEN_BCD     (CARD_NO_LEN_ASC/2) //����BCD�볤��
#define HEAD_lEN 4                  //ÿ����¼4�ֽڿ��ţ�
#define DEL_HEAD_lEN 4              //ÿ����¼ռ4���ֽ�,4�ֽ�Ϊ��ɾ����������(M*512+R)
#define MAX_HEAD_RECORD     30720   //���30720����¼
#define SECTOR_SIZE         4096    //ÿ��������С

#define MAX_HEAD_DEL_CARDNO     512   //��������ɾ��256�ſ�


#define CARD_NO_HEAD_SIZE   (HEAD_lEN*MAX_HEAD_RECORD)  //120K
#define CARD_DEL_HEAD_SIZE  (DEL_HEAD_lEN*MAX_HEAD_DEL_CARDNO)   //2K



#define CARD_HEAD_SECTOR_NUM     (CARD_NO_HEAD_SIZE/SECTOR_SIZE) //30������
#define HEAD_NUM_SECTOR          (SECTOR_SIZE/HEAD_lEN) //ÿ�������洢1024������

//��Ϊ�洢������
#define CARD_NO_HEAD_ADDR   0x0000
#define CARD_DEL_HEAD_ADDR  (CARD_NO_HEAD_ADDR+CARD_NO_HEAD_SIZE)


//���������洢���ȼ���ַ
#define DEVICE_BASE_PARAM_SIZE (896)
#define DEVICE_BASE_PARAM_ADDR (CARD_DEL_HEAD_ADDR+CARD_DEL_HEAD_SIZE)


//��ͷ�������洢���ȼ���ַ
#define RECORD_INDEX_SIZE   (128)
#define RECORD_INDEX_ADDR   (DEVICE_BASE_PARAM_ADDR+DEVICE_BASE_PARAM_SIZE)

//�����Ĵ洢��ַ
#define DEVICE_TEMPLATE_PARAM_SIZE   (1024*2)
#define DEVICE_TEMPLATE_PARAM_ADDR  (RECORD_INDEX_ADDR+RECORD_INDEX_SIZE) //�����洢����4K�ռ�


//FLASH�ռ����
#define ACCESS_RECORD_ADDR       0x500000
#define ACCESS_RECORD_ADDR_END   0x900000
#define DATA_SECTOR_NUM          ((ACCESS_RECORD_ADDR_END-ACCESS_RECORD_ADDR)/SECTOR_SIZE)

#define CARD_MODE                   1 //��ģʽ
#define CARD_DEL_MODE               2 //ɾ����ģʽ
#define NO_FIND_HEAD                (-1)
#define RECORD_MAX_LEN              128 //ÿ�����м�¼����󳤶�
#define RECORD_REAL_LEN              125 //ÿ�����м�¼����ʵ����


//��ͷ����
typedef union
{
	uint32_t id;        //����
	uint8_t sn[4];    //���Ű��ַ�
}HEADTPYE;

typedef struct CARDHEADINFO
{
    HEADTPYE headData;  //����
}HEADINFO_STRU;

extern HEADINFO_STRU gSectorBuff[1024];




typedef uint8_t(*CallBackParam)(void * struParam,uint8_t mode,uint32_t len,uint32_t addr);

void readTemplateData(void);

void RestoreDefaultSetting(void);

void SystemUpdate(void);

void SaveDevState(uint32_t     state);



//����ģ����Ϣ
SYSERRORCODE_E saveTemplateParam(uint8_t *jsonBuff);

//add 2020.07.06
void initTemplateParam(void);
uint8_t optTemplateParam(void *stParam,uint8_t mode,uint32_t len,uint32_t addr);

void initDevBaseParam(void);
uint8_t optDevBaseParam(void *stParam,uint8_t mode,uint32_t len,uint32_t addr);

void initRecordIndex(void);
uint8_t optRecordIndex(RECORDINDEX_STRU *recoIndex,uint8_t mode);


void ClearDevBaseParam(void);
void ClearTemplateParam(void);
void ClearRecordIndex(void);
void clearTemplateFRAM(void);


void eraseHeadSector(void);
void eraseDataSector(void);
void eraseUserDataAll(void);

void TestFlash(uint8_t mode);


#endif
