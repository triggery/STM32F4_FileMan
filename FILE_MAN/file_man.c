/*
 * file_man.c
 *
 *  Created on: 30 нояб. 2016 г.
 *      Author: dmitry
 */
#include "file_man.h"
#include "ff.h"
#include "string.h"
#include "SSD1289.h"
#include "stdbool.h"
#include "GesturesHandler.h"

#include "icons_menu.h"
#include "my_str_lib.h"

#include "audio_dac.h"
#include "libBMP_tiny.h"

#include "main.h"
#include "stdbool.h"

#include "gdisp_image_bmp.h"
#include "gif.h"

#define X_OFFSET_ITMEMS	5
#define X_WIDTH_ITMEMS	100

char Lfname[MAX_CHAR_NAME_ITEM];	// SIZE_OF_LFN_BUF
LIST_ITEMS_STRUCT listing;

int FileMen_ShowFilesList(void)
{
	int cnt;
	for( cnt = 0; cnt < listing.countItems; cnt++)
	{
		if( listing.list[cnt]->itemCoord.Y < 0 )	// если координата меньше 0, то пропускаем цикл
			continue;
		if( cnt == listing.currItem )
			tempSetTextColor(RED);

		LCD_PutString( 21, listing.list[cnt]->itemCoord.Y, listing.list[cnt]->ItemName );

		if( cnt == listing.currItem )
			tempRestoreTextColor();
	}
	return cnt;
}

int FileMan_Show(void)
{
	ICON_ITEM_STRUCT *item;
	int x_offsetText = 0,
		y_offsetText = ICON_SIZE,
		textLen = 8,
		selectW = ICON_SIZE + 2;
	int cnt = 0;

	if(listing.mtype == isTextMenu) {
		textLen = 18;
		x_offsetText = ICON_SIZE + 3;
		selectW = 180;

		if(ICON_SIZE == 16)
			y_offsetText = 0;
		else
			y_offsetText = ICON_SIZE/2;
	}

	for( cnt = 0; cnt < listing.countItems; cnt++ )
	{
		item = getNumItem(&listing, cnt);
		if( item->itemCoord.Y < 0 )
			continue;
		if( (item->itemCoord.Y) > DISP_H ) {
			return 0;
		}

		LCD_WriteBMP_16( item->itemCoord.X, item->itemCoord.Y+ICON_SIZE, ICON_SIZE, ICON_SIZE, (uint16_t*)item->icon.pIconBuff );
		LCD_PutStringShorten( item->itemCoord.X + x_offsetText, item->itemCoord.Y+y_offsetText, item->ItemName, textLen );
		if( listing.currItem == cnt ) {
			tempSetTextColor(RED);
			LCD_DrawRect( item->itemCoord.X-1, item->itemCoord.Y-1, selectW, ICON_SIZE );
			tempRestoreTextColor();
		}
	}
	return cnt;
}

FILINFO Finfo;
void FileMan_Init(void)
{
	Finfo.lfname = Lfname;
	Finfo.lfsize = sizeof(Lfname);

	ListingConstructor(&listing);
	setListingType(&listing, isTextMenu); // isIconMenu or isTextMenu
}

int FileMan_OpenFolder( const char *path )
{
	//char chBuff[10];
	int res = open_folder(path);
	setCoortItemsFM();


	/*itoa_m(listing.countItems, &chBuff[0]);
	LCD_PutString( 90, 304, &chBuff[0] );*/
	return res;
}

ICON_ITEM_STRUCT* FileMan_GetCurrIconItem(void)
{
	return listing.list[listing.currItem];
}

ICON_ITEM_STRUCT* FileMan_GetIconItemIndex(int index)
{
	return listing.list[index];
}

//------------------------------------------------------------------------------------

void setCoortItemsFM(void)
{
	setCoordItems(&listing);
}

