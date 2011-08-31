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
  Time-stamp: <2011-08-31 17:42:09 (hauspie)>
*/

/* RFLPC includes */
#include <rflpc17xx/drivers/uart.h>
#include <rflpc17xx/drivers/ethernet.h>
#include <rflpc17xx/drivers/rit.h>
#include <rflpc17xx/debug.h>
#include <rflpc17xx/printf.h>

/* Smews core includes */
#include "memory.h"

/* Mbed port includes */
#include "target.h"
#include "mbed_debug.h"
#include "eth_input.h"
#include "protocols.h"

/* transmission descriptors */
rfEthDescriptor _tx_descriptors[TX_DESCRIPTORS] __attribute__ ((aligned(4)));
/* reception descriptors */
rfEthDescriptor _rx_descriptors[RX_DESCRIPTORS] __attribute__ ((aligned(4)));;

/* transmission status */
rfEthTxStatus   _tx_status[TX_DESCRIPTORS] __attribute__ ((aligned(4)));;
/* reception status */
rfEthRxStatus   _rx_status[RX_DESCRIPTORS] __attribute__ ((aligned(4)));;


/* Transmission buffers */
uint8_t _tx_buffers[TX_DESCRIPTORS][TX_BUFFER_SIZE] __attribute__ ((aligned(4)));;
/* Reception buffers */
uint8_t _rx_buffers[RX_DESCRIPTORS][RX_BUFFER_SIZE] __attribute__ ((aligned(4)));;



int putchar(int c)
{
    static int uart_init = 0;
    RFLPC_ASSERT_STACK();
    if (!uart_init)
    {
	rflpc_uart0_init();
	uart_init = 1;
    }
    rflpc_uart0_putchar(c);
    return c;
}

RFLPC_IRQ_HANDLER _eth_irq_handler()
{
    rfEthDescriptor *d;
    rfEthRxStatus *s;

    if (rflpc_eth_irq_get_status() & RFLPC_ETH_IRQ_EN_RX_DONE) /* packet received */
    {
	rflpc_eth_irq_disable(RFLPC_ETH_IRQ_EN_RX_DONE);
    	if (rflpc_eth_get_current_rx_packet_descriptor(&d, &s))
    	{
/*	    mbed_dump_packet(d, s, 0);*/
	    if (mbed_process_input(d->packet, rflpc_eth_get_packet_size(s->status_info)) == ETH_INPUT_FREE_PACKET)
		rflpc_eth_done_process_rx_packet();
    	}
	rflpc_eth_irq_enable(RFLPC_ETH_IRQ_EN_RX_DONE);
    }
    rflpc_eth_irq_clear(rflpc_eth_irq_get_status());
}

RFLPC_IRQ_HANDLER _uart_irq()
{
    char c = rflpc_uart0_getchar();
    int i;
    switch (c)
    {
	case 'd':
	    MBED_DEBUG("RxDescriptor: %p\r\n", _rx_descriptors);
	    MBED_DEBUG("TxDescriptor: %p\r\n", _tx_descriptors);
	    for (i = 0 ; i < RX_DESCRIPTORS ; ++i)
	    {
		MBED_DEBUG("Buffer %d: %p\r\n", i, _rx_descriptors[i].packet);
	    }
	    rflpc_eth_dump_internals();
	    break;
	default:
	    break;
    }
}

static void _init_buffers()
{
    int i;
    RFLPC_ASSERT_STACK();
    for (i = 0 ; i < RX_DESCRIPTORS ; ++i)
    {
	_rx_descriptors[i].packet = _rx_buffers[i];
	MBED_DEBUG("Buffer %d: %p\r\n", i, _rx_descriptors[i].packet);
	_rx_descriptors[i].control = (RX_BUFFER_SIZE - 1) | (1 << 31); /* -1 encoding and enable irq generation on packet reception */
    }
    for (i = 0 ; i < TX_DESCRIPTORS ; ++i)
    {
	_tx_descriptors[i].packet = _tx_buffers[i];
    }
    rflpc_eth_set_tx_base_addresses(_tx_descriptors, _tx_status, TX_DESCRIPTORS);
    rflpc_eth_set_rx_base_addresses(_rx_descriptors, _rx_status, RX_DESCRIPTORS);
    rflpc_eth_set_irq_handler(_eth_irq_handler);
    rflpc_eth_irq_enable_set(RFLPC_ETH_IRQ_EN_RX_DONE);
}

const EthAddr local_eth_addr = {{2, 3, 4, 5, 6, 7}};

void mbed_eth_hardware_init(void)
{
    printf(" #####                                          #     # ######  ####### ######\r\n");
    printf("#     #  #    #  ######  #    #   ####          ##   ## #     # #       #     #\r\n");
    printf("#        ##  ##  #       #    #  #              # # # # #     # #       #     #\r\n");
    printf(" #####   # ## #  #####   #    #   ####          #  #  # ######  #####   #     #\r\n");
    printf("      #  #    #  #       # ## #       #         #     # #     # #       #     #\r\n");
    printf("#     #  #    #  #       ##  ##  #    #         #     # #     # #       #     #\r\n");
    printf(" #####   #    #  ######  #    #   ####          #     # ######  ####### ######\r\n");
    printf("\r\n");

    printf("ETH Init...");
    rflpc_eth_init();
    rflpc_eth_set_mac_address(local_eth_addr.addr);
    _init_buffers();

    while (!rflpc_eth_link_state());
    printf(" done! Link is up\r\n");

    rflpc_uart0_set_rx_callback(_uart_irq);
}
