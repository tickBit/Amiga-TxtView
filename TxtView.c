/*
	TxtView

	- meant for viewing text files
		
	This program is for AmigaOS 3.2.x so, that the required library versions are at least 47
	
	Not quite finished, just wanted to brush up my C skills a bit...
	
*/

#include <dos/dos.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <diskfont/diskfont.h>
#include <stdlib.h>
#include <stdio.h>

#include <clib/macros.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/diskfont_protos.h>
/*****************************************************************************/

#define	IDCMP_FLAGS	IDCMP_CLOSEWINDOW | IDCMP_VANILLAKEY | IDCMP_GADGETUP \
			| IDCMP_MOUSEMOVE | IDCMP_INTUITICKS | IDCMP_MOUSEBUTTONS | IDCMP_NEWSIZE

#define FILE_BUF_SIZE 8192

/*****************************************************************************/

/*****************************************************************************/

extern struct Library *SysBase, *DOSBase;
struct Library *IntuitionBase;
struct Library *GfxBase;
struct Library *DiskfontBase;

struct TextFont *courier18 = NULL;
BOOL endOfFile = FALSE;

/*****************************************************************************/

int main (int argc, char **argv)
{
    struct IntuiMessage *imsg;
    struct Screen *scr;
    struct Window *win;
    BOOL going = TRUE;
    ULONG sigr;
	struct FileHandle *file_handle;
	struct RastPort *rp;
	
	char *txtBuffer = NULL, *newTxt = NULL;
	long fileSize, readBytes;
	
	WORD tabSize = 3;
	int currentCursor, textCursor;
	long pages = 0, totalReadBytes = 0;
	int chrs = 0;
	
	if (argc != 2 && argc != 3) {
		printf("Wrong number of arguments.\nTxtView [file] [tab size]\nTab size is optional (default is 3). Max. tab size = 16\n");
		exit(0);
	}
	
    if (argc == 3) {
        tabSize = atoi(argv[2]);
		if (tabSize > 16) {
			printf("Too large tab size!\n");
			exit(0);
		}
    }
    
	// argv[1] has the filename
	file_handle = (struct FileHandle *)Open(argv[1], MODE_OLDFILE );
  
  	if( file_handle == NULL )
  	{
    	printf("Could not open %s\n", argv[1]);
    	exit(0);
  	}
	
	// read file size
	Seek((BPTR)file_handle, OFFSET_BEGINNING, OFFSET_END);
	fileSize = Seek((BPTR)file_handle, OFFSET_CURRENT, OFFSET_BEGINNING) + 1;
		
	txtBuffer = (char *)malloc(FILE_BUF_SIZE);
	
	newTxt = (char *)readBytesToBuffer(fileSize, tabSize, txtBuffer, newTxt, file_handle, &pages, &totalReadBytes);
	
	if (newTxt == NULL) {
		if (txtBuffer) free(txtBuffer);
		if (newTxt) free(newTxt);
		Close((BPTR)file_handle);
		exit(0);
	}
	
	IntuitionBase = OpenLibrary("intuition.library", 47);
	GfxBase = OpenLibrary("graphics.library", 47);
	DiskfontBase = OpenLibrary("diskfont.library", 47);
	
	if (!(IntuitionBase || GfxBase || DiskfontBase)) {
		
		if (IntuitionBase) CloseLibrary(IntuitionBase); else printf("Failed to open intuition.library v47\n");
		if (GfxBase) CloseLibrary(GfxBase); else printf("Failed to open graphics.library v47\n");
		if (DiskfontBase) CloseLibrary(DiskfontBase); else printf("Failed to open diskfont.library v47\n");
		
		if (newTxt) free(newTxt);
		if (txtBuffer) free(txtBuffer);
		Close((BPTR)file_handle);
		exit(0);
	}
	
	scr = ((struct IntuitionBase *)IntuitionBase)->FirstScreen;

	if (win = OpenWindowTags (NULL,
				      WA_Title,		"TxtView",
				      WA_InnerWidth,	320,
				      WA_InnerHeight,	184,
				      WA_IDCMP,		IDCMP_FLAGS,
				      WA_DragBar,	TRUE,
				      WA_DepthGadget,	TRUE,
				      WA_CloseGadget,	TRUE,
				      WA_SimpleRefresh,	TRUE,
				      WA_NoCareRefresh,	TRUE,
				      WA_Activate,	TRUE,
				      WA_SizeGadget,	TRUE,
				      WA_MinWidth,	30,
				      WA_MinHeight,	scr->BarHeight + 1 + 34,
				      WA_MaxWidth,	-1,
				      WA_MaxHeight,	-1,
				      WA_CustomScreen,	scr,
				      TAG_DONE))
	    {
			rp = win->RPort;
			
			WORD left   = win->BorderLeft;
			WORD top    = win->BorderTop;
			WORD right  = win->Width  - win->BorderRight  - 1;
			WORD bottom = win->Height - win->BorderBottom - 1;
	
			SetupFont(win);
			SetAPen(rp,1);
			
			currentCursor = 0;
			textCursor = printToWindow(newTxt, rp, win, currentCursor, FALSE, pages, fileSize);
			            
		    while (going)
		    {
			sigr = Wait ((1L << win->UserPort->mp_SigBit | SIGBREAKF_CTRL_C));

			if (sigr & SIGBREAKF_CTRL_C)
			    going = FALSE;

			while (imsg = (struct IntuiMessage *) GetMsg (win->UserPort))
			{
			    switch (imsg->Class)
			    {
				case IDCMP_CLOSEWINDOW:
				    going = FALSE;
				    break;

				case IDCMP_VANILLAKEY:
					
				    switch (imsg->Code)
				    {
					case  27:
					case 'q':
					case 'Q':
					    going = FALSE;
					    break;
				    }
					
					case ' ':
					
						if (endOfFile == TRUE) break;
						
						if (textCursor >= 7000) {
							Seek((BPTR)file_handle, textCursor, OFFSET_CURRENT);	
							newTxt = (char *)readBytesToBuffer(fileSize, tabSize, txtBuffer, newTxt, file_handle, &pages, &totalReadBytes);
							textCursor = 0;
							currentCursor = 0;
						}
						
						// error in reading file
						if (newTxt == NULL) {
							ReplyMsg ((struct Message *) imsg);
							
							if (txtBuffer) free(txtBuffer);
							if (newTxt) free(newTxt);
							Close((BPTR)file_handle);
							CloseWindow(win);
							
							if (courier18) CloseFont(courier18);
	
    						CloseLibrary(IntuitionBase);
							CloseLibrary(GfxBase);
							CloseLibrary(DiskfontBase);
	
							exit(0);
						}
						
						left   = win->BorderLeft;
						top    = win->BorderTop;
						right  = win->Width  - win->BorderRight  - 1;
						bottom = win->Height - win->BorderBottom - 1;
				
						SetAPen(rp, rp->BgPen);
						RectFill(rp, left, top, right, bottom);
						
						SetAPen(rp, 1);
						
						currentCursor = textCursor;
						textCursor = printToWindow(newTxt, rp, win, textCursor, FALSE, pages, fileSize);
						
						break;
						
				    break;
					
				case IDCMP_NEWSIZE:
										
					left   = win->BorderLeft;
					top    = win->BorderTop;
					right  = win->Width  - win->BorderRight  - 1;
					bottom = win->Height - win->BorderBottom - 1;
			
					SetAPen(rp, rp->BgPen);
					RectFill(rp, left, top, right, bottom);
					
					SetAPen(rp, 1);
										
					printToWindow(newTxt, rp, win, currentCursor, TRUE, pages, fileSize);
					
					// if needed, more characters must be read!
					break;
			    }

			    ReplyMsg ((struct Message *) imsg);
			}

		
	    }
		CloseWindow (win);
    }
	
    if (courier18) CloseFont(courier18);
	
    CloseLibrary(IntuitionBase);
	CloseLibrary(GfxBase);
	CloseLibrary(DiskfontBase);
	
	if (txtBuffer) free(txtBuffer);
	if (newTxt) free(newTxt);
	
	Close((BPTR)file_handle);
	
	return RETURN_OK;
}


