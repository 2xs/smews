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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <ctype.h>
#include <stdarg.h>
#include <dlfcn.h>

#include "panManager.h"

/* configuration macros */
#define DEVTUN "/dev/net/tun"
#define DEV_MTU 1500

/* globals */
unsigned char tbuffer_shadow[DEV_MTU+4]={0};
#define tbuffer (tbuffer_shadow+4)
unsigned char dbuffer_shadow[DEV_MTU+4]={0};
#define dbuffer (dbuffer_shadow+4)

/* misc. macros */
#define MAX(a,b) ((a)>(b)?(a):(b))

static int ret;
static int tun_fd;
static int dev_fd;
static struct dev_handlers_s *dev_handlers = NULL;
static int verbose = 0;

/* printf if verbose mode is on */
void message(char *str, ...) {
        va_list argp;
        va_start(argp, str);
        if(verbose) {
	      vprintf(str, argp);
        }
        va_end(argp);
}

/* check if test is ok, print error message if needed then exit */
void check(int test, const char *str, ...) {
        va_list argp;
        va_start(argp, str);
        if(!test) {
	      if(errno == 0) {
		    vfprintf(stderr, str, argp);
		    printf("\n");
	      } else {
		    char tmp[2048];
		    snprintf(tmp, 2048, str, argp);
		    perror(tmp);
	      }
	      exit(1);
        }
        va_end(argp);
}

/* usage for this program */
static void usage(const char *path, const char *plugin_help_string) {
        fprintf(stderr, "Usage: %s <plugin> <ip/masksize> [options]\n", path);
        fprintf(stderr, "Options available:\n");
        fprintf(stderr, "\t-h: display this help\n");
        fprintf(stderr, "\t-v: verbose\n");
        fprintf(stderr, "\t-p <arg>: plugin argument");
        if(plugin_help_string == NULL) {
	      fprintf(stderr, " (set a plugin to get detailed informations)");
        } else if(plugin_help_string[0] == '\0'){
	      fprintf(stderr, " (no option available for this plugin)");
        } else {
	      fprintf(stderr, " (%s)", plugin_help_string);
        }
        fprintf(stderr, "\n");
}

/* called by a plugin to forward data from dbuffer in to tun interface */
void forward_to_tun(int size) {
        message("Packet from device: %d bytes\n",size);
        ret = write(tun_fd, (void *)dbuffer_shadow, size + 4);
        check(ret != -1,"write() to tun error");
}

/* manages an incomming packet on the tun interface */
static void tun_input() {
        int nRead;
        nRead = read(tun_fd, (void *)tbuffer_shadow, DEV_MTU);
        check(nRead != -1,"read() from tun error");
        nRead -= 4;
        message("Packet from tun: %d bytes\n",nRead);
        /* call the plugin dependent function, dealing with the data in tbuffer */
        dev_handlers->forward_to_dev(nRead);
}

/* manages an incomming packet on the device interface */
static void dev_input() {
        /* call the plugin dependent function, which could possible call forward_to_tun */
        dev_handlers->read_from_dev();
}

/* this program loads a plugin and links it to a tun interface */
int main(int argc, char **argv) {
        int nfds;
        fd_set fdset;
        char tmpstr[2048];
        char c;
        char *cptr;
        void *dlhandle;
        int isv6 = 0;
        char *plugin = NULL;
        char *ifconfigopt = NULL;
        char *ipaddr = NULL;
        char display_help = 0;
        char wrong_arguments = 0;
        char *pluginopt = NULL;

        /* first try to get the plugin argument  */
        if (argc < 2 || argv[1][0] == '-') {
	      usage(argv[0], NULL);
	      exit(2);
        } else {
	      plugin = argv[1];
        }
        
        /* Open the plugin */
        snprintf(tmpstr, 2048, "./plugins/%s/lib%s.so", plugin, plugin);
        dlhandle = dlopen(tmpstr, RTLD_NOW);
        check(dlhandle != NULL, "dlopen() error: %s", dlerror());
        /* Get plugin_handlers, containing the plugin funciton handlers (struct dev_handlers_s) */
        dev_handlers = dlsym(dlhandle, "plugin_handlers");
        check(dev_handlers != NULL, "dlsym() error: %s", dlerror());
        
        /* optional arguments parsing */
        while ((c = getopt (argc, argv, "hp:v")) != -1) {
	      switch (c) {
		    case 'h':
			  display_help = 1;
			  break;
		    case 'p':
			  pluginopt = optarg;
			  break;
		    case 'v':
			  verbose = 1;
			  break;
		    case '?':
			  wrong_arguments = 1;
			  break;
	      }
        }

        /* call usage if needed */
#define N_STATIC_ARGS 2
        if (wrong_arguments || optind + N_STATIC_ARGS != argc) {
	      usage(argv[0], dev_handlers->help_string);
	      exit(2);
        }
        
        /* display help (-h option) */
        if(display_help) {
	      usage(argv[0], dev_handlers->help_string);
        }

        /* get ip configuration arguments */
        ipaddr = argv[optind + 1];
        cptr = ipaddr;
        
        /* detect if the IP is v4 or v6 */
        while(*cptr++) {
	      if(*cptr == ':') {
		    isv6 = 1;
		    break;
	      }
        }
        
        /* set ifconfig options and tun headers depending on the ip version */
        if(isv6) {
	      ifconfigopt = "inet6 add";
	      dbuffer_shadow[2] = 0x86;
	      dbuffer_shadow[3] = 0xDD;
        } else {
	      ifconfigopt = "";
	      dbuffer_shadow[2] = 0x08;
	      dbuffer_shadow[3] = 0x00;
        }

        /* Device initialization (done by the plugin) */
        dev_fd = dev_handlers->init_dev(tbuffer, dbuffer, DEV_MTU, pluginopt);
        check(dev_fd != -1,"init_dev() error");

        /* TUN interface creation */
        struct ifreq ifr;
        tun_fd = open(DEVTUN, O_RDWR);
        check(tun_fd != -1,"tun open() error");
        
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TUN;
        ret = ioctl(tun_fd, TUNSETIFF, (void *) &ifr);
        check(ret >= 0,"tun ioctl error");

        /* TUN attachement to an IP address */
        snprintf((char*)tmpstr, 64, "ifconfig %s %s %s up > /dev/null 2> /dev/null ",ifr.ifr_name,ifconfigopt,ipaddr);
        ret = system((char*)tmpstr);
        check(ret == 0, "system() ifconfig error");

        /* main loop */
        nfds = MAX(tun_fd, dev_fd) + 1;
        FD_ZERO(&fdset);
        message("PAN manager ready\n");
        while(1) {
	      FD_SET(tun_fd, &fdset);
	      FD_SET(dev_fd, &fdset);

	      /* wait until there is some data on either the tun or the device */
	      ret = select(nfds, &fdset, NULL, NULL, NULL);
	      check(ret != -1,"select() error");

	      /* process the input on tun or the device */
	      if(ret) {
		    if(FD_ISSET(tun_fd, &fdset)) {
			  tun_input();
		    }
		    if(FD_ISSET(dev_fd, &fdset)) {
			  dev_input();
		    }
	      }
        }
        return 0;
}

