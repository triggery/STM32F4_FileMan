#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"

#include "SSD1289.h"
//#include "TouchLib.h"
#include "GesturesHandler.h"
//#include "my_str_lib.h"
//#include "math.h"

#include "stm32_ub_usb_msc_host.h"
#include "ff.h"

#include "FILE_MAN/file_man.h"
#include "audio_dac.h"
#include "dispatcher.h"

#include "icons_menu.h"
#include "main.h"
//#include "gdisp_image_bmp.h"

//-------------------------------------------------
#define USED_USART		USART1
#define BAUDRATE		9600

void putChar_USARTx(USART_TypeDef* USARTx, char data );
void putString_USART(const char *string);

void init_USART1(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	USART_InitTypeDef USART_InitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1); // USART1_TX
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1); // USART1_RX

	USART_InitStruct.USART_BaudRate = BAUDRATE;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART1, &USART_InitStruct);

	USART_Cmd(USART1, ENABLE);
}

void putChar_USARTx(USART_TypeDef* USARTx, char data )
{
	  while(!(USARTx->SR & USART_SR_TC));
	  USARTx->DR = data;
}

void putString_USART(const char *string)
{
	while( *string )
	{
		putChar_USARTx( USED_USART, *string++ );
	}
}

//-------------------------------------------------

void LEDS_InitFunc(void);

void touch_on(void)
{
	GPIO_SetBits(GPIOD, GPIO_Pin_13);
}

void touch_off(void)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_13);
}

PROCESS_RESULT getUserPBStatus(void)
{
	if( GPIOA->IDR & GPIO_Pin_0 )
	{
		audio_play_stop();
		Delay(300);
	}
	return PROCESS_OK;
}

PROCESS_RESULT playLoopProc(void)
{
	audio_play_loop();
	return PROCESS_OK;
}

PROCESS_TYPE audioProc;
void initAudioProc(void)
{
	//setProcessFunc( &audioProc, playLoopProc, PROCESS_DEMON, NULL_PROCESS );
	audioProc.entryFunction = playLoopProc;
	audioProc.PROC_STATE = PROCESS_DEMON;
	audioProc.parentProc = NULL_PROCESS;
	addProcess( &audioProc );
}


PROCESS_TYPE userPBProc;

int main(void)
{
    if (SysTick_Config(SystemCoreClock / 1000)) {
    	while (1);
    }

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOD, ENABLE);

	LCD_Init();
	TouchInit();
	resetLinePos();
    LCD_SetTextColor(BLACK);
    LCD_SetBackColor(WHITE);
    LCD_Clear(LCD_BackColor());
    LEDS_InitFunc();

	// USER BUTTON INIT
    GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
/*
	LCD_PutStringShorten( 40, 40, "LCD INIT!", 10 );
	LCD_WriteBMP_16( 70, 70, 16, 16, (uint16_t*)getFolder16Icon() );
*/
	LCD_PutStringShorten( 5, currLinePos(), "LCD INIT!", 10 );
	nextLinePos();

    initProcessHandler();

	LCD_PutStringShorten( 5, currLinePos(), "init audio dac...", 20 );
	nextLinePos();

	audio_init();
	initAudioProc();

	LCD_PutStringShorten( 5, currLinePos(), "while init USB...", 20 );
	nextLinePos();

	UB_USB_MSC_HOST_Init();

    while( UB_USB_MSC_HOST_Do()!=USB_MSC_DEV_CONNECTED ){};

	LCD_PutStringShorten( 5, currLinePos(), "USB connected!", 20 );
	nextLinePos();

	if( UB_Fatfs_Mount(USB_0) == FATFS_OK )
	{
		LCD_PutStringShorten( 5, currLinePos(), "USB device mount OK!", 20 );
		nextLinePos();
		Delay(1000);
		LCD_Clear(LCD_BackColor());

		FileMan_Init();
		FileMan_OpenFolder("/");
		FileMan_Show();
	}

	setProcessFunc( &userPBProc, getUserPBStatus, PROCESS_DEMON, NULL_PROCESS );
	addProcess( &userPBProc );

	setProcessFunc( &FileMan_TouchProc, FileMan_TouchHandler, PROCESS_ACTIVE, NULL_PROCESS );
	addProcess( &FileMan_TouchProc );

	init_USART1();
	putString_USART("hello!!! \n");

	while(1)
    {
    	ProcessHandler();
    }
}

void LEDS_InitFunc(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	/** LEDS GPIO INIT **/
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}
