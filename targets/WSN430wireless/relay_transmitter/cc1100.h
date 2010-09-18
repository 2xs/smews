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
 * \defgroup cc1100 CC1100 radio driver
 * \ingroup wsn430
 * @{
 * This module allows the user to control the CC1100 radio chip.
 * 
 * The CC1100 is a sub 1GHz radio chip offering a wide
 * range of configuration options, such as frequency, bandwidth, 
 * datarate, modulation, power, etc...
 * It may be fine-tuned to do many wireless network protocols.
 */

/**
 * \file
 * \brief CC1100 driver header
 * \author Guillaume Chelius <guillaume.chelius@inria.fr>
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date October 08
 */
#ifndef _CC1100_H
#define _CC1100_H

#include "cc1100_globals.h"
#include "cc1100_gdo.h"

// other
void micro_delay(register unsigned int n);

// real functions
/**
 * \brief radio initialization procedure
 */
void cc1100_init(void);

/**
 * \brief Re-initialize the SPI driver only
 */
void cc1100_reinit(void);

/**
 * \brief read the content of any cc1100 register
 * \param addr the address of the register
 * \return the value of the register
 */
uint8_t cc1100_read_reg(uint8_t addr);

/**
 * \brief write a value to any cc1100 register
 * \param addr the address of the register
 * \param value the value of to be written
 */
void cc1100_write_reg(uint8_t addr, uint8_t value);

/**
 * \brief copy a buffer to the radio TX FIFO
 * \param buffer a pointer to the buffer
 * \param length the number of bytes to copy
 */
void cc1100_fifo_put(uint8_t* buffer, uint16_t length);

/**
 * \brief copy the content of the radio RX FIFO to a buffer
 * \param buffer a pointer to the buffer
 * \param length the number of bytes to copy
 **/
void cc1100_fifo_get(uint8_t* buffer, uint16_t length);

/**
 * \brief read a cc1100 status register
 * \param addr the address of the register
 * \return the value of the register
 */
uint8_t cc1100_read_status(uint8_t addr);

/**
 * \brief strobe a command to the CC1100
 * \param cmd the command to strobe
 * \return the status byte read while strobing
 */
uint8_t cc1100_strobe_cmd(uint8_t cmd);


// status byte
#define cc1100_status() \
    cc1100_strobe_cmd(CC1100_STROBE_SNOP)

// commands macros

/**
 * \brief force chip to reset all registers to default state
 */
#define cc1100_cmd_reset() \
    cc1100_strobe_cmd(CC1100_STROBE_SRES)

/**
 * \brief stop cristal oscillator
 */
#define cc1100_cmd_xoff() \
    cc1100_strobe_cmd(CC1100_STROBE_SXOFF)

/**
 * \brief calibrate frequency synthetizer
 */
void cc1100_cmd_calibrate(void);

/**
 * \brief enable rx.
 */
#define cc1100_cmd_rx() \
    cc1100_strobe_cmd(CC1100_STROBE_SRX)

/**
 * \brief enable tx. if in rx with cca enabled, go to tx if channel clear
 */
#define cc1100_cmd_tx() \
    cc1100_strobe_cmd(CC1100_STROBE_STX)

/**
 * \brief stop tx/rx/calibration/wor
 */
void cc1100_cmd_idle(void);

/**
 * \brief start wake-on-radio : periodic channel sampling with RX polling
 */
#define cc1100_cmd_wor() \
    cc1100_strobe_cmd(CC1100_STROBE_SWOR)

/**
 * \brief enter power down
 */
#define cc1100_cmd_pwd() \
    cc1100_strobe_cmd(CC1100_STROBE_SPWD)

/**
 * \brief flush RX FIFO
 */
#define cc1100_cmd_flush_rx() \
    cc1100_strobe_cmd(CC1100_STROBE_SFRX)

/**
 * \brief flush TX FIFO
 */
#define cc1100_cmd_flush_tx() \
    cc1100_strobe_cmd(CC1100_STROBE_SFTX)

/**
 * \brief reset real time clock to Event1 value for WOR
 */
#define cc1100_cmd_reset_wor() \
    cc1100_strobe_cmd(CC1100_STROBE_SWORRST)

/**
 * \brief does nothing, update status byte
 */
#define cc1100_cmd_nop() \
    cc1100_strobe_cmd(CC1100_STROBE_SNOP)


// Power Table Config
/**
 * \brief configure the radio chip with the given power
 * \param power the first table value to use (no shaping supported)
 */
#define cc1100_cfg_patable(table, length) \
    cc1100_write_reg(CC1100_PATABLE_ADDR | CC1100_ACCESS_WRITE_BURST, (table)[0])

/**
 * \name GDOx configuration constants
 * @{
 */
#define CC1100_GDOx_RX_FIFO           0x00  /* assert above threshold, deassert when below         */
#define CC1100_GDOx_RX_FIFO_EOP       0x01  /* assert above threshold or EOP, deassert when empty  */
#define CC1100_GDOx_TX_FIFO           0x02  /* assert above threshold, deassert when below         */
#define CC1100_GDOx_TX_THR_FULL       0x03  /* asserts TX FIFO full. De-asserts when below thr     */
#define CC1100_GDOx_RX_OVER           0x04  /* asserts when RX overflow, deassert when flushed     */
#define CC1100_GDOx_TX_UNDER          0x05  /* asserts when RX underflow, deassert when flushed    */
#define CC1100_GDOx_SYNC_WORD         0x06  /* assert SYNC sent/recv, deasserts on EOP             */
                                            /* In RX, de-assert on overflow or bad address         */
                                            /* In TX, de-assert on underflow                       */
#define CC1100_GDOx_RX_OK             0x07  /* assert when RX PKT with CRC ok, de-assert on 1byte  */
                                            /* read from RX Fifo                                   */
#define CC1100_GDOx_PREAMB_OK         0x08  /* assert when preamble quality reached : PQI/PQT ok   */
#define CC1100_GDOx_CCA               0x09  /* Clear channel assessment. High when RSSI level is   */
                                            /* below threshold (dependent on the current CCA_MODE) */
/**
 * @}
 */

/**
 * \brief Configure the gdo0 output pin.
 * 
 * Example : use 0x06 for sync/eop or 0x0 for rx fifo threshold
 * \param cfg the configuration value
 */
#define cc1100_cfg_gdo0(cfg) \
    cc1100_write_reg(CC1100_REG_IOCFG0, cfg)

/**
 * \brief Configure the gdo2 output pin.
 * 
 * Example : use 0x06 for sync/eop or 0x0 for rx fifo threshold
 * \param cfg the configuration value
 */
#define cc1100_cfg_gdo2(cfg) \
    cc1100_write_reg(CC1100_REG_IOCFG2, cfg)

/**
 * \brief Set the threshold for both RX and TX FIFOs.
 * corresponding values are : 
 * 
 * value   0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 \n
 * TX     61 | 57 | 53 | 49 | 45 | 41 | 37 | 33 | 29 | 25 | 21 | 17 | 13 |  9 |  5 | 1 \n
 * RX      4 |  8 | 12 | 16 | 20 | 24 | 28 | 32 | 36 | 40 | 44 | 48 | 52 | 56 | 60 | 64
 * 
 * \param cfg the configuration value
 */
#define cc1100_cfg_fifo_thr(cfg) \
  cc1100_write_reg(CC1100_REG_FIFOTHR, ((cfg)&0x0F))

/**
 * \brief Set the packet length in fixed packet length mode
 * or the maximum packet length in variable length mode
 * \param cfg the configuration value
 */
#define cc1100_cfg_packet_length(cfg) \
    cc1100_write_reg(CC1100_REG_PKTLEN, (cfg))

/**
 * \brief Set the preamble quality estimator threshold
 * (values are 0-7)
 * \param cfg the configuration value
 */
#define cc1100_cfg_pqt(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_PKTCTRL1); \
  reg = (reg & 0x1F) | (((cfg) << 5) & 0xE0); \
  cc1100_write_reg(CC1100_REG_PKTCTRL1, reg); \
} while (0) 

