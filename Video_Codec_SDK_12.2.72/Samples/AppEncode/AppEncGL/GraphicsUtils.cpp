////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2024 NVIDIA Corporation.  All rights reserved.
//
// Please refer to the NVIDIA end user license agreement (EULA) associated
// with this source code for terms and conditions that govern your use of
// this software. Any use, reproduction, disclosure, or distribution of
// this software and related documentation outside the terms of the EULA
// is strictly prohibited.
//
////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string.h>
#include <X11/Xlib.h>
#include <EGL/egl.h>
#include "../Utils/Logger.h"
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

void showErrorAndExit (const char* message)
{
    bool bThrowError = false;
    std::ostringstream oss;
    if (message)
    {
        bThrowError = true;
        oss << message << std::endl;
    }

    if (bThrowError)
    {
        throw std::invalid_argument(oss.str());
    }
    else
    {
        std::cout << oss.str();
        exit(0);
    }
}

// egl resources
static Display *display;
static int screen;
static Window win = 0;

static EGLDisplay eglDisplay = EGL_NO_DISPLAY;
static EGLSurface eglSurface = EGL_NO_SURFACE;
static EGLContext eglContext = EGL_NO_CONTEXT;

// glut window handle
static int window;

// Initialization function to create a simple X window and associate EGLContext and EGLSurface with it
bool SetupEGLResources(int xpos, int ypos, int width, int height, const char *windowname)
{
    bool status = true;

    EGLint configAttrs[] =
    {
        EGL_RED_SIZE,        1,
        EGL_GREEN_SIZE,      1,
        EGL_BLUE_SIZE,       1,
        EGL_DEPTH_SIZE,      16,
        EGL_SAMPLE_BUFFERS,  0,
        EGL_SAMPLES,         0,
        EGL_CONFORMANT,      EGL_OPENGL_BIT,
        EGL_NONE
    };

    EGLint contextAttrs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    EGLint windowAttrs[] = {EGL_NONE};
    EGLConfig* configList = NULL;
    EGLint configCount;

    display = XOpenDisplay(NULL);

    if (!display)
    {
        std::cout << "\nError opening X display\n";
        return false;
    }

    screen = DefaultScreen(display);

    eglDisplay = eglGetDisplay(display);

    if (eglDisplay == EGL_NO_DISPLAY)
    {
        std::cout << "\nEGL : failed to obtain display\n";
        return false;
    }

    if (!eglInitialize(eglDisplay, 0, 0))
    {
        std::cout << "\nEGL : failed to initialize\n";
        return false;
    }

    if (!eglChooseConfig(eglDisplay, configAttrs, NULL, 0, &configCount) || !configCount)
    {
        std::cout << "\nEGL : failed to return any matching configurations\n";
        return false;
    }

    configList = (EGLConfig*)malloc(configCount * sizeof(EGLConfig));

    if (!eglChooseConfig(eglDisplay, configAttrs, configList, configCount, &configCount) || !configCount)
    {
        std::cout << "\nEGL : failed to populate configuration list\n";
        status = false;
        goto end;
    }

    win = XCreateSimpleWindow(display, RootWindow(display, screen),
                              xpos, ypos, width, height, 0,
                              BlackPixel(display, screen),
                              WhitePixel(display, screen));

    eglSurface = eglCreateWindowSurface(eglDisplay, configList[0],
                                        (EGLNativeWindowType) win, windowAttrs);

    if (!eglSurface)
    {
        std::cout << "\nEGL : couldn't create window surface\n";
        status = false;
        goto end;
    }

    eglBindAPI(EGL_OPENGL_API);

    eglContext = eglCreateContext(eglDisplay, configList[0], NULL, contextAttrs);

    if (!eglContext)
    {
        std::cout << "\nEGL : couldn't create context\n";
        status = false;
        goto end;
    }

    if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
    {
        std::cout << "\nEGL : couldn't make context/surface current\n";
        status = false;
        goto end;
    }

end:
    free(configList);
    return status;
}

// Cleanup function to destroy the window and context
void DestroyEGLResources()
{
    if (eglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (eglContext != EGL_NO_CONTEXT)
        {
            eglDestroyContext(eglDisplay, eglContext);
        }

        if (eglSurface != EGL_NO_SURFACE)
        {
            eglDestroySurface(eglDisplay, eglSurface);
        }

        eglTerminate(eglDisplay);
    }

    if (win)
    {
        XDestroyWindow(display, win);
    }

    XCloseDisplay(display);
}

bool SetupGLXResources()
{
    int argc = 1;
    char *argv[1] = {(char*)"dummy"};

    // Use glx context/surface
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
    glutInitWindowSize(16, 16);

    window = glutCreateWindow("AppEncGL");
    if (!window)
    {
        std::cout << "\nUnable to create GLUT window.\n" << std::endl;
        return false;
    }

    glutHideWindow();
    return true;
}

// Cleanup function to destroy glut resources
void DestroyGLXResources()
{
    glutDestroyWindow(window);
}

void GraphicsCloseWindow(const char* contextType)
{
    if (!strcmp(contextType, "egl"))
    {
        DestroyEGLResources();
    }
    else if (!strcmp(contextType, "glx"))
    {
        DestroyGLXResources();
    }
    else
    {
        std::cout << "\nInvalid context type specified.\n";
    }
}

void GraphicsSetupWindow(const char* contextType)
{
    if (!strcmp(contextType, "egl"))
    {
        // Use egl context/surface
        if (!SetupEGLResources(0, 0, 16, 16, "AppEncGL"))
        {
            showErrorAndExit("\nFailed to setup window.\n");
        }
    }
    else if (!strcmp(contextType, "glx"))
    {
        // Use glx context/surface
        if (!SetupGLXResources())
        {
            showErrorAndExit("\nFailed to setup window.\n");
        }
    }
    else
    {
        showErrorAndExit("\nInvalid context type specified.\n");
    }

    char * vendor = (char*) glGetString(GL_VENDOR);
    if (strcmp(vendor, "NVIDIA Corporation"))
    {
        GraphicsCloseWindow(contextType);
        showErrorAndExit("\nFailed to find NVIDIA libraries\n");
    }
}
