// vblow.h - Definition file for vblow.c: don't touch it!
#define KEY_UP 0x1
#define KEY_LEFT 0x2
#define KEY_RIGHT 0x4
#define KEY_DOWN 0x8
#define KEY_ATTRIBUTES 0x10
#define KEY_CURSOR 0x20
#define KEY_HOME 0x40
#define KEY_MENU 0x80

typedef struct {
  unsigned char bigbuttons;
  char routingkey : 7;
  char keypressed : 1;
} vbButtons;

int vbinit();
void vbtranslate(char *,char *,int);
void BrButtons(vbButtons *);
void vbdisplay(char *);

