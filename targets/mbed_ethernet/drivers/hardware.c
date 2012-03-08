/*
* Copyright or © or Copr. 2011, Michael Hauspie
*
* Author e-mail: michael.hauspie@lifl.fr
*
* This software is a computer program whose purpose is to design an
* efficient Web server for very-constrained embedded system.
*
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use,
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info".
*
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability.
*
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or
* data to be ensured and,  more generally, to use and operate it in the
* same conditions as regards security.
*
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*/
/*
  Author: Michael Hauspie <michael.hauspie@univ-lille1.fr>
  Created: 2011-07-13
  Time-stamp: <2012-02-21 16:52:41 (hauspie)>
*/

#ifdef IPV6
#error "This target does not support IPv6 yet."
#endif

/* Mbed port includes */
#include "target.h"
#include "mbed_debug.h"
#include "debug_console.h"
#include "eth_input.h"
#include "protocols.h"
#include "out_buffers.h"

/* RFLPC includes */
#include <rflpc17xx/rflpc17xx.h>


/* Smews core includes */
#include "memory.h"
#include "connections.h"

/* transmission descriptors */
rfEthDescriptor _tx_descriptors[TX_DESCRIPTORS] __attribute__ ((section(".out_ram")));;
/* reception descriptors */
rfEthDescriptor _rx_descriptors[RX_DESCRIPTORS] __attribute__ ((section(".out_ram")));;

/* transmission status */
rfEthTxStatus   _tx_status[TX_DESCRIPTORS] __attribute__ ((section(".out_ram")));;
/* reception status */
rfEthRxStatus   _rx_status[RX_DESCRIPTORS] __attribute__ ((section(".out_ram")));;

/* Reception buffers */
uint8_t _rx_buffers[RX_DESCRIPTORS*RX_BUFFER_SIZE] __attribute__ ((section(".out_ram")));;


int putchar(int c)
{
    static int uart_init = 0;
    if (!uart_init)
    {
	rflpc_uart_init(RFLPC_UART0);
	uart_init = 1;
    }
    rflpc_uart_putchar(RFLPC_UART0, c);
    return c;
}

void mbed_eth_garbage_tx_buffers()
{
    int i;
    /* Free sent packets. This loop will be executed on RX IRQ and on TX IRQ */
    for (i = 0 ; i <= LPC_EMAC->TxDescriptorNumber ; ++i)
    {
	if (_tx_status[i].status_info != PACKET_BEEING_SENT_MAGIC && _tx_descriptors[i].packet != NULL)
	{
           if (mbed_eth_is_releasable_buffer(_tx_descriptors[i].packet)) /* static buffers */
               mbed_eth_release_tx_buffer(_tx_descriptors[i].packet);
	   _tx_descriptors[i].packet = NULL;
	}
    }
}

RFLPC_IRQ_HANDLER _eth_irq_handler()
{
    rfEthDescriptor *d;
    rfEthRxStatus *s;
    int i = 0;

    if (rflpc_eth_irq_get_status() & RFLPC_ETH_IRQ_EN_RX_DONE) /* packet received */
    {
	/* Process all pending packets, but limit to the number of descriptor
	 * to avoid beeing stuck in handler because of packet flood */
    	while (rflpc_eth_get_current_rx_packet_descriptor(&d, &s) && i++ < TX_DESCRIPTORS)
    	{
            if (mbed_process_input(d->packet, rflpc_eth_get_packet_size(s->status_info)) == ETH_INPUT_FREE_PACKET)
		rflpc_eth_done_process_rx_packet();
	    else
		break;
	}
    }
    if (rflpc_eth_irq_get_status() & RFLPC_ETH_IRQ_EN_TX_DONE)
	mbed_eth_garbage_tx_buffers();
    rflpc_eth_irq_clear(rflpc_eth_irq_get_status());
}

RFLPC_IRQ_HANDLER _uart_irq()
{
    char c = rflpc_uart_getchar(RFLPC_UART0);
#ifdef MBED_USE_CONSOLE
    mbed_console_add_char(c);
#endif
}