/**
 * \name CRC Autoflush configuration constants
 * @{
 */
#define CC1100_CRC_AUTOFLUSH_ENABLE  0x1
#define CC1100_CRC_AUTOFLUSH_DISABLE 0x0
/**
 * @}
 */

/**
 * \brief enable/disable the automatic flush of RX FIFO when CRC is not OK
 * \param cfg the configuration value
 */
#define cc1100_cfg_crc_autoflush(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_PKTCTRL1); \
  reg = (reg & 0xF7) | (((cfg) << 3) & 0x08); \
  cc1100_write_reg(CC1100_REG_PKTCTRL1, reg); \
} while (0)

/**
 * \name Append status configuration constants
 * @{
 */
#define CC1100_APPEND_STATUS_ENABLE  0x1
#define CC1100_APPEND_STATUS_DISABLE 0x0
/**
 * @}
 */

/**
 * \brief enable/disable the appending of 2 information bytes at the end of
 * a received packet.
 * 
 * Two extra bytes need to be read from the RXFIFO if the appending is set.
 * The first contains the RSSI of the received signal, the second contains
 * the CRC result on the most significant bit, and the LQI on the 7 others.
 * \param cfg the configuration value
 */
#define cc1100_cfg_append_status(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_PKTCTRL1); \
  reg = (reg & 0xFB) | (((cfg) << 2) & 0x04); \
  cc1100_write_reg(CC1100_REG_PKTCTRL1, reg); \
} while (0)

