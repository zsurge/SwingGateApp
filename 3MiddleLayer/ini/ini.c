/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : ini.c
  �� �� ��   : ����
  ��    ��   : �Ŷ�
  ��������   : 2019��5��28��
  ����޸�   :
  ��������   : ��������
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2019��5��28��
    ��    ��   : �Ŷ�
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#define LOG_TAG    "ini"
#include "elog.h"

#include "ini.h"
#include "stdlib.h"
#include "bsp_spi_flash.h"


TEMPLATE_PARAM_STRU gtemplateParam;
DEV_BASE_PARAM_STRU gDevBaseParam;
RECORDINDEX_STRU gRecordIndex;

HEADINFO_STRU gSectorBuff[1024] = {0};


static uint8_t opParam(void *Param,uint8_t mode,uint32_t len,uint32_t addr);

static void eraseUserDataIndex ( void );


/*****************************************************************************
 �� �� ��  : RestoreDefaultSetting
 ��������  : �ָ�Ĭ������,ʵ����д��ϵͳ��������Ҫ�Ĳ���
           ��ʹ��ef_env_set_default ����Ҫ��ef_port.c�ж�Ĭ��ֵ�����޸�
 �������  : 
             
 �������  : ��
 �� �� ֵ  : void
 
 �޸���ʷ      :
  1.��    ��   : 2019��5��28��
    ��    ��   : �Ŷ�

    �޸�����   : �����ɺ���

*****************************************************************************/
void RestoreDefaultSetting(void)
{
    if(ef_env_set_default()== EF_NO_ERR)
    {
        DBG("RestoreDefaultSetting success\r\n");
    }
    
}

void SystemUpdate(void)
{
    //д������־λ
    if(ef_set_env("WSPIFLASH","5050") == EF_NO_ERR)
    {
        ef_set_env("WMCUFLASH","6060");
        //jump iap
        DBG("jump iap\r\n");
        NVIC_SystemReset();
    }
}

void SaveDevState(uint32_t state)
{
    uint8_t ret = 0;
    //��¼SN
    ClearDevBaseParam();
    optDevBaseParam(&gDevBaseParam,READ_PRARM,sizeof(DEV_BASE_PARAM_STRU),DEVICE_BASE_PARAM_ADDR);
    gDevBaseParam.deviceState.iFlag = state; 
    ret = optDevBaseParam(&gDevBaseParam,WRITE_PRARM,sizeof(DEV_BASE_PARAM_STRU),DEVICE_BASE_PARAM_ADDR);

    if(ret != 1)
    {
        log_e("write device state error\r\n");
    }    
}

