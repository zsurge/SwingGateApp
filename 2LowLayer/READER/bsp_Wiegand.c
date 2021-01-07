#include "bsp_Wiegand.h"
#include "FreeRTOS.h"
#include "task.h"



WG_Rx_Struct WG_Rx_Str[DOORNUMBER];
uint8_t card_id[4][4];
uint8_t CheckBitStart[DOORNUMBER];
uint32_t WGSerial_Number[4];
WGin_Struct WGtime[4];



static void WeigenInD0(uint8_t WGNum);
static void WeigenInD1(uint8_t WGNum);
static uint8_t WG_Rx_Pro ( uint8_t WGNum );
static uint8_t ECC_ODD_Check ( uint32_t EvevData,uint8_t Bitlen );
static uint8_t ECC_Even_Check ( uint32_t EvevData,uint8_t Bitlen );


void bsp_WiegandInit ( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	RCC_AHB1PeriphClockCmd ( WG1_RCC, ENABLE ); //GPIOD时钟使能

	//GPIO
	GPIO_InitStructure.GPIO_Pin = WG1_IN_D0 | WG1_IN_D1|WG2_IN_D0|WG2_IN_D1;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      //管脚速率选择
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;        //管脚功能配置
	GPIO_Init ( WG1_GPIO_PORT, &GPIO_InitStructure );   //端口选择：如PA，PB口


	/* EXTI line gpio config */
	RCC_APB2PeriphClockCmd ( RCC_APB2Periph_SYSCFG, ENABLE ); //使能SYSCFG时钟

	/* 连接 EXTI Line6 到 PB6 引脚 */
	SYSCFG_EXTILineConfig ( WG1_PortSource, WG1_IN_D0_PinSource );

	/* 连接 EXTI Line7 到 PB7 引脚 */
	SYSCFG_EXTILineConfig ( WG1_PortSource, WG1_IN_D1_PinSource );

	/* 连接 EXTI Line8 到 PB8 引脚 */
	SYSCFG_EXTILineConfig ( WG2_PortSource, WG2_IN_D0_PinSource );

	/* 连接 EXTI Line9 到 PB9 引脚 */
	SYSCFG_EXTILineConfig ( WG2_PortSource, WG2_IN_D1_PinSource );


	EXTI_InitStructure.EXTI_Line = WG1_IN_D0_EXTI | WG1_IN_D1_EXTI|WG2_IN_D0_EXTI|WG2_IN_D1_EXTI;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿中断
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init ( &EXTI_InitStructure );

	/* 配置PB6 PB7,PB8,PB9为中断源 */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_Init ( &NVIC_InitStructure );


}

void EXTI9_5_IRQHandler ( void )
{

	if ( EXTI_GetITStatus ( WG1_IN_D0_EXTI ) != RESET ) //D0
	{
		//韦根取D0管脚数据
		delay_us(30); //消抖
		if ( WeiGen1D0==0 )
		{
			WeigenInD0 ( WeiGen1 );
		}

		/* Clear the EXTI line 0 pending bit */
		EXTI_ClearITPendingBit ( WG1_IN_D0_EXTI );

	}

	if ( EXTI_GetITStatus ( WG1_IN_D1_EXTI ) != RESET ) //D1
	{
		//韦根取D1管脚数据
		delay_us(30); //消抖

		if ( WeiGen1D1==0 )
		{
			WeigenInD1 ( WeiGen1 );
		}

		/* Clear the EXTI line 1 pending bit */
		EXTI_ClearITPendingBit ( WG1_IN_D1_EXTI );
		
	}

	if ( EXTI_GetITStatus ( WG2_IN_D0_EXTI ) != RESET ) //D0
	{
		//韦根取D0管脚数据
		delay_us(30); //消抖
		if ( WeiGen2D0==0 )
		{
			WeigenInD0 ( WeiGen2 );
		}

		/* Clear the EXTI line 0 pending bit */
		EXTI_ClearITPendingBit ( WG2_IN_D0_EXTI );
		
	}

	if ( EXTI_GetITStatus ( WG2_IN_D1_EXTI ) != RESET ) //D1
	{
		//韦根取D1管脚数据
		delay_us(30); //消抖

		if ( WeiGen2D1==0 )
		{
			WeigenInD1 ( WeiGen2 );
		}

		/* Clear the EXTI line 1 pending bit */
		EXTI_ClearITPendingBit ( WG2_IN_D1_EXTI );
		
	}

}


