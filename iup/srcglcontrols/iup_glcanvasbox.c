/** \file
 * \brief GLCanvasBox Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupgl.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_register.h"

#include "iup_glcontrols.h"



static Ihandle* iGLCanvasBoxPickChild(Ihandle* ih, int x, int y)
{
  Ihandle* child = ih->firstchild;
  while (child)
  {
    if (x > child->x && x < child->x + child->currentwidth &&
        y > child->y && y < child->y + child->currentheight)
    {
      ih = iGLCanvasBoxPickChild(child, x, y);
      if (ih)
        return ih;
      else
        return child;
    }

    child = child->brother;
  }

  return NULL;
}

static void iGLCanvasBoxCallChildAction(Ihandle* ih, Ihandle* gl_parent)
{
  Ihandle* child = ih->firstchild;
  while (child)
  {
    IFn cb = (IFn)IupGetCallback(child, "GL_ACTION");
    if (cb && iupAttribGetInt(child, "VISIBLE"))
    {
      iupGLSubCanvasSetTransform(child, gl_parent);
      cb(child);
    }

    iGLCanvasBoxCallChildAction(child, gl_parent);
    child = child->brother;
  }
}

static int iGLCanvasBoxSwapBuffers_CB(Ihandle* ih)
{
  iupGLSubCanvasSaveState(ih);

  /* redraw all children */
  iGLCanvasBoxCallChildAction(ih, ih);

  iupGLSubCanvasRestoreState(ih);

  return IUP_DEFAULT;
}

static int iGLCanvasBoxACTION(Ihandle* ih)
{
  IFn cb;

  IupGLMakeCurrent(ih);

  cb = (IFn)IupGetCallback(ih, "APP_ACTION");
  if (cb)
    cb(ih);

  if (!iupStrEqualNoCase(iupAttribGetStr(ih, "BUFFER"), "DOUBLE"))
    iGLCanvasBoxSwapBuffers_CB(ih);

  return IUP_DEFAULT;
}

static int iGLCanvasBoxBUTTON_CB(Ihandle* ih, int button, int pressed, int x, int y, char* status)
{
  IFniiiis cb;
  int ret;

  Ihandle* child = iGLCanvasBoxPickChild(ih, x, y);

  if (child || !pressed)
    iupAttribSet(ih, "_IUP_GLBOX_SELFBUTTON", NULL);
  else
    iupAttribSet(ih, "_IUP_GLBOX_SELFBUTTON", "1");

  if (child && iupAttribGetInt(child, "ACTIVE"))
  {
    if (button == IUP_BUTTON1)
    {
      if (pressed)
        iupAttribSet(child, "PRESSED", "1");
      else
        iupAttribSet(child, "PRESSED", NULL);
    }

    cb = (IFniiiis)IupGetCallback(child, "GL_BUTTON_CB");
    if (cb)
    {
      IupGLMakeCurrent(ih);
      iupGLSubCanvasSaveState(ih);
      iupGLSubCanvasSetTransform(child, ih);
      ret = cb(child, button, pressed, x - child->x, y - child->y, status);
      if (ret != IUP_CONTINUE)
        return IUP_DEFAULT;
    }
  }

  cb = (IFniiiis)IupGetCallback(ih, "APP_BUTTON_CB");
  if (cb)
  {
    IupGLMakeCurrent(ih);
    return cb(ih, button, pressed, x, y, status);
  }

  return IUP_DEFAULT;
}

static void iGLCanvasBoxEnterChild(Ihandle* ih, Ihandle* child)
{
  IFn cb;
  Ihandle* last_child = (Ihandle*)iupAttribGet(ih, "_IUP_GLBOX_LAST_ENTER");

  if (last_child && last_child != child)
  {
    if (iupAttribGetInt(last_child, "ACTIVE"))
    {
      iupAttribSet(last_child, "HIGHLIGHT", NULL);
      iupAttribSet(last_child, "PRESSED", NULL);

      cb = (IFn)IupGetCallback(last_child, "GL_LEAVEWINDOW_CB");
      if (cb)
      {
        IupGLMakeCurrent(ih);
        iupGLSubCanvasSaveState(ih);
        iupGLSubCanvasSetTransform(last_child, ih);
        cb(last_child);
      }
    }

    iupAttribSet(ih, "_IUP_GLBOX_LAST_ENTER", NULL);
  }

  if (child && child != last_child)
  {
    if (iupAttribGetInt(child, "ACTIVE"))
    {
      iupAttribSet(child, "HIGHLIGHT", "1");

      cb = (IFn)IupGetCallback(child, "GL_ENTERWINDOW_CB");
      if (cb)
      {
        IupGLMakeCurrent(ih);
        iupGLSubCanvasSaveState(ih);
        iupGLSubCanvasSetTransform(child, ih);
        cb(child);
      }
    }

    iupAttribSet(ih, "_IUP_GLBOX_LAST_ENTER", (char*)child);
  }
}

