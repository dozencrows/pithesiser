/*
 * gfx.cpp
 *
 *  Created on: 2 Dec 2012
 *      Author: ntuckett
 */

#define _GNU_SOURCE

#include "gfx.h"
#include "gfx_event.h"
#include "gfx_event_types.h"
#include "gfx_font.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include "EGL/egl.h"
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "interface/vmcs_host/vc_vchi_dispmanx.h"
#include "bcm_host.h"
#include "master_time.h"

#define PNG_DEBUG 3
#include "libpng/png.h"

#define FRAME_TIME_DELAY_US	16667
#define MAX_EGL_CONFIGS 32

//---------------------------------------------------------------
// EGL support
//

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

void gfx_openvg_init()
{
	VGfloat clear_colour[4] = { 0, 0, 0, 1 };
	vgSetfv(VG_CLEAR_COLOR, 4, clear_colour);
	vgClear(0, 0, dispman_mode_info.width, dispman_mode_info.height);
	vgLoadIdentity();
}

//---------------------------------------------------------------
// Rendering support
//
#include "gfx_font_resources.inc"

font_info_t	gfx_font_sans;

void gfx_init_fonts()
{
	gfx_font_sans = gfx_load_font(
					DejaVuSans_glyphPoints,
					DejaVuSans_glyphPointIndices,
					DejaVuSans_glyphInstructions,
					DejaVuSans_glyphInstructionIndices,
					DejaVuSans_glyphInstructionCounts,
					DejaVuSans_glyphAdvances,
					DejaVuSans_characterMap,
					DejaVuSans_glyphCount
					);
}

void gfx_deinit_fonts()
{
	gfx_unload_font(&gfx_font_sans);
}

void gfx_take_screenshot(FILE* outfile)
{
	size_t w = dispman_mode_info.width;
	size_t h = dispman_mode_info.height;
	size_t buffer_size = w * h * 4;
	void* screen_buffer = malloc(buffer_size);
	vgReadPixels(screen_buffer, w * 4, VG_sABGR_8888, 0, 0, w, h);

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, outfile);
	png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	for(int y = h - 1; y >= 0; y--)
	{
		png_write_row(png_ptr, (png_bytep)screen_buffer + (y * w * 4));
	}

	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free(screen_buffer);
}

//---------------------------------------------------------------
// Gfx processing thread

static int32_t total_frames = 0;
static int32_t render_exec_time;
static int32_t render_idle_time;
static size_t frame_progress = 0;
static size_t frame_complete_threshold = 0;
static int trigger_screenshot = 0;
static char screenshot_path[PATH_MAX];

void *gfx_thread()
{
	pthread_setname_np(gfx_thread_handle, "pithesiser-gfx");

	gfx_egl_init();
	gfx_openvg_init();
	gfx_init_fonts();
	sem_post(&gfx_init_semaphore);

	gfx_event_t postinit_event;
	postinit_event.type = GFX_EVENT_POSTINIT;
	postinit_event.flags = 0;
	postinit_event.receiver_id = GFX_ANY_OBJECT;
	gfx_process_event(&postinit_event);

	render_exec_time = 0;
	render_idle_time = 0;
	total_frames = 0;
	frame_progress = 0;

	while (1)
	{
		int32_t idle_start_timestamp = get_elapsed_time_ms();
		gfx_wait_for_event();
		int32_t idle_end_timestamp = get_elapsed_time_ms();
		render_idle_time += idle_end_timestamp - idle_start_timestamp;

		gfx_event_t event;
		gfx_pop_event(&event);
		gfx_process_event(&event);

		if (event.flags & GFX_EVENT_FLAG_OWNPTR)
		{
			free(event.ptr);
			event.ptr = NULL;
		}

		if (frame_progress >= frame_complete_threshold)
		{
			gfx_event_t swap_event;
			swap_event.type = GFX_EVENT_BUFFERSWAP;
			swap_event.flags = 0;
			swap_event.receiver_id = GFX_ANY_OBJECT;
			gfx_process_event(&swap_event);
			eglSwapBuffers(display, surface);
			total_frames++;
			frame_progress = 0;

			if (trigger_screenshot)
			{
				if (screenshot_path != NULL)
				{
					FILE* screenshot_file = fopen(screenshot_path, "wb");
					gfx_take_screenshot(screenshot_file);
					fclose(screenshot_file);
				}

				trigger_screenshot = 0;
			}
		}

		int32_t exec_end_timestamp = get_elapsed_time_ms();
		int32_t render_elapsed = exec_end_timestamp - idle_end_timestamp;
		render_exec_time += render_elapsed;
	}

	gfx_deinit_fonts();

	return NULL;
}

//---------------------------------------------------------------
// Public interface
//
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

void gfx_get_screen_resolution(int *width, int *height)
{
	if (width != NULL)
	{
		*width = dispman_mode_info.width;
	}

	if (height != NULL)
	{
		*height = dispman_mode_info.height;
	}
}

void gfx_set_frame_complete_threshold(size_t complete_threshold)
{
	frame_complete_threshold = complete_threshold;
}

size_t gfx_get_frame_complete_threshold()
{
	return frame_complete_threshold;
}

void gfx_advance_frame_progress(size_t progress_delta)
{
	frame_progress += progress_delta;
}

void gfx_set_frame_progress(size_t progress)
{
	frame_progress = progress;
}

void gfx_screenshot(const char* screenshot_file_path)
{
	strncpy(screenshot_path, screenshot_file_path, PATH_MAX);
	trigger_screenshot = 1;
}