//����ģ����Ϣ
SYSERRORCODE_E saveTemplateParam(uint8_t *jsonBuff)
{
    SYSERRORCODE_E result = NO_ERR;
    cJSON* root,*data,*templateData,*templateMap,*holidayTimeMap,*peakTimeMap;   
    cJSON* tmpArray,*arrayElement;
    
    int holidayTimeMapCnt=0,peakTimeMapCnt=0,index = 0;
    uint8_t ret = 0;
    char tmpbuf[8] = {0};
    char tmpIndex[2] = {0};
    char tmpKey[32] = {0};
    char *callingWay[4] = {0}; //��ŷָ������ַ��� 
    int callingWayNum = 0;

    TEMPLATE_PARAM_STRU *templateParam = &gtemplateParam; 

    uint32_t curtick  =  xTaskGetTickCount();    

    ClearTemplateParam();    
    ret = optTemplateParam(templateParam,READ_PRARM,sizeof(TEMPLATE_PARAM_STRU),DEVICE_TEMPLATE_PARAM_ADDR); 
    if(ret != NO_ERR)
    {
        return FLASH_R_ERR;
    }
    
    root = cJSON_Parse((char *)jsonBuff);    //�������ݰ�
    if (!root)  
    {  
        cJSON_Delete(root);
        log_d("Error before: [%s]\r\n",cJSON_GetErrorPtr());  
        return CJSON_PARSE_ERR;
    } 

    data = cJSON_GetObjectItem(root, "data");
    if(data == NULL)
    {
        log_d("data NULL\r\n");
        result = CJSON_GETITEM_ERR;
        cJSON_Delete(root);
        return result;        
    }    

    templateData = cJSON_GetObjectItem(data, "template");
    if(templateMap == NULL)
    {
        log_d("templateData NULL\r\n");
        result = CJSON_GETITEM_ERR;
        cJSON_Delete(root);
        return result;   
    }    

    templateMap = cJSON_GetObjectItem(templateData, "templateMap");
    if(templateMap == NULL)
    {
        log_d("templateMap NULL\r\n");
        result = CJSON_GETITEM_ERR;
        cJSON_Delete(root);
        return result;   
    }

    holidayTimeMap = cJSON_GetObjectItem(templateData, "hoildayTimeMap");
    if(holidayTimeMap == NULL)
    {
        log_d("hoildayTimeMap NULL\r\n");
        result = CJSON_GETITEM_ERR;
        cJSON_Delete(root);
        return result;   
    }

    peakTimeMap = cJSON_GetObjectItem(templateData, "peakTimeMap");
    if(peakTimeMap ==NULL)
    {
        log_d("peakTimeMap NULL\r\n");
        result = CJSON_GETITEM_ERR;
        cJSON_Delete(root);
        return result;   
    }


//--------------------------------------------------    
    //��ȡtemplateMap����
    cJSON *json_item = cJSON_GetObjectItem(templateMap, "id");
    templateParam->id = json_item->valueint;
//    sprintf(tmpbuf,"%8d",templateParam->id);
//    ef_set_env_blob("templateID",tmpbuf,8); 
    log_d("templateParam->id = %d\r\n",templateParam->id);

    json_item = cJSON_GetObjectItem(templateMap, "templateCode");
    strcpy((char *)templateParam->templateCode,json_item->valuestring);
//    ef_set_env_blob("templateCode",templateParam->templateCode,strlen((const char *)templateParam->templateCode)); 
    log_d("templateParam->templateCode = %s\r\n",templateParam->templateCode);

    json_item = cJSON_GetObjectItem(templateMap, "templateName");
    strcpy((char *)templateParam->templateName,json_item->valuestring);
//    ef_set_env_blob("templateName",templateParam->templateName,strlen(templateParam->templateName)); 
    log_d("templateParam->templateName = %s\r\n",templateParam->templateName);    

    json_item = cJSON_GetObjectItem(templateMap, "templateStatus");
    templateParam->templateStatus = json_item->valueint;
//    memset(tmpbuf,0x00,sizeof(tmpbuf));
//    sprintf(tmpbuf,"%8d",templateParam->templateStatus);
//    ef_set_env_blob("templateStatus",tmpbuf,8); 
    log_d("templateParam->templateStatus = %d\r\n",templateParam->templateStatus);      
    
    json_item = cJSON_GetObjectItem(templateMap, "callingWay");
    strcpy((char *)templateParam->callingWay,json_item->valuestring);
    //����ģ��ʶ��ʽ    
    split((char *)templateParam->callingWay,",",callingWay,&callingWayNum); //���ú������зָ� 
    while(callingWayNum)
    {
        switch(atoi(callingWay[callingWayNum-1]))
        {
            case 1:
                templateParam->templateCallingWay.isFace = 1;
                break;
            case 2:
                templateParam->templateCallingWay.isQrCode = 1;
                break;
            case 3:
                templateParam->templateCallingWay.isIcCard = 1;
                break; 
        }
        callingWayNum-- ;  
    }       
//    ef_set_env_blob("T_callingWay",templateParam->callingWay,strlen((const char*)templateParam->callingWay));     
    log_d("templateParam->callingWay = %s\r\n",templateParam->callingWay);    

    json_item = cJSON_GetObjectItem(templateMap, "offlineProcessing");
    templateParam->offlineProcessing = json_item->valueint;
//    memset(tmpbuf,0x00,sizeof(tmpbuf));
//    sprintf(tmpbuf,"%8d",templateParam->offlineProcessing);
//    ef_set_env_blob("offlineStatus",tmpbuf,8);     
    log_d("templateParam->offlineProcessing = %d\r\n",templateParam->offlineProcessing);     


    json_item = cJSON_GetObjectItem(templateMap, "modeType");
    strcpy((char *)templateParam->modeType,json_item->valuestring);

    //�����Ƿ����ڼ���ģʽ
    memset(callingWay,0x00,sizeof(callingWay));
    callingWayNum = 0;
    split((char *)templateParam->modeType,",",callingWay,&callingWayNum); //���ú������зָ� 
    while(callingWayNum)
    {
        switch(atoi(callingWay[callingWayNum-1]))
        {
            case 1:
                templateParam->workMode.isPeakMode = 1;
                break;
            case 2:
                templateParam->workMode.isHolidayMode = 1;
                break;
            case 5:
                templateParam->workMode.isNormalMode = 1;
                break; 
        }
        callingWayNum-- ;  
    }  
    
//    ef_set_env_blob("modeType",templateParam->modeType,strlen((const char*)templateParam->modeType));     
    log_d("templateParam->modeType = %s\r\n",templateParam->modeType);

    json_item = cJSON_GetObjectItem(templateMap, "peakCallingWay");
    strcpy((char *)templateParam->peakInfo[0].callingWay,json_item->valuestring);

    //����ʶ��ʽ    
    memset(callingWay,0x00,sizeof(callingWay));
    callingWayNum = 0;
    split((char *)templateParam->peakInfo[0].callingWay,",",callingWay,&callingWayNum); //���ú������зָ� 
    while(callingWayNum)
    {
        switch(atoi(callingWay[callingWayNum-1]))
        {
            case 1:
                templateParam->peakCallingWay.isFace = 1;
                break;
            case 2:
                templateParam->peakCallingWay.isQrCode = 1;
                break;
            case 3:
                templateParam->peakCallingWay.isIcCard = 1;
                break; 
        }
        callingWayNum-- ;  
    }  
    
//    ef_set_env_blob("peakCallingWay",templateParam->peakInfo[0].callingWay,strlen((const char *)templateParam->peakInfo[0].callingWay));
    log_d("templateParam->peakInfo[0].callingWay = %s\r\n",templateParam->peakInfo[0].callingWay);

    json_item = cJSON_GetObjectItem(templateMap, "peakStartDate");
    strcpy((char *)templateParam->peakInfo[0].beginTime,json_item->valuestring);
//    ef_set_env_blob("peakStartDate",templateParam->peakInfo[0].beginTime,strlen((const char*)templateParam->peakInfo[0].beginTime));
    log_d("templateParam->peakInfo[0].beginTime = %s\r\n",templateParam->peakInfo[0].beginTime);

    json_item = cJSON_GetObjectItem(templateMap, "peakEndDate");
    strcpy((char *)templateParam->peakInfo[0].endTime,json_item->valuestring);
//    ef_set_env_blob("peakEndDate",templateParam->peakInfo[0].endTime,strlen((const char*)templateParam->peakInfo[0].endTime));
    log_d("templateParam->peakInfo[0].endTime = %s\r\n",templateParam->peakInfo[0].endTime);

    json_item = cJSON_GetObjectItem(templateMap, "peakHolidaysType");
    strcpy((char *)templateParam->peakInfo[0].outsideTimeMode,json_item->valuestring);
//    ef_set_env_blob("peakHolidaysType",templateParam->peakInfo[0].outsideTimeMode,strlen((const char*)templateParam->peakInfo[0].outsideTimeMode));
    log_d("templateParam->peakInfo[0].outsideTimeMode = %s\r\n",templateParam->peakInfo[0].outsideTimeMode);

    json_item = cJSON_GetObjectItem(templateMap, "peakHolidays");
    strcpy((char *)templateParam->peakInfo[0].outsideTimeData,json_item->valuestring);
//    ef_set_env_blob("peakHolidays",templateParam->peakInfo[0].outsideTimeData,strlen((const char*)templateParam->peakInfo[0].outsideTimeData));
    log_d("templateParam->peakInfo[0].outsideTimeData = %s\r\n",templateParam->peakInfo[0].outsideTimeData);    
//------------------------------------------------------------------------------
    json_item = cJSON_GetObjectItem(templateMap, "holidayCallingWay");
    strcpy((char *)templateParam->hoildayInfo[0].callingWay,json_item->valuestring);
//    ef_set_env_blob("holidayCallingWay",templateParam->hoildayInfo[0].callingWay,strlen((const char*)templateParam->hoildayInfo[0].callingWay));
    log_d("templateParam->hoildayInfo[0].callingWay = %s\r\n",templateParam->hoildayInfo[0].callingWay);

    json_item = cJSON_GetObjectItem(templateMap, "holidayStartDate");
    strcpy((char *)templateParam->hoildayInfo[0].beginTime,json_item->valuestring);
//    ef_set_env_blob("holidayStartDate",templateParam->hoildayInfo[0].beginTime,strlen((const char*)templateParam->hoildayInfo[0].beginTime));
    log_d("templateParam->hoildayInfo[0].beginTime = %s\r\n",templateParam->hoildayInfo[0].beginTime);

    json_item = cJSON_GetObjectItem(templateMap, "holidayEndDate");
    strcpy((char *)templateParam->hoildayInfo[0].endTime,json_item->valuestring);
//    ef_set_env_blob("holidayEndDate",templateParam->hoildayInfo[0].endTime,strlen((const char*)templateParam->hoildayInfo[0].endTime));
    log_d("templateParam->hoildayInfo[0].endTime = %s\r\n",templateParam->hoildayInfo[0].endTime);

    json_item = cJSON_GetObjectItem(templateMap, "holidayHolidaysType");
    strcpy((char *)templateParam->hoildayInfo[0].outsideTimeMode,json_item->valuestring);
//    ef_set_env_blob("holidayHolidaysType",templateParam->hoildayInfo[0].outsideTimeMode,strlen((const char*)templateParam->hoildayInfo[0].outsideTimeMode));
    log_d("templateParam->hoildayInfo[0].outsideTimeMode = %s\r\n",templateParam->hoildayInfo[0].outsideTimeMode);

    json_item = cJSON_GetObjectItem(templateMap, "holidayHolidays");
    strcpy((char *)templateParam->hoildayInfo[0].outsideTimeData,json_item->valuestring);
//    ef_set_env_blob("holidayHolidays",templateParam->hoildayInfo[0].outsideTimeData,strlen((const char*)templateParam->hoildayInfo[0].outsideTimeData));
    log_d("templateParam->hoildayInfo[0].outsideTimeData = %s\r\n",templateParam->hoildayInfo[0].outsideTimeData);       

//--------------------------------------------------
    //�洢hoildayTimeMap������
    holidayTimeMapCnt = cJSON_GetArraySize(holidayTimeMap); /*��ȡ���鳤��*/
    log_d("array len = %d\r\n",holidayTimeMapCnt);

//    //�洢���ܿ�ʱ��εĸ���
//    if(holidayTimeMapCnt > 0)
//    {
//        sprintf(tmpKey,"%04d",holidayTimeMapCnt);
//        ef_set_env_blob("holidayTimeMapCnt",tmpKey,4);        
//    }

    for(index=0; index<holidayTimeMapCnt; index++)
    {
        tmpArray = cJSON_GetArrayItem(holidayTimeMap, index);

        memset(tmpKey,0x00,sizeof(tmpKey));
        memset(tmpIndex,0x00,sizeof(tmpIndex));       
        
        arrayElement = cJSON_GetObjectItem(tmpArray, "templateType");
        templateParam->holidayMode[index].templateType = arrayElement->valueint;        
        log_d("templateType = %d\r\n",templateParam->holidayMode[index].templateType);

        arrayElement = cJSON_GetObjectItem(tmpArray, "voiceSize");
        templateParam->holidayMode[index].voiceSize = arrayElement->valueint;        
        log_d("voiceSize = %d\r\n",templateParam->holidayMode[index].voiceSize);
        
        arrayElement = cJSON_GetObjectItem(tmpArray, "modeType");
        templateParam->holidayMode[index].channelType = arrayElement->valueint;
        log_d("modeType = %d\r\n",templateParam->holidayMode[index].channelType);
        
        arrayElement = cJSON_GetObjectItem(tmpArray, "startTime");
        
        //��Ϊ�ڼ��ո��߷干�ã�����ֻ��¼��FLASHһ�־Ϳ�����
        strcpy((char*)templateParam->holidayMode[index].startTime,arrayElement->valuestring);
        
        sprintf(tmpIndex,"%d",index);
        strcpy(tmpKey,"hoildayModeStartTime");
        strcat(tmpKey,tmpIndex); 
//        ef_set_env_blob(tmpKey,templateParam->holidayMode[index].startTime,strlen((const char*)templateParam->holidayMode[index].startTime));        
        log_d("%s = %s\r\n",tmpKey,templateParam->holidayMode[index].startTime);
        
        
        arrayElement = cJSON_GetObjectItem(tmpArray, "endTime");
        strcpy((char*)templateParam->holidayMode[index].endTime,arrayElement->valuestring);  
        
        memset(tmpKey,0x00,sizeof(tmpKey));
        strcpy(tmpKey,"hoildayModeEndTime");
        strcat(tmpKey,tmpIndex);      
//        ef_set_env_blob(tmpKey,templateParam->holidayMode[index].endTime,strlen((const char*)templateParam->holidayMode[index].endTime));                
        log_d("%s= %s\r\n",tmpKey,templateParam->holidayMode[index].endTime);        
    }
    
    log_d("=====================================================\r\n");
//--------------------------------------------------
    peakTimeMapCnt = cJSON_GetArraySize(peakTimeMap); /*��ȡ���鳤��*/
    log_d("peakTimeMapCnt len = %d\r\n",peakTimeMapCnt);

    for(index=0; index<peakTimeMapCnt; index++)
    {
        tmpArray = cJSON_GetArrayItem(peakTimeMap, index);

        memset(tmpKey,0x00,sizeof(tmpKey));
        memset(tmpIndex,0x00,sizeof(tmpIndex));       
        
        arrayElement = cJSON_GetObjectItem(tmpArray, "templateType");
        templateParam->peakMode[index].templateType = arrayElement->valueint;        
        log_d("templateType = %d\r\n",templateParam->holidayMode[index].templateType);

        arrayElement = cJSON_GetObjectItem(tmpArray, "voiceSize");
        templateParam->peakMode[index].voiceSize = arrayElement->valueint;        
        log_d("voiceSize = %d\r\n",templateParam->holidayMode[index].voiceSize);
        
        arrayElement = cJSON_GetObjectItem(tmpArray, "modeType");
        templateParam->peakMode[index].channelType = arrayElement->valueint;
        log_d("modeType = %d\r\n",templateParam->holidayMode[index].channelType);
        
        arrayElement = cJSON_GetObjectItem(tmpArray, "startTime");
        
        //��Ϊ�ڼ��ո��߷干�ã�����ֻ��¼��FLASHһ�־Ϳ�����
        strcpy((char*)templateParam->peakMode[index].startTime,arrayElement->valuestring);
        sprintf(tmpIndex,"%d",index);
        strcpy(tmpKey,"hoildayModeStartTime");
        strcat(tmpKey,tmpIndex); 
        log_d("%s = %s\r\n",tmpKey,templateParam->peakMode[index].startTime);
        
        
        arrayElement = cJSON_GetObjectItem(tmpArray, "endTime");
        strcpy((char*)templateParam->peakMode[index].endTime,arrayElement->valuestring);  
        
        memset(tmpKey,0x00,sizeof(tmpKey));
        strcpy(tmpKey,"hoildayModeEndTime");
        strcat(tmpKey,tmpIndex);      
        log_d("%s= %s\r\n",tmpKey,templateParam->peakMode[index].endTime);        
        
    }
    
    cJSON_Delete(root);
    
    ret = optTemplateParam(templateParam,WRITE_PRARM,sizeof(TEMPLATE_PARAM_STRU),DEVICE_TEMPLATE_PARAM_ADDR); 
    log_d("saveTemplateParam took %d ms to save\r\n",xTaskGetTickCount()-curtick);

    if(ret != NO_ERR)
    {
        return FLASH_W_ERR;
    }    

    return result;
}