static int iGLCanvasBoxMOTION_CB(Ihandle* ih, int x, int y, char *status)
{
  IFniis cb;
  int ret;

  /* only handle child if not pressed at self */
  if (!iupAttribGet(ih, "_IUP_GLBOX_SELFBUTTON"))
  {
    Ihandle* child = iGLCanvasBoxPickChild(ih, x, y);

    iGLCanvasBoxEnterChild(ih, child);

    if (child && iupAttribGetInt(child, "ACTIVE"))
    {
      cb = (IFniis)IupGetCallback(child, "GL_MOTION_CB");
      if (cb)
      {
        IupGLMakeCurrent(ih);
        iupGLSubCanvasSaveState(ih);
        iupGLSubCanvasSetTransform(child, ih);
        ret = cb(child, x - child->x, y - child->y, status);
        if (ret != IUP_CONTINUE)
          return IUP_DEFAULT;
      }
    }
  }

  cb = (IFniis)IupGetCallback(ih, "APP_MOTION_CB");
  if (cb)
  {
    IupGLMakeCurrent(ih);
    return cb(ih, x, y, status);
  }

  return IUP_DEFAULT;
}

static int iGLCanvasBoxWHEEL_CB(Ihandle* ih, float delta, int x, int y, char *status)
{
  IFnfiis cb;
  int ret;

  Ihandle* child = iGLCanvasBoxPickChild(ih, x, y);
  if (child)
  {
    cb = (IFnfiis)IupGetCallback(child, "GL_WHEEL_CB");
    if (cb && iupAttribGetInt(child, "ACTIVE"))
    {
      IupGLMakeCurrent(ih);
      iupGLSubCanvasSaveState(ih);
      iupGLSubCanvasSetTransform(child, ih);
      ret = cb(child, delta, x - child->x, y - child->y, status);
      if (ret != IUP_CONTINUE)
        return IUP_DEFAULT;
    }
  }

  cb = (IFnfiis)IupGetCallback(ih, "APP_WHEEL_CB");
  if (cb)
  {
    IupGLMakeCurrent(ih);
    return cb(ih, delta, x, y, status);
  }

  return IUP_DEFAULT;
}

static int iGLCanvasBoxLEAVEWINDOW_CB(Ihandle* ih)
{
  IFn app_cb;

  iGLCanvasBoxEnterChild(ih, NULL);

  app_cb = (IFn)IupGetCallback(ih, "APP_LEAVEWINDOW_CB");
  if (app_cb)
    app_cb(ih);
  return IUP_DEFAULT;
}

static int iGLCanvasBoxSetRedrawAttrib(Ihandle* ih, const char* value)
{
  iGLCanvasBoxACTION(ih);
  (void)value;
  return 0;
}

static void iGLCanvasBoxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    /* update child natural size only */
    iupBaseComputeNaturalSize(child);
  }

  /* use this to overwrite container behavior in iupBaseComputeNaturalSize */
  *children_expand = ih->expand;

  (void)w;
  (void)h;
}

static void iGLCanvasBoxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    /* update children to their own natural size */
    iupBaseSetCurrentSize(child, child->naturalwidth, child->naturalheight, shrink);
  }
}

static int iGLCanvasBoxGetVerticalAlign(Ihandle* child)
{
  char* value = iupAttribGet(child, "VERTICALALIGN");
  if (!value)
    return -1;  /* FLOAT */
  if (iupStrEqualNoCase(value, "ABOTTOM"))
    return IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value, "ACENTER"))
    return IUP_ALIGN_ACENTER;
  else if (iupStrEqualNoCase(value, "ATOP"))
    return IUP_ALIGN_ATOP;
  return -1;  /* FLOAT */
}

static int iGLCanvasBoxGetHorizontalAlign(Ihandle* child)
{
  char* value = iupAttribGet(child, "HORIZONTALALIGN");
  if (!value)
    return -1; /* FLOAT */
  if (iupStrEqualNoCase(value, "ARIGHT"))
    return IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value, "ACENTER"))
    return IUP_ALIGN_ACENTER;
  else if (iupStrEqualNoCase(value, "ALEFT"))
    return IUP_ALIGN_ALEFT;
  return -1;  /* FLOAT */
}

