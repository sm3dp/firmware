#ifndef __GCODEPLAYER_H
#define __GCODEPLAYER_H 			   
#include "ff.h"	   
#include "sys.h"

typedef enum {
   LS_SerialPrint = 0,
   LS_Count,
   LS_GetFilename
 }LsAction;

typedef struct {
 bool saving;
 bool sdprinting;	
 bool cardOK;
 bool cardrelase;
 bool pause;
  //int16_t n;
 unsigned long autostart_atmillis;
 unsigned long  sdpos; 
 bool autostart_stilltocheck;
 LsAction lsAction; //stored for recursion.
 u8 filename[64];
 FIL fgcode;
}CardReader;

extern CardReader card;
extern u8 *pname;

void card_ls(void);
void card_initsd(void);
void card_release(void);						
u8 card_openFile(char* fname,bool read);
void card_startFileprint(void);
void card_pauseSDPrint(void);
void card_setIndex(long index);	  
void card_getStatus(void);  
void card_closefile(void);
void card_removeFile(char* fname);

void card_printingHasFinished(void);
void card_checkautostart(void);
void card_write_command(char *buf);

bool card_eof(void);
int16_t card_get(void);

int16_t file_read(void);

#endif



























