
all:	brl.o
IFLAGS = -I..

LD = ld

variolow.o:	variolow.c variolow.h
	$(CC) $(CFLAGS) $(IFLAGS) -c variolow.c

vario.o:	vario.c
	$(CC) $(CFLAGS) $(IFLAGS) -c vario.c
	
brl.o:	vario.o	variolow.o
	$(LD) -r -o brl.o variolow.o vario.o 

clean:	
	rm -f vario.o brl.o variolow.o test 
