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

#ifndef _PANMANAGER_H_
#define _PANMANAGER_H_

/* functions prototypes for plugins */
typedef int (init_dev_t)(unsigned char *tbuff, unsigned char *dbuff, unsigned dmtu, char *argument);
typedef void (read_from_dev_t)();
typedef void (forward_to_dev_t)(int size);

/* this structure contains the functions pointers of a plugin */
struct dev_handlers_s {
        const char *help_string;
        init_dev_t *init_dev;
        read_from_dev_t *read_from_dev;
        forward_to_dev_t *forward_to_dev;
};

/* check if test is ok, prints an error message if needed and exit */
extern void check(int test, const char *str, ...);
/* prints a message if the verbose mode is on */
extern void message(char *str, ...);
/* forwards a packet to the tun interface */
extern void forward_to_tun(int size);
        
#endif
