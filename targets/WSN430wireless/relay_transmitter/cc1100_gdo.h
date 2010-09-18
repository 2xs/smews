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
 * \brief CC1100 GDO PIN hardware abstraction
 * \author Guillaume Chelius <guillaume.chelius@inria.fr>
 * \author Antoine Fraboulet <antoine.fraboulet@insa-lyon.fr>
 * \author Colin Chaballier
 * \author Clément Burin des Roziers <clement.burin-des-roziers@inria.fr>
 * \date October 08
 */

/**
 * @}
 */

#ifndef _CC1100_GDO_H
#define _CC1100_GDO_H

#define GDO0_PIN (1<<3)
#define GDO2_PIN (1<<4)

/**
 * \brief Initialize IO PORT for GDO connectivity
 **/
#define GDO_INIT() do \
{ \
  P1SEL &= ~(GDO0_PIN | GDO2_PIN); \
  P1DIR &= ~(GDO0_PIN | GDO2_PIN); \
  P1IE  &= ~(GDO0_PIN | GDO2_PIN); \
} while (0)

/**
 * \brief Enable Interrupt for GDO0 pin
 **/
#define GDO0_INT_ENABLE() P1IE |= GDO0_PIN

/**
 * \brief Enable Interrupt for GDO2 pin
 **/
#define GDO2_INT_ENABLE() P1IE |= GDO2_PIN

/**
 * \brief Disable Interrupt for GDO0 pin
 **/
#define GDO0_INT_DISABLE() P1IE &= ~GDO0_PIN

/**
 * \brief Disable Interrupt for GDO2 pin
 **/
#define GDO2_INT_DISABLE() P1IE &= ~GDO2_PIN

/**
 * \brief Clear interrupt flag for GDO0 pin
 **/
#define GDO0_INT_CLEAR() P1IFG &= ~GDO0_PIN
/**
 * \brief Clear interrupt flag for GDO2 pin
 **/
#define GDO2_INT_CLEAR() P1IFG &= ~GDO2_PIN

/**
 * \brief Set interrupt on rising edge for GDO0 pin
 **/
#define GDO0_INT_SET_RISING()  P1IES &= ~GDO0_PIN
/**
 * \brief Set interrupt on falling edge for GDO0 pin
 **/
#define GDO0_INT_SET_FALLING() P1IES |=  GDO0_PIN
/**
 * \brief Set interrupt on rising edge for GDO2 pin
 **/
#define GDO2_INT_SET_RISING()  P1IES &= ~GDO2_PIN
/**
 * \brief Set interrupt on falling edge for GDO2 pin
 **/
#define GDO2_INT_SET_FALLING() P1IES |=  GDO2_PIN

/**
 * \brief Read GDO0 pin value
 **/
#define GDO0_READ() (P1IN & GDO0_PIN)
/**
 * \brief Read GDO2 pin value
 **/
#define GDO2_READ() (P1IN & GDO2_PIN)

#endif