/**
 * \name Address check configuration constants
 * @{
 */
#define CC1100_ADDR_NO_CHECK                 0x0
#define CC1100_ADDR_CHECK_NO_BROADCAST       0x1
#define CC1100_ADDR_CHECK_BROADCAST_0        0x2
#define CC1100_ADDR_CHECK_NO_BROADCAST_0_255 0x3
/**
 * @}
 */

/**
 * \brief control the address check mode
 * \param cfg the configuration value
 */
#define cc1100_cfg_adr_check(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_PKTCTRL1); \
  reg = (reg & 0xFC) | (((cfg) << 0) & 0x03); \
  cc1100_write_reg(CC1100_REG_PKTCTRL1, reg); \
} while (0)

/**
 * \name Data whitening configuration constants
 * @{
 */
#define CC1100_DATA_WHITENING_ENABLE  0x1
#define CC1100_DATA_WHITENING_DISABLE 0x0
/**
 * @}
 */

/**
 * \brief turn data whitening on/off
 * \param cfg the configuration value
 */
#define cc1100_cfg_white_data(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_PKTCTRL0); \
  reg = (reg & 0xBF) | (((cfg) << 6) & 0x40); \
  cc1100_write_reg(CC1100_REG_PKTCTRL0, reg); \
} while (0)


/**
 * \name CRC calculation configuration constants
 * @{
 */
#define CC1100_CRC_CALCULATION_ENABLE  0x1
#define CC1100_CRC_CALCULATION_DISABLE 0x0
/**
 * @}
 */

/**
 * \brief turn CRC calculation on/off
 * \param cfg the configuration value
 */
#define cc1100_cfg_crc_en(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_PKTCTRL0); \
  reg = (reg & 0xFB) | (((cfg) << 2) & 0x04); \
  cc1100_write_reg(CC1100_REG_PKTCTRL0, reg); \
} while (0)

/**
 * \name Packet length configuration constants
 * @{
 */
#define CC1100_PACKET_LENGTH_FIXED    0x0
#define CC1100_PACKET_LENGTH_VARIABLE 0x1
#define CC1100_PACKET_LENGTH_INFINITE 0x2
/**
 * @}
 */

/**
 * \brief configure the packet length mode
 * \param cfg the configuration value
 */
#define cc1100_cfg_length_config(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_PKTCTRL0); \
  reg = (reg & 0xFC) | (((cfg) << 0) & 0x03); \
  cc1100_write_reg(CC1100_REG_PKTCTRL0, reg); \
} while (0)

/**
 * \brief Set the device address for packet filtration
 * \param cfg the configuration value
 */
#define cc1100_cfg_device_addr(cfg) \
    cc1100_write_reg(CC1100_REG_ADDR, (cfg))

/**
 * \brief Set the channel number.
 * \param cfg the configuration value
 */
#define cc1100_cfg_chan(cfg) \
    cc1100_write_reg(CC1100_REG_CHANNR, (cfg))

/**
 * \brief Set the desired IF frequency.
 * (values are 0-31)
 * \param cfg the configuration value
 */
