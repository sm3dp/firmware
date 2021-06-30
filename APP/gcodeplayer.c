#include "gcodeplayer.h"
#include "sm_firmware.h"
#include "sdio_sdcard.h"
#include "usart.h"
#include "exfuns.h"
#include "malloc.h"
#include "string.h"
#include "language.h"
#include "temperature.h"
#include "stepper.h"
#include "24cxx.h"
#include "sm_plus.h"
#include "sd_spi.h"
#include "mmc_sd.h"

#include "App_Timer.h"
#include "App_Language.h"


#ifdef SDSUPPORT
extern uint32_t CurrentFileSize;
extern USR_TIMER usr_timer;
void gcode_ls(u8 *path)
{
	u8 res;
	u8 *fn;   /* This function is assuming non-Unicode cfg. */
	#if _USE_LFN
	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize);
	#endif

	res = f_opendir(&dir, (const TCHAR *)path);
	if(res == FR_OK)
	{

		while(1)
		{
			res = f_readdir(&dir, &fileinfo);
			if(res != FR_OK || fileinfo.fname[0] == 0) break;
			//if (fileinfo.fname[0] == '.') continue;
			#if _USE_LFN
			fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);
			#else
			fn = (u8 *)(fileinfo.fname);
			#endif
			res = f_typetell(fn);
			if((res & 0XF0) == 0X60)
			{
				USR_UsrLog("%s",  fn);
			}
		}
	}
	myfree(SRAMIN, fileinfo.lfname);
	//  return res;
}

void card_ls(void)
{
	if(card.lsAction == LS_Count)
		// nrFiles=0;
		card.lsAction = LS_SerialPrint;
	//gcode_ls("0:\\GCODE");
	gcode_ls("0:");
}

void card_initsd(void)
{
	card.cardOK = false;
	SERIAL_ECHO_START;
	if(SD_Initialize())
	{
		USR_UsrLog(MSG_SD_INIT_FAIL);
	}
	else
	{
		card.cardOK = true;
		card.cardrelase = 0;
		if(f_mount(fs[0], "0:", 1))
		{
			USR_UsrLog("f_mount spi sdcard error");
		}
		else
			USR_UsrLog(MSG_SD_CARD_OK);
	}
}
void card_release(void)
{
	uint8_t temp[32];
	card.sdprinting = false;
	card.cardOK = false;
	card.cardrelase = 1;
	if(PrintInfo.printsd != 0)
	{
		manual_stop_diy();
	}
	f_mount(NULL, "0:", 1);
}


u8 *pname;
u8 card_openFile(char *fname, bool read)
{
	u8 res;
	if(!card.cardOK)
		return 1;
	// file.close();
	// printf("--");
	card.sdprinting = false;

	if(read)
	{
		memset(pname, 0, sizeof(pname));
		//strcpy((char *)pname, "0:/GCODE/");
		strcpy((char*)pname,"0:");
		strcat((char *)pname, (const char *)fname);
		res = f_open(&card.fgcode, (const TCHAR *)pname, FA_READ);
		if(res == FR_OK)
		{
			USR_UsrLog(MSG_SD_FILE_OPENED);
			USR_UsrLog(fname);
			USR_UsrLog(MSG_SD_SIZE);
			USR_UsrLog("%ld", f_size(&card.fgcode));
			strcpy(card.filename, fname);
			CurrentFileSize = f_size(&card.fgcode);
			printf(MSG_SD_FILE_SELECTED);
			return 0;
		}
		else
		{
			USR_UsrLog(MSG_SD_OPEN_FILE_FAIL);
			USR_UsrLog(fname);
			USR_UsrLog(".");
			return 1;
		}
	}
	else
	{
		res = f_open(&card.fgcode, (const TCHAR *)fname, FA_WRITE | FA_OPEN_ALWAYS);
		if(res != FR_OK)
		{
			USR_UsrLog(MSG_SD_OPEN_FILE_FAIL);
			USR_UsrLog(fname);
			USR_UsrLog(".");
			return 1;
		}
		else
		{
			card.saving = true;
			USR_UsrLog(MSG_SD_WRITE_TO_FILE);
			USR_UsrLog(fname);
			return 0;
		}

	}
}

