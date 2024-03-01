#include "stm32f10x.h"
#include "stm32f1_rc522.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f1_delay.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"

uchar i;
uchar serNum[MAX_LEN];
char buffer[30];

/* Private function prototypes -----------------------------------------------*/
void GPIO_Config(void);
void Output_Config(void);
void TurnOnPC13(void);
void TurnOffPC13(void);
void delay_ms(uint32_t ms);					 
/* Private functions ---------------------------------------------------------*/
int main(void)
{    
    UART1_Config();     // config uart1 -> esp32
    MFRC522_Init();     // config spi + on anten
    Output_Config();    // set led o chan PA8 de biet stm32 dang hoat dong
    GPIO_Config();      // C?u hình chân PC13
    while(1)
    {
        if(!MFRC522_Request(PICC_REQIDL, serNum))
        {
            if(!MFRC522_Anticoll(serNum))              //str luu so serial number
            {
                //strcpy(buffer,(char *) serNum);
                sprintf(buffer, "Ma RFID: %02X%02X%02X%02X%02X\n", serNum[0], serNum[1], serNum[2], serNum[3], serNum[4]);
                while(!USART_GetFlagStatus(USART1, USART_FLAG_TXE));                 // kiem tra bo dem
                    for(i=0; i<strlen(buffer); i++)
                    {
                        USART_SendData(USART1, buffer[i]);              // gui tung byte data
                        while(!USART_GetFlagStatus(USART1, USART_FLAG_TC));                 // kiem tra da hoan thanh viec chuyen data
                    }
                TurnOnPC13(); // B?t chân PC13 khi qu?t th? RFID
										delay_ms(200);
										TurnOffPC13(); // T?t chân PC13 khi không qu?t th? RFID

            }
        }
        else
        {
            TurnOffPC13(); // T?t chân PC13 khi không qu?t th? RFID
        }
    }
}

void Output_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, GPIO_Pin_8);
}

// Hàm c?u hình chân PC13
void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

// Hàm b?t chân PC13
void TurnOnPC13(void)
{
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
}

// Hàm t?t chân PC13
void TurnOffPC13(void)
{
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
}

void delay_ms(uint32_t ms) {
    uint32_t i;
    for(i = 0; i < ms; i++) {
        uint32_t j;
        for(j = 0; j < 7200; j++) { // Tùy ch?nh s? l?n l?p d? d?t du?c d? tr? x?p x? 1ms v?i t?n s? clock 72MHz
            __NOP();
        }
    }
}