#define cc1100_cfg_freq_if(cfg) \
    cc1100_write_reg(CC1100_REG_FSCTRL1, ((cfg) & 0x1F))

/**
 * \brief Set the desired base frequency.
 * \param cfg the configuration value (22bits)
 */
#define cc1100_cfg_freq(cfg) do { \
  uint8_t reg; \
  reg = (uint8_t) ( ((cfg)>>16)&0xFF ); \
  cc1100_write_reg(CC1100_REG_FREQ2, reg); \
  reg = (uint8_t) ( ((cfg)>>8)&0xFF ); \
  cc1100_write_reg(CC1100_REG_FREQ1, reg); \
  reg = (uint8_t) ( (cfg)&0xFF ); \
  cc1100_write_reg(CC1100_REG_FREQ0, reg); \
} while (0)

/**
 * \brief Set the exponent of the channel bandwidth
 * (values are 0-3)
 * \param cfg the configuration value
 */
#define cc1100_cfg_chanbw_e(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MDMCFG4); \
  reg = (reg & 0x3F) | (((cfg) << 6) & 0xC0); \
  cc1100_write_reg(CC1100_REG_MDMCFG4, reg); \
} while (0)

/**
 * \brief Set mantissa of the channel bandwidth
 * (values are 0-3)
 * \param cfg the configuration value
 */
#define cc1100_cfg_chanbw_m(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MDMCFG4); \
  reg = (reg & 0xCF) | (((cfg)<<4) & 0x30); \
  cc1100_write_reg(CC1100_REG_MDMCFG4, reg); \
} while (0)

/**
 * \brief Set the exponent of the data symbol rate
 * (values are 0-16)
 * \param cfg the configuration value
 */
#define cc1100_cfg_drate_e(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MDMCFG4); \
  reg = (reg & 0xF0) | (((cfg)) & 0x0F); \
  cc1100_write_reg(CC1100_REG_MDMCFG4, reg); \
} while (0)

/**
 * \brief Set the mantissa of the data symbol rate
 * (values are 0-255)
 * \param cfg the configuration value
 */
#define cc1100_cfg_drate_m(cfg) \
  cc1100_write_reg(CC1100_REG_MDMCFG3, (cfg))

/**
 * \name Modulation configuration constants
 * @{
 */
#define CC1100_MODULATION_2FSK 0x00
#define CC1100_MODULATION_GFSK 0x01
#define CC1100_MODULATION_ASK  0x03
#define CC1100_MODULATION_MSK  0x07
/**
 * @}
 */
/**
 * \brief Set the signal modulation
 * \param cfg the configuration value
 */
#define cc1100_cfg_mod_format(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MDMCFG2); \
  reg = (reg & 0x8F) | (((cfg) << 4) & 0x70); \
  cc1100_write_reg(CC1100_REG_MDMCFG2, reg); \
} while (0)

/**
 * \name Manchester encoding configuration constants
 * @{
 */
#define CC1100_MANCHESTER_ENABLE  0x1
#define CC1100_MANCHESTER_DISABLE 0x0
/**
 * @}
 */
/**
 * \brief Set manchester encoding on/off
 * \param cfg the configuration value
 */
#define cc1100_cfg_manchester_en(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MDMCFG2); \
  reg = (reg & 0xF7) | (((cfg) << 3) & 0x08); \
  cc1100_write_reg(CC1100_REG_MDMCFG2, reg); \
} while (0)


/**
 * \name Sync mode configuration constants
 * @{
 */
#define CC1100_SYNCMODE_NO_PREAMB      0x0
#define CC1100_SYNCMODE_15_16          0x1
#define CC1100_SYNCMODE_16_16          0x2
#define CC1100_SYNCMODE_30_32          0x3
#define CC1100_SYNCMODE_NO_PREAMB_CS   0x4
#define CC1100_SYNCMODE_15_16_CS       0x5
#define CC1100_SYNCMODE_16_16_CS       0x6
#define CC1100_SYNCMODE_30_32_CS       0x7
/**
 * @}
 */
/**
 * \brief select the sync-word qualifier mode
 * \param cfg the configuration value
 */
#define cc1100_cfg_sync_mode(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MDMCFG2); \
  reg = (reg & 0xF8) | (((cfg) << 0) & 0x07); \
  cc1100_write_reg(CC1100_REG_MDMCFG2, reg); \
} while (0)

