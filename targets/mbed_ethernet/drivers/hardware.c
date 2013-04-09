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
  Time-stamp: <2013-03-22 11:02:49 (hauspie)>
*/


/* Mbed port includes */
#include "target.h"
#include "mbed_debug.h"
#include "eth_input.h"
#include "protocols.h"
#include "out_buffers.h"
#include "lcd.h"

/* RFLPC includes */
#include <rflpc17xx/rflpc17xx.h>


/* Smews core includes */
#include "memory.h"
#include "connections.h"

/* transmission descriptors */
rflpc_eth_descriptor_t _tx_descriptors[TX_DESCRIPTORS] __attribute__ ((section(".out_ram")));;
/* reception descriptors */
rflpc_eth_descriptor_t _rx_descriptors[RX_DESCRIPTORS] __attribute__ ((section(".out_ram")));;

/* transmission status */
rflpc_eth_tx_status_t   _tx_status[TX_DESCRIPTORS] __attribute__ ((section(".out_ram")));;
/* reception status */
rflpc_eth_rx_status_t   _rx_status[RX_DESCRIPTORS] __attribute__ ((section(".out_ram")));;

/* Reception buffers */
uint8_t _rx_buffers[RX_DESCRIPTORS*RX_BUFFER_SIZE] __attribute__ ((section(".out_ram")));;


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
    rflpc_eth_descriptor_t *d;
    rflpc_eth_rx_status_t *s;
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

#ifdef KERNEL_CONSOLE
#include "kernel_console.h"
#endif

