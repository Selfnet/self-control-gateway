
#include "tcp_app.h"
#include "io-helper.h"
#include "uip.h"
#include "can.h"

#include <string.h>

#define STATE_WAITING 0
#define STATE_OUTPUT  1

#define ISO_nl      0x0a


void send_ascii(uint32_t len, char *txt)
{
//TODO dummy function for sending text
}


int send_tcp(uip_tcp_appstate_t *s, char *txt, uint16_t len)
{
    if(s == 0)
        return -3;

    //queue voll
    if(s->output_queue_pos >= TCP_APP_MAX_QUEUE_LEN)
        return -2;

    if( (s->output_queue_pt[ s->output_queue_pos ] + len + 2 ) - s->output_buf >= TCP_APP_BUFFER_SIZE)
        return -1;

    // hole aktuelle pos und length
    s->output_buf_pt = s->output_queue_pt[ s->output_queue_pos ];
    uint16_t *l = (uint16_t *)s->output_buf_pt;

    //kopiere daten in buffer und setze queuept
    if( memcpy( s->output_buf_pt+2 , txt , len ) || len == 0 )
    {
        *l = len;
        s->output_queue_pos += 1;
        s->output_queue_pt[ s->output_queue_pos ] = s->output_buf_pt+len+2;
        return 1;
    }

    return -3;
}


int append_to_cur_tcp(uip_tcp_appstate_t *s, char c)
{
    if(s == 0)
        return -3;

    //queue schon zuvoll
    if(s->output_queue_pos > 10)
        return -2;

    // nachricht passt nicht mehr in buffer
    s->output_buf_pt = s->output_queue_pt[ s->output_queue_pos ];
    if( s->output_buf_pt - s->output_buf + 1 >= TCP_APP_BUFFER_SIZE)
        return -1;

    s->output_buf_pt = s->output_queue_pt[ s->output_queue_pos-1 ];
    uint16_t *l = (uint16_t *)s->output_buf_pt;

    *l += 1;

    *(*l + s->output_buf_pt + 1) = c;

    //set the next start to the correct address
    s->output_queue_pt[ s->output_queue_pos ] = s->output_buf_pt+*l+2;

    //*(char *)(s->output_queue_pt[ s->output_queue_pos ] + (++ (*(uint16_t *)s->output_queue_pt[ s->output_queue_pos ]))) = c;
    return 1;
}

/*---------------------------------------------------------------------------*/

void handle_input(uip_tcp_appstate_t *s)
{
    char * tmp = ((char *)uip_appdata);
    int    len = uip_len;

    // ethernet bytes:
    // SENDER0 | SENDER1 | EMPFAENGER0 | EMPFAENGER1 | TYPE | LEN | DATA0 - DATA7

    //ping vom server
    if(len == 2 && tmp[0] == 0x01 )
    {
        send_tcp(s, tmp, 2);
    }
    else if(tmp[0] == 'A')
    {
        send_tcp(s, tmp , len);
    }
    // can gateway nachricht
    else if(tmp[0] == 0x15 && len >= 5 && len <= 14)
    {
        //              Sender   Empf     Proto    extID    (reverse due to endianess)
        char extID[] = {tmp[4] , tmp[3] , tmp[2] , tmp[1]};

        CanTxMsg TxMessage;
        TxMessage.IDE = CAN_ID_EXT;                                 //immer extended can frames
        TxMessage.ExtId = *((uint32_t *)extID);
        TxMessage.RTR = 0;
        TxMessage.DLC = tmp[5]; //0 bis 8
        if(TxMessage.DLC > 8)
            TxMessage.DLC = 8;
        int i;
        for(i = 0 ; i <= TxMessage.DLC ; i++)
        {
            TxMessage.Data[i] = tmp[i+6];
        }
        CAN_Transmit(CAN1, &TxMessage);

        LED_Toggle(1);
    }
}

static void
handle_connection(uip_tcp_appstate_t *s)
{
    if( uip_newdata() )
        handle_input(s);
    if( s != 0 && s->output_queue_pos > 0 )
    {
        int len = s->output_queue_pt[ s->output_queue_pos ] - s->output_buf;
        uip_send(s->output_buf , len );
        memset(s->output_buf, 0, len); //leeren
        s->output_queue_pos = 0;
        s->output_queue_pt[0] = s->output_buf;
    }
}

void tcp_test_appcall(void)
{
    struct tcp_test_app_state  *s = (uip_tcp_appstate_t  *)&(uip_conn->appstate);

    if(uip_closed() || uip_aborted() || uip_timedout())
    {
    }
    else if(uip_connected()) //neue verbindung
    {
        // tcp_app_data struct initalisieren
        s->timer = 0;
        memset(s->output_buf, 0, TCP_APP_BUFFER_SIZE); //leeren
        s->output_queue_pos = 0;
        s->output_queue_pt[0] = s->output_buf;

        #ifdef TEST_GATEWAY
            send_tcp(s, "hallo - ich bin test node" , strlen("hallo - ich bin test node"));
        #else
            send_tcp(s, "hallo - ich bin gateway" , strlen("hallo - ich bin gateway"));
        #endif
        append_to_cur_tcp(s, '!');

        handle_connection(s);
    }
    else if(s != 0)
    {
        if(uip_poll())
        {
            ++s->timer;
            if(s->timer >= 200)
                uip_abort(); //Timeout
        }
        else
        {
            s->timer = 0;
        }
        handle_connection(s);
    }
    else
    {
        uip_abort();
    }
}

