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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <iostream>
#include <mutex>

#include <cuda.h>
#include <cudaGL.h>

#include "../Utils/NvCodecUtils.h"
#include "FramePresenter.h"

#include <thread>
#include <queue>

#pragma once

// Controls the number of OpenGL and CUDA resources to be created.
// Increasing this will increase the GPU memory utilization.
#define BUFFER_COUNT 2

// Singleton class to manage graphics resources
class FramePresenterGLX : public FramePresenter {

    CUgraphicsResource cuResource[BUFFER_COUNT];
    GLuint pbo[BUFFER_COUNT];                 /*!< Buffer object to upload texture data */
    GLuint tex[BUFFER_COUNT];                 /*!< OpenGL texture handle */
    GLuint program;

    // GLX resources
    Display *display;
    Window win;
    GLXContext ctx;
    GLXContext shared_ctx;
    Colormap cmap;

    CUcontext cuContext;

    int currentFrame;
    float totalWaitTime;

    NvThread renderingThread;

    public:

    FramePresenterGLX(int w, int h);

    ~FramePresenterGLX();

    void init(int w, int h, CUcontext context);

    // Following are pure virtual functions in FramePresenter class
    void initWindowSystem();
    void initOpenGLResources();
    void releaseWindowSystem();

    bool isVendorNvidia();
    void Render();

    bool GetDeviceFrameBuffer(CUdeviceptr*, int *);
    void ReleaseDeviceFrameBuffer();

    ConcurrentQueue<int> frameFeeder;

    Display*& getDisplay() {
        return this->display;
    }

    Window& getWindow() {
        return this->win;
    }

    GLXContext& getContext() {
        return this->ctx;
    }

    GLXContext& getSharedContext() {
        return this->shared_ctx;
    }

    Colormap& getColorMap() {
        return this->cmap;
    }

    GLuint* getPBO() {
        return this->pbo;
    }

    GLuint* getTextures() {
        return this->tex;
    }

    GLuint& getProgramObject() {
        return this->program;
    }

    void setDimensions(unsigned int w, unsigned int h) {

        this->width = w;
        this->height = h;
    }

    int getWidth() {
        return this->width;
    }

    int getHeight() {
        return this->height;
    }

    void mapBufferObject(CUdeviceptr*);
    void unmapBufferObject();
};
