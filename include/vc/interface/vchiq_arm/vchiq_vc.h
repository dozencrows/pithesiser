/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VCHIQ_VC_H
#define VCHIQ_VC_H

#include "vchiq_core.h"

extern char _frdata[];
extern char *__MEMPOOL_START;
extern char *__MEMPOOL_END;

#define VCHIQ_IS_SAFE_DATA(x) (((char *)RTOS_ALIAS_NORMAL(x) >= _frdata) || \
   (((char *)RTOS_ALIAS_NORMAL(x) >= __MEMPOOL_START) && ((char *)RTOS_ALIAS_NORMAL(x) <= __MEMPOOL_END)))

#define VCOS_LOG_CATEGORY (&vchiq_vc_log_category)

#define VC_MEMCPY(d,s,l) do { if (l < DMA_MEMCPY_THRESHOLD) memcpy(d,s,l); else dma_memcpy(d,s,l); } while (0)

extern VCOS_LOG_CAT_T vchiq_vc_log_category;

typedef struct vchiq_instance_struct
{
   VCHIQ_STATE_T state;
   int connected;
} VCHIQ_INSTANCE_STRUCT_T;


typedef struct vchiq_vc_state_struct {
   VCOS_EVENT_T                remote_use_active;
   VCHIQ_REMOTE_USE_CALLBACK_T remote_use_callback;
   void*                       remote_use_cb_arg;
   int                         remote_use_sent;
   VCOS_MUTEX_T                remote_use_mutex;

   int                         suspending;
} VCHIQ_VC_STATE_T;


extern VCHIQ_INSTANCE_STRUCT_T vchiq_instances[];
extern int vchiq_num_instances;

extern VCHIQ_STATUS_T
vchiq_platform_init(void);

extern VCHIQ_STATUS_T
vchiq_vc_init_state(VCHIQ_VC_STATE_T* vc_state);

extern VCHIQ_VC_STATE_T*
vchiq_platform_get_vc_state(VCHIQ_STATE_T *state);

#endif
