/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */
 
/* api_client.c: handles connection with BrlApi */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
#include <alloca.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include "brlapi.h"
#include "api_common.h"

/* macros */
#define MIN(a, b)  (((a) < (b))? (a): (b)) 
#define MAX(a, b)  (((a) > (b))? (a): (b)) 

/* Some useful global variables */
static uint32_t brlx = 0;
static uint32_t brly = 0;
static int fd = -1; /* Descriptor of the socket connected to BrlApi */
static int deliver_mode = -1; /* How keypresses are delivered (codes | commands) */

/* what a key press does */
typedef enum { NONE=0, STROKE=1, MOD=2, UNMOD=3 } actions;
#define NBMAXMODS 32
static uint32_t mods_state;
/* key binding */
struct bind {
 brl_keycode_t code;
 uint32_t mods_state;
 union {
  char *cmd;
  int mod;
 } modif;
 actions action;
};
static struct bind *bindings = NULL;
static char *cmdnames;

/* brlapi_waitForAck */
/* Wait for an acknowledgement */
static int brlapi_waitForAck()
{
 brl_type_t type;
 int res;
 uint32_t code;
 while (1)
 {
  if ((res=brlapi_readPacket(fd,sizeof(code),&type, (unsigned char *) &code))<0)
  {
   if (res==-1) perror("waiting for ack");
   return -1;
  }
  switch (type)
  {
   case BRLPACKET_ACK: return 0;
   case BRLPACKET_ERROR:
   {
    fprintf(stderr,"BrlApi error %u !\n",ntohl(code));
    errno=0;
    return -1;
   }
   /* default is ignored, but logged */
   default:
   {
    fprintf(stderr,"(brlapi_waitForAck) Received unknown packet of type %u and size %d\n",type,res);
    break;
   }
  }
 }
}

/* Function : updateSettings */
/* Updates the content of a brlapi_settings_t structure according to */
/* another structure of th same type */
void updateSettings(brlapi_settings_t *s1, const brlapi_settings_t *s2)
{
  if (s2==NULL) return;
  if ((s2->authKey) && (*s2->authKey))
    s1->authKey = s2->authKey;
  if ((s2->hostName) && (*s2->hostName))
    s1->hostName = s2->hostName;
}

/* brlapi_initializeConnection */
/* Creates a socket to connect to BrlApi */
/* Returns the file descriptor, or -1 if an error occured */
/* Note: We return the file descriptor in case the client wants to */
/* communicate with the server without using our routines, but, if he uses us, */
/* He will not have to pass the fd later, since we keep a copy of it */
int brlapi_initializeConnection(const brlapi_settings_t *clientSettings, brlapi_settings_t *usedSettings)
{
 struct addrinfo *res,*cur;
 struct addrinfo hints;
 char *hostname = NULL;
 char *port = NULL;
 int authlength;
 int err;
 char auth[BRLAPI_MAXPACKETSIZE];

 brlapi_settings_t settings = { BRLAPI_AUTHNAME, ":" BRLAPI_SOCKETPORT };

 /* Here update settings with the parameters from misc sources (files, env...) */
 updateSettings(&settings, clientSettings);

 if (brlapi_loadAuthKey(settings.authKey,&authlength,(void *) &auth[0])<0)
 {
  fprintf(stderr,"unable to load authentication key: %s\n",settings.authKey);
  return -1;
 }

 if ((port = strchr(settings.hostName,':')))
 {
  if (port != settings.hostName)
  {
   hostname = alloca(port-settings.hostName+1);
   strncpy(hostname,settings.hostName,port-settings.hostName);
   hostname[port-settings.hostName]='\0';
  }
  port++;
 }
 else
 {
  port = BRLAPI_SOCKETPORT;
  hostname = settings.hostName;
 }

 memset(&hints, 0, sizeof(hints));
 hints.ai_family = PF_UNSPEC;
 hints.ai_socktype = SOCK_STREAM;

 err = getaddrinfo(hostname, port, &hints, &res);
 if (err)
 {
  fprintf(stderr,"getaddrinfo(%s) : %s\n",settings.hostName,gai_strerror(err));
  return -1;
 }
 for(cur = res; cur; cur = cur->ai_next)
 {
  fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
  if (fd<0)
   continue;
  if (connect(fd, cur->ai_addr, cur->ai_addrlen)<0)
  {
   close(fd);
   fd = -1;
   continue;
  }
  break;
 }
 freeaddrinfo(res);
 if (!cur)
 {
  fprintf(stderr,"Can't connect to BrlApi on %s !\n",settings.hostName);
  return -1;
 }
 if (brlapi_writePacket(fd, authlength, BRLPACKET_AUTHKEY,&auth[0])<0)
 {
  close(fd);
  fd = -1;
  return -1;
 }
 brlapi_waitForAck();
 if (usedSettings!=NULL) updateSettings(usedSettings, &settings); 
 return fd;
}

