/*
* Copyright 2017-2024 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/

#include "FramePresenterGLX.h"
#include <iostream>
#include <cstring>
#include <thread>

#include "../Utils/NvCodecUtils.h"
/**
 *   @brief  Constructor to initialize GLX, OpenGL and CUDA resources. Also launches rendering thread.
 *   @param  None
 *   @return None
 */
FramePresenterGLX::FramePresenterGLX(int w, int h) {

    frameFeeder.setSize(BUFFER_COUNT);

    currentFrame = 0;
    endOfDecoding = false;
    endOfRendering = false;

    cuResource[0] = 0;
    cuResource[1] = 0;

    // Initialize the width and height to be used in texture updates
    setDimensions(w, h);

    // Initliaze GLX window, context
    initWindowSystem();

    // Initialize OpenGL resources
    initOpenGLResources();

    for (int i = 0; i < BUFFER_COUNT; i++) {

        // Attach pbo to each of the cuda graphics resource
        ck(cuGraphicsGLRegisterBuffer(&cuResource[i], pbo[i], CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
    }

    // Create a thread handle using Render function of the class for execution
    renderingThread = NvThread(std::thread(&FramePresenterGLX::Render, this));
}

/**
 *   @brief  Check if valid NVIDIA libraries are installed.
 *   @param  None
 *   @return None
 */
bool FramePresenterGLX::isVendorNvidia() {

    char * vendor = (char*) glGetString(GL_VENDOR);

    if (!strcmp(vendor, "NVIDIA Corporation")) {
        return true;
    } else {
        return false;
    }
}

/**
 *   @brief  Create PBO and CUResource binding.
 *   @param  None
 *   @return None
 */
bool FramePresenterGLX::GetDeviceFrameBuffer(CUdeviceptr *dpFrame, int *pitch) {

    mapBufferObject(dpFrame);
    *pitch = getWidth()*4;

    return true;
}

/**
 *   @brief  Release the mapping so that, Rendering thread can consume the updated PBO.
 *   @param  None
 *   @return None
 */
void FramePresenterGLX::ReleaseDeviceFrameBuffer() {

    unmapBufferObject();
}

/**
 *   @brief  Binds CUResource to PBO and returns the CUdeviceptr that caller can work with.
 *   @param  None
 *   @return None
 */
void FramePresenterGLX::mapBufferObject(CUdeviceptr* dpBuffer) {

    ck(cuGraphicsMapResources(1, &cuResource[currentFrame], 0));

    size_t nSize = 0;
    ck(cuGraphicsResourceGetMappedPointer(dpBuffer, &nSize, cuResource[currentFrame]));
}

/**
 *   @brief  Unmap the binding between CUResource and PBO.
 *   @param  None
 *   @return None
 */
void FramePresenterGLX::unmapBufferObject() {

    ck(cuGraphicsUnmapResources(1, &cuResource[currentFrame], 0));

    frameFeeder.push_back(currentFrame);

    // CurrentFrame goes from between 0 and (BUFFER_COUNT - 1)
    currentFrame = (currentFrame + 1) % BUFFER_COUNT;
}

/**
 *   @brief  Function to render decoded frame using OpenGL calls.
 *           The PBO used here is populated by Decode function.
 *           Rendering is done using shared GLX context.
 *   @param  None
 *   @return None
 */
void FramePresenterGLX::Render() {

    int w = getWidth();
    int h = getHeight();

    glXMakeCurrent(display, win, shared_ctx);

    int currentRender = 0;

    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    glDisable(GL_DEPTH_TEST);

    while (!endOfDecoding) {

        currentRender = frameFeeder.front();

        // Bind OpenGL buffer object and upload the data
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[currentRender]);

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex[currentRender]);
        glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, 0);

        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

        glBegin(GL_QUADS);
        glTexCoord2f(0, (GLfloat)h);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f((GLfloat)w, (GLfloat)h);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f((GLfloat)w, 0);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0, 0);
        glVertex2f(-1.0f, 1.0f);
        glEnd();
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

        glXSwapBuffers(display, win);

        frameFeeder.pop_front();
    }

    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    glXMakeCurrent(display, 0, 0);

    endOfRendering = true;
}

/**
 *   @brief  Function to release GLX resources like display, window, context and colormap.
 *   @param  None
 *   @return None
 */
void FramePresenterGLX::releaseWindowSystem() {

    glXMakeCurrent(display, 0, 0);
    glXDestroyContext(display, ctx);
    glXDestroyContext(display, shared_ctx);

    XDestroyWindow(display, win);
    XFreeColormap(display, cmap);
    XCloseDisplay(display);
}

