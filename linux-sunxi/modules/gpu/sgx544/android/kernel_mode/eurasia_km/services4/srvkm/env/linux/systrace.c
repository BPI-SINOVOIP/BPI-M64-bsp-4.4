/*************************************************************************/ /*!
@Title          Systrace related functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include "systrace.h"

#define CREATE_TRACE_POINTS
#include <trace/events/gpu.h>

#include <linux/debugfs.h>
#include <linux/sched.h>
#include <linux/fs.h>

#include "img_types.h"

#if defined(EUR_CR_TIMER)

/*Kernel debugfs variables*/
#define PVRSRV_SYSTRACE_TIMEINDEX_LIMIT 32
static unsigned int capture_hwperfdata;
static struct dentry *pvrdir_ret;

static PVRSRV_SYSTRACE_ERROR CreateJob(PVRSRV_SYSTRACE_DATA *psSystraceData, IMG_UINT32 ui32PID, IMG_UINT32 ui32FrameNum, IMG_UINT32 ui32RTData)
{
	PVRSRV_SYSTRACE_CONTEXT *psContext = NULL;
	PVRSRV_SYSTRACE_JOB 	*psJob = NULL;
	IMG_UINT32 i = 0;
	
	if(psSystraceData == NULL)
		return PVRSRV_SYSTRACE_NOT_INITIALISED;
	
	/*Look for the PID in the context CB*/
	for(i = 0; i < 8; ++i)
	{
		if(psSystraceData->asSystraceContext[i].ui32PID == ui32PID)
		{
			psContext = &(psSystraceData->asSystraceContext[i]);
			break;
		}
	}

	/*If we find it lets check its jobs, otherwise we create it*/
	if(psContext == NULL)
	{
		psSystraceData->ui32Index = (psSystraceData->ui32Index+1)%8;
		
		psSystraceData->asSystraceContext[psSystraceData->ui32Index].ui32CtxID = psSystraceData->ui32CurrentCtxID;
		++psSystraceData->ui32CurrentCtxID;
		psSystraceData->asSystraceContext[psSystraceData->ui32Index].ui32PID = ui32PID;
		psSystraceData->asSystraceContext[psSystraceData->ui32Index].ui32Start = 0;
		psSystraceData->asSystraceContext[psSystraceData->ui32Index].ui32End = 0;
		psSystraceData->asSystraceContext[psSystraceData->ui32Index].ui32CurrentJobID = 0;
		
		psContext = &(psSystraceData->asSystraceContext[psSystraceData->ui32Index]);
	}

	/*This is just done during the first kick so it must not be in the job list*/
	/*JobID not found, we create it*/
	psJob = &(psContext->asJobs[psContext->ui32End]);
	psJob->ui32JobID = psContext->ui32CurrentJobID;
	++psContext->ui32CurrentJobID;
	
	psJob->ui32FrameNum = ui32FrameNum;
	psJob->ui32RTData = ui32RTData;
	/*Advance the CB*/
	psContext->ui32End = (psContext->ui32End + 1)%16;
	if(psContext->ui32End == psContext->ui32Start)
		psContext->ui32Start = (psContext->ui32Start + 1)%16;

	return PVRSRV_SYSTRACE_OK;
}

static PVRSRV_SYSTRACE_ERROR GetCtxAndJobID(PVRSRV_SYSTRACE_DATA *psSystraceData, IMG_UINT32 ui32PID, IMG_UINT32 ui32FrameNum, IMG_UINT32 ui32RTData, 
					IMG_UINT32 *pui32CtxID, IMG_UINT32 *pui32JobID)
{
	PVRSRV_SYSTRACE_CONTEXT *psContext = NULL;
	//PVRSRV_SYSTRACE_JOB 	*psJob = NULL;
	IMG_UINT32 i = 0;
	
	if(psSystraceData == NULL)
		return PVRSRV_SYSTRACE_NOT_INITIALISED;
	
	/*Look for the PID in the context CB*/
	for(i = 0; i < 8; ++i)
	{
		if(psSystraceData->asSystraceContext[i].ui32PID == ui32PID)
		{
			psContext = &(psSystraceData->asSystraceContext[i]);
			break;
		}
	}
	/*If we find it lets check its jobs, otherwise we create it*/
	if(psContext == NULL)
	{
		/*Don't create anything here*/
		return PVRSRV_SYSTRACE_JOB_NOT_FOUND;
	}
	/*Look for the JobID in the jobs CB otherwise create it and return ID*/
	for(i = 0; i < 16; ++i)
	{
		if((psContext->asJobs[i].ui32FrameNum == ui32FrameNum) &&
			(psContext->asJobs[i].ui32RTData == ui32RTData))
		{
			*pui32CtxID = psContext->ui32CtxID;
			*pui32JobID = psContext->asJobs[i].ui32JobID;
			return PVRSRV_SYSTRACE_OK;
		}
	}
	/*Not found*/
	return PVRSRV_SYSTRACE_JOB_NOT_FOUND;
}