/* brlapi_closeConnection */
/* Cleanly close the socket */
void brlapi_closeConnection()
{
 brlapi_writePacket(fd, 0, BRLPACKET_BYE,NULL);
 brlapi_waitForAck();
 close(fd);
 fd = -1;
}

/* brlapi_getRaw */
/* Switch to Raw mode */
/* Returns 1 if successfull, 0 if unavailable for the moment, -1 on error */
int brlapi_getRaw()
{
 static const unsigned char paquet[sizeof(uint32_t)];
 int res;
 *((uint32_t *)paquet) = htonl(BRLRAW_MAGIC);
 if ((res=brlapi_writePacket(fd, sizeof(uint32_t), BRLPACKET_GETRAW, paquet))<0)
 {
  perror("getting raw");
  return -1;
 }
 return brlapi_waitForAck();
}

/* brlapi_leaveRaw */
/* Leave Raw mode */
/* Return -1 on error, 0 else */
int brlapi_leaveRaw()
{
 int res;
 if ((res=brlapi_writePacket(fd, 0, BRLPACKET_LEAVERAW, NULL))<0)
 {
  perror("leaving raw");
  return -1;
 }
 return brlapi_waitForAck();
}

/* brlapi_sendRaw */
/* Send a Raw Packet */
/* Return -1 on error, 0 else */
int brlapi_sendRaw(const unsigned char *buf, size_t size)
{
 int res;
 if ((res=brlapi_writePacket(fd, size, BRLPACKET_PACKET, buf))<0)
 {
  perror("writing raw packet");
  return -1;
 }
 return 0;
}

/* brlapi_recvRaw */
/* Get a Raw packet */
/* Returns its size, -1 on error, or -2 on EOF */
int brlapi_recvRaw(unsigned char *buf, size_t size)
{
 int res;
 brl_type_t type;
 while (1)
 {
  if ((res=brlapi_readPacket(fd, size, &type, buf))<0)
  {
   if (res==-1) perror("reading raw packet");
   return res;
  }
  if (type==BRLPACKET_PACKET) return res;
  if (type==BRLPACKET_ERROR)
  {
   fprintf(stderr,"BrlApi error %u !\n",ntohl(*((uint32_t*)buf)));
   errno=0;
   return -1;
  }
  /* other packets are ignored, but logged */
  fprintf(stderr,"(RecvRaw) Received unknown packet of type %u and size %d\n",type,res);
 }
 return 0;
}

/* Function : brlapi_getDriverId */
/* Identify the driver BrlApi loaded */
char *brlapi_getDriverId()
{
 brl_type_t type;
 int res;
 static char id[4] = { '\0', '\0', '\0', '\0' };
 uint32_t *code = (uint32_t *) &id[0];
 if (brlapi_writePacket(fd, 0, BRLPACKET_GETDRIVERID,NULL)<0)
 {
  perror("getting driver id");
  return NULL;
 }
 while (1)
 {
  if ((res=brlapi_readPacket(fd,sizeof(id),&type, (unsigned char *) &id[0]))<0)
  {
   if (res==-1) perror("Getting driver id");
   return NULL;
  }
  switch (type)
  {
   case BRLPACKET_GETDRIVERID: return &id[0];
   case BRLPACKET_ERROR:
   {
    fprintf(stderr,"BrlApi error %d while getting driver id\n",ntohl(*code));
    return NULL;
   }
   /* default is ignored, but logged */
   default:
   {
    fprintf(stderr,"(brlapi_getDriverId) Received unknown packet of type %u and size %d\n",type,res);
    break;
   }
  }
 }
}