void initTemplateParam(void)
{
    uint8_t ret = 0;    
    
    ClearTemplateParam();
    ret = opParam(&gtemplateParam,READ_PRARM,sizeof(TEMPLATE_PARAM_STRU),DEVICE_TEMPLATE_PARAM_ADDR);

    if(ret == false)
    {
        log_e("read param error\r\n");
        return;
    }


	if(gtemplateParam.initFlag.iFlag != DEFAULT_INIVAL)
	{	    
        //ģ�����ݸ�ֵ
        gtemplateParam.id = 1;
        gtemplateParam.templateStatus = 1;
        gtemplateParam.offlineProcessing = 2;
        gtemplateParam.initFlag.iFlag = DEFAULT_INIVAL;
        strcpy((char *)gtemplateParam.modeType,"5");
        strcpy((char *)gtemplateParam.templateCode,"100000");
        strcpy((char *)gtemplateParam.templateName,DEFAULT_DEV_NAME);
        strcpy((char *)gtemplateParam.callingWay,"1,2,3");      
      
        
        ret = opParam(&gtemplateParam,WRITE_PRARM,sizeof(TEMPLATE_PARAM_STRU),DEVICE_TEMPLATE_PARAM_ADDR);
        if(ret == false)
        {
            log_e("read param error\r\n");
            return;            
        }
	}  
	log_d("init param success\r\n");
}

