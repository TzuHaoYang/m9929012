/****************************************************************************
*
*  <:copyright-BRCM:2019:proprietary:standard
*  
*     Copyright (c) 2019 Broadcom 
*     All Rights Reserved
*  
*   This program is the proprietary software of Broadcom and/or its
*   licensors, and may only be used, duplicated, modified or distributed pursuant
*   to the terms and conditions of a separate, written license agreement executed
*   between you and Broadcom (an "Authorized License").  Except as set forth in
*   an Authorized License, Broadcom grants no license (express or implied), right
*   to use, or waiver of any kind with respect to the Software, and Broadcom
*   expressly reserves all rights in and to the Software and all intellectual
*   property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*   NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*   BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
*  
*   Except as expressly set forth in the Authorized License,
*  
*   1. This program, including its structure, sequence and organization,
*      constitutes the valuable trade secrets of Broadcom, and you shall use
*      all reasonable efforts to protect the confidentiality thereof, and to
*      use this information only in connection with your use of Broadcom
*      integrated circuit products.
*  
*   2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*      AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*      WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*      RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*      ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*      FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*      COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*      TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*      PERFORMANCE OF THE SOFTWARE.
*  
*   3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*      ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*      INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*      WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*      IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*      OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*      SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*      SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*      LIMITED REMEDY.
*  :>
****************************************************************************
*
*  Filename: bosSemLinuxUser.c
*
****************************************************************************
*  Description:
*
*
****************************************************************************/
/**
*
*  @file    bosSemLiteLinuxUser.c
*
*  @brief   LinuxUser implementation of Fast BOS Semaphore Module. Execution time of this implementation is less than normal semaphores. 
*
****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */

#include <bosCfg.h>


#include <bosSem.h>
#include <bosSemPrivate.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
//#include <bosTypes.h>



/* ---- Public Variables ------------------------------------------------- */
/* ---- Private Constants and Types -------------------------------------- */
/* ---- Private Variables ------------------------------------------------ */
/* ---- Private Function Prototypes -------------------------------------- */
/* ---- Functions -------------------------------------------------------- */

/***************************************************************************/

BOS_STATUS bosSemLiteCreate
(
   const char *name,
   int         initCount,
   int         maxCount,
   BOS_SEMLITE    *sem
)
{
   (void)name;
   (void)maxCount;

   sem->value = initCount;
   if (!pthread_mutex_init(&sem->lock, NULL))
   {
       pthread_cond_init(&sem->cond, NULL);
   }

   return BOS_STATUS_OK;

} /* bosSemLiteCreate */

/***************************************************************************/

BOS_STATUS bosSemLiteDestroy( BOS_SEMLITE *sem )
{
   sem->value = 0;
   pthread_mutex_destroy(&sem->lock);
   pthread_cond_destroy (&sem->cond);

   return BOS_STATUS_OK;
} /* bosSemLiteDestroy */

/***************************************************************************/

BOS_STATUS bosSemLiteTake( BOS_SEMLITE *sem )
{
   pthread_mutex_lock(&sem->lock);
   sem->value--;
   if( sem->value < 0)
   {
      pthread_cond_wait(&sem->cond, &sem->lock);
   }
   pthread_mutex_unlock(&sem->lock);

   return BOS_STATUS_OK;

} /* bosSemLiteTake */

/***************************************************************************/

BOS_STATUS bosSemLiteTimedTake( BOS_SEMLITE *sem, BOS_TIME_MS timeoutMsec )
{

   struct timespec   ts;
   struct timeval    tp;

   pthread_mutex_lock(&sem->lock);
   gettimeofday(&tp, NULL);
   ts.tv_nsec = (tp.tv_usec * 1000) + ((timeoutMsec % 1000) * 1000000);
   ts.tv_sec  = tp.tv_sec + (timeoutMsec/1000) + (ts.tv_nsec / 1000000000L);
   ts.tv_nsec = ts.tv_nsec % 1000000000L; 
   sem->value--;
   if( sem->value < 0)
   {
      pthread_cond_timedwait(&sem->cond, &sem->lock, &ts);
   }
   pthread_mutex_unlock(&sem->lock);

   return BOS_STATUS_OK;
} /* bosSemLiteTimedTake */

/***************************************************************************/

BOS_STATUS bosSemLiteGive( BOS_SEMLITE *sem )
{

   pthread_mutex_lock(&sem->lock);
   sem->value++;
   pthread_cond_signal(&sem->cond);
   pthread_mutex_unlock(&sem->lock);

   return BOS_STATUS_OK;
} /* bosSemLiteGive */