/* Function : brlapi_getDriverName */
/* Name of the driver BrlApi loaded */
char *brlapi_getDriverName()
{
 brl_type_t type;
 int res;
 static char name[80]; /* should be enough */
 uint32_t *code = (uint32_t *) &name[0];
 if (brlapi_writePacket(fd, 0, BRLPACKET_GETDRIVERNAME,NULL)<0)
 {
  perror("getting driver name");
  return NULL;
 }
 while (1)
 {
  if ((res=brlapi_readPacket(fd,sizeof(name),&type, (unsigned char *) &name[0]))<0)
  {
   if (res==-1) perror("Getting driver name");
   return NULL;
  }
  switch (type)
  {
   case BRLPACKET_GETDRIVERNAME: return &name[0];
   case BRLPACKET_ERROR:
   {
    fprintf(stderr,"BrlApi error %d while getting driver name\n",ntohl(*code));
    return NULL;
   }
   /* default is ignored, but logged */
   default:
   {
    fprintf(stderr,"(GetDriverName) Received unknown packet of type %u and size %d\n",type,res);
    break;
   }
  }
 }
}

/* Function : brlapi_getDisplaySize */
/* Returns the size of the braille display */
int brlapi_getDisplaySize(int *x, int *y)
{
 brl_type_t type;
 uint32_t DisplaySize[2];
 int res;
 
 if (brlx*brly)
 {
  *x = brlx;
  *y = brly;
  return 0;
 }
 if (brlapi_writePacket(fd, 0, BRLPACKET_GETDISPLAYSIZE,NULL)<0)
 {
  perror("Getting braille display size");
  return -1;
 }
 while (1)
 {
  if ((res=brlapi_readPacket(fd,sizeof(DisplaySize),&type,(unsigned char *) &DisplaySize[0]))<0)
  {
   if (res==-1) perror("getting display size");
   return -1;
  }
  switch (type)
  {
   case BRLPACKET_GETDISPLAYSIZE: 
   {
    brlx = ntohl(DisplaySize[0]);
    brly = ntohl(DisplaySize[1]);
    *x = brlx;
    *y = brly;
    return 0;
   }
   case BRLPACKET_ERROR:
   {
    fprintf(stderr,"BrlApi error %d while getting display size\n",ntohl(DisplaySize[0]));
    return -1;
   }
   /* default is ignored, but logged */
   default:
   {
    fprintf(stderr,"(brlapi_getDisplaySize) Received unknown packet of type %u and size %d\n",type,res);
    break;
   }
  }
 }
}

/* Function : brlapi_getControllingTty */
/* Returns the number of the caller's controlling terminal */
/* -1 if error or unknown */
int brlapi_getControllingTty()
{
 int tty;
 FILE *f;
 int i = (int) NULL;
 char c[100];
 f = fopen("/proc/self/stat","r");
 if (f==NULL)
 {
  perror("open");
  return -1;
 }
 if (fscanf(f,"%d %s %c %d %d %d %d",&i,c,c,&i,&i,&i,&tty)==-1)
 {
  perror("fscanf");
  return -1;
 }
 fclose(f);
 return tty & 0xff;
}