/**
 *   @brief  Function to create GLX resources like display, window, context and colormap.
 *           Exits the app in case any GLX API fails.
 *   @param  None
 *   @return None
 */
void FramePresenterGLX::initWindowSystem() {

    XInitThreads();

	XVisualInfo *visinfo;
	GLXFBConfig config;

    display = XOpenDisplay(NULL);
    if (display == NULL) {
		std::cout << "\nDisplay not found ! Make sure X server is running and DISPLAY environment variable set appropriately !\n";
		exit(1);
    }

    int configAttr[] = {
        GLX_CONFIG_CAVEAT   ,GLX_NONE,
        GLX_RENDER_TYPE     ,GLX_RGBA_BIT,
        GLX_RED_SIZE        , 8,
        GLX_GREEN_SIZE      , 8,
        GLX_BLUE_SIZE       , 8,
        GLX_ALPHA_SIZE      , 8,
        GLX_DEPTH_SIZE      , 24,
        GLX_STENCIL_SIZE    , 8,
        GLX_DOUBLEBUFFER    , True,
        None,
    };

    GLXFBConfig *configs = NULL;
    int numConfigs = 0;

	int screen = DefaultScreen(display);

    configs = glXChooseFBConfig(display, screen, configAttr, &numConfigs);
    if (numConfigs <= 0 || configs == NULL) {
        std::cout << "\nFailed to find a suitable GLXFBConfig!\n";
		exit(1);
    }

    config = configs[0];
    XFree(configs);

    visinfo = glXGetVisualFromFBConfig(display, config);
    if (!visinfo) {
        std::cout << "\nFailed to find a suitable visual!\n";
        exit(1);
    }

    Window root;
    XSetWindowAttributes wattr;
    int wattr_mask;

    root = RootWindow(display, screen);

    cmap = XCreateColormap(display, root, visinfo->visual, AllocNone);

    if (!cmap) {
        std::cout << "\nFailed to create colormap!\n";
        exit(1);
    }

    wattr_mask = CWBackPixmap | CWBorderPixel | CWColormap;
    wattr.background_pixmap = None;
    wattr.border_pixel = 0;
    wattr.bit_gravity = StaticGravity;
    wattr.colormap = cmap;

    win = XCreateWindow(display, root, 0, 0, 640, 480, 0,
                            visinfo->depth, InputOutput,
                            visinfo->visual, wattr_mask, &wattr);

    if (!win) {
        std::cout << "\nFailed to create window!\n";
		exit(1);
    }

	XMapWindow(display, win);

	ctx = glXCreateNewContext(display, config, GLX_RGBA_TYPE, 0, True);

	if (!ctx) {
		std::cout << "\nFailed to create GLX context !\n";
		exit(1);
	}

    shared_ctx = glXCreateNewContext(display, config, GLX_RGBA_TYPE, ctx, True);

	if (!shared_ctx) {
		std::cout << "\nFailed to create shared GLX context !\n";
		exit(1);
	}

    glXMakeCurrent(display, win, ctx);
}

/**
 *   @brief  Function to create OpenGL resources viz. pixel buffer objects, textures, program objects.
 *   @param  None
 *   @return None
 */
void FramePresenterGLX::initOpenGLResources() {

    glewInit();

    GLuint &shader = getProgramObject();

    glGenTextures(BUFFER_COUNT, tex);
    glGenBuffers(BUFFER_COUNT, pbo);

    for (int i=0; i<BUFFER_COUNT; i++) {

        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo[i]);
        glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, width * height * 4, NULL, GL_DYNAMIC_DRAW_ARB);
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    }

    // Create and initialize fragment program
    static const char *code =
        "!!ARBfp1.0\n"
        "TEX result.color, fragment.texcoord, texture[0], RECT; \n"
        "END";

    glGenProgramsARB(1, &shader);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader);
    glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(code), (GLubyte *)code);
}

/**
 *   @brief  Function to release OpenGL resources viz. pixel buffer objects, textures, program objects.
 *   @param  None
 *   @return None
 */
FramePresenterGLX::~FramePresenterGLX() {

    endOfDecoding = true;

    // Don't release resources till rendering is finished.
    while (!endOfRendering) {
    }

    renderingThread.join();

    for (int i = 0; i < BUFFER_COUNT; i++) {
        ck(cuGraphicsUnregisterResource(cuResource[i]));
    }

    GLuint &shader = getProgramObject();

    // Release OpenGL resources
    glDeleteBuffersARB(BUFFER_COUNT, pbo);
    glDeleteTextures(BUFFER_COUNT, tex);
    glDeleteProgramsARB(1, &shader);

    // Release GLX resources
    releaseWindowSystem();
}


