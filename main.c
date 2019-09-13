/*******************************************************************************/
/*  © Université Lille 1, The Pip Development Team (2015-2016)                 */
/*                                                                             */
/*  This software is a computer program whose purpose is to run a minimal,     */
/*  hypervisor relying on proven properties such as memory isolation.          */
/*                                                                             */
/*  This software is governed by the CeCILL license under French law and       */
/*  abiding by the rules of distribution of free software.  You can  use,      */
/*  modify and/ or redistribute the software under the terms of the CeCILL     */
/*  license as circulated by CEA, CNRS and INRIA at the following URL          */
/*  "http://www.cecill.info".                                                  */
/*                                                                             */
/*  As a counterpart to the access to the source code and  rights to copy,     */
/*  modify and redistribute granted by the license, users are provided only    */
/*  with a limited warranty  and the software's author,  the holder of the     */
/*  economic rights,  and the successive licensors  have only  limited         */
/*  liability.                                                                 */
/*                                                                             */
/*  In this respect, the user's attention is drawn to the risks associated     */
/*  with loading,  using,  modifying and/or developing or reproducing the      */
/*  software by the user in light of its specific status of free software,     */
/*  that may mean  that it is complicated to manipulate,  and  that  also      */
/*  therefore means  that it is reserved for developers  and  experienced      */
/*  professionals having in-depth computer knowledge. Users are therefore      */
/*  encouraged to load and test the software's suitability as regards their    */
/*  requirements in conditions enabling the security of their systems and/or   */
/*  data to be ensured and,  more generally, to use and operate it in the      */
/*  same conditions as regards security.                                       */
/*                                                                             */
/*  The fact that you are presently reading this means that you have had       */
/*  knowledge of the CeCILL license and that you accept its terms.             */
/*******************************************************************************/

#include <stdint.h>
#include <pip/fpinfo.h>
#include <pip/paging.h>
#include <pip/vidt.h>
#include <pip/api.h>
#include <pip/compat.h>

#include "coliposte.h"
#include "stdlib.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

#define COLIPOSTE_SEND_INT	0x90
#define COLIPOSTE_RECV_INT	0x91
#define COLIPOSTE_REG_INT	0x92
#define COLIPOSTE_ACK_INT	0x93

static uint32_t buffer = 0;
task_t srcDescriptor, dstDescriptor;
uint32_t curPart;

#define PANIC() {vcli(); for(;;);}
#define MIN(a,b) ((a)<(b) ? (a) : (b))

extern void* _source, *_esource;
extern void* _destination, *_edestination;

static const struct {uint32_t start, end;} src = {
	(uint32_t)&_source, (uint32_t)&_esource,
};

static const struct {uint32_t start, end;} dst = {
	(uint32_t)&_destination, (uint32_t)&_edestination,
};

/**
 * Page fault irq handler
 */
INTERRUPT_HANDLER(pfAsm, pfHandler)
log("Page fault :(\n");
for(;;);
END_OF_INTERRUPT

/* 
 * init_memory_mapping: [mem 0x00700000-0x011fffff]
 * init_memory_mapping: [mem 0x01202000-0x0fbfffff]
 */
INTERRUPT_HANDLER(signalAsm, signalHandler)

END_OF_INTERRUPT

/**
 * General protection failure irq handler
 */
INTERRUPT_HANDLER(gpfAsm, gpfHandler)

END_OF_INTERRUPT

/* Coliposte SEND command */
INTERRUPT_HANDLER(sendAsm, sendHandler)
	log("The mailman is doing his duty. Let's see what I can do with this letter at "); puthex(data1); puts("...\n");
	if(dstDescriptor.state == READY) {
		log("Good! I'm removing the letter from the source mailbox.\n");
		/* data1 = 0x80000000 - sent by source partition  */
		uint32_t success = Pip_RemoveVAddr(srcDescriptor.part, data1);
		if(!success)
		{
			log("What? Your mailbox is locked. Please contact your nearest post office.\n");
			for(;;);
		}
		log("Okay, now I'm distributing your letter!\n");
		/* buffer : global variable - initialized with alloc_page in main */
		if(Pip_MapPageWrapper(buffer, dstDescriptor.part, 0x801000))
		{
			log("Our mailman encountered a problem while distributing your mail. Please contact your nearest post office.\n");
		}
		log("Perfect! Now ringing on the destination's door...\n");
		Pip_Notify(dstDescriptor.part, COLIPOSTE_RECV_INT, 0x801000, 0);
	} else
		log("Well, our mailman couldn't reach your mailbox. Please contact your nearest post office.\n");

	Pip_Resume(srcDescriptor.part, 0);
	for(;;);
END_OF_INTERRUPT

INTERRUPT_HANDLER(registerAsm, registerHandler)
	log("Hey! I just received a REGISTER message from destination. Its mailbox is now open!\n");
	dstDescriptor.state = READY;
	Pip_Resume(dstDescriptor.part, 0);