/* Function : brlapi_getTty */
/* Takes control of a tty */
/* If tty>0, we take control of the specified tty */
/* if tty==0, we try to determine in wich tty we are running and */
/* take control of this one */
/* how tells whether we want ReadKey to return keycodes or brltty commands */
/* client tells which file the library will open for name constants for ReadKeyCmd */
/* Returns 0 if ok, -1 if an error occured */
struct def {
 brl_keycode_t code;
 char *name;
};
int brlapi_getTty(uint32_t tty, uint32_t how, brlapi_keybinding_t *keybinding)
{
 int truetty = -1;
 uint32_t uints[2];
 char *home;
 char *driverid;
 char *fichname;
 int fdbinds,fddefs;
 struct stat statbinds,statdefs;
 char *keybinds,*keydefs;
 int res;
 char *c,*d;
 struct def *defs,*deftmp;
 int nbdefs,idefs;
 int nbbinds,cmdsize; // number of final bindings, and size of final command names
 int modunmod;
 unsigned long mod;
 char *cmdnames_tmp;
 struct bind *bindtmp;
 uint32_t mods_state;
 
 /* Check if "how" is valid */
 if ((how!=BRLKEYCODES) && (how!=BRLCOMMANDS)) return -1;

 /* Get dimensions of braille display */
 if (brlapi_getDisplaySize(&brlx,&brly)<0) return -1; /* failed, we stop here */
 
 /* Then determine of which tty to take control */
 if (tty==0) truetty = brlapi_getControllingTty(); else truetty = tty;
 if (truetty == -1) return -1;
 
 /* OK, Now we know where we are, so get the effective control of the terminal! */
 uints[0] = htonl(truetty); 
 uints[1] = htonl(how);
 if (brlapi_writePacket(fd,sizeof(uints),BRLPACKET_GETTTY,(unsigned char *) &uints[0])<0) return -1;
 if (brlapi_waitForAck()!=1) return -1;
 
 deliver_mode = how;

 // ok, grabbed it, see if a key binding is required
 if (!keybinding) return 0;
 if (!keybinding->client) return 0;
 if (!(*keybinding->client)) return 0;

 res = -1;
 // first fetch pieces of information
 home = getenv("HOME");
 if (!home) goto gettty_end;
 driverid = brlapi_getDriverId();
 if (!driverid) goto gettty_end;

 // first look for key bindings
 fichname = malloc(strlen(home)+strlen("/" BRLAPI_HOMEKEYDIR "/")+strlen(keybinding->client)
   +strlen("-")+strlen(driverid)+strlen(BRLAPI_HOMEKEYEXT)+1);
 if (!fichname) goto gettty_end;
 strcpy(fichname,home);
 strcat(fichname,"/" BRLAPI_HOMEKEYDIR "/");
 strcat(fichname,keybinding->client);
 strcat(fichname,"-");
 strcat(fichname,driverid);
 strcat(fichname,BRLAPI_HOMEKEYEXT);
 fdbinds = open(fichname,O_RDONLY);
 free(fichname);
 if (fdbinds<0) goto gettty_end;
 if (fstat(fdbinds,&statbinds)!=0) goto gettty_endbindsfd;
 keybinds = (char *) mmap(NULL, statbinds.st_size+1, PROT_READ|PROT_WRITE, MAP_PRIVATE, fdbinds, 0);
 if (!keybinds) goto gettty_endbindsfd;
 keybinds[statbinds.st_size]='\0';

 // now look for key definitions
 fichname = malloc(strlen(BRLAPI_ETCDIR "/" BRLAPI_ETCKEYFILE "-")
   +strlen(driverid)+strlen(".h")+1);
 strcpy(fichname,BRLAPI_ETCDIR "/" BRLAPI_ETCKEYFILE "-");
 strcat(fichname,driverid);
 strcat(fichname,".h");
 fddefs = open(fichname,O_RDONLY);
 free(fichname);
 if (fddefs<0) goto gettty_endbindsmmap;
 if (fstat(fddefs,&statdefs)!=0) goto gettty_enddefsfd;
 keydefs = (char *) mmap(NULL, statdefs.st_size+1, PROT_READ|PROT_WRITE, MAP_PRIVATE, fddefs, 0);
 if (!keydefs) goto gettty_enddefsfd;
 keydefs[statdefs.st_size]='\0';

 // ok, these files were found, first parse key defs to record where they are
 // we used MAP_PRIVATE, so we can modify it like crazy
 c = keydefs;
 nbdefs = 16;	// arbitrarily enough
 idefs = 0;
 defs = (struct def *) malloc(nbdefs * sizeof(struct def));
 if (!defs) goto gettty_enddefsmmap;

 while(*c) {
 // parse one definition, ie a line which looks like
 // #define keyA1 0x00 anythingUnImportant
 // be warned that code MUST be an integer, not a formula

 // first #define
  if (strncmp(c,"#define",strlen("#define"))) goto gettty_skiplndefs;
  c+=strlen("#define");
  if (*c && *c!=' ' && *c!='\t') goto gettty_skiplndefs;

 // skip blank
  while (*c && *c!='\n' && (*c==' ' || *c=='\t')) c++;
  if (!*c) break;
  if (*c == '\n') { c++; continue; } // ?! line unfinished, give up

 // key name, skip it
  defs[idefs].name = c;
  while (*c && *c!=' ' && *c!='\t' && *c != '\n') c++;
  if (!*c) break;
  if (*c == '\n') { c++; continue; } // ?! line unfinished, give up
  *c++ = '\0'; // close key name, no modification in file thanks to MAP_PRIVATE

 // skip blank
  while (*c && *c!='\n' && (*c==' ' || *c=='\t')) c++;
  if (!*c) break;
  if (*c == '\n') { c++; continue; } // ?! line unfinished, give up

 // code value
  defs[idefs].code = strtoul(c, &d, 0);
  if (c==d || (*d!=' ' && *d!='\t' && *d!='\n' && *d!='\0')) goto gettty_skiplndefs; // invalid #define

 // great ! at last we got one, increment i, and check for overflow
  if (++idefs == nbdefs) {
   nbdefs <<= 1;
   deftmp=realloc(defs, nbdefs * sizeof(struct def));
   if (!deftmp) goto gettty_enddefs;
  }

 gettty_skiplndefs:
  while (*c && *c!='\n') c++;
  c++;
 }
 defs[idefs].name = NULL;
 nbdefs = idefs;

 // OK, we have every key definition, let's look at user's binding
 // they look like
 // keyB5 4 mod 1 anythingUnImportant
 // keyB6 unmod 1 anythingUnImportant
 // keyA1 1 4 BLIP stillUnImportant
 // keyA1 BLOP alwaysUnImportant

 // first pass to count how many definitions are there
 c = keybinds;
 nbbinds = 0;
 cmdsize = 0;
 while(*c) {
  while (*c && (*c==' ' || *c=='\t' || *c=='\n')) c++;
  if (!*c) break;
  for (deftmp = defs; deftmp->name; deftmp++)
   if (!strncmp(c, deftmp->name, strlen(deftmp->name))
    && c[strlen(deftmp->name)]
    && (c[strlen(deftmp->name)]==' ' || c[strlen(deftmp->name)]=='\t'))
    break;
  if (!deftmp->name) goto gettty_skiplnbinds1;	// not found
  c+=strlen(deftmp->name);

 // check that there's really a command name
  while (*c && (*c==' ' || *c=='\t') && *c!='\n') c++;
  if (!*c) break;
  if (*c == '\n') { c++; continue; }

  do strtoul(c,&d,0); while (d!=c && (*d==' ' || *d=='\t') && (c=d+1));

  while (*c && (*c==' ' || *c=='\t') && *c!='\n') c++;
  if (!*c) break;
  if (*c == '\n') { c++; continue; }

  if ((!strncmp(c,"mod",3  ) && c[3] && (c[3]==' ' || c[3]=='\t') && (c+=4))
   || (!strncmp(c,"unmod",5) && c[5] && (c[5]==' ' || c[5]=='\t') && (c+=6)))
  {
   strtoul(c,&d,0);
   if (c!=d) nbbinds++;
   goto gettty_skiplnbinds1;
  }

  // this one will be ok, count it
  nbbinds++;

  while (*c && *c!=' ' && *c!='\t' && *c!='\n') { c++; cmdsize++; }
  cmdsize++; // for final \0
  if (!*c) break;

 gettty_skiplnbinds1:
  while (*c && *c!='\n') c++;
  c++;
 }
 if (!nbbinds) goto gettty_enddefs;

 // we've found nbbinds definitions
 // if there were already, get rid of them
 // note that if GetTty fails, old definition still apply, and ReadKeyCmd can
 // still be called
 if (bindings) { free(bindings); free(cmdnames); }
 bindings = (struct bind *) malloc((nbbinds+1) * sizeof(struct bind));
 if (!bindings) goto gettty_enddefs;
 cmdnames = (char *) malloc(cmdsize);
 if (!cmdnames) { free(bindings); goto gettty_enddefs; }
 cmdnames_tmp = cmdnames;
 bindings[nbbinds].action = NONE;

 // parse again, but record this time
 c = keybinds;
 while(*c) {
 // cut & paste of what is just 20 lines upper
  while (*c && (*c==' ' || *c=='\t' || *c=='\n')) c++;
  if (!*c) break;
  for (deftmp = defs; deftmp->name; deftmp++)
   if (!strncmp(c, deftmp->name, strlen(deftmp->name))
    && c[strlen(deftmp->name)]
    && (c[strlen(deftmp->name)]==' ' || c[strlen(deftmp->name)]=='\t'))
     break;
  if (!deftmp->name) goto gettty_skiplnbinds2;	// not found
  c+=strlen(deftmp->name);

  // check that there's really a command name
  while (*c && (*c==' ' || *c=='\t') && *c!='\n') c++;
  if (!*c) break;
  if (*c == '\n') { c++; continue; }

  mods_state=0;
  do mod=strtoul(c,&d,0); while (d!=c && (*d==' ' || *d=='\t') && (c=d+1) && (mods_state|=1<<mod,1));

  while (*c && (*c==' ' || *c=='\t') && *c!='\n') c++;
  if (!*c) break;
  if (*c == '\n') { c++; continue; }

  if ((!strncmp(c,"mod",3  ) && c[3] && (c[3]==' ' || c[3]=='\t') && (c+=4) && (modunmod=MOD))
   || (!strncmp(c,"unmod",5) && c[5] && (c[5]==' ' || c[5]=='\t') && (c+=6) && (modunmod=UNMOD)))
  {
   mod=strtoul(c,&d,0);
   if (c!=d) {
    nbbinds--;
    bindings[nbbinds].modif.mod = mod;
    bindings[nbbinds].code = deftmp->code;
    bindings[nbbinds].action = modunmod;
    bindings[nbbinds].mods_state = mods_state;
    //fprintf(stderr,"bound 0x%x(0x%x) to %smodify %u\n", bindings[nbbinds].code, bindings[nbbinds].mods_state,modunmod==MOD?"":"un", bindings[nbbinds].mod);
   }
   goto gettty_skiplnbinds2;
  }
 // we point on the command name
 // we can record it

  nbbinds--;
  bindings[nbbinds].modif.cmd = cmdnames_tmp;
  bindings[nbbinds].code = deftmp->code;
  bindings[nbbinds].action = STROKE;
  bindings[nbbinds].mods_state = mods_state;

 // and close command name
  while (*c && *c!=' ' && *c!='\t' && *c!='\n') *cmdnames_tmp++ = *c++;
  *cmdnames_tmp++='\0';
  if (!*c) break;
  if (*c == '\n')
  {
   *c++='\0';
   //fprintf(stderr,"bound 0x%x(0x%x) to \"%s\"\n", bindings[nbbinds].code, bindings[nbbinds].mods_state, bindings[nbbinds].cmd);
   continue;
  }
  *c++='\0';
  //fprintf(stderr,"bound 0x%x(0x%x) to \"%s\"\n", bindings[nbbinds].code, bindings[nbbinds].mods_state, bindings[nbbinds].cmd);

 gettty_skiplnbinds2:
  while (*c && *c!='\n') c++;
  c++;
 }

 // great ! it worked, so unmask only those keys
 if (brlapi_ignoreKeys(0U, ~0U)!=1) goto gettty_enddefs;
 for(bindtmp = bindings; bindtmp->action; bindtmp++)
  if (brlapi_unignoreKeys(bindtmp->code, bindtmp->code)!=1) goto gettty_enddefs;

 res = 0;
 gettty_enddefs:
 free(defs);
 gettty_enddefsmmap:
 munmap(keydefs,statdefs.st_size+1);
 gettty_enddefsfd:
 close(fddefs);
 gettty_endbindsmmap:
 munmap(keybinds,statbinds.st_size+1);
 gettty_endbindsfd:
 close(fdbinds);
 gettty_end:
 if (res != 0 ) brlapi_leaveTty();
 return res;
}