static uint8_t opParam(void *Param,uint8_t mode,uint32_t len,uint32_t addr)
{
    uint8_t ret = 0;

    char *buff = mymalloc(SRAMIN,len);

    if(buff == NULL)
    {
        log_e("malloc memery error\r\n");
        myfree(SRAMIN,buff);
        return false;
    }     

    if(mode == WRITE_PRARM)
    {
        memcpy(buff,Param,len);   
        
    	ret = FRAM_Write ( FM24V10_1, addr, buff,len );
    	if ( ret == 0 )
    	{
            log_e("write template param error\r\n");
            myfree(SRAMIN,buff);
            return ret;
    	} 

    	log_d("write param success\r\n");
	}
	else
	{
        ret = FRAM_Read (FM24V10_1, addr, buff, len);
        if(ret != 1)
        {
            log_e("read param error\r\n");
            myfree(SRAMIN,buff);
            return ret ;        
        }	

        memcpy (Param, buff,  len );  
        
        log_d("read param success\r\n");        
	}

	myfree(SRAMIN,buff);

	return ret;
}




uint8_t optTemplateParam(void *stParam,uint8_t mode,uint32_t len,uint32_t addr)
{
    return opParam(stParam,mode,len,addr); 
}



//uint8_t optParam(void *stParam,uint8_t mode,uint32_t len,uint32_t addr,CallBackParam callBack)
//{
//    return callBack(stParam,mode,len,addr);
//}