void SystraceCreateFS(void)
{
	struct dentry *capture_sgx_hwperfdata_ret;
	
	pvrdir_ret = debugfs_create_dir("pvr", NULL);
	capture_sgx_hwperfdata_ret = debugfs_create_bool("gpu_tracing_on", S_IFREG | S_IRUGO | S_IWUSR, pvrdir_ret, &capture_hwperfdata);
}

void SystraceDestroyFS(void)
{
	debugfs_remove_recursive(pvrdir_ret);
}

IMG_BOOL SystraceIsCapturingHWData(void)
{
	return capture_hwperfdata;
}

void SystraceTAKick(PVRSRV_SGXDEV_INFO *psDevInfo, IMG_UINT32 ui32FrameNum, IMG_UINT32 ui32RTData, IMG_BOOL bIsFirstKick)
{
	IMG_UINT32 ui32PID = OSGetCurrentProcessIDKM();
	IMG_UINT32 ui32JobID = 0;
	IMG_UINT32 ui32CtxID = 0;
	PVRSRV_SYSTRACE_ERROR eError = PVRSRV_SYSTRACE_OK;
	
	if(psDevInfo->bSystraceInitialised)
	{
		if(bIsFirstKick)
		{
			eError = CreateJob(psDevInfo->psSystraceData, ui32PID, ui32FrameNum, ui32RTData);
			if(eError != PVRSRV_SYSTRACE_OK)
			{
				PVR_DPF((PVR_DBG_WARNING,"Systrace: Error creating a Job"));
			}
		}
		
		eError = GetCtxAndJobID(psDevInfo->psSystraceData, ui32PID, ui32FrameNum, ui32RTData, &ui32CtxID, &ui32JobID);
		
		if(eError != PVRSRV_SYSTRACE_OK)
		{
			PVR_DPF((PVR_DBG_WARNING,"Systrace: Job not found"));
		}
		
		trace_gpu_job_enqueue(ui32CtxID, ui32JobID, "TA");
	}
}

void SystraceInitializeTimeCorr(PVRSRV_SGXDEV_INFO *psDevInfo)
{
	IMG_UINT32 ui32CurrentIndex = psDevInfo->psSystraceData->ui32TimeCorrIndex;
	SGXMKIF_HOST_CTL	*psSGXHostCtl = (SGXMKIF_HOST_CTL *)psDevInfo->psSGXHostCtl;
	
	if(psSGXHostCtl->ui32TicksAtPowerUp != 0)
	{
		IMG_UINT32 ui32Clocksx16Difference = 0;
		IMG_UINT64 ui64TimeDifference = 0;
		IMG_UINT64 ui64HostTime = 0;
		IMG_UINT32 ui32SGXClocksx16 = 0;
		IMG_UINT32 ui32ClockMultiplier = 0;

		ui64HostTime = sched_clock();
		ui32SGXClocksx16 = OSReadHWReg(psDevInfo->pvRegsBaseKM, SGX_MP_CORE_SELECT(EUR_CR_TIMER,0));

		if(ui32SGXClocksx16 > psSGXHostCtl->ui32TicksAtPowerUp)
		{
		
			/* Get the ui32ClockMultipliertiplier per 1us*/
			ui32ClockMultiplier = (psDevInfo->ui32CoreClockSpeed) / (1000*1000);

			ui32Clocksx16Difference = ui32SGXClocksx16 - psSGXHostCtl->ui32TicksAtPowerUp;
			
			/* Multiply it by 16 and 1000 to convert from us to ns
			 * Breaking it in two steps to avoid overflow */
			ui64TimeDifference = (16 * ui32Clocksx16Difference) / ui32ClockMultiplier;
			ui64TimeDifference = (unsigned long long)1000 * ui64TimeDifference;

			psDevInfo->psSystraceData->asTimeCorrArray[ui32CurrentIndex].ui64HostTime = ui64HostTime - ui64TimeDifference;
			psDevInfo->psSystraceData->asTimeCorrArray[ui32CurrentIndex].ui32SGXClocksx16 = psSGXHostCtl->ui32TicksAtPowerUp;

			ui32CurrentIndex =  (ui32CurrentIndex + 1) % PVRSRV_SYSTRACE_TIMEINDEX_LIMIT;
		}
	}

	/* Initialize with current GPU ticks and host clock */
	psDevInfo->psSystraceData->asTimeCorrArray[ui32CurrentIndex].ui64HostTime = sched_clock();
	psDevInfo->psSystraceData->asTimeCorrArray[ui32CurrentIndex].ui32SGXClocksx16 = OSReadHWReg(psDevInfo->pvRegsBaseKM, SGX_MP_CORE_SELECT(EUR_CR_TIMER,0));

	psDevInfo->psSystraceData->ui32TimeCorrIndex = ui32CurrentIndex;
	psSGXHostCtl->ui32SystraceIndex = psDevInfo->psSystraceData->ui32TimeCorrIndex;
	psSGXHostCtl->ui32TicksAtPowerUp = 0;
}

