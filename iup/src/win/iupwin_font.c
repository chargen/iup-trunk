/** \file
 * \brief Windows Font mapping
 *
 * See Copyright Notice in iup.h
 */


#include <stdlib.h>
#include <stdio.h>

#include <windows.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_array.h"
#include "iup_attrib.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_assert.h"

#include "iupwin_drv.h"
#include "iupwin_info.h"


typedef struct IwinFont_
{
  char standardfont[200];
  HFONT hfont;
} IwinFont;

static Iarray* win_fonts = NULL;

static HFONT winGetFont(const char *standardfont)
{
  HFONT new_font;
  int height_pixels;
  char typeface[50] = "";
  int height = 8;
  int is_bold = 0,
    is_italic = 0, 
    is_underline = 0,
    is_strikeout = 0;
  int res = iupwinGetScreenRes();
  int i, count = iupArrayCount(win_fonts);
  const char* mapped_name;

  /* Check if the standardfont already exists in cache */
  IwinFont* fonts = (IwinFont*)iupArrayGetData(win_fonts);
  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(standardfont, fonts[i].standardfont))
      return fonts[i].hfont;
  }

  /* parse the old format first */
  if (!iupFontParseWin(standardfont, typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
  {
    if (!iupFontParsePango(standardfont, typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
      return NULL;
  }

  mapped_name = iupFontGetWinName(typeface);
  if (mapped_name)
    strcpy(typeface, mapped_name);

  /* get size in pixels */
  if (height < 0)
    height_pixels = height;  /* already in pixels */
  else
    height_pixels = -IUPWIN_PT2PIXEL(height, res);

  if (height_pixels == 0)
    return NULL;

  new_font = CreateFont(height_pixels,
                        0,0,0,
                        (is_bold) ? FW_BOLD : FW_NORMAL,
                        is_italic,
                        is_underline,
                        is_strikeout,
                        DEFAULT_CHARSET,OUT_TT_PRECIS,
                        CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                        FF_DONTCARE|DEFAULT_PITCH,
                        typeface);
  if (!new_font)
    return NULL;

  /* create room in the array */
  fonts = (IwinFont*)iupArrayInc(win_fonts);

  strcpy(fonts[i].standardfont, standardfont);
  fonts[i].hfont = new_font;

  return fonts[i].hfont;
}

static HDC winFontGetDC(Ihandle* ih)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID)
    return GetDC(NULL);
  else
    return GetDC(ih->handle);  /* handle can be NULL here */
}

void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h)
{
  const char *curstr; 
  const char *nextstr;
  int len, num_lin, max_w;
  SIZE size;
  HDC hdc;
  HFONT oldhfont, hfont;

  if (!str || str[0]==0)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  hfont = (HFONT)iupwinGetHFontAttrib(ih);
  if (!hfont)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  hdc = winFontGetDC(ih);
  oldhfont = SelectObject(hdc, hfont);

  max_w = 0;
  curstr = str;
  num_lin = 0;
  do
  {
    nextstr = iupStrNextLine(curstr, &len);
    GetTextExtentPoint32(hdc, curstr, len, &size);
    max_w = iupMAX(max_w, size.cx);

    curstr = nextstr;
    num_lin++;
  } while(*nextstr);

  if (w) *w = max_w;

  if (h) 
  {
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    *h = tm.tmHeight*num_lin;  /* (charheight*number_of_lines) */
  }

  SelectObject(hdc, oldhfont);
  ReleaseDC(ih->handle, hdc);
}

int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  HDC hdc;
  HFONT oldhfont, hfont;
  SIZE size;
  int len;
  char* line_end;

  if (!str || str[0]==0)
    return 0;

  hfont = (HFONT)iupwinGetHFontAttrib(ih);
  if (!hfont)
    return 0;

  hdc = winFontGetDC(ih);
  oldhfont = SelectObject(hdc, hfont);

  line_end = strchr(str, '\n');
  if (line_end)
    len = line_end-str;
  else
    len = strlen(str);

  GetTextExtentPoint32(hdc, str, len, &size);

  SelectObject(hdc, oldhfont);
  ReleaseDC(ih->handle, hdc);

  return size.cx;
}

