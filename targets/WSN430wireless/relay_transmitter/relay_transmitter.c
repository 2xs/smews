/*
* Copyright or © or Copr. 2010, Guillaume Marchand and Damien Riquet
* 
* Authors e-mail: guillaume.marchand@etudiant.univ-lille1.fr
*                 damien.riquet@etudiant.univ-lille1.fr
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

#include "target.h"
#include "serial_line.h"
#include "cc1100.h"

/* LEDs meanings :
* RED   : Hardware is working
* BLUE  : Radio Rx/Tx
* GREEN : Serial Rx/Tx
*/

int main(void) {
  uint16_t i;

  /* Hardware initialisation : serie, radio */
  hardware_init();
  cc1100_init();
  LED_ON(LED_RED); /* Hardware is working */

  /* Relay loop */
  while(1) {}

  return 0;
}