int open_folder( const char *path )
{
	DIR fddir;
	FRESULT res;

	if( ( res = f_opendir( &fddir, path ) ) == FR_OK )
	{
		while ( ( ( res = f_readdir( &fddir, &Finfo ) ) == FR_OK ) && Finfo.fname[0] )
		{
            ICON_ITEM_STRUCT *item = newIconItem();
            strcpy(item->ItemName, Finfo.lfname);
            addItem(&listing, item);

            if( listing.countItems > MAX_ITEMS_LISTING )	//FILE_ITEMS_MAX
            	return 1;
		}
	}
	else
		putErrorString( "Open dir error!!!" );

	return 0;
}

char *getCurrFileName(void)
{
	//return listing.list[listing.currItem]->ItemName;
	return getCurrItem(&listing)->ItemName;
}

int getCurrIndexFile(void)
{
	return listing.currItem;
}

Point getCoordCurrIcon(void)
{
	return getCurrItem(&listing)->itemCoord;
}

Point getCoordIconIndex(int findex)
{
	return getNumItem(&listing, findex)->itemCoord;
}

mMenuType getMenuListType(void)
{
	return getListingType(&listing);
}

bool fileList_IsEmpty(void)
{
	return (listing.countItems > 0)? 0 : 1; // if file list is EMPTY - return 1
}

int setNextItem(FILE_MAN_ERROR *ferr)
{
	if( listing.currItem < listing.countItems - 1 ) {
		listing.currItem++;
		*ferr = FILE_MAN_OK;
	}
	else
		*ferr = FILE_MAN_LAST_FILE;
	return listing.currItem;
}

int setPrevItem(FILE_MAN_ERROR *ferr)
{
	if( listing.currItem > 0 ) {
		listing.currItem--;
		*ferr = FILE_MAN_OK;
	}
	else
		*ferr = FILE_MAN_FIRST_FILE;
	return listing.currItem;
}

//------------------------------------------------------------------------------------

int FindTouchedItem(TOUCH_EVENT *touchEvent)
{
	int index = 0;
	bool find = 0;

	Point touch_point = touchEvent->point;
	Point item_coord;

	/*if( fileList_IsEmpty() )
		return -1;*/

	while( index < listing.countItems && !find )
	{
		item_coord = listing.list[index]->itemCoord;
		if( listing.mtype == isTextMenu )
		{
			if( (touch_point.Y > X_OFFSET_ITMEMS) && (touch_point.Y < X_OFFSET_ITMEMS+X_WIDTH_ITMEMS)
			 && (touch_point.X > item_coord.Y) && (touch_point.X < item_coord.Y+LCD_GetCharSize()) )
			{
				find = 1;
			}
			else
				index++;
		}
		else	// listing.mtype == isIconMenu
		{
			if( (touch_point.Y > item_coord.X) && (touch_point.Y < item_coord.X+ICON_SIZE)
			 && (touch_point.X > item_coord.Y) && (touch_point.X < item_coord.Y+ICON_SIZE) )
			{
				find = 1;
			}
			else
				index++;
		}
	}

	if(!find)
		return -1;
	return index;
}

void moveItems_UpDown(bool up);