RFLPC_IRQ_HANDLER _uart_irq()
{
    char c = rflpc_uart_getchar(RFLPC_UART0);
#ifdef KERNEL_CONSOLE
    kernel_console_add_char(c);
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

void mbed_auto_set_mac(EthAddr *mac_addr)
{
    unsigned long serial[4];

    if (rflpc_iap_get_serial_number(serial) == 0)
    {
	mac_addr->addr[0] = 2;
	mac_addr->addr[1] = 3;
	mac_addr->addr[2] = serial[0] & 0xFF;
	mac_addr->addr[3] = serial[1] & 0xFF;
	mac_addr->addr[4] = serial[2] & 0xFF;
	mac_addr->addr[5] = serial[3] & 0xFF;
    }
}

#ifdef IPV6
/* This function checks if the set IPv6 addr is the link-local network addr (i.e. in fe80::/64)
 * if so, it generates the link local address from the MAC address*/
void mbed_ipv6_check_and_set_link_local(EthAddr *mac_addr)
{
    unsigned short s = (local_ip_addr[15] << 8) | local_ip_addr[14];
    if (s == 0xfe80) /* link local */
    {
	int i;
	for (i = 0 ; i < 13 ; ++i)
	{
	    if (local_ip_addr[i] != 0)
		break;
	}
	if (i == 13) /* the addr is fe80:: */
	{
	    printf("Configured IPv6 is fe80::, auto-configuring from mac\r\n");
	    local_ip_addr[0] = mac_addr->addr[5];
	    local_ip_addr[1] = mac_addr->addr[4];
	    local_ip_addr[2] = mac_addr->addr[3];
	    local_ip_addr[3] = 0xfe;
	    local_ip_addr[4] = 0xff;
	    local_ip_addr[5] = mac_addr->addr[2];
	    local_ip_addr[6] = mac_addr->addr[1];
	    /* The Universal/Local bit (2) should be inverted when using MAC addr to generated IPv6 addr
	     * See EUI-64 usage for IPv6 */
	    local_ip_addr[7] = ((~mac_addr->addr[0]) & 2) | (mac_addr->addr[0] & ~2);
	}
    }
}
#endif

void mbed_display_ip(const unsigned char *ip)
{
#ifdef IPV6
    int i;
    int was_zero = 0;

    for (i = 7 ; i >= 0 ; --i)
    {
		unsigned short s = (ip[i*2+1] << 8) | ip[i*2];

		if (s != 0)
		{
			if (was_zero)
			printf(":");
			printf("%x", s);
			if (i != 0)
			printf(":");
		}
		if (s == 0 && i == 0)
			printf(":");
		was_zero = (s == 0);
    }
#else
    printf("%d.%d.%d.%d", ip[3], ip[2], ip[1], ip[0]);
#endif
}

void mbed_display_mac(void)
{
    printf("My MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n", local_eth_addr.addr[0],
           local_eth_addr.addr[1],
           local_eth_addr.addr[2],
           local_eth_addr.addr[3],
           local_eth_addr.addr[4],
           local_eth_addr.addr[5]);
}

void mbed_init_timer(void)
{
   /* Configure and start the timer. Timer 0 will be used for timestamping */
    rflpc_timer_enable(RFLPC_TIMER0);
    /* Clock the timer with the slower clock possible. Enough for millisecond precision */
    rflpc_timer_set_clock(RFLPC_TIMER0, RFLPC_CCLK_8);
    /* Set the pre scale register so that timer counter is incremented every 1ms */
    rflpc_timer_set_pre_scale_register(RFLPC_TIMER0, rflpc_clock_get_system_clock() / 8000);
    /* Start the timer */
    rflpc_timer_start(RFLPC_TIMER0);
}

void mbed_print_configuration(void)
{
	printf("Disabled options: ");
#ifdef DISABLE_COROUTINES
	printf("coroutines ");
#endif
#ifdef DISABLE_COMET
	printf("comet ");
#endif
#ifdef DISABLE_POST
	printf("post ");
#endif
#ifdef DISABLE_GP_IP_HANDLER
	printf("gpip ");
#endif
#ifdef DISABLE_ARGS
	printf("args ");
#endif
#ifdef DISABLE_TIMERS
	printf("timers");
#endif
	printf("\r\n");
}

void mbed_print_motd(void)
{
    printf("\r\n  ______\r\n");
    printf(" / _____)                             \r\n");
    printf("( (____   ____   _____  _ _ _   ___\r\n");
    printf(" \\____ \\ |    \\ | ___ || | | | /___)\r\n");
    printf(" _____) )| | | || ____|| | | ||___ |\r\n");
    printf("(______/ |_|_|_||_____) \\___/ (___/\r\n");
    printf("    ___\r\n");
    printf("   / __)\r\n");
    printf(" _| |__  ___    ____ \r\n");
    printf("(_   __)/ _ \\  / ___)\r\n");
    printf("  | |  | |_| || |    \r\n");
    printf("  |_|   \\___/ |_|\r\n");
    printf(" _______  ______   _______  ______\r\n");
    printf("(_______)(____  \\ (_______)(______)\r\n");
    printf(" _  _  _  ____)  ) _____    _     _\r\n");
    printf("| ||_|| ||  __  ( |  ___)  | |   | |\r\n");
    printf("| |   | || |__)  )| |_____ | |__/ /\r\n");
    printf("|_|   |_||______/ |_______)|_____/\r\n");

    printf("Compiled on %s %s\r\n", __DATE__, __TIME__);
	mbed_print_configuration();
    printf("\r\n");

    printf(".data  size: %d B\r\n", &_data_end - &_data_start);
    printf(".bss   size: %d B\r\n", &_bss_end - &_bss_start);
    printf(".stack size: %d B\r\n", RFLPC_STACK_SIZE);
    printf("Total: %d B\r\n", ((&_data_end - &_data_start) + (&_bss_end - &_bss_start) + RFLPC_STACK_SIZE));
}

void mbed_configure_eth(void)
{
    /* Init output buffers */
    mbed_eth_init_tx_buffers();


    /* Set the MAC addr from the flash serial number */
    mbed_auto_set_mac(&local_eth_addr);
#ifdef IPV6
    mbed_ipv6_check_and_set_link_local(&local_eth_addr);
#endif
    printf("Initializing ethernet. ");
    rflpc_eth_init();
    rflpc_eth_set_mac_address(local_eth_addr.addr);
    _init_buffers();

    printf("Waiting for link to be up...");
    while (!rflpc_eth_link_state());
    printf(" done! Link is up\r\n");
    mbed_display_mac();
	printf("My ip: ");
    mbed_display_ip(local_ip_addr);
	printf("\r\n");
}

#ifdef KERNEL_CONSOLE
void mbed_console_init();
#endif

#ifdef MBED_USE_LCD_DISPLAY
int mbed_smews_putchar(int c)
{
    rflpc_uart_putchar(RFLPC_UART0, c);
    lcd_putchar(c);
    return c;
}
#endif

void mbed_eth_hardware_init(void)
{
    /* Init the UART for printf */
    rflpc_uart_init(RFLPC_UART0);

#ifdef MBED_USE_LCD_DISPLAY
    lcd_init_ports();
    lcd_clear();
    rflpc_printf_set_putchar(mbed_smews_putchar);
    printf("Using LCD display\r\n");
#endif
    /* Init rand */
    srand(LPC_RTC->CTIME0);


    mbed_print_motd();

    mbed_init_timer();

    rflpc_dma_init();

#ifdef KERNEL_CONSOLE
	mbed_console_init();
#endif
    /* Init output buffers */
    mbed_eth_init_tx_buffers();

    mbed_configure_eth();

    printf("Starting system takes %d ms\r\n", rflpc_timer_get_counter(RFLPC_TIMER0));
    rflpc_uart_set_rx_callback(RFLPC_UART0, _uart_irq);
}
