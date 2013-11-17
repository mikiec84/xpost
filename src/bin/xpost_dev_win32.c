/*
 * Xpost - a Level-2 Postscript interpreter
 * Copyright (C) 2013, Michael Joshua Ryan
 * Copyright (C) 2013, Vincent Torri
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the Xpost software product nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <string.h>

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include "xpost_log.h"
#include "xpost_memory.h"  /* save/restore works with mtabs */
#include "xpost_object.h"  /* save/restore examines objects */
#include "xpost_stack.h"  /* save/restore manipulates (internal) stacks */

#include "xpost_context.h"
#include "xpost_dict.h"
#include "xpost_string.h"
#include "xpost_name.h"
#include "xpost_operator.h"
#include "xpost_op_dict.h"
#include "xpost_dev_win32.h"

typedef struct _BITMAPINFO_XPOST
{
   BITMAPINFOHEADER bih;
   DWORD            masks[3];
} BITMAPINFO_XPOST;

typedef struct
{
    HINSTANCE instance;
    HWND window;
    HDC ctx;
    BITMAPINFO_XPOST *bitmap_info;
    HBITMAP bitmap;
    unsigned char *buf;
} PrivateData;


static
unsigned int _create_cont_opcode;

static LRESULT CALLBACK
_xpost_dev_win32_procedure(HWND   window,
                           UINT   message,
                           WPARAM window_param,
                           LPARAM data_param)
{
    switch (message)
    {
        default:
            return DefWindowProc(window, message, window_param, data_param);
    }
}

/* create an instance of the device
   using the class .copydict procedure */
static
int _create (Xpost_Context *ctx,
             Xpost_Object width,
             Xpost_Object height,
             Xpost_Object classdic)
{
    xpost_stack_push(ctx->lo, ctx->os, width);
    xpost_stack_push(ctx->lo, ctx->os, height);
    xpost_stack_push(ctx->lo, ctx->os, classdic);
    xpost_stack_push(ctx->lo, ctx->es,
                     operfromcode(_create_cont_opcode));
    xpost_stack_push(ctx->lo, ctx->es,
                     bdcget(ctx, classdic, consname(ctx, ".copydict")));

    return 0;
}

/* initialize the C-level data
   and define in the device instance */