PROCESS_RESULT FileMan_TouchHandler(void)
{
	if ( getRefreshScreen() == 1 )
	{
		LCD_Clear(LCD_BackColor());
		FileMan_Show();
		setRefreshScreen(0);
	}

	Point point;
	Point movpoint[2];
	TOUCH_EVENT touchEvent;

	touchEvent = getTouchMode();
	point = touchEvent.point;

    char chBuff[10];
    int findItem;

    if ( touchEvent.mode == TOUCH_MODE_TOUCH_DOT )
    {
    	findItem = FindTouchedItem(&touchEvent);
    	if( findItem >= 0 )
    	{
    		listing.currItem = findItem;

        	LCD_Clear(LCD_BackColor());
        	FileMan_Show();

    		itoa_m(findItem, &chBuff[0]);
    		LCD_PutString( 130, 304, &chBuff[0] );
    		ICON_ITEM_STRUCT *item = FileMan_GetIconItemIndex(findItem);
    		LCD_PutString( 5, 288, item->ItemName );

    		openCurrentFile();
    	}

    	LCD_DrawFullCircle( point.Y, point.X, 2 );
    	LCD_PutString( 5, 304, "dot" );

    	itoa_m(point.X, &chBuff[0]);
    	LCD_PutString( 50, 304, &chBuff[0] );
    	itoa_m(point.Y, &chBuff[0]);
    	LCD_PutString( 90, 304, &chBuff[0] );
    }
    else if ( touchEvent.mode == TOUCH_MODE_MOVE )
    {
    	STATUS_TOCH status;
    	static int i = 0;
    	if( i < 2 )
    	{
    		movpoint[i] = GetTouchPoint(&status);
    		i++;
    		Delay(15);
    	}
    	else
    	{
    		i = 0;
    		if( ( movpoint[1].X > movpoint[0].X ) && ( movpoint[1].X - movpoint[0].X > 5) )
    		{
    			moveItems_UpDown(1);
    		}
    		else if( ( movpoint[0].X > movpoint[1].X ) && ( movpoint[0].X - movpoint[1].X > 5 ) )
    		{
    			moveItems_UpDown(0);
    		}
    		movpoint[0].X = 0;
    		movpoint[0].Y = 0;
    		movpoint[1].X = 0;
    		movpoint[1].Y = 0;

    		LCD_Clear(LCD_BackColor());
    		FileMan_Show();
    	}
    }

    return PROCESS_OK;
}

PROCESS_TYPE PictureProc;
PROCESS_RESULT showPictureProc(void);

PROCESS_TYPE GifProc;
PROCESS_RESULT showGifProc(void);

char processEnd = 0;
PROCESS_TYPE PlayWaveProc;
PROCESS_RESULT PlayWaveProcess(void);

char *filename;
FILE_MAN_ERROR openCurrentFile(void)
{
	filename = listing.list[listing.currItem]->ItemName;

	if ( getStatusPlay() )
		audio_play_stop();

	if ( strstr( filename, ".wav" ) )
	{
		processEnd = 0;
		setProcessFunc( &PlayWaveProc, PlayWaveProcess, PROCESS_ACTIVE, &FileMan_TouchProc );
		addProcess( &PlayWaveProc );
	}
	else if ( strstr( filename, ".bmp" ) )
	{
		setProcessFunc( &PictureProc, showPictureProc, PROCESS_ACTIVE, &FileMan_TouchProc );
		addProcess( &PictureProc );
	}
	else if ( strstr( filename, ".txt" ) )
	{

	}
	else if( strstr( filename, ".gif" ) ) {
		setProcessFunc( &GifProc, showGifProc, PROCESS_ACTIVE, &FileMan_TouchProc );
		addProcess( &GifProc );
	}
	else
		return FILE_MAN_NOT_SUPPORT;

	return FILE_MAN_OK;
}

PROCESS_RESULT showPictureProc(void)
{
	FIL file;
	gdispImage image;
	gdispImageError gerr;
	image.fp = &file;

	LCD_Clear(LCD_BackColor());
	FRESULT res = f_open( image.fp, filename, FA_READ );
	if( res )
		return PROCESS_END;

	gerr = gdispImageOpen_BMP(&image);
	if ( gerr == GDISP_IMAGE_ERR_OK )
	{
		gerr = gdispGImageDraw_BMP( &image, 65, 65 );
	}
	else
		LCD_PutString( 10, 20, "image is not BMP!!!" );

	LCD_PutString( 5, 300, filename );

	TOUCH_EVENT touchEvent;
	char touch = 0;
    while ( touch == 0 )
    {
    	touchEvent = getTouchMode();
    	if ( touchEvent.mode == TOUCH_MODE_TOUCH_DOT || touchEvent.mode == TOUCH_MODE_MOVE )
    	{
    		Delay(100);
    		touch = 1;
    	}
    }

	return PROCESS_END;
}

