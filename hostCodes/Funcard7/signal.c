
#include "scard.h"
#include <signal.h>

extern utun tunnel;
extern SCARDCONTEXT hContext;
extern SCARDHANDLE hCard;

int signals[6] = {SIGINT,SIGILL,SIGFPE,SIGSEGV,SIGTERM,SIGABRT } ;

static void  test_and_close(int sig_ptr)
{
  switch(sig_ptr)
  {
  case SIGINT:
  case SIGTERM:
      fprintf(stderr, "Recv signal %d\n", sig_ptr);
      fflush(stderr);
      close_tunnel(&tunnel);
      release_terminal(&hContext,&hCard);
      break;
  default:
      break;
  }
  exit(0);
}

void init_signals(void)
{
  int i;
  for (i=0 ; i < 6 ; i++)
    signal( signals[i], test_and_close) ;
}