END_OF_INTERRUPT

INTERRUPT_HANDLER(ackAsm, ackHandler)
	log("Message received! Stopping.\n");
	Pip_VCLI();
	for(;;);
END_OF_INTERRUPT

INTERRUPT_HANDLER(timerAsm, timerHandler)
	curPart = 1 - curPart;
	/* Partition 1 is running, schedule part 0 ONLY IF the destination partition is ready */
	if(curPart && dstDescriptor.state == READY)
	{
		if(srcDescriptor.state == RUNNING) {
			Pip_Resume(srcDescriptor.part, 0);
		} else {
			srcDescriptor.state = RUNNING;
			Pip_Notify(srcDescriptor.part, 0, 0, 0);
		}

	} else {
		/* Partition 0 is running, schedule part 1 */
		if(dstDescriptor.state == RUNNING || dstDescriptor.state == READY) {
			Pip_Resume(dstDescriptor.part, 0);
		} else {
			dstDescriptor.state = RUNNING;
			Pip_Notify(dstDescriptor.part, 0, 0, 0);
		}
	}
END_OF_INTERRUPT

/*
 * Prepares the fake interrupt vector to receive new interrupts
 */
void initInterrupts()
{
	//registerInterrupt(33, &timerAsm, (uint32_t*)0x2020000); // We can use the same stack for both interrupts, or choose different stacks, let's play a bit
	registerInterrupt(33, &timerAsm, (uint32_t*)0x2020000); // We can use the same stack for both interrupts, or choose different stacks, let's play a bit
	registerInterrupt(14, &gpfAsm, (uint32_t*)0x2030000); /* General Protection Fault */
	registerInterrupt(15, &pfAsm, (uint32_t*)0x2040000); /* Page Fault */
	registerInterrupt(0x88, &signalAsm, (uint32_t*)0x2050000); /* Generic guest-to-host signal handler */
	registerInterrupt(COLIPOSTE_SEND_INT, &sendAsm, (uint32_t*)0x2020000);
	registerInterrupt(COLIPOSTE_REG_INT, &registerAsm, (uint32_t*)0x2020000);
	registerInterrupt(COLIPOSTE_ACK_INT, &ackAsm, (uint32_t*)0x2020000);
	return;
}

void parse_bootinfo(pip_fpinfo* bootinfo)
{
	if(bootinfo->magic == FPINFO_MAGIC)
	log("\tBootinfo seems to be correct.\n");
	else {
	log("\tBootinfo is invalid. Aborting.\n");
	PANIC();
	}

	log("\tAvailable memory starts at ");
	puthex((uint32_t)bootinfo->membegin);
	puts(" and ends at ");
	puthex((uint32_t)bootinfo->memend);
	puts("\n");

	log("\tPip revision ");
	puts(bootinfo->revision);
	puts("\n");

	return;
}

void main(pip_fpinfo* bootinfo)
{
	uint32_t i;

    log("Loading Coliposte...\n");

	log("Pip BootInfo: \n");
	parse_bootinfo(bootinfo);

	log("Initializing paging.\n");
	initPaging((void*)bootinfo->membegin, (void*)bootinfo->memend);

	log("Initializing interrupts... ");
	initInterrupts();
	puts("done.\n");

	log("Image info: \n");
	log("\tSource address : "); puthex(src.start); puts("\n");
	log("\tSource address end : "); puthex(src.end); puts("\n");
	log("\tDestination address : "); puthex(dst.start); puts("\n");
	log("\tDestination address end : "); puthex(dst.end); puts("\n");
	
	log("\tSource size : "); puthex(src.end - src.start); puts(" bytes\n");
	log("\tDestination size : "); puthex(dst.end - dst.start); puts(" bytes\n");
	
/*	if (LinuxBootstrap(linuxInfo.start, linuxInfo.end - linuxInfo.start, 0x700000, &linuxDescriptor)){
		log("Linux image failed\n");
		PANIC();
	}


	log("I'm done. I'll be booting Linux in a few seconds.\n");
*/
	
	if(BootstrapPartition(src.start, src.end - src.start, 0x700000, &srcDescriptor)) {
		log("Couldn't bootstrap source.\n");
		PANIC();
	}
	
	if(BootstrapPartition(dst.start, dst.end - dst.start, 0x700000, &dstDescriptor)) {
		log("Couldn't bootstrap destination.\n");
		PANIC();
	}
	
	/* Map Coliposte buffer in sender */
	buffer = (uint32_t)allocPage();
	if(mapPageWrapper((uint32_t)buffer, (uint32_t)srcDescriptor.part, (uint32_t)0x800000))
	{
		log("Couldn't map Coliposte buffer.\n");
		PANIC();
	} else {
		log("Coliposte buffer mapped.\n");
	}
	
	/* discard main context.. */
	curPart = 0;
	Pip_VSTI();
	for(;;);
	PANIC();
}