/**
 * \name FEC configuration constants
 * @{
 */
#define CC1100_FEC_ENABLE  0x1
#define CC1100_FEC_DISABLE 0x0
/**
 * @}
 */
/**
 * \brief Set forward error correction on/off
 * supported in fixed packet length mode only
 * \param cfg the configuration value
 */
#define cc1100_cfg_fec_en(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MDMCFG1); \
  reg = (reg & 0x7F) | (((cfg) << 7) & 0x80); \
  cc1100_write_reg(CC1100_REG_MDMCFG1, reg); \
} while (0)

/**
 * \brief Set the minimum number of preamble bytes to be tramsitted \n
 * Setting :      0  |  1  |  2  |  3  |  4  |  5  |  6  |  7 \n
 * nb. of bytes : 2  |  3  |  4  |  6  |  8  |  12 |  16 |  24 
 * \param cfg the configuration value
 */
#define cc1100_cfg_num_preamble(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MDMCFG1); \
  reg = (reg & 0x8F) | (((cfg) << 4) & 0x70); \
  cc1100_write_reg(CC1100_REG_MDMCFG1, reg); \
} while (0)

/**
 * \brief Set the channel spacing exponent
 * (values are 0-3)
 * \param cfg the configuration value
 */
#define cc1100_cfg_chanspc_e(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MDMCFG1); \
  reg = (reg & 0xFE) | (((cfg) << 0) & 0x01); \
  cc1100_write_reg(CC1100_REG_MDMCFG1, reg); \
} while (0)

/**
 * \brief Set the channel spacing mantissa
 * (values are 0-255)
 * \param cfg the configuration value
 */
#define cc1100_cfg_chanspc_m(cfg) \
    cc1100_write_reg(CC1100_REG_MDMCFG0, (cfg))

/**
 * \name RC oscillator configuration constants
 * @{
 */
#define CC1100_RX_TIME_RSSI_ENABLE  0x1
#define CC1100_RX_TIME_RSSI_DISABLE 0x0
/**
 * @}
 */
/**
 * \brief Set direct RX termination based on rssi measurement
 * \param cfg the configuration value
 */
#define cc1100_cfg_rx_time_rssi(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MCSM2); \
  reg = (reg & 0xEF) | (((cfg) << 4) & 0x10); \
  cc1100_write_reg(CC1100_REG_MCSM2, reg); \
} while (0)

/**
 * \brief Set timeout for syncword search in RX for WOR and normal op
 * (values are 0-7)
 * \param cfg the configuration value
 */
#define cc1100_cfg_rx_time(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MCSM2); \
  reg = (reg & 0xF8) | (((cfg) << 0) & 0x07); \
  cc1100_write_reg(CC1100_REG_MCSM2, reg); \
} while (0)

/**
 * \name CCA mode configuration constants
 * @{
 */
#define CC1100_CCA_MODE_ALWAYS      0x0
#define CC1100_CCA_MODE_RSSI        0x1
#define CC1100_CCA_MODE_PKT_RX      0x2
#define CC1100_CCA_MODE_RSSI_PKT_RX 0x3
/**
 * @}
 */
/**
 * \brief Set the CCA mode reflected in CCA signal
 * \param cfg the configuration value
 */
#define cc1100_cfg_cca_mode(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MCSM1); \
  reg = (reg & 0xCF) | (((cfg) << 4) & 0x30); \
  cc1100_write_reg(CC1100_REG_MCSM1, reg); \
} while (0)

/**
 * \name RXOFF mode configuration constants
 * @{
 */
#define CC1100_RXOFF_MODE_IDLE     0x00
#define CC1100_RXOFF_MODE_FSTXON   0x01 /* freq synth on, ready to Tx */
#define CC1100_RXOFF_MODE_TX       0x02 
#define CC1100_RXOFF_MODE_STAY_RX  0x03
/**
 * @}
 */
/**
 * \brief Set the behavior after a packet RX
 * \param cfg the configuration value
 */
#define cc1100_cfg_rxoff_mode(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MCSM1); \
  reg = (reg & 0xF3) | (((cfg) << 2) & 0x0C); \
  cc1100_write_reg(CC1100_REG_MCSM1, reg); \
} while (0)

