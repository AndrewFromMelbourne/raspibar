//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2014 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "bcm_host.h"

//-----------------------------------------------------------------------

#define NDEBUG

//-----------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

//-----------------------------------------------------------------------

const char* program = NULL;

//-----------------------------------------------------------------------

int main(int argc, char *argv[])
{
    program = basename(argv[0]);

    if (argc == 1)
    {
        fprintf(stderr, "Usage: %s program arguments ...\n\n", program);
        fprintf(stderr, "%s: you must specify a program to run\n", program);
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T displayHandle = vc_dispmanx_display_open(0);
    assert(displayHandle != 0);

    DISPMANX_MODEINFO_T modeInfo;
    int result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);
    assert(result == 0);

    //---------------------------------------------------------------------

    uint32_t vc_image_ptr;
    uint32_t background = 0;

    DISPMANX_RESOURCE_HANDLE_T bgResource =
        vc_dispmanx_resource_create(VC_IMAGE_RGB565,
                                    1,
                                    1,
                                    &vc_image_ptr);
    assert(bgResource != 0);

    //---------------------------------------------------------------------

    VC_RECT_T src_rect;
    VC_RECT_T dst_rect;


    vc_dispmanx_rect_set(&dst_rect, 0, 0, 1, 1);

    result = vc_dispmanx_resource_write_data(bgResource,
                                             VC_IMAGE_RGB565,
                                             sizeof(background),
                                             &background,
                                             &dst_rect);
    assert(result == 0);

    //-------------------------------------------------------------------

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
    assert(update != 0);

    //---------------------------------------------------------------------

    VC_DISPMANX_ALPHA_T alpha;
    alpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
    alpha.opacity = 255;
    alpha.mask = 0;

    //---------------------------------------------------------------------

    vc_dispmanx_rect_set(&src_rect, 0, 0, 1, 1);
    vc_dispmanx_rect_set(&dst_rect, 0, 0, 0, 0);

    DISPMANX_ELEMENT_HANDLE_T bgElement = 
        vc_dispmanx_element_add(update,
                                displayHandle,
                                0,
                                &dst_rect,
                                bgResource,
                                &src_rect,
                                DISPMANX_PROTECTION_NONE,
                                &alpha,
                                NULL,
                                DISPMANX_NO_ROTATE);
    assert(bgElement != 0);

    //---------------------------------------------------------------------

    result = vc_dispmanx_update_submit_sync(update);
    assert(result == 0);

    //---------------------------------------------------------------------

    pid_t pid = fork();
    int status;

    if (pid == -1)
    {
        perror("Error: calling fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        execvp(argv[1], argv + 1);
        perror("Error: calling exec");
        _exit(EXIT_FAILURE);
    }
    else
    {
        waitpid(pid, &status, 0);
    }

    //---------------------------------------------------------------------

    update = vc_dispmanx_update_start(0);
    assert(update != 0);
    result = vc_dispmanx_element_remove(update, bgElement);
    assert(result == 0);
    result = vc_dispmanx_update_submit_sync(update);
    assert(result == 0);

    //-------------------------------------------------------------------

    return status;
}

