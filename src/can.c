
#include "stm32f10x.h"
#include "can.h"
#include "uip.h"
#include "io-helper.h"
#include "tcp_app.h"

/**
  * @brief  Configures the CAN.
  * @param  None
  * @retval None
  */
void CAN_config(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    CAN_InitTypeDef         CAN_InitStructure;
    CAN_FilterInitTypeDef   CAN_FilterInitStructure;

    // GPIO clock enable 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    // Configure CAN pin: RX 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // Configure CAN pin: TX 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_PinRemapConfig(GPIO_Remap2_CAN1 , ENABLE);

    // CANx Periph clock enable 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    // CAN register init 
    CAN_DeInit(CAN1);
    CAN_StructInit(&CAN_InitStructure);

    // CAN cell init 
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;

    // Baudrate = 125kbps
    CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1=CAN_BS1_2tq;
    CAN_InitStructure.CAN_BS2=CAN_BS2_3tq;
    CAN_InitStructure.CAN_Prescaler=48;
    CAN_Init(CAN1, &CAN_InitStructure);

    // CAN filter init 
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);


    // Enable the CAN RX Interrupt
    NVIC_InitTypeDef NVIC_InitStructure_CAN;
    NVIC_InitStructure_CAN.NVIC_IRQChannel = CAN1_RX0_IRQn;
    NVIC_InitStructure_CAN.NVIC_IRQChannelPreemptionPriority = 0x0;
    NVIC_InitStructure_CAN.NVIC_IRQChannelSubPriority = 0x0;
    NVIC_InitStructure_CAN.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure_CAN);

    //  Enable CAN Interrupt
    CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}


// *** structure of ExtID ***
//Sender        = RxMessage.ExtId & 0b00000111111110000000000000000 (8Bit)
//Empfaenger    = RxMessage.ExtId & 0b00000000000001111111100000000 (8Bit)
//Type          = RxMessage.ExtId & 0b00000000000000000000011111111 (8Bit)
//ID-Type       = RxMessage.IDE (CAN_Id_Standard or CAN_Id_Extended) DEFAULT=1
//RTR           = RxMessage.RTR: immer 1 (nie daten anfragen)

void send_pong(CanRxMsg RxMessage)
{
    //ping request
    if( getTyp(RxMessage.ExtId) == CAN_PROTO_PING )
    {
        CanTxMsg TxMessage;
        TxMessage.IDE = CAN_ID_EXT;
        TxMessage.ExtId = CAN_EXT_ID;
        TxMessage.ExtId |= setSender( NODE_CAN_ID );
        TxMessage.ExtId |= setType( CAN_PROTO_PONG );
        TxMessage.ExtId |= setRecipient( getSender(RxMessage.ExtId) );
        TxMessage.RTR = CAN_RTR_Data; //how many bytes have been recept, send them back

        // send back all received packages
        TxMessage.DLC = RxMessage.DLC;
        int i;
        for(i = 0 ; i < RxMessage.DLC ; i++)
        {
            TxMessage.Data[i] = RxMessage.Data[i];
        }
        CAN_Transmit(CAN1, &TxMessage);
    }
}

uint32_t last_ping_send = 0;

// send broadcast ping message
uint32_t send_ping(char data)
{
    CanTxMsg TxMessage;
    TxMessage.IDE = CAN_ID_EXT;
    TxMessage.ExtId = CAN_EXT_ID;
    TxMessage.ExtId |= setSender( NODE_CAN_ID );
    TxMessage.ExtId |= setType( CAN_PROTO_PING );
    TxMessage.ExtId |= setRecipient( NODE_CAN_BROADCAST );
    TxMessage.RTR = 0;
    TxMessage.DLC = 1;
    TxMessage.Data[0] = data;
    CAN_Transmit(CAN1, &TxMessage);
    last_ping_send = SysTick->VAL;
}

// send sync package
void send_sync(char data)
{
    CanTxMsg TxMessage;
    TxMessage.IDE = CAN_ID_EXT;
    TxMessage.ExtId = CAN_EXT_ID;
    TxMessage.ExtId |= setSender( NODE_CAN_ID );
    TxMessage.ExtId |= setType( CAN_PROTO_SYNC );
    TxMessage.ExtId |= setRecipient( NODE_CAN_BROADCAST );
    TxMessage.RTR = 0;
    TxMessage.DLC = 1;
    TxMessage.Data[0] = data;
    CAN_Transmit(CAN1, &TxMessage);
}

// deal with new can packages
void prozess_can_it(void)
{
    CanRxMsg RxMessage;
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);

    if(RxMessage.IDE == CAN_Id_Standard)
    {
        //not used at the moment
    }
    else
    {
        // wenn es nicht von mir kommt
        if( getRecipient(RxMessage.ExtId) == NODE_CAN_ID || getRecipient(RxMessage.ExtId) == NODE_CAN_BROADCAST )
        //if( getSender(RxMessage.ExtId) != NODE_CAN_ID ) // recevie all packages
        {
            //PING
            if( getTyp(RxMessage.ExtId) == CAN_PROTO_PING )
                send_pong(RxMessage);
            //SYNC
            else if( getTyp(RxMessage.ExtId) == CAN_PROTO_SYNC )
            {
                if(RxMessage.Data[0] == 0)
                    LED_Off(2);
                else if(RxMessage.Data[0] == 1)
                    LED_On(2);
                else
                    LED_Toggle(2);
            }
            else //relay can msg to ehternet connection
            {
                if(uip_conn != 0) // wenn tcp vorhanden antwort raushauen
                {
                    // TODO send msg when finished
                    uip_tcp_appstate_t  *s = (uip_tcp_appstate_t  *)&(uip_conn->appstate);
                    //TODO figure out why the uip_conn is not null eaven if there is no connection!!!
                    if(s!=0 && uip_conn->ripaddr[0] != 0 && uip_conn->ripaddr[1] != 0)
                    {
                        int i;

                        //CanMsg per Ethernet verschicken
                        send_tcp(s, "", 0);
                        append_to_cur_tcp(s, 0x15 );
                        append_to_cur_tcp(s, 0x00 );
                        append_to_cur_tcp(s, getTyp(RxMessage.ExtId) );
                        append_to_cur_tcp(s, getRecipient(RxMessage.ExtId) );
                        append_to_cur_tcp(s, getSender(RxMessage.ExtId) );
                        append_to_cur_tcp(s, RxMessage.DLC );
                        //data
                        
                        for(i=0 ; i < RxMessage.DLC ; i++)
                        {
                            append_to_cur_tcp(s, RxMessage.Data[i] );
                        }
                        
                        //append the time between send and receive
                        if( getTyp(RxMessage.ExtId) == CAN_PROTO_PONG )
                        {
                            append_to_cur_tcp(s, ((SysTick->VAL - last_ping_send)>>8) & 0xFF );
                            append_to_cur_tcp(s, (SysTick->VAL - last_ping_send) & 0xFF );
                        }
                        
                        //append_to_cur_tcp(s, RxMessage.Data[ RxMessage.DLC-1 ] );
                    }
                }
            }
        }
    }
}

