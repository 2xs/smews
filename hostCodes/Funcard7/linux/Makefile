
ROOTDIR=.

CFLAGS	= -g -Wall -D LINUX
SRCDIR  = $(ROOTDIR)/..
OUTDIR  = $(ROOTDIR)
LIBS    = -lpcsclite

BINARIES= scard
OBJECTS	= $(addsuffix .o,scard )
	  
default: 
	-make allnocall ROOTDIR=`pwd`
	
all: 
	-make allnocall ROOTDIR=`pwd`

allnocall: $(OBJECTS) $(BINARIES) 

scard: scard.o apdu.o terminal.o tunnel.o signal.o utils.o utun.o
	cd $(OUTDIR) && $(CC) $(CFLAGS) -o ../$@ $^ $(LIBS)
	
scard.o: 
	$(CC) $(CFLAGS) -c $< $(INCDIR) $(SRCDIR)/scard.c -o $(OUTDIR)/scard.o
apdu.o: 
	$(CC) $(CFLAGS) -c $< $(INCDIR) $(SRCDIR)/apdu.c -o $(OUTDIR)/apdu.o
terminal.o: 
	$(CC) $(CFLAGS) -c $< $(INCDIR) $(SRCDIR)/terminal.c -o $(OUTDIR)/terminal.o
tunnel.o: 
	$(CC) $(CFLAGS) -c $< $(INCDIR) $(SRCDIR)/tunnel.c -o $(OUTDIR)/tunnel.o
signal.o: 
	$(CC) $(CFLAGS) -c $< $(INCDIR) $(SRCDIR)/signal.c -o $(OUTDIR)/signal.o
utils.o: 
	$(CC) $(CFLAGS) -c $< $(INCDIR) $(SRCDIR)/utils.c -o $(OUTDIR)/utils.o
utun.o: 
	$(CC) $(CFLAGS) -c $< $(INCDIR) $(SRCDIR)/utun/nixtun.c -o $(OUTDIR)/utun.o

clean: 
	rm -f *.o scard 


