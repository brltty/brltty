#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "variolow.h"
#include "brl.h"
#include "brlconf.h"
#include "../driver.h"


static unsigned char lastbuff[40];

void
identbrl (void)
{
		/*	Do nothing .. dumdidumm */ 
	printf("HT Protocol driver\n");
}

void 
initbrl (brldim *brl, const char *dev)
{
		/*	Seems to signal en error */ 
	brl->x=-1;
	if(!varioinit((char*)dev)) {
			/*	Theese are pretty static */ 
		brl->x=40;
		brl->y=1;
		if(!(brl->disp=malloc(brl->x*brl->y))) {
			perror(dev);
			brl->x=-1;
		}
	}
}

void 
closebrl (brldim *brl)
{
	varioclose();
	free(brl->disp);
}

void 
writebrl (brldim *brl)
{
	char		outbuff[40];
	int			i;

		/*	Idiot check one */ 
	if(!brl||!brl->disp) {
		return;
	}
		/*	Only display something if the data actually differs, this 
		 *	could most likely cause some problems in redraw situations etc
		 *	but since the darn thing wants to redraw quite frequently otherwise 
		 *	this still makes a better lookin result */ 
	for(i=0;i<40;i++) {
		if(lastbuff[i]!=brl->disp[i]) {
			memcpy(lastbuff,brl->disp,40*sizeof(char));
				/*	Redefine the given dot-pattern to match ours */
			variotranslate(brl->disp, outbuff, 40);
			variodisplay(outbuff);
			break;
		}
	}
}

void
setbrlstat (const unsigned char *st)
{
	/*	Dumdidummm */ 
}

int 
readbrl (int type)
{
	static int shift_button_down=0;
	int	decoded=EOF;
	int c;
		/*	Since we are nonblocking this will happen quite a lot */ 
	if((c=varioget())==-1) {
		return EOF;
	}

	switch(c) {
			/*	Should be handled in a better manner, this will do for now tho */ 
		case VARIO_DISPLAY_DATA_ACK:
			decoded = CMD_NOOP;
		break;
			/*	Always ignore press codes */ 
		case VARIO_PUSHBUTTON_PRESS_1:
		case VARIO_PUSHBUTTON_PRESS_2:
		case VARIO_PUSHBUTTON_PRESS_4:
		case VARIO_PUSHBUTTON_PRESS_5:
		case VARIO_PUSHBUTTON_PRESS_6:
			decoded = CMD_NOOP;
		break;
			/*	We define the button 3 as shift ... */ 
		case VARIO_PUSHBUTTON_PRESS_3:
			shift_button_down=1;
		break;	

		case VARIO_PUSHBUTTON_RELEASE_1:
			decoded = (shift_button_down?CMD_TOP_LEFT:CMD_LNUP);
		break;
		case VARIO_PUSHBUTTON_RELEASE_2:
			decoded = (shift_button_down?CMD_SKPIDLNS:CMD_FWINLT);
		break;
		case VARIO_PUSHBUTTON_RELEASE_3:
			shift_button_down=0;
		break;
		case VARIO_PUSHBUTTON_RELEASE_4:
			decoded = (shift_button_down?CMD_BOT_LEFT:CMD_LNDN);
		break;
		case VARIO_PUSHBUTTON_RELEASE_5:
			decoded = (shift_button_down?CMD_CSRTRK:CMD_FWINRT);
		break;
		case VARIO_PUSHBUTTON_RELEASE_6:
			decoded = (shift_button_down?CMD_CONFMENU:CMD_HOME);
		break;
		default:
			if(c>=VARIO_CURSOR_BASE&&c<=VARIO_CURSOR_BASE+VARIO_CURSOR_COUNT) {
				decoded = CR_ROUTEOFFSET+c-VARIO_CURSOR_BASE;
			}
		break;
	}
	return decoded;
}
