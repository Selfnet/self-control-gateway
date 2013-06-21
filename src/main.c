/**
******************************************************************************
* This file is based on the sample Ethernet Project of the MCD Application Team
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "includes.h"
#include "io-helper.h" //dirks button+led func

#include "ethernet_init.h"

#include "can.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define UI32_DELAY_TIME 0x1FFFFF


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

void SysTickStart(uint32_t Tick)
{
    RCC_ClocksTypeDef Clocks;
    volatile uint32_t dummy;

    RCC_GetClocksFreq(&Clocks);

    dummy = SysTick->CTRL;
    SysTick->LOAD = (Clocks.HCLK_Frequency/8)/Tick;

    SysTick->CTRL = 1;
}


void SysTickStop(void)
{
    SysTick->CTRL = 0;
}


/**
* @brief  Main program.
* @param  None
* @retval None
*/
int main(void)
{
    SysTickStart(0xFFFF);

    /* Button Init */
    button_init();

    /* LED Init */
    LED_init();
    LED_On(1);

    // initialize CAN-Bus and enable CAN Interrupts
    CAN_config();

    // initialize Ethernet (Pins, Speed, Interrupt, etc...)
    ethernet_init();

    /* uIP stack main loop */
    uIPMain(); //lokated in ethernet.c

    //wenn uIPMain zu ende --> aufräumen
    ethernet_deinit();
}


#ifdef  USE_FULL_ASSERT
/**
* @brief  Reports the name of the source file and the source line number
*   where the assert_param error has occurred.
* @param  file: pointer to the source file name
* @param  line: assert_param error line source number
* @retval None
*/
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