/**
 * \name TXOFF mode configuration constants
 * @{
 */
#define CC1100_TXOFF_MODE_IDLE     0x00
#define CC1100_TXOFF_MODE_FSTXON   0x01 /* freq synth on, ready to Tx */
#define CC1100_TXOFF_MODE_STAY_TX  0x02
#define CC1100_TXOFF_MODE_RX       0x03
/**
 * @}
 */
/**
 * \brief Set the behavior after packet TX
 * \param cfg the configuration value
 */
#define cc1100_cfg_txoff_mode(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MCSM1); \
  reg = (reg & 0xFC) | (((cfg) << 0) & 0x03); \
  cc1100_write_reg(CC1100_REG_MCSM1, reg); \
} while (0)


/**
 * \name Automatic calibration configuration constants
 * @{
 */
#define CC1100_AUTOCAL_NEVER             0x00
#define CC1100_AUTOCAL_IDLE_TO_TX_RX     0x01
#define CC1100_AUTOCAL_TX_RX_TO_IDLE     0x02
#define CC1100_AUTOCAL_4TH_TX_RX_TO_IDLE 0x03
/**
 * @}
 */
/**
 * \brief Set auto calibration policy
 * \param cfg the configuration value
 */
#define cc1100_cfg_fs_autocal(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_MCSM0); \
  reg = (reg & 0xCF) | (((cfg) << 4) & 0x30); \
  cc1100_write_reg(CC1100_REG_MCSM0, reg); \
} while (0)

/**
 * \brief Set the relative threshold for asserting Carrier Sense \n
 * Setting :    0     |  1  |  2   |  3 \n
 * thr     : disabled | 6dB | 10dB | 14dB \n
 * \param cfg the configuration value
 */
#define cc1100_cfg_carrier_sense_rel_thr(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_AGCCTRL1); \
  reg = (reg & 0xCF) | (((cfg) << 4) & 0x30); \
  cc1100_write_reg(CC1100_REG_AGCCTRL1, reg); \
} while (0)

/**
 * \brief Set the absolute threshold for asserting Carrier Sense
 * referenced to MAGN_TARGET \n
 * Setting :    -8    |  -7  |  -1  |       0        |  1  |   7 \n
 * thr     : disabled | -7dB | -1dB | at MAGN_TARGET | 1dB |  7dB \n
 * \param cfg the configuration value
 */
#define cc1100_cfg_carrier_sense_abs_thr(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_AGCCTRL1); \
  reg = (reg & 0xF0) | (((cfg) << 0) & 0x0F); \
  cc1100_write_reg(CC1100_REG_AGCCTRL1, reg); \
} while (0)

/**
 * \brief Set event0 timeout register for WOR operation
 * \param cfg the configuration value
 */
#define cc1100_cfg_event0(cfg) do { \
  uint8_t reg; \
  reg = (uint8_t)((cfg >> 8) & 0xFF); \
  cc1100_write_reg(CC1100_REG_WOREVT1, reg); \
  reg = (uint8_t)((cfg) & 0xFF); \
  cc1100_write_reg(CC1100_REG_WOREVT0, reg); \
} while (0)

/**
 * \name RC oscillator configuration constants
 * @{
 */
#define CC1100_RC_OSC_ENABLE  0x0
#define CC1100_RC_OSC_DISABLE 0x1
/**
 * @}
 */

/**
 * \brief Set the RC oscillator on/off, needed by WOR
 * \param cfg the configuration value
 */
#define cc1100_cfg_rc_pd(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_WORCTRL); \
  reg = (reg & 0x7F) | (((cfg) << 7) & 0x80); \
  cc1100_write_reg(CC1100_REG_WORCTRL, reg); \
} while (0)

/**
 * \brief Set the event1 timeout register
 * \param cfg the configuration value
 */
#define cc1100_cfg_event1(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_WORCTRL); \
  reg = (reg & 0x8F) | (((cfg) << 4) & 0x70); \
  cc1100_write_reg(CC1100_REG_WORCTRL, reg); \
} while (0)

/**
 * \brief Set the WOR resolution
 * \param cfg the configuration value
 */