void WeigenInD0 ( uint8_t WGNum )
{
	WG_Rx_Str[WGNum].WG_Bit=0;

	WG_Rx_Str[WGNum].End_TIME=TimeOut;   //40~50ms

	if ( WG_Rx_Str[WGNum].WG_Rx_Len<34 )
	{
		WG_Rx_Str[WGNum].WG_Rx_Len++;
	}

	else if ( WG_Rx_Str[WGNum].WG_Rx_Len==34 )
	{
		WG_Rx_Str[WGNum].WG_Rx_End=1;
		WG_Rx_Str[WGNum].END_TIME_C_EN=0;
	}
	else
	{
		WG_Rx_Str[WGNum].WG_Rx_Len=0;
		WG_Rx_Str[WGNum].WG_Rx_End=0;
		WG_Rx_Str[WGNum].END_TIME_C_EN=0;
	}

	if ( WG_Rx_Str[WGNum].WG_Rx_Len==1 )
	{
		WG_Rx_Str[WGNum].WG_Rx_Data=0;
		WG_Rx_Str[WGNum].END_TIME_C_EN=1;
		CheckBitStart[WGNum] = WG_Rx_Str[WGNum].WG_Bit;  //2012-10-13 ,增加奇偶效验位;
	}

	if ( ( 1<WG_Rx_Str[WGNum].WG_Rx_Len ) && ( WG_Rx_Str[WGNum].WG_Rx_Len<34 ) ) 	//2012-09-19 Leo 修改，解决Wiegand接收数据判定长度有误的bug
	{
		if ( WG_Rx_Str[WGNum].WG_Bit )
		{
			WG_Rx_Str[WGNum].WG_Rx_Data|=0x01;
		}
		else
		{
			WG_Rx_Str[WGNum].WG_Rx_Data&=0xFFFFFFFE;
		}
		if ( WG_Rx_Str[WGNum].WG_Rx_Len<33 )
		{
			WG_Rx_Str[WGNum].WG_Rx_Data<<=1;
		}
	}
}

void WeigenInD1 ( uint8_t WGNum )
{
	WG_Rx_Str[WGNum].WG_Bit=1;
	WG_Rx_Str[WGNum].End_TIME=TimeOut;   //40~50ms
	if ( WG_Rx_Str[WGNum].WG_Rx_Len<34 )
	{
		WG_Rx_Str[WGNum].WG_Rx_Len++;
	}
	else if ( WG_Rx_Str[WGNum].WG_Rx_Len==34 )
	{
		WG_Rx_Str[WGNum].WG_Rx_End=1;
		WG_Rx_Str[WGNum].END_TIME_C_EN=0;

	}
	else
	{
		WG_Rx_Str[WGNum].WG_Rx_Len=0;
		WG_Rx_Str[WGNum].WG_Rx_End=0;
		WG_Rx_Str[WGNum].END_TIME_C_EN=0;
	}
	if ( WG_Rx_Str[WGNum].WG_Rx_Len==1 )
	{
		WG_Rx_Str[WGNum].WG_Rx_Data=0;
		WG_Rx_Str[WGNum].END_TIME_C_EN=1;
		CheckBitStart[WGNum] = WG_Rx_Str[WGNum].WG_Bit; ;  //2012-10-13 ,增加奇偶效验位;
	}
	//  if(1<WG_Rx_Str[1].WG_Rx_Len<34)
	if ( ( 1<WG_Rx_Str[WGNum].WG_Rx_Len ) && ( WG_Rx_Str[WGNum].WG_Rx_Len<34 ) )		//2012-09-19 Leo 修改，解决Wiegand接收数据判定长度有误的bug
	{
		if ( WG_Rx_Str[WGNum].WG_Bit )
		{
			WG_Rx_Str[WGNum].WG_Rx_Data|=0x01;
		}
		else
		{
			WG_Rx_Str[WGNum].WG_Rx_Data&=0xFFFFFFFE;
		}
		if ( WG_Rx_Str[WGNum].WG_Rx_Len<33 )
		{
			WG_Rx_Str[WGNum].WG_Rx_Data<<=1;
		}
	}
}
/**********************************************************************************/
/* 函 数 名：WG_Rx_Pro			 */
/* 功    能：韦根接收处理			   */
/* 说    明：							*/
/* 入口参数：无							  */
/* 返 回 值：接收结果  0:无完整数据；非0：有完整数据，返回数据为韦根接收位长度	  */
/**********************************************************************************/
uint8_t WG_Rx_Pro ( uint8_t WGNum )
{
	uint8_t wg_len=0;
	uint8_t WGBit;

	wg_len=WG_Rx_Str[WGNum].WG_Rx_Len;
	
	WG_Rx_Str[WGNum].WG_Rx_Len=0;

	
	if ( wg_len!=26 && wg_len!=34 )
	{
		return 0;
	}
	
	if ( wg_len==34 ) //wg34
	{
		card_id[WGNum][0] = ( WG_Rx_Str[WGNum].WG_Rx_Data&0xFF000000 ) >>24;	//wg34
	}
	else //wg26
	{
		card_id[WGNum][0]=0;
		WGBit = ( WG_Rx_Str[WGNum].WG_Rx_Data>>1 ) &0x01;
		if ( ECC_ODD_Check ( WG_Rx_Str[WGNum].WG_Rx_Data>>2,12 ) != WGBit )
		{
			return 0;
		}

		WG_Rx_Str[WGNum].WG_Rx_Data>>=2;
		if ( ECC_Even_Check ( WG_Rx_Str[WGNum].WG_Rx_Data>>12,12 ) != CheckBitStart[WGNum] )
		{
			return 0;
		}
	}


	card_id[WGNum][1]= ( WG_Rx_Str[WGNum].WG_Rx_Data&0xFF0000 ) >>16;
	card_id[WGNum][2]= ( WG_Rx_Str[WGNum].WG_Rx_Data&0xFF00 ) >>8;
	card_id[WGNum][3]=WG_Rx_Str[WGNum].WG_Rx_Data&0xFF;
	WG_Rx_Str[WGNum].WG_Rx_Data=0;


	return wg_len;
}

