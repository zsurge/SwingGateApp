/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : bsp.c
  �� �� ��   : ����
  ��    ��   : �Ŷ�
  ��������   : 2019��7��9��
  ����޸�   :
  ��������   : ����Ӳ���ײ�������������ļ���ÿ������ #include "bsp.h" ���������е���������ģ�顣
            bsp = Borad surport packet �弶֧�ְ�
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2019��7��9��
    ��    ��   : �Ŷ�
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include "bsp.h"
/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#define HARDWARE_VERSION               "V1.0.1"
#define SOFTWARE_VERSION               "V1.0.1"

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

static void my_mem_init(void)
{
	mymem_init(SRAMIN);								//��ʼ���ڲ��ڴ��
	mymem_init(SRAMEX);								//��ʼ���ⲿ�ڴ��
	mymem_init(SRAMCCM);	  					    //��ʼ��CCM�ڴ��
}


 void bsp_Init(void)
{
    NVIC_SetVectorTable(NVIC_VectTab_FLASH,0x30000);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4
	delay_init(168);            //��ʼ����ʱ����

	bsp_TIM6_Init();            //��ʱ��6��ʼ��
	
    bsp_InitUart();
//    bsp_Usart5_Init(9600);

    FRAM_Init();              //eeprom��ʼ��
    bsp_ds1302_init(); //DS1302_GPIO_Init();             //ʱ��оƬ��ʼ��
    
//    bsp_rtc_init();             //��ʼ��RTC��ֻ�ܷ���UART����

	bsp_key_Init();             //������ʼ��

	bsp_LED_Init();		        //��ʼ��LED�˿�	  

    bsp_HC595Init();            //��ʼ�������
    
    STM_FLASH_Init();           //оƬ�ڲ�FLASH��ʼ��
    easyflash_init();           //�ⲿFLASH��ʼ����ʹ��easyflash    
    
//    bsp_spi_flash_init();

//    bsp_beep_init();            //��������ʼ��        

    bsp_WiegandInit();          //Τ����������ʼ��

    my_mem_init();                  //���ڴ���г�ʼ��

  /* CmBacktrace initialize */
//   cm_backtrace_init("SmartDoorAPP", HARDWARE_VERSION, SOFTWARE_VERSION);

}
