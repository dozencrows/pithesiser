#
# For compilation, the following environment vars should be defined before running cmake:
# 	USERLANDPATH	- path to the root of the Pi userland files (from https://github.com/raspberrypi/userland.git)
#
# For cross-compilation, the following environment vars should be defined before running cmake:
# 	ROOTFSPATH		- path to the root of the Pi file system (for include and library file paths)
# 	TOOLPATH 	- path to the root of the cross-compiler (e.g. a path ending in "tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian" for the Linaro toolchain)
# 	TOOLPREFIX	- platform specific prefix for compiler tool names (e.g. arm-linux-gnueabihf-)
#
# Optional environment vars:
#	DEBUG		- if set to any value, will build debug version (no optimisation, full symbol information).
#

project(pithesiser)
cmake_minimum_required(VERSION 2.8)

set(ROOTFSPATH $ENV{ROOTFSPATH})
set(USERLANDPATH $ENV{USERLANDPATH})

execute_process(COMMAND uname -m OUTPUT_VARIABLE MACHINE)

if(NOT MACHINE MATCHES "arm*")
	set(TOOLPATH $ENV{TOOLPATH})
	set(TOOLPREFIX $ENV{TOOLPREFIX})
	set(CMAKE_C_COMPILER ${TOOLPATH}/bin/${TOOLPREFIX}gcc)
	set(CMAKE_ASM_COMPILER ${TOOLPATH}/bin/${TOOLPREFIX}gcc)
	set(CMAKE_SYSTEM_NAME Linux)

	include_directories(
						${ROOTFSPATH}/usr/include/arm-linux-gnueabihf
						${ROOTFSPATH}/usr/include
						${ROOTFSPATH}/usr/local/include
						)

	add_definitions(
					-march=armv6zk 
					-mfpu=vfp 
					-mfloat-abi=hard
					)
					
	link_directories(
					${TOOLPATH}/arm-linux-gnueabihf/libc/lib/arm-linux-gnueabihf
					${TOOLPATH}/arm-linux-gnueabihf/libc/usr/lib/arm-linux-gnueabihf
					${ROOTFSPATH}/lib/arm-linux-gnueabihf
					${ROOTFSPATH}/lib
					${ROOTFSPATH}/usr/lib/arm-linux-gnueabihf
					${ROOTFSPATH}/usr/lib
					${ROOTFSPATH}/usr/local/lib
					)					
endif()

enable_language(ASM)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
					${PROJECT_SOURCE_DIR}/dependencies/include
					${USERLANDPATH}
					${USERLANDPATH}/interface/khronos/include
					${USERLANDPATH}/interface/vcos
					${USERLANDPATH}/interface/vcos/pthreads
					${USERLANDPATH}/interface/vmcs_host/linux
					${USERLANDPATH}/interface/vmcs_host
					${USERLANDPATH}/host_applications/linux/libs/bcm_host/include
					)
					
add_definitions(	-DLOGGING_ENABLED
					-Wall 
					-fmessage-length=0 
					-std=gnu99
					)
					
if ($ENV{DEBUG})
	add_definitions(-O0 -g3 -D_DEBUG)
else()
	add_definitions(-O3)
endif()					
					
link_directories(
				${PROJECT_SOURCE_DIR}/dependencies/lib
				${ROOTFSPATH}/opt/vc/lib
				)
					
add_executable(pithesiser 	
				alsa.c
				code_timing_tests.c
				envelope.c
				error_handler.c
				filter_arm.s
				filter.c
				fixed_point_math.c
				float_filter.c
				float_waveform.c
				gfx.c
				gfx_envelope_render.c
				gfx_event.c
				gfx_font.c
				gfx_image.c
				gfx_setting_render.c
				gfx_wave_render.c
				lfo.c
				logging.c
				main.c
				master_time.c
				midi.c
				midi_controller.c
				midi_controller_parser.c
				mixer_arm.s
				mixer.c
				modulation_matrix.c
				modulation_matrix_controller.c
				oscillator.c
				piglow.c
				recording.c
				setting.c
				synth_controllers.c
				synth_model.c
				voice.c
				waveform.c
				waveform_procedural.c
				waveform_wavetable.c
				tests/filter_timing_test.c
				tests/mixer_timing_test.c
				tests/output_conversion_timing_test.c
				tests/waveform_timing_test.c
				)

target_link_libraries(pithesiser
						asound 
						pthread
						m
						stdc++
						z 
						png 
						GLESv2 
						vchiq_arm 
						bcm_host 
						EGL 
						OpenVG 
						vcos 
						rt 
						config 
						FLAC 
						ogg 
						vorbis 
						vorbisenc 
						sndfile 
						profiler 
						wiringPi 
						wiringPiDev 
						expat 
						log4c
						)

