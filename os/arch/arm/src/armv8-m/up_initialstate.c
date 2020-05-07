/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * arch/arm/src/armv8-m/up_initialstate.c
 *
 *   Copyright (C) 2009, 2011-2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <string.h>

#include <tinyara/arch.h>

#include "up_internal.h"
#include "up_arch.h"

#include "psr.h"
#include "exc_return.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_initial_state
 *
 * Description:
 *   A new thread is being started and a new TCB
 *   has been created. This function is called to initialize
 *   the processor specific portions of the new TCB.
 *
 *   This function must setup the intial architecture registers
 *   and/or  stack so that execution will begin at tcb->start
 *   on the next context switch.
 *
 ****************************************************************************/

void up_initial_state(struct tcb_s *tcb)
{
	struct xcptcontext *xcp = &tcb->xcp;

	/* Initialize the initial exception register context structure */

	memset(xcp, 0, sizeof(struct xcptcontext));

	/* Save the initial stack pointer */

	xcp->regs[REG_SP] = (uint32_t)tcb->adj_stack_ptr;

#ifdef CONFIG_ARMV8M_STACKCHECK
	/* Set the stack limit value */

	xcp->regs[REG_R10] = (uint32_t)tcb->stack_alloc_ptr + 64;
#endif
#ifdef CONFIG_ARMV8M_TRUSTZONE
	xcp->regs[REG_R8] = (uint32_t)tcb->secure_handle;
	xcp->regs[REG_R9] = (uint32_t)tcb->stack_alloc_ptr;
#endif
	/* Save the task entry point (stripping off the thumb bit) */

	xcp->regs[REG_PC] = (uint32_t)tcb->start & ~1;

	/* Specify thumb mode */

	xcp->regs[REG_XPSR] = ARMV8M_XPSR_T;

	/* If this task is running PIC, then set the PIC base register to the
	 * address of the allocated D-Space region.
	 */

#ifdef CONFIG_PIC
	if (tcb->dspace != NULL) {
		/* Set the PIC base register (probably R10) to the address of the
		 * alloacated D-Space region.
		 */

		xcp->regs[REG_PIC] = (uint32_t)tcb->dspace->region;
	}
#ifdef CONFIG_NXFLAT
	/* Make certain that bit 0 is set in the main entry address.  This
	 * is only an issue when NXFLAT is enabled.  NXFLAT doesn't know
	 * anything about thumb; the addresses that NXFLAT sets are based
	 * on file header info and won't have bit 0 set.
	 */

	tcb->entry.main = (main_t)((uint32_t)tcb->entry.main | 1);
#endif
#endif							/* CONFIG_PIC */

#if defined(CONFIG_ARM_CMNVECTOR) || defined(CONFIG_BUILD_PROTECTED)
	/* All tasks start via a stub function in kernel space.  So all
	 * tasks must start in privileged thread mode.  If CONFIG_BUILD_PROTECTED
	 * is defined, then that stub function will switch to unprivileged
	 * mode before transferring control to the user task.
	 */

#ifdef CONFIG_ARMV8M_TRUSTZONE
	xcp->regs[REG_EXC_RETURN] = 0xFFFFFFBC;
#else
	xcp->regs[REG_EXC_RETURN] = EXC_RETURN_PRIVTHR;
#endif
#endif							/* CONFIG_ARM_CMNVECTOR || CONFIG_BUILD_PROTECTED */

#if defined(CONFIG_ARM_CMNVECTOR) && defined(CONFIG_ARCH_FPU)

	xcp->regs[REG_FPSCR] = 0;	// XXX initial FPSCR should be configurable
	xcp->regs[REG_FPReserved] = 0;

#endif							/* CONFIG_ARM_CMNVECTOR && CONFIG_ARCH_FPU */

	/* Enable or disable interrupts, based on user configuration */

#ifdef CONFIG_SUPPRESS_INTERRUPTS

#ifdef CONFIG_ARMV8M_USEBASEPRI
	xcp->regs[REG_BASEPRI] = NVIC_SYSH_DISABLE_PRIORITY;
#else
	xcp->regs[REG_PRIMASK] = 1;
#endif

#else							/* CONFIG_SUPPRESS_INTERRUPTS */

#ifdef CONFIG_ARMV8M_USEBASEPRI
	xcp->regs[REG_BASEPRI] = NVIC_SYSH_PRIORITY_MIN;
#endif

#endif							/* CONFIG_SUPPRESS_INTERRUPTS */
}
