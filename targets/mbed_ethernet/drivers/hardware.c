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
  Time-stamp: <2011-07-13 17:45:13 (hauspie)>
*/
#include <rflpc17xx/drivers/uart.h>
#include <rflpc17xx/drivers/ethernet.h>
#include <rflpc17xx/debug.h>
#include <rflpc17xx/printf.h>


typedef struct
{
    uint8_t addr[6];
} rfMacAddr;

static rfMacAddr _my_addr = {{2, 3, 4, 5, 6, 7}};

int putchar(int c)
{
    rflpc_uart0_putchar(c);
    return c;
}

void mbed_eth_hardware_init(void)
{
    if (rflpc_uart0_init() == -1)
	RFLPC_STOP(RFLPC_LED_1 | RFLPC_LED_4, 10000);
    rflpc_eth_init();
    while (!rflpc_eth_link_state());
    rflpc_eth_set_mac_address(_my_addr.addr);
    printf("Mbed Ethernet smews initialization done\r\n");
}
