#ifndef __BSP_WIEGAND_H
#define __BSP_WIEGAND_H

#include "sys.h"


#define TimeOut   	4

#define WeiGen1		0
#define WeiGen2		1

#define WGTIMEOUT	10
#define BEEPTIME	200

#define DOORNUMBER 2


#define WG1_RCC         RCC_AHB1Periph_GPIOB
#define WG1_GPIO_PORT   GPIOB
#define WG1_IN_D0       GPIO_Pin_6//Τ���ӿ�
#define WG1_IN_D1       GPIO_Pin_7//Τ���ӿ�
#define WG1_PortSource   EXTI_PortSourceGPIOB
#define WG1_IN_D0_PinSource EXTI_PinSource6
#define WG1_IN_D1_PinSource EXTI_PinSource7
#define WG1_IN_D0_EXTI  EXTI_Line6//Τ���ӿ�
#define WG1_IN_D1_EXTI  EXTI_Line7//Τ���ӿ�


#define WG2_RCC         RCC_AHB1Periph_GPIOB
#define WG2_GPIO_PORT   GPIOB
#define WG2_IN_D0       GPIO_Pin_8//Τ���ӿ�
#define WG2_IN_D1       GPIO_Pin_9//Τ���ӿ�
#define WG2_PortSource   EXTI_PortSourceGPIOB
#define WG2_IN_D0_PinSource EXTI_PinSource8
#define WG2_IN_D1_PinSource EXTI_PinSource9
#define WG2_IN_D0_EXTI  EXTI_Line8//Τ���ӿ�
#define WG2_IN_D1_EXTI  EXTI_Line9//Τ���ӿ�




//BEEP
//#define	BEEP1						GPIOF
//#define	P_BEEP1						GPIO_Pin_7

//#define	BEEP2						GPIOG
//#define	P_BEEP2						GPIO_Pin_10


//Τ��-LED
//#define LED1	         	GPIOG
//#define P_LED1				GPIO_Pin_12 

//#define LED2	         	GPIOG 
//#define P_LED2				GPIO_Pin_9 



#define WeiGen1D0 GPIO_ReadInputDataBit(WG1_GPIO_PORT,WG1_IN_D0) 
#define WeiGen1D1 GPIO_ReadInputDataBit(WG1_GPIO_PORT,WG1_IN_D1) 

#define WeiGen2D0  GPIO_ReadInputDataBit(WG2_GPIO_PORT,WG2_IN_D0)
#define WeiGen2D1  GPIO_ReadInputDataBit(WG2_GPIO_PORT,WG2_IN_D1)


//#pragma pack(1)//�ñ�����������ṹ��1�ֽڶ���;

typedef struct
{
  volatile uint8_t WG_Rx_Len; //���յ�λ����
  volatile uint8_t WG_Rx_End; //���ս���
  volatile uint8_t WG_Bit;	  //λֵ
  volatile uint8_t WG_Rx_Bit;  //�յ�λ
  volatile uint8_t END_TIME_C_EN;  //���ս�����ʱʹ��
  volatile uint8_t End_TIME;      //���ս���ʱ��  
  volatile uint32_t WG_Rx_Data;   //��������
}WG_Rx_Struct;   //Τ�����սṹ��
//#pragma pack()//�ñ�����������ṹ��1�ֽڶ���;



typedef struct
{
	uint8_t TimeEn;	
	u16 Time;
}WGin_Struct;  
//#pragma pack(4) //ȡ��1�ֽڶ��룬�ָ�Ϊ4�ֽڶ���;


extern WGin_Struct WGtime[4];
extern WG_Rx_Struct WG_Rx_Str[DOORNUMBER];   //Τ�����սṹ��
extern uint8_t card_id[4][4];
extern uint8_t CheckBitStart[DOORNUMBER];


uint32_t bsp_WeiGenScanf ( uint8_t WGNum);
void bsp_WiegandInit(void);

#endif

