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
  Time-stamp: <2011-07-15 17:11:16 (hauspie)>
*/
#include <rflpc17xx/drivers/uart.h>
#include <rflpc17xx/drivers/ethernet.h>
#include <rflpc17xx/drivers/rit.h>
#include <rflpc17xx/debug.h>
#include <rflpc17xx/printf.h>

#include "target.h"
#include "memory.h"
#include "eth.h"


/* reception descriptors */
static rfEthDescriptor _rx_descriptors[RX_DESCRIPTORS];
/* reception status */
static rfEthRxStatus   _rx_status[RX_DESCRIPTORS];

/* transmission descriptors */
static rfEthDescriptor _tx_descriptors[TX_DESCRIPTORS];
/* transmission status */
static rfEthTxStatus   _tx_status[TX_DESCRIPTORS];

int putchar(int c)
{
    static int uart_init = 0;
    if (!uart_init)
    {
	rflpc_uart0_init();
	uart_init = 1;
    }
    rflpc_uart0_putchar(c);
    return c;
}

static void _dump_packet(rfEthDescriptor *d, rfEthRxStatus *s)
{
    printf("= %p %p %p",d, s, d->packet);
    if (s->status_info & (1 << 18))
	printf("cf ");
    else
	printf("   ");
    if (s->status_info & (1 << 21))
	printf("mf ");
    else
	printf("   ");
    if (s->status_info & (1 << 22))
	printf("bf ");
    else
	printf("   ");
    if (s->status_info & (1 << 23))
	printf("crc ");
    else
	printf("    ");
    if (s->status_info & (1 << 24))
	printf("se ");
    else
	printf("   ");
    if (s->status_info & (1 << 25))
	printf("le ");
    else
	printf("   ");
    if (s->status_info & (1 << 27))
	printf("ae ");
    else
	printf("   ");
    if (s->status_info & (1 << 28))
	printf("ov ");
    else
	printf("   ");
    if (s->status_info & (1 << 29))
	printf("nd ");
    else
	printf("   ");
    if (s->status_info & (1 << 30))
	printf("lf ");
    else
	printf("   ");

    printf("size:%d ", (s->status_info & 0x7FF) + 1);
/*    for (i = 0 ; i < (s->status_info & 0x7FF) + 1 ; ++i)
    {
	if (i % 16 == 0)
	    printf("\r\n");
	printf("%x ", d->packet[i]);
	}*/
    printf("\r\n");

}


RFLPC_IRQ_HANDLER _eth_irq_handler()
{
    static rfEthDescriptor *d;
    static rfEthRxStatus *s;

    if (rflpc_eth_irq_get_status() & RFLPC_ETH_IRQ_EN_RX_DONE) /* packet received */
    {
    	if (rflpc_eth_get_current_rx_packet_descriptor(&d, &s))
    	{
    	    /* packet received */
    	    process_rx_packet(d,s);
    	    /* done with it */
    	}
    }
    rflpc_eth_irq_clear(rflpc_eth_irq_get_status());
}

static void _init_rx()
{
    int i;
    for (i = 0 ; i < RX_DESCRIPTORS ; ++i)
    {
	_rx_descriptors[i].packet = (uint8_t*) mem_alloc(RX_BUFFER_SIZE);
	_rx_descriptors[i].control = (RX_BUFFER_SIZE - 1) | (1 << 31); /* -1 encoding and enable irq generation on packet reception */
    }
    rflpc_eth_set_irq_handler(_eth_irq_handler);
    rflpc_eth_irq_enable_set(RFLPC_ETH_IRQ_EN_RX_DONE);
    rflpc_eth_set_rx_base_addresses(_rx_descriptors, _rx_status, RX_DESCRIPTORS);
}

RFLPC_IRQ_HANDLER _rit_handler()
{
    rflpc_rit_clear_pending_interrupt();
}

void mbed_eth_hardware_init(void)
{
    rflpc_eth_init();
    while (!rflpc_eth_link_state());
    rflpc_eth_set_mac_address(local_eth_addr.addr);
    rflpc_eth_set_tx_base_addresses(_tx_descriptors, _tx_status, TX_DESCRIPTORS);
    _init_rx();
    printf("Mbed Ethernet smews initialization done\r\n");

   /* setting rit timer to periodicaly check for link state */
    rflpc_rit_enable();
    rflpc_rit_set_callback(0xFFFFFF, 0, 1, _rit_handler);
}
