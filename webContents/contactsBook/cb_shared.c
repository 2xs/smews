/*
* Copyright or © or Copr. 2008, Simon Duquennoy
* 
* Author e-mail: simon.duquennoy@lifl.fr
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

#include "cb_shared.h"

PERSISTENT_VAR(struct contact_t, contacts[MAX_CONTACTS]) = {
	{"David%20Simplot-Ryl", "david.simplot-ryl@lifl.fr", "06 07 08 09 10", "POPS", "La Madeleine, 59"},
	{"Gilles%20Grimaud", "gilles.grimaud@lifl.fr",  "06 07 08 09 11", "POPS", "Lille, 59"},
	{"Jean%20Carle", "jean.carle@lifl.fr",  "06 07 08 09 12", "POPS", "La Madeleine, 59"},
	{"Michaël%20Hauspie", "michael.hauspie@lifl.fr",  "06 07 08 09 13", "POPS", "Lille, 59"},
	{"Samuel%20Hym", "samuel.hym@lifl.fr",  "06 07 08 09 14", "POPS", "Lille, 59"},
	{"Nathalie%20Mitton", "nathalie.mitton@lifl.fr",  "06 07 08 09 15", "POPS", "Lille, 59"},
	{"Tahiry%20Razafindralambo", "tahiry.razafindralambo@lifl.fr",  "06 07 08 09 16", "POPS", "Lille, 59"},
	{"Isabelle%20Simplot-Ryl", "isabelle.simplot-ryl@lifl.fr",  "06 07 08 09 17", "POPS", "La Madeleine, 59"},
	{"Marie-Emilie%20Voge", "marie-emilie.voge@lifl.fr",  "06 07 08 09 18", "POPS", "Lille, 59"},
};

PERSISTENT_VAR(uint16_t, n_contacts) = 9;
