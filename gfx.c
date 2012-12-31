/*
 * gfx.cpp
 *
 *  Created on: 2 Dec 2012
 *      Author: ntuckett
 */

#include "gfx.h"
#include "gfx_event.h"
#include "gfx_event_types.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "EGL/egl.h"
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "interface/vmcs_host/vc_vchi_dispmanx.h"
#include "bcm_host.h"
#include "master_time.h"

#define FRAME_TIME_DELAY_US	16667
#define MAX_EGL_CONFIGS 32

static EGLDisplay display;
static EGLContext context;
static EGLSurface surface;

EGLConfig get_desired_egl_config(int red_bits_wanted, int green_bits_wanted, int blue_bits_wanted)
{
	static EGLint attribute_list[] = {
		EGL_RED_SIZE, 0,
		EGL_GREEN_SIZE, 0,
		EGL_BLUE_SIZE, 0,
		EGL_ALPHA_SIZE, 0,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_CONFORMANT, EGL_OPENVG_BIT,
		EGL_NONE
	};

	EGLint num_egl_configs;
	EGLConfig found_config = NULL;

	attribute_list[1] = red_bits_wanted;
	attribute_list[3] = green_bits_wanted;
	attribute_list[5] = blue_bits_wanted;

	if (eglChooseConfig(display, attribute_list, NULL, 0, &num_egl_configs) == EGL_TRUE)
	{
		EGLConfig *egl_configs = (EGLConfig*)alloca(sizeof(EGLConfig) * num_egl_configs);

		if (eglChooseConfig(display, attribute_list, egl_configs, num_egl_configs, &num_egl_configs) == EGL_TRUE)
		{
			for (int i = 0; i < num_egl_configs; i++) {
				EGLint red_bits, green_bits, blue_bits;
				eglGetConfigAttrib(display, egl_configs[i], EGL_RED_SIZE, &red_bits);
				eglGetConfigAttrib(display, egl_configs[i], EGL_GREEN_SIZE, &green_bits);
				eglGetConfigAttrib(display, egl_configs[i], EGL_BLUE_SIZE, &blue_bits);

				if (red_bits == red_bits_wanted && green_bits == green_bits_wanted && blue_bits == blue_bits_wanted)
				{
					found_config = egl_configs[i];
					break;
				}
			}
		}
	}

	return found_config;
}

static EGL_DISPMANX_WINDOW_T nativewindow;

static DISPMANX_DISPLAY_HANDLE_T dispman_display;
static DISPMANX_MODEINFO_T dispman_mode_info;

pthread_t	gfx_thread_handle;
sem_t		gfx_init_semaphore;

void gfx_egl_init()
{
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (eglInitialize(display, NULL, NULL) == EGL_TRUE)
	{
		eglBindAPI(EGL_OPENVG_API);

		EGLConfig egl_config = get_desired_egl_config(5, 6, 5);

		if (egl_config != NULL)
		{
			context = eglCreateContext(display, egl_config, EGL_NO_CONTEXT, NULL);
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

			surface = eglCreateWindowSurface(display, egl_config, &nativewindow, NULL);
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

//void gfx_test_render()
//{
//	VGfloat clear_colour[4] = { 0, 0, 0, 1 };
//	VGfloat test_colour[4] = { 255, 0, 0, 1 };
//
//	vgSetfv(VG_CLEAR_COLOR, 4, clear_colour);
//	vgClear(0, 0, dispman_mode_info.width, dispman_mode_info.height);
//	vgLoadIdentity();
//	vgScale(dispman_mode_info.width, dispman_mode_info.height);
//
//	gfx_set_fill_colour(test_colour);
//	gfx_set_stroke_colour(test_colour);
//	gfx_set_stroke_width(0);
//
//	VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
//	vguRect(path, 0.f, 0.f, 0.25f, 0.25f);
//	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
//	vgDestroyPath(path);
//
//	eglSwapBuffers(display, surface);
//}
//
//void gfx_test_render_tick()
//{
//	static int call_count = 0;
//	VGfloat test_colour_1[4] = { 255, 0, 0, 1 };
//	VGfloat test_colour_2[4] = { 0, 255, 0, 1 };
//	VGfloat *test_colour;
//
//	call_count++;
//
//	if ((call_count & 4))
//	{
//		test_colour = test_colour_1;
//	}
//	else
//	{
//		test_colour = test_colour_2;
//	}
//
//	vgClear(0, 0, dispman_mode_info.width, dispman_mode_info.height);
//	gfx_set_fill_colour(test_colour);
//
//	VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
//	vguRect(path, 0.f, 0.f, 0.25f, 0.25f);
//	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
//	vgDestroyPath(path);
//}

//---------------------------------------------------------------
// Gfx processing thread

static int32_t total_frames = 0;
static int32_t render_exec_time;
static int32_t render_idle_time;

void *gfx_thread()
{
	gfx_egl_init();
	sem_post(&gfx_init_semaphore);

	render_exec_time = 0;
	render_idle_time = 0;
	total_frames = 0;

	while (1)
	{
		int gfx_events = gfx_get_event_count();

		int32_t start_timestamp = get_elapsed_time_ms();
		while (gfx_events-- > 0)
		{
			gfx_event_t event;
			gfx_pop_event(&event);
			gfx_handle_event(&event);
		}

		gfx_event_t swap_event;
		swap_event.type = GFX_EVENT_BUFFERSWAP;
		gfx_handle_event(&swap_event);

		eglSwapBuffers(display, surface);
		int32_t end_timestamp = get_elapsed_time_ms();
		total_frames++;

		int32_t render_elapsed = end_timestamp - start_timestamp;
		render_exec_time += render_elapsed;
		useconds_t elapsed_us = render_elapsed * 1000;

		if (elapsed_us < FRAME_TIME_DELAY_US)
		{
			useconds_t idle_us = FRAME_TIME_DELAY_US - elapsed_us;
			render_idle_time += idle_us / 1000;
			usleep(idle_us);
		}
	}

	return NULL;
}

void gfx_initialise()
{
	bcm_host_init();
	sem_init(&gfx_init_semaphore, 0, 0);
	pthread_create(&gfx_thread_handle, NULL, gfx_thread, NULL);
	sem_wait(&gfx_init_semaphore);
}

void gfx_deinitialise()
{
	pthread_cancel(gfx_thread_handle);
	pthread_join(gfx_thread_handle, NULL);

	int32_t render_elapsed = render_exec_time + render_idle_time;

	printf("Render time: %d ms\n", render_elapsed);
	printf("  exec: %d ms  idle: %d ms\n", render_exec_time, render_idle_time);
	printf("Render frame rate: %f\n", (float)(total_frames * 1000) / (float)render_elapsed);
}