char* readBytesToBuffer(long fileSize, int tabSize, char *txtBuffer, char *newTxt, BPTR file_handle, long *pages, long *totalReadBytes) {

	long readBytes = 0;
	
	// read bytes of file into txtBuffer
	readBytes = Read(file_handle, txtBuffer, FILE_BUF_SIZE);
	
	if (!(readBytes != FILE_BUF_SIZE || readBytes != fileSize)) {
		printf("Something went wrong while reading the file\n");
		return NULL;
	}
	
	(*totalReadBytes) += readBytes;
	
	
    int tabs = 0;
    for (int i = 0; i < readBytes; i++) {
        if (txtBuffer[i] == 9) tabs++;
    }
    
	if (sizeof(newTxt) > 0) free(newTxt);
	
    newTxt = (char *)malloc(FILE_BUF_SIZE + tabSize * tabs + 1);
    	
    int newI = 0;
		
	// replace tabs with spaces, filter ASCII 13 (carriage return) out
    for (int i = 0; i < readBytes; i++) {
		
        if (txtBuffer[i] != 9 && txtBuffer[i] != 13) {
			newTxt[newI] = txtBuffer[i];
			newI++;
        } else if (txtBuffer[i] == 9) {
            for (int t = 0; t < tabSize; t++) {
				newTxt[newI] = ' ';
                newI++;
            }
        }
    }

	(*pages)++;
		
	if (*totalReadBytes == fileSize) {
		newTxt[newI] = '\0';
	}
	
	return newTxt;
}