static void _init_buffers()
{
    int i;
    for (i = 0 ; i < RX_DESCRIPTORS ; ++i)
    {
	_rx_descriptors[i].packet = _rx_buffers + RX_BUFFER_SIZE*i;
	_rx_descriptors[i].control = (RX_BUFFER_SIZE - 1) | (1 << 31); /* -1 encoding and enable irq generation on packet reception */
    }
    for (i = 0 ; i < TX_DESCRIPTORS ; ++i)
	_tx_descriptors[i].packet = NULL;
    rflpc_eth_set_tx_base_addresses(_tx_descriptors, _tx_status, TX_DESCRIPTORS);
    rflpc_eth_set_rx_base_addresses(_rx_descriptors, _rx_status, RX_DESCRIPTORS);
    rflpc_eth_set_irq_handler(_eth_irq_handler);
    rflpc_eth_irq_enable_set(RFLPC_ETH_IRQ_EN_RX_DONE | RFLPC_ETH_IRQ_EN_TX_DONE);
}

EthAddr local_eth_addr;
extern char _data_start;
extern char _data_end;
extern char _bss_start;
extern char _bss_end;
void mbed_eth_hardware_init(void)
{
    rflpc_uart_init(RFLPC_UART0);
    /* Configure and start the timer. Timer 0 will be used for timestamping */
    rflpc_timer_enable(RFLPC_TIMER0);   
    /* Clock the timer with the slower clock possible. Enough for millisecond precision */
    rflpc_timer_set_clock(RFLPC_TIMER0, RFLPC_CCLK_8);
    /* Set the pre scale register so that timer counter is incremented every 1ms */
    rflpc_timer_set_pre_scale_register(RFLPC_TIMER0, rflpc_clock_get_system_clock() / 8000);
    /* Start the timer */
    rflpc_timer_start(RFLPC_TIMER0);
    /* Init the GDMA */
    rflpc_dma_init();


    /* Init output buffers */
    mbed_eth_init_tx_buffers();

    printf(" #####                                          #     # ######  ####### ######\r\n");
    printf("#     #  #    #  ######  #    #   ####          ##   ## #     # #       #     #\r\n");
    printf("#        ##  ##  #       #    #  #              # # # # #     # #       #     #\r\n");
    printf(" #####   # ## #  #####   #    #   ####          #  #  # ######  #####   #     #\r\n");
    printf("      #  #    #  #       # ## #       #         #     # #     # #       #     #\r\n");
    printf("#     #  #    #  #       ##  ##  #    #         #     # #     # #       #     #\r\n");
    printf(" #####   #    #  ######  #    #   ####          #     # ######  ####### ######\r\n");
    printf("Compiled on %s %s\r\n", __DATE__, __TIME__);
    printf("\r\n");

    printf(".data  size: %d\r\n", &_data_end - &_data_start);
    printf(".bss   size: %d\r\n", &_bss_end - &_bss_start);
    printf(".stack size: %d\r\n", RFLPC_STACK_SIZE);
    printf("Total: %d\r\n", (&_data_end - &_data_start) + (&_bss_end - &_bss_start) + RFLPC_STACK_SIZE);



    /* Set the MAC addr from the local ip */
    /* @todo: use hardware ID chip from MBED */
    local_eth_addr.addr[0] = 2;
    local_eth_addr.addr[1] = 3;
    local_eth_addr.addr[2] = local_ip_addr[3];
    local_eth_addr.addr[3] = local_ip_addr[2];
    local_eth_addr.addr[4] = local_ip_addr[1];
    local_eth_addr.addr[5] = local_ip_addr[0];

    printf("ETH Init...");
    rflpc_eth_init();
    rflpc_eth_set_mac_address(local_eth_addr.addr);
    _init_buffers();

    while (!rflpc_eth_link_state());
    printf(" done! Link is up\r\n");
    printf("My MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n", local_eth_addr.addr[0],
           local_eth_addr.addr[1],
           local_eth_addr.addr[2],
           local_eth_addr.addr[3],
           local_eth_addr.addr[4],
           local_eth_addr.addr[5]);
    printf("My ip: %d.%d.%d.%d\r\n", local_ip_addr[3], local_ip_addr[2], local_ip_addr[1], local_ip_addr[0]);
    printf("Starting system takes %d ms\r\n", rflpc_timer_get_counter(RFLPC_TIMER0));
    mbed_console_prompt();
    rflpc_uart_set_rx_callback(RFLPC_UART0, _uart_irq);
}
