/*
* Copyright or © or Copr. 2012, Michael Hauspie
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
<generator>
        <handlers init="udp_test_init" doGet="udp_test_get"/>
</generator>
 */

#ifdef DISABLE_GP_IP_HANDLER
	#error "This application can not work with disable_general_purpose_ip_handler"
#endif

#include "apps/udp/udp.h"

static char udp_test_get(struct args_t *args)
{
    return 0;
}

char udp_test_in(struct udp_args_t *udp_args)
{
    int i;
    uint16_t tmp;

    for (i = 0 ; i < udp_args->payload_size ; ++i)
    {
	if (i % 16 == 0)
	    printf("\r\n%04x ", i);
	printf("%02x ", in());
    }
    /* Switch ports for answer */
    tmp = udp_args->src_port;
    udp_args->src_port = udp_args->dst_port;
    udp_args->dst_port = tmp;
    /* Returning 1 requests a call to the out callback */
    return 1;
}

char udp_test_out(struct udp_args_t *udp_args)
{
    const char *str = "Received!\r\n";
    udp_outa(str, 11);
    return 0;
}

char udp_test_init(void)
{
    udp_listen(2000, udp_test_in, udp_test_out);
    return 1;
}