int printToWindow(char *newTxt, struct RastPort *rp, struct Window *win, int textCursor, BOOL resize, long pages, int fileSize) {
	
	struct TextExtent constrainingExtent;
	WORD maxWidth  = win->Width - win->BorderLeft - win->BorderRight;		
	int i = textCursor;
	
	int prevI = textCursor;
	WORD fHeight = rp->Font->tf_YSize;
	WORD rows = (win->Height - win->BorderTop - win->BorderBottom) / fHeight;
	UWORD baseLine = rp->Font->tf_Baseline;				
	
	WORD length = 0;
	
	for (int j = 0; j < rows; j++) {
		
		// "Before V47, this function suffered from an off-by-one error
		//  in case the input font was not fixed width."
		if (newTxt[prevI] != 10) TextExtent(rp, newTxt + prevI, i - prevI + 1, &constrainingExtent);
		else TextExtent(rp, newTxt + prevI + 1, i-prevI - 1, &constrainingExtent);
								
		while(constrainingExtent.te_Width < maxWidth && newTxt[i] != 10) {
			
			i++;

			if (newTxt[i] == '\0') {
				endOfFile = TRUE;
				break;
			}
			
			if (newTxt[prevI] != 10) TextExtent(rp, newTxt + prevI, i-prevI + 1, &constrainingExtent);
			else TextExtent(rp, newTxt + prevI + 1, i-prevI, &constrainingExtent);
			
		}
        
		
		if (endOfFile == FALSE || (endOfFile == TRUE && resize == TRUE)) {
			                      
			Move(rp, win->BorderLeft, win->BorderTop + baseLine + fHeight * j);
			
			if (newTxt[prevI] == 10) {
				Text(rp, newTxt + prevI + 1, i - prevI - 1);
			} else if(newTxt[i] == 10) {
				Text(rp, newTxt + prevI, i - prevI);
			}
			else {
				Text(rp, newTxt + prevI, i - prevI);
			}
			
			prevI = i;
			
			if (newTxt[i] == '\0') {
				endOfFile = TRUE;
				break;
			}
			
			i++;
		}
		
	}
	
	return prevI;
}

void SetupFont(struct Window *win)
{
    struct TextAttr ta =
    {
        "courier.font",
        18,0,0
    };

    courier18 = OpenDiskFont(&ta);
    if (courier18)
    {
        SetFont(win->RPort, courier18);
    }
}