void iupdrvFontGetCharSize(Ihandle* ih, int *charwidth, int *charheight)
{
  TEXTMETRIC tm;
  HDC hdc;
  HFONT oldfont;
  HFONT hfont = (HFONT)iupwinGetHFontAttrib(ih);
  if (!hfont)
  {
    if (charwidth)  *charwidth = 0;
    if (charheight) *charheight = 0;
    return;
  }

  hdc = winFontGetDC(ih);
  oldfont = SelectObject(hdc, hfont);

  GetTextMetrics(hdc, &tm);

  SelectObject(hdc, oldfont);
  ReleaseDC(ih->handle, hdc);
  
  if (charwidth)  *charwidth = tm.tmAveCharWidth;
  if (charheight) *charheight = tm.tmHeight;
}

static void winFontFromLogFont(LOGFONT* logfont, char * font)
{
  int is_bold = (logfont->lfWeight == FW_NORMAL)? 0: 1;
  int is_italic = logfont->lfItalic;
  int is_underline = logfont->lfUnderline;
  int is_strikeout = logfont->lfStrikeOut;
  int height_pixels = logfont->lfHeight;
  int res = iupwinGetScreenRes();
  int height = IUPWIN_PIXEL2PT(-height_pixels, res);

  sprintf(font, "%s, %s%s%s%s %d", logfont->lfFaceName, 
                                   is_bold?"Bold ":"", 
                                   is_italic?"Italic ":"", 
                                   is_underline?"Underline ":"", 
                                   is_strikeout?"Strikeout ":"", 
                                   height);
}

char* iupdrvGetSystemFont(void)
{
  static char systemfont[200] = "";
  NONCLIENTMETRICS ncm;
  ncm.cbSize = sizeof(NONCLIENTMETRICS);
  if (SystemParametersInfo (SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, FALSE))
    winFontFromLogFont(&ncm.lfMessageFont, systemfont);
  else
    strcpy(systemfont, "Tahoma, 10");
  return systemfont;
}

HFONT iupwinFontCreateNativeFont(const char* value)
{
  HFONT hfont = winGetFont(value);
  if (!hfont)
  {
    iupERROR1("Failed to create Font: %s", value); 
    return NULL;
  }

  return hfont;
}

static HFONT winFontCreateNativeFont(Ihandle* ih, const char* value)
{
  HFONT hfont = winGetFont(value);
  if (!hfont)
  {
    iupERROR1("Failed to create Font: %s", value); 
    return NULL;
  }

  iupAttribSetStr(ih, "HFONT", (char*)hfont);
  return hfont;
}

char* iupwinGetHFontAttrib(Ihandle *ih)
{
  HFONT hfont = (HFONT)iupAttribGetStr(ih, "HFONT");
  if (!hfont)
    return (char*)winFontCreateNativeFont(ih, iupGetFontAttrib(ih));
  else
    return (char*)hfont;
}

int iupdrvSetStandardFontAttrib(Ihandle* ih, const char* value)
{
  HFONT hfont = winFontCreateNativeFont(ih, value);
  if (!hfont)
    return 1;

  /* If FONT is changed, must update the SIZE attribute */
  iupBaseUpdateSizeAttrib(ih);

  /* FONT attribute must be able to be set before mapping, 
      so the font is enable for size calculation. */
  if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID))
    SendMessage(ih->handle, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE,0));

  return 1;
}

void iupdrvFontInit(void)
{
  win_fonts = iupArrayCreate(50, sizeof(IwinFont));
}

void iupdrvFontFinish(void)
{
  int i, count = iupArrayCount(win_fonts);
  IwinFont* fonts = (IwinFont*)iupArrayGetData(win_fonts);
  for (i = 0; i < count; i++)
  {
    DeleteObject(fonts[i].hfont);
    fonts[i].hfont = NULL;
  }
  iupArrayDestroy(win_fonts);
}