static void iGLCanvasBoxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  Ihandle* child;
  int horiz_margin = 0, vert_margin = 0;

  IupGetIntInt(ih, "MARGIN", &horiz_margin, &vert_margin);

  /* since GLCanvas is a native container ignore x and y */

  for (child = ih->firstchild; child; child = child->brother)
  {
    int vert_pos = iGLCanvasBoxGetVerticalAlign(child);
    int horiz_pos = iGLCanvasBoxGetHorizontalAlign(child);

    if (vert_pos == IUP_ALIGN_ACENTER)
      y = (ih->currentheight - child->currentheight) / 2;
    else if (vert_pos == IUP_ALIGN_ABOTTOM)
      y = ih->currentheight - child->currentheight - vert_margin;
    else if (vert_pos == IUP_ALIGN_ATOP)
      y = vert_margin;
    else  /* FLOAT */
      y = child->y;

    if (horiz_pos == IUP_ALIGN_ACENTER)
      x = (ih->currentwidth - child->currentwidth) / 2;
    else if (horiz_pos == IUP_ALIGN_ARIGHT)
      x = ih->currentwidth - child->currentwidth - horiz_margin;
    else if (horiz_pos == IUP_ALIGN_ALEFT)
      x = horiz_margin;
    else  /* FLOAT */ 
      x = child->x;

    iupBaseSetPosition(child, x, y);
  }
}

#define CB_NAMES_COUNT 5
static const char* iglcanvasbox_cb_names[CB_NAMES_COUNT] = {
  "ACTION", 
  "BUTTON_CB", "MOTION_CB", "WHEEL_CB",
  "LEAVEWINDOW_CB" };
static Icallback iglcanvasbox_cbs[CB_NAMES_COUNT] = {
  (Icallback)iGLCanvasBoxACTION, 
  (Icallback)iGLCanvasBoxBUTTON_CB, (Icallback)iGLCanvasBoxMOTION_CB, (Icallback)iGLCanvasBoxWHEEL_CB,
  (Icallback)iGLCanvasBoxLEAVEWINDOW_CB };

static int iGLCanvasBoxMapMethod(Ihandle* ih)
{
  int i;
  char new_name[50];

  for (i = 0; i < CB_NAMES_COUNT; i++)
  {
    Icallback cb = IupGetCallback(ih, iglcanvasbox_cb_names[i]);
    if (cb)
    {
      /* save the application callbacks */
      sprintf(new_name, "APP_%s", iglcanvasbox_cb_names[i]);
      IupSetCallback(ih, new_name, cb);
    }

    /* always set GL_* callbacks */
    IupSetCallback(ih, iglcanvasbox_cb_names[i], iglcanvasbox_cbs[i]);
  }

  return IUP_NOERROR;
}

static void iGLCanvasBoxUnMapMethod(Ihandle* ih)
{
  iupGLFontRelease(ih);
}

static int iGLCanvasBoxCreateMethod(Ihandle* ih, void** params)
{
  int i;
  char new_name[50];

  for (i = 0; i < CB_NAMES_COUNT; i++)
  {
    /* save the internal callbacks */
    sprintf(new_name, "GLBOX_%s", iglcanvasbox_cb_names[i]);
    IupSetCallback(ih, new_name, iglcanvasbox_cbs[i]);
  }

  IupSetCallback(ih, "SWAPBUFFERS_CB", iGLCanvasBoxSwapBuffers_CB);

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    while (*iparams)
    {
      IupAppend(ih, *iparams);
      iparams++;
    }
  }

  return IUP_NOERROR;
}

Iclass* iupGLCanvasBoxNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("glcanvas"));

  ic->name = "glcanvasbox";
  ic->format = "g"; /* array of Ihandle */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype  = IUP_CHILDMANY;
  ic->is_interactive = 1;

  ic->New = iupGLCanvasBoxNewClass;
  ic->Create = iGLCanvasBoxCreateMethod;
  ic->Map = iGLCanvasBoxMapMethod;
  ic->UnMap = iGLCanvasBoxUnMapMethod;

  ic->ComputeNaturalSize = iGLCanvasBoxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iGLCanvasBoxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iGLCanvasBoxSetChildrenPositionMethod;

  /* Base Container */
  /* DO not set the default container behavior for EXPAND */
  /* iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT); */
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  {
    IattribGetFunc drawsize_get = NULL;
    iupClassRegisterGetAttribute(ic, "DRAWSIZE", &drawsize_get, NULL, NULL, NULL, NULL);
    iupClassRegisterAttribute(ic, "CLIENTSIZE", drawsize_get, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  }

  iupClassRegisterAttribute(ic, "REDRAW", NULL, iGLCanvasBoxSetRedrawAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGIN", NULL, NULL, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* At Children:
       VERTICALALIGN
       HORIZONTALALIGN
  */

  return ic;
}

Ihandle* IupGLCanvasBoxv(Ihandle** children)
{
  return IupCreatev("glcanvasbox", (void**)children);
}

Ihandle* IupGLCanvasBox(Ihandle* child, ...)
{
  Ihandle* *children;
  Ihandle* ih;

  va_list arglist;
  va_start(arglist, child);
  children = (Ihandle**)iupObjectGetParamList(child, arglist);
  va_end(arglist);

  ih = IupCreatev("glcanvasbox", (void**)children);
  free(children);

  return ih;
}