/****************************************************************************/
/* 函 数 名：															*/
/* 功    能：													*/
/* 说    明：																	*/
/* 入口参数：																*/
/* 返 回 值：		 									*/
/****************************************************************************/
uint32_t bsp_WeiGenScanf ( uint8_t WGNum)
{
	uint8_t len=0;
	uint32_t Serial_Number = 0;
	/*wiegand处理*/

	if ( WG_Rx_Str[WGNum].END_TIME_C_EN )
	{
		if ( !WG_Rx_Str[WGNum].End_TIME )
		{
			WG_Rx_Str[WGNum].END_TIME_C_EN=0;
			if ( ( WG_Rx_Str[WGNum].WG_Rx_Len==26 ) || ( WG_Rx_Str[WGNum].WG_Rx_Len==34 ) )
			{
				WG_Rx_Str[WGNum].WG_Rx_End=1;
			}
			else
			{
				WG_Rx_Str[WGNum].WG_Rx_End=0;
				WG_Rx_Str[WGNum].WG_Rx_Len = 0;
				WG_Rx_Str[WGNum].End_TIME = TimeOut;
			}
		}
	}
	else
	{
		if ( !WG_Rx_Str[WGNum].End_TIME )
		{
			WG_Rx_Str[WGNum].WG_Rx_Len=0;
			WG_Rx_Str[WGNum].WG_Rx_End=0;
			WG_Rx_Str[WGNum].End_TIME = TimeOut;
		}
	}

	if ( WG_Rx_Str[WGNum].WG_Rx_End )
	{
		WG_Rx_Str[WGNum].WG_Rx_End=0;

		len = WG_Rx_Pro ( WGNum );

		if ( len==34 )
		{
			Serial_Number |= card_id[WGNum][0]<<24;
			Serial_Number |= card_id[WGNum][1]<<16;
			Serial_Number |= card_id[WGNum][2]<<8;
			Serial_Number |= card_id[WGNum][3];

			card_id[WGNum][0] = 0;
			card_id[WGNum][1] = 0;
			card_id[WGNum][2] = 0;
			card_id[WGNum][3] = 0;
		}
		else if ( len==26 )
		{

			Serial_Number |= card_id[WGNum][1]<<16;
			Serial_Number |= card_id[WGNum][2]<<8;
			Serial_Number |= card_id[WGNum][3];

			if ( ( card_id[WGNum][1] == 0xFF ) && ( card_id[WGNum][2] == 0xFF ) )
			{
				if ( card_id[WGNum][3] >= 0xF0 )
				{
					len = 4;
				}
				else
				{
					len = 8;
				}
			}
            
			card_id[WGNum][0] = 0;
			card_id[WGNum][1] = 0;
			card_id[WGNum][2] = 0;
			card_id[WGNum][3] = 0;
		}

	}

	return Serial_Number;
}


uint8_t ECC_Even_Check ( uint32_t EvevData,uint8_t Bitlen )
{
	uint8_t i,Num=0;
	uint32_t Data,EccEven = 0x00000001;

	for ( i=0; i<Bitlen; i++ )
	{
		Data = EccEven&EvevData;
		if ( Data==1 )
		{
			Num ++;
		}
		EvevData>>=1;
	}
	Num = Num%2;

	if ( Num )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


uint8_t ECC_ODD_Check ( uint32_t EvevData,uint8_t Bitlen )
{
	uint8_t i,Num=0;
	uint32_t Data,EccEven = 0x00000001;

	for ( i=0; i<Bitlen; i++ )
	{
		Data = EccEven&EvevData;
		if ( Data==1 )
		{
			Num ++;
		}
		EvevData>>=1;
	}
	Num = Num%2;

	if ( Num )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}