#define cc1100_cfg_wor_res(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_WORCTRL); \
  reg = (reg & 0xFC) | (((cfg) << 0) & 0x03); \
  cc1100_write_reg(CC1100_REG_WORCTRL, reg); \
} while (0)

/**
 * \brief select the PA power setting, index of the patable
 * \param cfg the configuration value
 */
#define cc1100_cfg_pa_power(cfg) do { \
  uint8_t reg; \
  reg = cc1100_read_reg(CC1100_REG_FREND0); \
  reg = (reg & 0xF8) | (((cfg) << 0) & 0x07); \
  cc1100_write_reg(CC1100_REG_FREND0, reg); \
} while (0)

// Status Registers access
/**
 * \brief read the register containing the last CRC calculation match
 * and LQI estimate
 */
#define cc1100_status_crc_lqi() \
    cc1100_read_status(CC1100_REG_LQI)

/**
 * \brief read the RSSI
 */
#define cc1100_status_rssi() \
    cc1100_read_status(CC1100_REG_RSSI)

/**
 * \brief read the main radio state machine state
 */
#define cc1100_status_marcstate() \
    cc1100_read_status(CC1100_REG_MARCSTATE)

/**
 * \brief read the high byte of the WOR timer
 */
#define cc1100_status_wortime1() \
    cc1100_read_status(CC1100_REG_WORTIME1)

/**
 * \brief read the low byte of the WOR timer 
 */
#define cc1100_status_wortime0() \
    cc1100_read_status(CC1100_REG_WORTIME0)

/**
 * \brief read the packet status register
 */
#define cc1100_status_pktstatus() \
    cc1100_read_status(CC1100_REG_PKTSTATUS)

/**
 * \brief read the number of bytes in TX FIFO
 */
#define cc1100_status_txbytes() \
    cc1100_read_status(CC1100_REG_TXBYTES)

/**
 * \brief read the number of bytes in RX FIFO
 */
#define cc1100_status_rxbytes() \
    cc1100_read_status(CC1100_REG_RXBYTES)


// GDOx int config & access

/**
 * \brief enable interrupt for GDO0
 */
#define cc1100_gdo0_int_enable() \
    GDO0_INT_ENABLE()
/**
 * \brief disable interrupt for GDO0
 */
#define cc1100_gdo0_int_disable() \
    GDO0_INT_DISABLE()
/**
 * \brief clear interrupt for GDO0
 */
#define cc1100_gdo0_int_clear() \
    GDO0_INT_CLEAR()
/**
 * \brief configure interrupt for GDO0 on high to low transition
 */
#define cc1100_gdo0_int_set_falling_edge() \
    GDO0_INT_SET_FALLING()
/**
 * \brief configure interrupt for GDO0 on low to high transition
 */
#define cc1100_gdo0_int_set_rising_edge() \
    GDO0_INT_SET_RISING()
/**
 * \brief read the state of GDO0
 */
#define cc1100_gdo0_read() \
    GDO0_READ()

/**
 * \brief register a callback function for GDO0 interrupt
 * \param cb a function pointer
 */
void cc1100_gdo0_register_callback(uint16_t (*cb)(void));

/**
 * \brief enable interrupt for GDO2
 */
#define cc1100_gdo2_int_enable() \
    GDO2_INT_ENABLE()
/**
 * \brief disable interrupt for GDO2
 */
#define cc1100_gdo2_int_disable() \
    GDO2_INT_DISABLE()
/**
 * \brief clear interrupt for GDO2
 */
#define cc1100_gdo2_int_clear() \
    GDO2_INT_CLEAR()
/**
 * \brief configure interrupt for GDO2 on high to low transition
 */
#define cc1100_gdo2_int_set_falling_edge() \
    GDO2_INT_SET_FALLING()
/**
 * \brief configure interrupt for GDO2 on low to high transition
 */
#define cc1100_gdo2_int_set_rising_edge() \
    GDO2_INT_SET_RISING()
/**
 * \brief read the state of GDO2
 */
#define cc1100_gdo2_read() \
    GDO2_READ()
/**
 * \brief register a callback function for GDO2 interrupt
 * \param cb a function pointer
 */
void cc1100_gdo2_register_callback(uint16_t (*cb)(void));

#endif

/**
 * @}
 */
