/** \file
 * \brief Button Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_button.h"
#include "iup_image.h"


char* iupButtonGetPaddingAttrib(Ihandle* ih)
{
  char *str = iupStrGetMemory(50);
  sprintf(str, "%dx%d", ih->data->horiz_padding, ih->data->vert_padding);
  return str;
}

static int iButtonSetImagePositionAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)  /* set only before map */
  {
    if (iupStrEqualNoCase(value, "RIGHT"))
      ih->data->img_position = IUP_IMGPOS_RIGHT;
    else if (iupStrEqualNoCase(value, "BOTTOM"))
      ih->data->img_position = IUP_IMGPOS_BOTTOM;
    else if (iupStrEqualNoCase(value, "TOP"))
      ih->data->img_position = IUP_IMGPOS_TOP;
    else /* "LEFT" */
      ih->data->img_position = IUP_IMGPOS_LEFT;
  }
  return 0;
}

static char* iButtonGetImagePositionAttrib(Ihandle *ih)
{
  char* img_pos2str[4] = {"LEFT", "RIGHT", "TOP", "BOTTOM"};
  char *str = iupStrGetMemory(50);
  sprintf(str, "%s", img_pos2str[ih->data->img_position]);
  return str;
}

static int iButtonSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)  /* set only before map */
    iupStrToInt(value, &ih->data->spacing);
  return 0;
}

static char* iButtonGetSpacingAttrib(Ihandle *ih)
{
  char *str = iupStrGetMemory(50);
  sprintf(str, "%d", ih->data->spacing);
  return str;
}


/*****************************************************************************************/


static int iButtonCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    if (params[0]) iupAttribStoreStr(ih, "TITLE", (char*)(params[0]));
    if (params[1]) iupAttribStoreStr(ih, "ACTION", (char*)(params[1]));
  }
  ih->data = iupALLOCCTRLDATA();

  ih->data->spacing = 2;

  /* used only by the Windows driver */
  ih->data->horiz_alignment = IUP_ALIGN_ACENTER;
  ih->data->vert_alignment = IUP_ALIGN_ACENTER;
  return IUP_NOERROR;
}

static void iButtonComputeNaturalSizeMethod(Ihandle* ih)
{
  /* always initialize the natural size using the user size */
  ih->naturalwidth = ih->userwidth;
  ih->naturalheight = ih->userheight;

  /* if user size is not defined, then calculate the natural size */
  if (ih->naturalwidth <= 0 || ih->naturalheight <= 0)
  {
    int natural_w = 0, 
        natural_h = 0, 
        type = ih->data->type;

    if (!ih->handle)
    {
      /* if not mapped must initialize the internal values */
      char* value = iupAttribGet(ih, "IMAGE");
      if (value)
      {
        type = IUP_BUTTON_IMAGE;
        if (iupAttribGet(ih, "TITLE"))
          type |= IUP_BUTTON_TEXT;
      }
      else
        type = IUP_BUTTON_TEXT;
    }

    if (type & IUP_BUTTON_IMAGE)
    {
      iupImageGetInfo(iupAttribGet(ih, "IMAGE"), &natural_w, &natural_h, NULL);

      if (type & IUP_BUTTON_TEXT)
      {
        int text_w, text_h;
        /* must use IupGetAttribute to check from the native implementation */
        char* title = IupGetAttribute(ih, "TITLE");
        iupdrvFontGetMultiLineStringSize(ih, title, &text_w, &text_h);

        if (ih->data->img_position == IUP_IMGPOS_RIGHT ||
            ih->data->img_position == IUP_IMGPOS_LEFT)
        {
          natural_w += text_w + ih->data->spacing;
          natural_h = iupMAX(natural_h, text_h);
        }
        else
        {
          natural_w = iupMAX(natural_w, text_w);
          natural_h += text_h + ih->data->spacing;
        }
      }
    }
    else /* IUP_BUTTON_TEXT only */
    {
      /* must use IupGetAttribute to check from the native implementation */
      char* title = IupGetAttribute(ih, "TITLE");
      char* str = iupStrProcessMnemonic(title, NULL, 0);   /* remove & */
      iupdrvFontGetMultiLineStringSize(ih, str, &natural_w, &natural_h);
      if (str && str!=title) free(str);
    }

    /* even when IMPRESS is set, must compute the borders space */
    iupdrvButtonAddBorders(&natural_w, &natural_h);

    natural_w += 2*ih->data->horiz_padding;
    natural_h += 2*ih->data->vert_padding;

    /* only update the natural size if user size is not defined. */
    if (ih->naturalwidth <= 0) ih->naturalwidth = natural_w;
    if (ih->naturalheight <= 0) ih->naturalheight = natural_h;
  }
}


/******************************************************************************/


Ihandle* IupButton(const char* title, const char* action)
{
  void *params[3];
  params[0] = (void*)title;
  params[1] = (void*)action;
  params[2] = NULL;
  return IupCreatev("button", params);
}

Iclass* iupButtonGetClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "button";
  ic->format = "SA"; /* one optional string, and one optional callback name */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->Create = iButtonCreateMethod;
  ic->ComputeNaturalSize = iButtonComputeNaturalSizeMethod;

  ic->SetCurrentSize = iupBaseSetCurrentSizeMethod;
  ic->SetPosition = iupBaseSetPositionMethod;

  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "BUTTON_CB", "iiiis");
  iupClassRegisterCallback(ic, "ACTION", "");

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* IupButton only */
  iupClassRegisterAttribute(ic, "SPACING", iButtonGetSpacingAttrib, iButtonSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "2", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", iButtonGetImagePositionAttrib, iButtonSetImagePositionAttrib, IUPAF_SAMEASSYSTEM, "LEFT", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANFOCUS", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESSBORDER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FLAT", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  iupdrvButtonInitClass(ic);

  return ic;
}