static
int _create_cont (Xpost_Context *ctx,
                  Xpost_Object w,
                  Xpost_Object h,
                  Xpost_Object devdic)
{
    Xpost_Object privatestr;
    PrivateData private;
    integer width = w.int_.val;
    integer height = h.int_.val;
    WNDCLASSEX wc;
    RECT rect;

    privatestr = consbst(ctx, sizeof(PrivateData), NULL);
    bdcput(ctx, devdic, consname(ctx, "Private"), privatestr);
    bdcput(ctx, devdic, consname(ctx, "width"), w);
    bdcput(ctx, devdic, consname(ctx, "height"), h);

    private.instance = GetModuleHandle(NULL);

    memset (&wc, 0, sizeof (WNDCLASSEX));
    wc.cbSize = sizeof (WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = _xpost_dev_win32_procedure;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = private.instance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = "XPOST_DEV_WIN32";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
    {
        XPOST_LOG_ERR("RegisterClass() failed");
        FreeLibrary(private.instance);
        return -1; /* FIXME: what should I return ? */
    }

    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    if (!AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW | WS_SIZEBOX, FALSE, 0))
    {
        XPOST_LOG_ERR("AdjustWindowRect() failed");
        FreeLibrary(private.instance);
        UnregisterClass("XPOST_DEV_WIN32", private.instance);
        return -1; /* FIXME: what should I return ? */
    }

    private.window = CreateWindow("XPOST_DEV_WIN32", "",
                                  WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
                                  0, 0,
                                  rect.right - rect.left,
                                  rect.bottom - rect.top,
                                  NULL, NULL,
                                  private.instance, NULL);

    if (!private.window)
    {
        XPOST_LOG_ERR("CreateWindowEx() failed %ld", GetLastError());
        FreeLibrary(private.instance);
        UnregisterClass("XPOST_DEV_WIN32", private.instance);
        return -1; /* FIXME: what should I return ? */
    }

    ShowWindow(private.window, SW_SHOWNORMAL);
    if (!UpdateWindow(private.window))
    {
        XPOST_LOG_ERR("UpdateWindow() failed");
        DestroyWindow(private.window);
        FreeLibrary(private.instance);
        UnregisterClass("XPOST_DEV_WIN32", private.instance);
        return -1; /* FIXME: what should I return ? */
    }

    private.ctx = GetDC(private.window);
    if (!private.ctx)
    {
        XPOST_LOG_ERR("GetDC() failed");
        DestroyWindow(private.window);
        FreeLibrary(private.instance);
        UnregisterClass("XPOST_DEV_WIN32", private.instance);
        return -1; /* FIXME: what should I return ? */
    }

    private.bitmap_info = (BITMAPINFO_XPOST *)malloc(sizeof(BITMAPINFO_XPOST));
    if (!private.ctx)
    {
        XPOST_LOG_ERR("GetDC() failed");
        ReleaseDC(private.window, private.ctx);
        DestroyWindow(private.window);
        FreeLibrary(private.instance);
        UnregisterClass("XPOST_DEV_WIN32", private.instance);
        return -1; /* FIXME: what should I return ? */
    }

    private.bitmap_info->bih.biSize = sizeof(BITMAPINFOHEADER);
    private.bitmap_info->bih.biWidth = width;
    private.bitmap_info->bih.biHeight = -height;
    private.bitmap_info->bih.biPlanes = 1;
    private.bitmap_info->bih.biSizeImage = 4 * width * height;
    private.bitmap_info->bih.biXPelsPerMeter = 0;
    private.bitmap_info->bih.biYPelsPerMeter = 0;
    private.bitmap_info->bih.biClrUsed = 0;
    private.bitmap_info->bih.biClrImportant = 0;
    private.bitmap_info->bih.biBitCount = 32;
    private.bitmap_info->bih.biCompression = BI_BITFIELDS;
    private.bitmap_info->masks[0] = 0x00ff0000;
    private.bitmap_info->masks[1] = 0x0000ff00;
    private.bitmap_info->masks[2] = 0x000000ff;

    private.bitmap = CreateDIBSection(private.ctx,
                                      (const BITMAPINFO *)private.bitmap_info,
                                      DIB_RGB_COLORS,
                                      (void **)(&private.buf),
                                      NULL,
                                      0);
    if (!private.bitmap)
    {
        XPOST_LOG_ERR("GetDC() failed");
        free(private.bitmap_info);
        ReleaseDC(private.window, private.ctx);
        DestroyWindow(private.window);
        FreeLibrary(private.instance);
        UnregisterClass("XPOST_DEV_WIN32", private.instance);
        return -1; /* FIXME: what should I return ? */
    }

    xpost_memory_put(xpost_context_select_memory(ctx, privatestr),
            privatestr.comp_.ent, 0, sizeof private, &private);

    xpost_stack_push(ctx->lo, ctx->os, devdic);
    return 0;
}

