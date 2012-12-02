/*
 * gfx.cpp
 *
 *  Created on: 2 Dec 2012
 *      Author: ntuckett
 */

#include "EGL/egl.h"
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "interface/vmcs_host/vc_vchi_dispmanx.h"
#include "bcm_host.h"

static EGLDisplay display;
static EGLContext context;
static EGLSurface surface;

static const EGLint attribute_list[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_NONE
};

static EGL_DISPMANX_WINDOW_T nativewindow;

static DISPMANX_DISPLAY_HANDLE_T dispman_display;
static DISPMANX_MODEINFO_T dispman_mode_info;

void gfx_init()
{
	bcm_host_init();

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (eglInitialize(display, NULL, NULL) == EGL_TRUE)
	{
		eglBindAPI(EGL_OPENVG_API);

		EGLConfig config;
		EGLint num_config;

		if (eglChooseConfig(display, attribute_list, &config, 1, &num_config) == EGL_TRUE)
		{
			context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
			dispman_display = vc_dispmanx_display_open(0);
			vc_dispmanx_display_get_info(dispman_display, &dispman_mode_info);

			VC_RECT_T dst_rect;
			VC_RECT_T src_rect;

			dst_rect.x = 0;
			dst_rect.y = 0;
			dst_rect.width = dispman_mode_info.width;
			dst_rect.height = dispman_mode_info.height;

			src_rect.x = 0;
			src_rect.y = 0;
			src_rect.width = dispman_mode_info.width << 16;
			src_rect.height = dispman_mode_info.height << 16;

			DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);
			DISPMANX_ELEMENT_HANDLE_T dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 0 /*layer */ ,
																					&dst_rect, 0 /*src */ , &src_rect,
																					DISPMANX_PROTECTION_NONE,
																					0 /*alpha */ , 0 /*clamp */ , 0 /*transform */ );

			nativewindow.element = dispman_element;
			nativewindow.width = dispman_mode_info.width;
			nativewindow.height = dispman_mode_info.height;
			vc_dispmanx_update_submit_sync(dispman_update);

			surface = eglCreateWindowSurface(display, config, &nativewindow, NULL);
			eglSurfaceAttrib(display, surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
			eglMakeCurrent(display, surface, surface, context);
			eglSwapInterval(display, 0);
		}
	}
}

void gfx_set_fill_colour(VGfloat *colour)
{
	VGPaint fillPaint = vgCreatePaint();
	vgSetParameteri(fillPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(fillPaint, VG_PAINT_COLOR, 4, colour);
	vgSetPaint(fillPaint, VG_FILL_PATH);
	vgDestroyPaint(fillPaint);
}

void gfx_set_stroke_colour(VGfloat *colour)
{
	VGPaint strokePaint = vgCreatePaint();
	vgSetParameteri(strokePaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(strokePaint, VG_PAINT_COLOR, 4, colour);
	vgSetPaint(strokePaint, VG_STROKE_PATH);
	vgDestroyPaint(strokePaint);
}

void gfx_set_stroke_width(VGfloat width)
{
	vgSetf(VG_STROKE_LINE_WIDTH, width);
	vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
	vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_MITER);
}

void gfx_test_render()
{
	VGfloat clear_colour[4] = { 0, 0, 0, 1 };
	VGfloat test_colour[4] = { 255, 0, 0, 1 };

	vgSetfv(VG_CLEAR_COLOR, 4, clear_colour);
	vgClear(0, 0, dispman_mode_info.width, dispman_mode_info.height);
	vgLoadIdentity();

	gfx_set_fill_colour(test_colour);
	gfx_set_stroke_colour(test_colour);
	gfx_set_stroke_width(0);

	VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
	vguRect(path, 0.f, 0.f, dispman_mode_info.width / 4, dispman_mode_info.height / 4);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);

	eglSwapBuffers(display, surface);
}

void gfx_test_render_tick()
{
	static int call_count = 0;
	VGfloat test_colour_1[4] = { 255, 0, 0, 1 };
	VGfloat test_colour_2[4] = { 0, 255, 0, 1 };
	VGfloat *test_colour;

	call_count++;

	if ((call_count & 4))
	{
		test_colour = test_colour_1;
	}
	else
	{
		test_colour = test_colour_2;
	}

//	vgClear(0, 0, dispman_mode_info.width, dispman_mode_info.height);
//	gfx_set_fill_colour(test_colour);
//
//	VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
//	vguRect(path, 0.f, 0.f, dispman_mode_info.width / 4, dispman_mode_info.height / 4);
//	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
//	vgDestroyPath(path);

	eglSwapBuffers(display, surface);
}