void card_removeFile(char *fname)
{
	u8 res;
	res = f_unlink(fname);
	if(res == FR_OK)
	{
		USR_UsrLog("File deleted:");
		USR_UsrLog(fname);
	}
	else
	{
		USR_UsrLog("Deletion failed, File: ");
		USR_UsrLog(fname);
		USR_UsrLog(".\n");
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void card_startFileprint(void)
{
	if(card.cardOK)
	{
		card.sdprinting = true;		
		if((PrintInfo.printsd == 0) || (PrintInfo.printsd == 2))
		{
			PrintInfo.printsd = 1;
			strcpy(PrintInfo.printfile, card.filename);	
			PrintInfo.filesize = f_size(&card.fgcode);
			CurrentFileSize = f_size(&card.fgcode);
			PrintInfo.printper = 0;
			PrintInfo.printtime = 0;

			/*Printer Timer Start*/
			usr_timer.sec = 0;
			usr_timer.min = 0;
			usr_timer.hour = 0;
			usr_timer.date = 0;
			usr_timer.mounth = 0;

			ReadChar = 0;
			#ifdef REPRINTSUPPORT

			//save file name
			u8 len;
			len = strlen(pname);
			len++;
			AT24CXX_WriteOneByte(E_FILENAME, len);	//
			AT24CXX_Write(E_FILENAME + 1, pname, len);	//
			USR_UsrLog("save file lenth is %d name is %s", len, pname);
			#endif
		}
	}

}

void card_pauseSDPrint(void)
{
	if(card.sdprinting)
	{
		card.sdprinting = false;
	}
	Z_EN(1);
}

void card_setIndex(long index)
{
	//card.sdpos = index;
	f_lseek(&card.fgcode, index);
}

void card_getStatus(void)
{
	if(card.cardOK)
	{
		USR_UsrLog(MSG_SD_PRINTING_BYTE);
		USR_UsrLog("%d", f_tell(&card.fgcode));
		USR_UsrLog("/");
		USR_UsrLog("%d", f_size(&card.fgcode));
	}
	else
	{
		USR_UsrLog(MSG_SD_NOT_PRINTING);
	}
}

void card_closefile(void)
{
	f_close(&card.fgcode);
	card.saving = false;
}

void card_write_command(char *buf)
{
}

void card_checkautostart()
{
	if(card.cardrelase == 0)
	{
		if(!card.cardOK)
		{
			card_initsd();
			if(!card.cardOK) //fail
				return;
		}
	}

}
void card_printingHasFinished(void)
{
	st_synchronize();
	quickStop();
	card_closefile();
	starttime = 0;
	card.sdprinting = false;
	PrintInfo.printsd = 0;		//¥Ú”°ÕÍ≥…
	if(SD_FINISHED_STEPPERRELEASE)
	{
		//  finishAndDisableSteppers();
		enquecommand(PSTR(SD_FINISHED_RELEASECOMMAND));
	}
	autotempShutdown();
	#ifdef REPRINTSUPPORT
	AT24CXX_WriteOneByte(E_SDPRINT, false);
	USR_UsrLog("sdcard print has finished;save sd print flase flg");
	#endif

}

bool card_eof(void)
{
	return f_eof(&card.fgcode);
}

int16_t card_get(void)
{
	return  file_read();
}

int16_t file_read(void)
{
	u8 buffer[MAX_CMD_SIZE];
	u8 res;
	UINT br;

	res = f_read(&card.fgcode, buffer, 1, &br);

	ReadChar++;
	if(res == FR_OK)
	{
		return *buffer;//
	}
	else
	{
		return -1;// printf("f_read() fail .. \r\n");
	}


}
#endif //SDSUPPORT
