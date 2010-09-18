/*
 * Copyright  2008-2009 INRIA/SensTools
 * 
 * <dev-team@sentools.info>
 * 
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
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

/**
 * \addtogroup cc1100
 * @{
 */

/**
 * \file
 * \brief CC1100 driver implementation
 * \author Guillaume Chelius <guillaume.chelius@inria.fr>
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date October 08
 */

/**
 * @}
 */

#include <io.h>
#include <signal.h>

#include "spi1.h"

#include "cc1100.h"
#include "cc1100_gdo.h"
#include "cc1100_globals.h"

static uint16_t (*gdo0_cb)(void);
static uint16_t (*gdo2_cb)(void);

void inline micro_delay(register unsigned int n)
{
    __asm__ __volatile__ (
  "1: \n"
  " dec %[n] \n"
  " jne 1b \n"
        : [n] "+r"(n));
}

void cc1100_reinit(void)
{
    spi1_init();
}

critical void cc1100_init(void)
{
  gdo0_cb = 0x0;
  gdo2_cb = 0x0;
  
  spi1_init();
  GDO_INIT();
  
  spi1_select(SPI1_CC1100);
  spi1_deselect(SPI1_CC1100);
  spi1_select(SPI1_CC1100);
  spi1_deselect(SPI1_CC1100);
  micro_delay(80);
  spi1_select(SPI1_CC1100);
  while (spi1_read_somi()) ;
  spi1_write_single(CC1100_STROBE_SRES | CC1100_ACCESS_STROBE);
  while (spi1_read_somi()) ;
  spi1_deselect(SPI1_CC1100);
  
  
  // write default frequency : 868MHz
  cc1100_write_reg(CC1100_REG_FREQ2, 0x20);
  cc1100_write_reg(CC1100_REG_FREQ1, 0x25);
  cc1100_write_reg(CC1100_REG_FREQ0, 0xED);
  
  // value from SmartRF
  cc1100_write_reg(CC1100_REG_DEVIATN, 0x0);
}

critical uint8_t cc1100_read_reg(uint8_t addr)
{
  uint8_t reg;
  spi1_select(SPI1_CC1100);
  spi1_write_single(addr | CC1100_ACCESS_READ);
  reg = spi1_read_single();
  spi1_deselect(SPI1_CC1100);
  return reg;
}

critical void cc1100_write_reg(uint8_t addr, uint8_t value)
{
  spi1_select(SPI1_CC1100);
  spi1_write_single(addr | CC1100_ACCESS_WRITE);
  spi1_write_single(value);
  spi1_deselect(SPI1_CC1100);
}

critical uint8_t cc1100_strobe_cmd(uint8_t cmd)
{
  uint8_t ret;
  spi1_select(SPI1_CC1100);
  ret = spi1_write_single(cmd | CC1100_ACCESS_STROBE);
  spi1_deselect(SPI1_CC1100);
  return ret;
}

critical void cc1100_fifo_put(uint8_t* buffer, uint16_t length)
{
  spi1_select(SPI1_CC1100);
  spi1_write_single(CC1100_DATA_FIFO_ADDR | CC1100_ACCESS_WRITE_BURST);
  spi1_write(buffer, length);
  spi1_deselect(SPI1_CC1100);
}

critical void cc1100_fifo_get(uint8_t* buffer, uint16_t length)
{
  spi1_select(SPI1_CC1100);
  spi1_write_single(CC1100_DATA_FIFO_ADDR | CC1100_ACCESS_READ_BURST);
  spi1_read(buffer, length);
  spi1_deselect(SPI1_CC1100);
}

critical uint8_t cc1100_read_status(uint8_t addr)
{
  return cc1100_read_reg(addr | CC1100_ACCESS_STATUS);
}


void cc1100_gdo0_register_callback(uint16_t (*cb)(void))
{
  gdo0_cb = cb;
}

void cc1100_gdo2_register_callback(uint16_t (*cb)(void))
{
  gdo2_cb = cb;
}

#define STATE_IDLE    0
#define STATE_RX      1
#define STATE_TX      2
#define STATE_FSTXON  3
#define STATE_CALIB   4
#define STATE_SETTL   5
#define STATE_RXOVER  6
#define STATE_TXUNDER 7

#define WAIT_STATUS(status) \
    while ( ((cc1100_cmd_nop()>>4) & 0x7) != status) ;

void cc1100_cmd_calibrate(void)
{
  cc1100_cmd_idle();
  cc1100_strobe_cmd(CC1100_STROBE_SCAL);
  WAIT_STATUS(STATE_IDLE);
}

void cc1100_cmd_idle(void)
{
  switch ((cc1100_cmd_nop() >> 4) & 0x7)
  {
    case STATE_RXOVER:
      cc1100_cmd_flush_rx();
      break;
    case STATE_TXUNDER:
      cc1100_cmd_flush_tx();
      break;
    default:
      cc1100_strobe_cmd(CC1100_STROBE_SIDLE);
  }
  WAIT_STATUS(STATE_IDLE);
}


void port1irq(void);
/**
 * Interrupt service routine for PORT1.
 * Used for handling CC1100 interrupts triggered on
 * the GDOx pins.
 */
interrupt(PORT1_VECTOR) port1irq(void)
{
  if (P1IFG & GDO0_PIN)
  {
    GDO0_INT_CLEAR();
    if (gdo0_cb != 0x0)
    {
      if (gdo0_cb())
      {
          LPM4_EXIT;
      }
    }
  }

  if (P1IFG & GDO2_PIN)
  {
    GDO2_INT_CLEAR();
    if (gdo2_cb != 0x0)
    {
      if (gdo2_cb())
      {
          LPM4_EXIT;
      }
    }
  } 
}