void SystraceUpdateTimeCorr(PVRSRV_SGXDEV_INFO *psDevInfo, IMG_UINT32 ui32ClockMultiplier)
{
	IMG_UINT32 ui32LastIndex = psDevInfo->psSystraceData->ui32TimeCorrIndex;
	IMG_UINT32 ui32CurrentIndex = 0;
	SGXMKIF_HOST_CTL	*psSGXHostCtl = (SGXMKIF_HOST_CTL *)psDevInfo->psSGXHostCtl;

	if(psSGXHostCtl->ui32SGXPoweredOn)
	{
		if((psDevInfo->psSystraceData->bLastPowerDown == IMG_TRUE) && (psSGXHostCtl->ui32TicksAtPowerUp != 0))
		{
			IMG_UINT32 ui32Clocksx16Difference = 0;
			IMG_UINT64 ui64TimeDifference = 0;
			IMG_UINT64 ui64HostTime = 0;
			IMG_UINT32 ui32SGXClocksx16 = 0;

			ui64HostTime = sched_clock();
			ui32SGXClocksx16 = OSReadHWReg(psDevInfo->pvRegsBaseKM, SGX_MP_CORE_SELECT(EUR_CR_TIMER,0));

			if(ui32SGXClocksx16 > psSGXHostCtl->ui32TicksAtPowerUp)
			{
				ui32CurrentIndex =  ui32LastIndex;

				ui32Clocksx16Difference = ui32SGXClocksx16 - psSGXHostCtl->ui32TicksAtPowerUp;
			
				/* Multiply it by 16 and 1000 to convert from us to ns
				 * Breaking it in two steps to avoid overflow */
				ui64TimeDifference = (16 * ui32Clocksx16Difference) / ui32ClockMultiplier;
				ui64TimeDifference = (unsigned long long)1000 * ui64TimeDifference;
			
				psDevInfo->psSystraceData->asTimeCorrArray[ui32CurrentIndex].ui64HostTime = ui64HostTime - ui64TimeDifference;
				psDevInfo->psSystraceData->asTimeCorrArray[ui32CurrentIndex].ui32SGXClocksx16 = psSGXHostCtl->ui32TicksAtPowerUp;
			}
		}
		ui32CurrentIndex =  (ui32LastIndex + 1) % PVRSRV_SYSTRACE_TIMEINDEX_LIMIT;

		psDevInfo->psSystraceData->asTimeCorrArray[ui32CurrentIndex].ui64HostTime = sched_clock();
		psDevInfo->psSystraceData->asTimeCorrArray[ui32CurrentIndex].ui32SGXClocksx16 = OSReadHWReg(psDevInfo->pvRegsBaseKM, SGX_MP_CORE_SELECT(EUR_CR_TIMER,0));
		
		psDevInfo->psSystraceData->ui32TimeCorrIndex = ui32CurrentIndex ;
		psSGXHostCtl->ui32SystraceIndex = psDevInfo->psSystraceData->ui32TimeCorrIndex;
		psSGXHostCtl->ui32TicksAtPowerUp = 0;
		psDevInfo->psSystraceData->bLastPowerDown = IMG_FALSE;
	}
	else
	{
		if(psDevInfo->psSystraceData->bLastPowerDown != IMG_TRUE)
		{
			/* Device is powered down first time from powered up state,
			 * pre-increment index so it will be used for initial packets generated before first MISR after power up */

			ui32CurrentIndex =  (ui32LastIndex + 1) % PVRSRV_SYSTRACE_TIMEINDEX_LIMIT;
			psDevInfo->psSystraceData->ui32TimeCorrIndex = ui32CurrentIndex ;
			psSGXHostCtl->ui32SystraceIndex = psDevInfo->psSystraceData->ui32TimeCorrIndex;
		}
		psDevInfo->psSystraceData->bLastPowerDown = IMG_TRUE;
		psSGXHostCtl->ui32TicksAtPowerUp = 0;
		PVR_DPF((PVR_DBG_WARNING, "Systrace: Device PoweredOff, skipping update!"));
	}
}