void initDevBaseParam(void)
{
    uint8_t ret = 0; 
    char mac[6+1] = {0};
    char temp[20] = {0};
    char asc[12+1] = {0};
    
    ClearDevBaseParam();
    
    ret = opParam(&gDevBaseParam,READ_PRARM,sizeof(DEV_BASE_PARAM_STRU),DEVICE_BASE_PARAM_ADDR);

    if(ret == false)
    {
        log_e("read param error\r\n");
        return;
    }

    if(gDevBaseParam.deviceCode.downLoadFlag.iFlag != DEFAULT_INIVAL)
	{	
	    log_d("wirte default base data\r\n");
	    ClearDevBaseParam();
	    
        //�豸״̬Ϊ����״̬
        gDevBaseParam.deviceState.iFlag = DEVICE_ENABLE;     

        calcMac ( (unsigned char*)mac);
        bcd2asc ( (unsigned char*)asc, (unsigned char*)mac, 12, 0 );
        Insertchar ( asc,temp,':' );
        gDevBaseParam.deviceCode.deviceSnLen = strlen ( temp )-1 ;
        memcpy ( gDevBaseParam.deviceCode.deviceSn,temp,gDevBaseParam.deviceCode.deviceSnLen);
        strcpy ( gDevBaseParam.mqttTopic.publish,DEV_FACTORY_PUBLISH );
        strcpy ( gDevBaseParam.mqttTopic.subscribe,DEV_FACTORY_SUBSCRIBE );
        strncat ( gDevBaseParam.mqttTopic.subscribe,gDevBaseParam.deviceCode.deviceSn,gDevBaseParam.deviceCode.deviceSnLen);
        memcpy ( gDevBaseParam.deviceCode.qrSn,asc,8); //ʹ��ǰ8λ��Ϊ������QRID

        log_d("gDevBaseParam.deviceCode.deviceSn = %s,len = %d\r\n",gDevBaseParam.deviceCode.deviceSn,gDevBaseParam.deviceCode.deviceSnLen);
        log_d("gDevBaseParam.mqttTopic.publish = %s\r\n",gDevBaseParam.mqttTopic.publish);
        log_d("gDevBaseParam.mqttTopic.subscribe = %s\r\n",gDevBaseParam.mqttTopic.subscribe);       
        log_d("gDevBaseParam.deviceCode.qrSn = %s\r\n",gDevBaseParam.deviceCode.qrSn);



        log_d("wirte default base data\r\n");

        ret = opParam(&gDevBaseParam,WRITE_PRARM,sizeof(DEV_BASE_PARAM_STRU),DEVICE_BASE_PARAM_ADDR);
        
        if(ret == false)
        {
            log_e("read param error\r\n");
            return;            
        }
	}  
	
	log_d("init param success\r\n"); 
}