/* Function : brlapi_leaveTty */
/* Gives back control of our tty to brltty */
int brlapi_leaveTty()
{
 brlx = 0; brly = 0; deliver_mode = -1;
 if (brlapi_writePacket(fd,0,BRLPACKET_LEAVETTY,NULL)<0) return -1;
 return brlapi_waitForAck();
}

/* Function : brlapi_writeBrl */
/* Writes a string to the braille display */
/* If the string is too long, it is cut. If it's to short, some spaces are added */
int brlapi_writeBrl(uint32_t cursor,char *str)
{
 static unsigned char disp[sizeof(uint32_t)+256];
 uint32_t *csr = (uint32_t *) &disp[0];
 unsigned char *s = &disp[sizeof(uint32_t)];
 uint32_t tmp = brlx * brly, min, i;
 if ((tmp == 0) || (tmp > 256)) return -1;
 *csr = htonl(cursor);
 min = MIN( strlen(str), tmp);
 strncpy(s,str,min);
 for (i = min; i<tmp; i++) *(s+i) = ' '; 
 if (brlapi_writePacket(fd,sizeof(uint32_t)+tmp,BRLPACKET_WRITE,disp)<0) return -1;
 return brlapi_waitForAck();
} 

/* Function : brlapi_readKey */
/* Reads a key from the braille keyboard */
/* If the read is successfull, the keycode is put in code, and 0 is returned */
/* If an error occurs, we return -1 and *code is undefined */
int brlapi_readKey(brl_keycode_t *code)
{
 int res;
 brl_type_t type;

 if (deliver_mode!=BRLKEYCODES) return -1;
  while (1)
 {
  if ((res=brlapi_readPacket(fd, sizeof(*code), &type, (unsigned char *) code))<0)
  {
   if (res==-1) perror("reading braille key");
   return -1;
  }
  if (type==BRLPACKET_KEY)
  {
   *code = ntohl(*code);
   return 0;
  }
  if (type==BRLPACKET_ERROR)
  {
   fprintf(stderr,"BrlApi error %u !\n",ntohl(*code));
   errno=0;
   return -1;
  }
  /* other packets are ignored, but logged */
  fprintf(stderr,"(ReadKey) Received unknown packet of type %u and size %d\n",type,res);
 }
 return 0;
}