static
int _putpix (Xpost_Context *ctx,
             Xpost_Object val,
             Xpost_Object x,
             Xpost_Object y,
             Xpost_Object devdic)
{
    Xpost_Object privatestr;
    PrivateData private;
    HDC dc;
    RECT rect;
    int w;
    int h;

    if (xpost_object_get_type(val) == realtype)
        val = xpost_cons_int(val.real_.val);
    if (xpost_object_get_type(x) == realtype)
        x = xpost_cons_int(x.real_.val);
    if (xpost_object_get_type(y) == realtype)
        y = xpost_cons_int(y.real_.val);

    privatestr = bdcget(ctx, devdic, consname(ctx, "Private"));
    xpost_memory_get(xpost_context_select_memory(ctx, privatestr),
            privatestr.comp_.ent, 0, sizeof private, &private);

    GetClientRect(private.window, &rect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;

    private.buf[y.int_.val * w * 4 + x.int_.val * 4 + 0] = 0;
    private.buf[y.int_.val * w * 4 + x.int_.val * 4 + 1] = 255;
    private.buf[y.int_.val * w * 4 + x.int_.val * 4 + 2] = 0;

    dc = CreateCompatibleDC(private.ctx);
    SelectObject(dc, private.bitmap);
    BitBlt(private.ctx, 0, 0, w, h,
           dc, 0, 0, SRCCOPY);
    DeleteDC(dc);

    return 0;
}

static
int _getpix (Xpost_Context *ctx,
             Xpost_Object x,
             Xpost_Object y,
             Xpost_Object devdic)
{
    Xpost_Object privatestr;
    PrivateData private;

    privatestr = bdcget(ctx, devdic, consname(ctx, "Private"));
    xpost_memory_get(xpost_context_select_memory(ctx, privatestr),
            privatestr.comp_.ent, 0, sizeof private, &private);

    /* ?? I don't know ...
       make a 1-pixel image and use copy_area?  ... */
    return 0;
}

static
int _emit (Xpost_Context *ctx,
           Xpost_Object devdic)
{
    Xpost_Object privatestr;
    PrivateData private;

    privatestr = bdcget(ctx, devdic, consname(ctx, "Private"));
    xpost_memory_get(xpost_context_select_memory(ctx, privatestr),
            privatestr.comp_.ent, 0, sizeof private, &private);

    UpdateWindow(private.window);

    return 0;
}

static
int _destroy (Xpost_Context *ctx,
              Xpost_Object devdic)
{
    Xpost_Object privatestr;
    PrivateData private;

    privatestr = bdcget(ctx, devdic, consname(ctx, "Private"));
    xpost_memory_get(xpost_context_select_memory(ctx, privatestr), privatestr.comp_.ent, 0,
            sizeof private, &private);

    free(private.bitmap_info);
    ReleaseDC(private.window, private.ctx);

    if (!UnregisterClass("XPOST_DEV_WIN32", private.instance))
        XPOST_LOG_INFO("UnregisterClass() failed");

    if (!FreeLibrary(private.instance))
        XPOST_LOG_INFO("FreeLibrary() failed");

    return 0;
}


static
int newwin32device (Xpost_Context *ctx,
                    Xpost_Object width,
                    Xpost_Object height)
{
    Xpost_Object classdic;
    int ret;

    xpost_stack_push(ctx->lo, ctx->os, width);
    xpost_stack_push(ctx->lo, ctx->os, height);
    ret = Aload(ctx, consname(ctx, "win32DEVICE"));
    if (ret)
        return ret;
    classdic = xpost_stack_topdown_fetch(ctx->lo, ctx->os, 0);
    xpost_stack_push(ctx->lo, ctx->es, bdcget(ctx, classdic, consname(ctx, "Create")));

    return 0;
}

static
unsigned int _loadwin32devicecont_opcode;

/* load PGMIMAGE
   load and call .copydict
   leaves copy on stack */
static
int loadwin32device (Xpost_Context *ctx)
{
    Xpost_Object classdic;
    int ret;

    ret = Aload(ctx, consname(ctx, "PGMIMAGE"));
    if (ret)
        return ret;
    classdic = xpost_stack_topdown_fetch(ctx->lo, ctx->os, 0);
    xpost_stack_push(ctx->lo, ctx->es, operfromcode(_loadwin32devicecont_opcode));
    xpost_stack_push(ctx->lo, ctx->es, bdcget(ctx, classdic, consname(ctx, ".copydict")));

    return 0;
}

/* replace procedures with operators */
static
int loadwin32devicecont (Xpost_Context *ctx,
                       Xpost_Object classdic)
{
    Xpost_Object userdict;
    Xpost_Object op;

    op = consoper(ctx, "win32CreateCont", _create_cont, 1, 3, integertype, integertype, dicttype);
    _create_cont_opcode = op.mark_.padw;
    op = consoper(ctx, "win32Create", _create, 1, 3, integertype, integertype, dicttype);
    bdcput(ctx, classdic, consname(ctx, "Create"), op);

    op = consoper(ctx, "win32PutPix", _putpix, 0, 4, numbertype, numbertype, numbertype, dicttype);
    bdcput(ctx, classdic, consname(ctx, "PutPix"), op);

    op = consoper(ctx, "win32GetPix", _getpix, 1, 3, numbertype, numbertype, dicttype);
    bdcput(ctx, classdic, consname(ctx, "GetPix"), op);

    op = consoper(ctx, "win32Emit", _emit, 0, 1, dicttype);
    bdcput(ctx, classdic, consname(ctx, "Emit"), op);

    op = consoper(ctx, "win32Destroy", _destroy, 0, 1, dicttype);
    bdcput(ctx, classdic, consname(ctx, "Destroy"), op);

    userdict = xpost_stack_bottomup_fetch(ctx->lo, ctx->ds, 2);

    bdcput(ctx, userdict, consname(ctx, "win32DEVICE"), classdic);

    op = consoper(ctx, "newwin32device", newwin32device, 1, 2, integertype, integertype);
    bdcput(ctx, userdict, consname(ctx, "newwin32device"), op);

    return 0;
}

int initwin32ops (Xpost_Context *ctx,
                  Xpost_Object sd)
{
    unsigned int optadr;
    oper *optab;
    Xpost_Object n,op;

    xpost_memory_table_get_addr(ctx->gl,
            XPOST_MEMORY_TABLE_SPECIAL_OPERATOR_TABLE, &optadr);
    optab = (oper *)(ctx->gl->base + optadr);
    op = consoper(ctx, "loadwin32device", loadwin32device, 1, 0); INSTALL;
    op = consoper(ctx, "loadwin32devicecont", loadwin32devicecont, 1, 1, dicttype);
    _loadwin32devicecont_opcode = op.mark_.padw;

    return 0;
}