uint8_t optDevBaseParam(void *stParam,uint8_t mode,uint32_t len,uint32_t addr)
{
    return opParam(stParam,mode,len,addr); 
}





void initRecordIndex(void)
{
    uint8_t ret = 0;    
    
    ClearRecordIndex();
    ret = opParam(&gRecordIndex,READ_PRARM,sizeof(RECORDINDEX_STRU),RECORD_INDEX_ADDR);

    if(ret == false)
    {
        log_e("read param error\r\n");
        return;
    }

    log_d("gCurCardHeaderIndex = %d\r\n",gRecordIndex.cardNoIndex);
    log_d("gDelCardHeaderIndex = %d\r\n",gRecordIndex.delCardNoIndex);
    

	log_d("init param success\r\n");
}



uint8_t optRecordIndex(RECORDINDEX_STRU *recoIndex,uint8_t mode)
{
    uint8_t ret = 0;

    char *buff = mymalloc(SRAMIN,sizeof(RECORDINDEX_STRU));

    if(buff == NULL)
    {
        log_e("malloc memery error\r\n");
        myfree(SRAMIN,buff);
        return false;
    }     

    if(mode == WRITE_PRARM)
    {
        memcpy(buff,recoIndex,sizeof(RECORDINDEX_STRU));   

        log_d("write index %d,%d\r\n",recoIndex->cardNoIndex,recoIndex->delCardNoIndex);
        
    	ret = FRAM_Write ( FM24V10_1, RECORD_INDEX_ADDR, buff,sizeof(RECORDINDEX_STRU) );
    	if ( ret == 0 )
    	{
            log_e("write template param error\r\n");
            myfree(SRAMIN,buff);
            return ret;
    	} 

//    	log_d("write param success\r\n");
	}
	else
	{
        ret = FRAM_Read (FM24V10_1, RECORD_INDEX_ADDR, buff, sizeof(RECORDINDEX_STRU));
        if(ret != 1)
        {
            log_e("read param error\r\n");
            myfree(SRAMIN,buff);
            return ret ;        
        }	

        memcpy (recoIndex, buff,  sizeof(RECORDINDEX_STRU) );  
        
//        log_d("read param success\r\n");        
	}

	myfree(SRAMIN,buff);

	return ret;
}