PROCESS_RESULT PlayWaveProcess(void)
{
	if( !processEnd )
	{
		processEnd = 1;
		int x = 100, y = 100;
		LCD_Clear(LCD_BackColor());
		LCD_WriteBMP_16( x, y+60, 64, 60, getSpeackerIcon() );
		LCD_PutString(15, y + 60 + 25, filename);
		audio_play_start(filename);
	}

	TOUCH_EVENT touchEvent;

	touchEvent = getTouchMode();
	if ( touchEvent.mode == TOUCH_MODE_TOUCH_DOT || touchEvent.mode == TOUCH_MODE_MOVE )
	{
		audio_play_stop();
		Delay(10);
		processEnd = 0;
		return PROCESS_END;
	}

	return PROCESS_OK;
}

PROCESS_RESULT showGifProc(void)
{
	uint8_t *frame, *color;
	int ret, i, j;

	LCD_Clear(LCD_BackColor());

	gd_GIF *gif = gd_open_gif(filename);
	if(gif == NULL)
		putErrorString("error: not open file!!!");
	frame = (uint8_t *)malloc(gif->width * gif->height * 3);
	if(!frame)
		putErrorString("error: not allocate frame bufer!!!");

	LCD_SetPORightUp_Win( (240/2)-gif->height/2, (320/2)+gif->width/2, gif->width, gif->height );
	LCD_WriteReg(LCD_REG_17, 0x6830); // AM=0; ID[1:0]=01
	LCD_WriteRAM_Prepare();

	TOUCH_EVENT touchEvent;
	char touch = 0;

	while(touch == 0) {
				ret = gd_get_frame(gif);
				if(ret == -1)
					putErrorString("error: can't get frame!!!");
				gd_render_frame(gif, frame);

				color = frame;

				for (i = 0; i < gif->height; i++) {
					for (j = 0; j < gif->width; j++) {
						//								R		  G		    B
						LCD_WriteRAM(getColor565(color[0], color[1], color[2]));
						color += 3;
					}
				}

				if(gif->height < 64) {
					Delay(50);
				}

		        if (ret == 0)
		            gd_rewind(gif);

		    	touchEvent = getTouchMode();
		    	if ( touchEvent.mode == TOUCH_MODE_TOUCH_DOT || touchEvent.mode == TOUCH_MODE_MOVE )
		    	{
		    		Delay(100);
		    		touch = 1;

			    	LCD_WriteReg(LCD_REG_17, 0x6830);
			    	LCD_WriteReg(0x0044,0xEF00); // Horizontal position = 239
			    	LCD_WriteReg(0x0045,0x0000); // start positions = 0

					free(frame);
					gd_close_gif(gif);
		    		return PROCESS_END;
		    	}

			}

	return PROCESS_OK;
}

void moveItems_UpDown(bool up)
{
	int cnt;
	__disable_irq();

		if(up)	// MOVE DOWN!!!
		{
			if( (listing.list[0]->itemCoord.Y) < Y_OFFSET_ITEMS ){
				for( cnt = 0; cnt < listing.countItems; cnt++ )
				{
						listing.list[cnt]->itemCoord.Y += 8;
				}
			}
			else {
				__enable_irq();
				return;
			}
		}
		else	// MOVE UP!!!
		{
			if( (listing.list[listing.countItems-1]->itemCoord.Y + ICON_SIZE + 16) > DISP_H )
			{
				for( cnt = 0; cnt < listing.countItems; cnt++ )
				{
					listing.list[cnt]->itemCoord.Y -= 8;
				}
			}
			else {
				__enable_irq();
				return;
			}
		}

	__enable_irq();
}
