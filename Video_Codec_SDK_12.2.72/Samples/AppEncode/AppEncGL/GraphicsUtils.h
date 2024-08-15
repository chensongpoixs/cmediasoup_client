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

#pragma once

/**
* @brief Set up graphics resources targeting the specified context type
* The input argument to this function must be either "glx" or "egl".
*/
int GraphicsSetupWindow(const char *);

/**
* @brief Tear down graphics resources targeting the specified context type
* The input argument to this function must be the same as that used for the
* GraphicsSetupWindow() call.
*/
void GraphicsCloseWindow(const char *);