void ClearRecordIndex(void)
{
	memset(&gRecordIndex,0x00,sizeof(RECORDINDEX_STRU)); 
}

void ClearTemplateParam(void)
{
	memset(&gtemplateParam,'\0',sizeof(TEMPLATE_PARAM_STRU));

	log_d("TEMPLATE_PARAM_STRU = %d\r\n",sizeof(TEMPLATE_PARAM_STRU));
}

void ClearDevBaseParam(void)
{
	memset(&gDevBaseParam,'\0',sizeof(DEV_BASE_PARAM_STRU));
}

void clearTemplateFRAM(void)
{
    FRAM_Erase ( FM24V10_1,DEVICE_TEMPLATE_PARAM_ADDR,DEVICE_TEMPLATE_PARAM_SIZE);
}

static void eraseUserDataIndex ( void )
{
    gRecordIndex.cardNoIndex = 0;
    gRecordIndex.delCardNoIndex = 0;
    gRecordIndex.accessRecoIndex = 0;
    optRecordIndex(&gRecordIndex,WRITE_PRARM);
}


void eraseUserDataAll ( void )
{
	int32_t iTime1, iTime2;
	iTime1 = xTaskGetTickCount();	/* ���¿�ʼʱ�� */
	eraseHeadSector();
	eraseDataSector();
    eraseUserDataIndex();
	clearTemplateFRAM();
    initTemplateParam();	
	iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_d ( "eraseUserDataAll�ɹ�����ʱ: %dms\r\n",iTime2 - iTime1 );
}