void SystraceHWPerfPackets(PVRSRV_SGXDEV_INFO *psDevInfo, PVRSRV_SGX_HWPERF_CB_ENTRY* psSGXHWPerf, IMG_UINT32 ui32DataCount, IMG_UINT32 ui32SgxClockspeed)
{
	IMG_UINT32 ui32PID, ui32FrameNo, ui32EvtType, ui32RTData, ui32Clocksx16Difference, ui32ClockMultiplier, ui32SystraceIndex;
	
	IMG_UINT32 ui32SgxClocksx16 = 0;
	IMG_UINT32 i = 0;
	IMG_UINT64 ui64TimeDifference = 0;
	IMG_UINT64 ui64PacketTimeStamp = 0;
	
	IMG_UINT32 ui32JobID = 0;
	IMG_UINT32 ui32CtxID = 0;

	/* Get the ui32ClockMultipliertiplier per 1us*/
	ui32ClockMultiplier = (ui32SgxClockspeed)/(1000*1000);

	SystraceUpdateTimeCorr(psDevInfo, ui32ClockMultiplier);

	for(i = 0; i < ui32DataCount; ++i)
	{
		ui32SgxClocksx16 = psSGXHWPerf[i].ui32Clocksx16;
		ui32EvtType = psSGXHWPerf[i].ui32Type;
		ui32FrameNo = psSGXHWPerf[i].ui32FrameNo;
		ui32PID = psSGXHWPerf[i].ui32PID;
		ui32RTData = psSGXHWPerf[i].ui32RTData;
		ui32SystraceIndex = psSGXHWPerf[i].ui32SystraceIndex;

		if ((ui32EvtType == PVRSRV_SGX_HWPERF_TYPE_TA_START) ||
			(ui32EvtType == PVRSRV_SGX_HWPERF_TYPE_TA_END) ||
			(ui32EvtType == PVRSRV_SGX_HWPERF_TYPE_3D_START) ||
			(ui32EvtType == PVRSRV_SGX_HWPERF_TYPE_3D_END))
		{
			/*Get the JobID*/
			GetCtxAndJobID(psDevInfo->psSystraceData, ui32PID, ui32FrameNo, ui32RTData, &ui32CtxID, &ui32JobID);
			if (ui32SgxClocksx16 < psDevInfo->psSystraceData->asTimeCorrArray[ui32SystraceIndex].ui32SGXClocksx16)
			{
				PVR_DPF((PVR_DBG_ERROR, "Systrace: Dropping current HW packet!"));
				continue;
			}

			ui32Clocksx16Difference = (ui32SgxClocksx16 - psDevInfo->psSystraceData->asTimeCorrArray[ui32SystraceIndex].ui32SGXClocksx16);

			/* Multiply it by 16 and 1000 to convert from us to ns
			 * Breaking it in two steps to avoid overflow */
			ui64TimeDifference = (16 * ui32Clocksx16Difference) / ui32ClockMultiplier;
			ui64TimeDifference = (unsigned long long)1000 * ui64TimeDifference;

			/* Add the time diff to the last time-stamp, in nanoseconds*/
			ui64PacketTimeStamp = (unsigned long long) psDevInfo->psSystraceData->asTimeCorrArray[ui32SystraceIndex].ui64HostTime \
							+ (unsigned long long)ui64TimeDifference;
			
			switch(ui32EvtType)
			{
				case PVRSRV_SGX_HWPERF_TYPE_TA_START:
					trace_gpu_sched_switch("TA", ui64PacketTimeStamp, ui32CtxID, ui32FrameNo, ui32JobID);
					break;
				
				case PVRSRV_SGX_HWPERF_TYPE_TA_END:
					trace_gpu_sched_switch("TA", ui64PacketTimeStamp, 0, ui32FrameNo, ui32JobID);
					break;
				
				case PVRSRV_SGX_HWPERF_TYPE_3D_START:
					trace_gpu_sched_switch("3D", ui64PacketTimeStamp, ui32CtxID, ui32FrameNo, ui32JobID);
					break;
				
				case PVRSRV_SGX_HWPERF_TYPE_3D_END:
					trace_gpu_sched_switch("3D", ui64PacketTimeStamp, 0, ui32FrameNo, ui32JobID);
					break;
				
				default:
				break;
			}
		}
	}
}
#endif /* #if defined(EUR_CR_TIMER) */