/* Function : brlapi_readCommand */
/* Reads a command from the braille keyboard */
/* If the read is successfull, the command is put in code, and 0 is returned */
/* If an error occurs, we return -1 and *code is undefined */
int brlapi_readCommand(brl_keycode_t *code)
{
 int res;
 brl_type_t type;

 if (deliver_mode!=BRLCOMMANDS) return -1;
 while (1)
 {
  if ((res=brlapi_readPacket(fd, sizeof(*code), &type, (unsigned char *) code))<0)
  {
   if (res==-1) perror("reading braille key");
   return -1;
  }
  if (type==BRLPACKET_COMMAND)
  {
   *code = ntohl(*code);
   return 0;
  }
  if (type==BRLPACKET_ERROR)
  {
   fprintf(stderr,"BrlApi error %u !\n",ntohl(*code));
   errno=0;
   return -1;
  }
  /* other packets are ignored, but logged */
  fprintf(stderr,"(ReadKey) Received unknown packet of type %u and size %d\n",type,res);
 }
 return 0;
}

/* Function : brlapi_readBinding */
/* Reads a key from the braille keyboard */
/* If the read is successfull, a pointer on a string is returned : this is */
/* the string defined in $HOME/.brlkeys/appli-xy.kbd for the key which was */
/* read, else NULL is returned */
const char *brlapi_readBinding(void)
{
 brl_keycode_t code;
 struct bind *bindtmp;
 if (!bindings) return NULL;
 while (1) {
  if (brlapi_readKey(&code)!=0)
   return NULL;
  for(bindtmp = bindings; bindtmp->action; bindtmp++)
   if (bindtmp->code == code && bindtmp->mods_state == mods_state) {
    switch (bindtmp->action) {
     case STROKE: return bindtmp->modif.cmd;
     case MOD: mods_state|=1<<bindtmp->modif.mod; break;
     case UNMOD: mods_state&=~(1<<bindtmp->modif.mod); break;
     default: break;
    }
   mods_state=0;
   }
  // if not found, drop it
 }
}

/* Function : Mask_Unmask */
/* Common tasks for masking and unmasking keys */
/* what = 0 for masing, !0 for unmasking */
static int Mask_Unmask(int what, brl_keycode_t x, brl_keycode_t y)
{
 brl_keycode_t ints[2];

 ints[0] = htonl(x);
 ints[1] = htonl(y);

 if (brlapi_writePacket(fd,sizeof(ints),(what ? BRLPACKET_UNMASKKEYS : BRLPACKET_MASKKEYS),(unsigned char *) &ints[0])<0) return -1;
 return brlapi_waitForAck();
}

/* Function : brlapi_ignoreKeys */
int brlapi_ignoreKeys(brl_keycode_t x, brl_keycode_t y)
{
 return Mask_Unmask(0,x,y);
}

/* Function : brlapi_unignoreKeys */
int brlapi_unignoreKeys(brl_keycode_t x, brl_keycode_t y)
{
 return Mask_Unmask(!0,x,y);
}