void eraseHeadSector ( void )
{
	FRAM_Erase ( FM24V10_1,0,122880 );	
}

void eraseDataSector ( void )
{
	uint16_t i = 0;
	uint16_t num = 0;
	
	ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);

    num = gRecordIndex.accessRecoIndex * RECORD_MAX_LEN/SECTOR_SIZE + 1;

	for ( i=0; i<num; i++ )
	{
		bsp_sf_EraseSector ( ACCESS_RECORD_ADDR+i*SECTOR_SIZE );
	}

    gRecordIndex.accessRecoIndex = 0;
    
    optRecordIndex(&gRecordIndex,WRITE_PRARM);	
}

void TestFlash ( uint8_t mode )
{
	char buff[156] = {0};
    HEADINFO_STRU tmp;
	uint32_t addr = 0;
	uint32_t data_addr = 0;
	uint16_t i = 0;
	uint32_t num = 0;

	if ( buff == NULL )
	{
		//log_d("my_malloc error\r\n");
		return ;
	}
	
	ClearRecordIndex();
    optRecordIndex(&gRecordIndex,READ_PRARM);
    


	if ( mode == CARD_MODE )
	{
		addr = CARD_NO_HEAD_ADDR;
		num = gRecordIndex.cardNoIndex;
	}

	else if ( mode == CARD_DEL_MODE )
	{
		addr = CARD_DEL_HEAD_ADDR;
		num = gRecordIndex.delCardNoIndex;
	}


	for ( i=0; i<num; i++ )
	{
        memset ( &tmp,0x00,sizeof ( HEADINFO_STRU ) );
		memset ( buff,0x00,sizeof ( buff ) );
		
		FRAM_Read ( FM24V10_1, addr+i*sizeof ( HEADINFO_STRU ), &tmp, sizeof ( HEADINFO_STRU ) );
		bcd2asc ( ( uint8_t* ) buff, tmp.headData.sn, CARD_NO_LEN_ASC, 0 );
		log_d("the %d card id = %s\r\n",i,buff);
	}

//	for ( i=0; i<num; i++ )
//	{
//		memset ( buff,0x00,sizeof ( buff ) );
//		bsp_sf_ReadBuffer ( ( uint8_t* ) buff, data_addr+i * ( sizeof ( USERDATA_STRU ) ), sizeof ( USERDATA_STRU ) );
//		printf ( "the %d data ====================== \r\n",i );
//		dbh ( "data", buff, ( sizeof ( USERDATA_STRU ) ) );

//	}

}




