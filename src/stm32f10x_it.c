/**
******************************************************************************
* This file is based on the sample Interrupt Project of the MCD Application Team
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"

#include "io-helper.h"
#include "uip.h"
#include "tcp_app.h"
#include "can.h"

#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
}


/******************************************************************************/
/******************************************************************************/


/*******************************************************************************
* Function Name  : TIM2_IRQHandler
* Description    : This function handles TIM2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM2_IRQHandler(void) //fuer ethernet
{
extern  void Tim2Handler (void);
    Tim2Handler();
}


/*******************************************************************************
* Function Name  : TIM3_IRQHandler
* Description    : This function handles TIM2 global interrupt request.
* Input          : None
* Output         : None

* Return         : None
*******************************************************************************/
void TIM6_IRQHandler(void) 
{
    TIM_ClearITPendingBit(TIM6, TIM_IT_Update );
}


/******************************************************************************/

/**
  * @brief  This function handles ExternalInterrupt 0 (Port[A-D] Pin0) Handler.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void) //Button2
{
    //Check if EXTI_Line0 is asserted
    if(EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        // alle orangen leds on
        send_sync(1);
        LED_On(2);
    }
    
    //we need to clear line pending bit manually
    EXTI_ClearITPendingBit(EXTI_Line0);
}

void EXTI15_10_IRQHandler(void) //Button1
{
    if (EXTI->PR & (1<<13)) {                       // EXTI0 interrupt pending?
        EXTI->PR |= (1<<13);                          // clear pending interrupt
    }
    //Check if EXTI_Line0 is asserted
    //if(EXTI_GetITStatus(EXTI_Line15) != RESET)
    {
        LED_On(1);
        send_ping(0xaa);
    }
}

// TODO
// *** ETH Interrupt ***
void ETH_IRQHandler(void)
{
    #if defined (_DEBUG) && DEBUG_DMA_INT
    debug_printf("DMA Int:");
    #endif

    // Normal interrupt summary
    if ((ETH->DMASR & ETH_DMASR_NIS) == ETH_DMASR_NIS)
    {
        //#if INT_ENABLE_RI
        if ((ETH->DMASR & ETH_DMASR_RS) == ETH_DMASR_RS)
        {
            // Ethernet frame received
            LED_Toggle(1);

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_RS;

            // Process receive interrupt
            /*if (receiveIntHandler)
            {
                receiveIntHandler();
            }*/
        }
        //#endif
        #if INT_ENABLE_TI
        if ((ETH->DMASR & ETH_DMASR_TS) == ETH_DMASR_TS)
        {
            // Ethernet frame sent
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" TS");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_TS;
        }
        #endif
        #if INT_ENABLE_TBUI
        if ((ETH->DMASR & ETH_DMASR_TBUS) == ETH_DMASR_TBUS)
        {
            // Transmit buffer unavailable
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" TBUS");
            #endif

            // Clear interrupt flag, transmition is resumed after descriptors have been prepared
            ETH->DMASR = ETH_DMASR_TBUS;
        }
        #endif
        #if INT_ENABLE_ERI
        if ((ETH->DMASR & ETH_DMASR_ERS) == ETH_DMASR_ERS)
        {
            // Early receive
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" ERS");
            #endif

            // Clear interrupt flag. Also cleared automatically by RI
            ETH->DMASR = ETH_DMASR_ERS;
        }
        #endif

        // Clear normal interrupt flag
        ETH->DMASR = ETH_DMASR_NIS;
    }

    // Abnormal interrupt summary
    if ((ETH->DMASR & ETH_DMASR_AIS) == ETH_DMASR_AIS)
    {
        LED_Toggle(2);
    
        #if INT_ENABLE_FBEI
        if ((ETH->DMASR & ETH_DMASR_FBES) == ETH_DMASR_FBES)
        {
            // Fatal bus error
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" FBES");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_FBES;
        }
        #endif
        #if INT_ENABLE_TPSI
        if ((ETH->DMASR & ETH_DMASR_TPSS) == ETH_DMASR_TPSS)
        {
            // Transmit process stopped
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" TPSS");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_TPSS;
        }
        #endif
        #if INT_ENABLE_TJTI
        if ((ETH->DMASR & ETH_DMASR_TJTS) == ETH_DMASR_TJTS)
        {
            // Transmit jabber timeout
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" TJTS");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_TJTS;
        }
        #endif
        #if INT_ENABLE_ROI
        if ((ETH->DMASR & ETH_DMASR_ROS) == ETH_DMASR_ROS)
        {
            // Receive overflow
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" ROS");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_ROS;
        }
        #endif
        #if INT_ENABLE_TUI
        if ((ETH->DMASR & ETH_DMASR_TUS) == ETH_DMASR_TUS)
        {
            // Transmit underflow
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" TUS");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_TUS;
        }
        #endif
        #if INT_ENABLE_RBUI
        if ((ETH->DMASR & ETH_DMASR_RBUS) == ETH_DMASR_RBUS)
        {
            // Receive buffer unavailable
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" RBUS");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_RBUS;
        }
        #endif
        #if INT_ENABLE_RPSI
        if ((ETH->DMASR & ETH_DMASR_RPSS) == ETH_DMASR_RPSS)
        {
            // Receive process stopped
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" RPSS");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_RPSS;
        }
        #endif
        #if INT_ENABLE_RWTI
        if ((ETH->DMASR & ETH_DMASR_RWTS) == ETH_DMASR_RWTS)
        {
            // Receive watchdog timeout
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" RWTS");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_RWTS;
        }
        #endif
        #if INT_ENABLE_ETI
        if ((ETH->DMASR & ETH_DMASR_ETS) == ETH_DMASR_ETS)
        {
            // Early transmit interrupt
            #if defined (_DEBUG) && DEBUG_DMA_INT
            debug_printf(" ETS");
            #endif

            // Clear interrupt flag
            ETH->DMASR = ETH_DMASR_ETS;
        }
        #endif

        // Clear abnormal interrupt flag
        ETH->DMASR = ETH_DMASR_AIS;
    }
}

// *** CAN Interrupt ***
void CAN1_RX0_IRQHandler(void)
{
    prozess_can_it();
}

