/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <process/Scheduler.h>
#include <pthread-syscalls.h>
#include <syscallError.h>
#include "errors.h"

extern "C"
{
    extern void pthread_stub();
    extern char pthread_stub_end;
}

struct pthreadInfoBlock
{
    void *entry;
    void *arg;
} __attribute__((packed));

/**
 * pthread_kernel_enter: Kernel side entry point for all POSIX userspace threads.
 *
 * Excuse the "story" littered throughout this (and other) function(s).
 * When you're working with pthreads, sometimes you need small things to
 * lighten up your coding, and this is one way that works (so far).
 *
 * \param fn A pointer to a pthreadInfoBlock structure that contains information
 *           about the entry point and arguments for the new thread.
 * \return Nothing. Had you going for a moment there with that int, didn't I? If
 *         this function does return, kindly ignore the resulting fatal error
 *         and look for the flying pigs.
 */
int pthread_kernel_enter(void *blk)
{
    PT_NOTICE("pthread_kernel_enter");

    // Grab our backpack with our map and additional information
    pthreadInfoBlock *args = reinterpret_cast<pthreadInfoBlock*>(blk);
    void *entry = args->entry;
    void *new_args = args->arg;

    // It's grabbed now, so we don't need any more room on the bag rack.
    delete args;

    PT_NOTICE("pthread child [" << reinterpret_cast<uintptr_t>(entry) << "] is starting");

    // Just keep using our current stack, don't bother to save any state. We'll
    // never return, so there's no need to be sentimental (and no need to worry
    // about return addresses etc)
    uintptr_t stack = 0;
    asm volatile("mov %%esp, %%eax" : "=a" (stack));
    if(!stack) // Because sanity costs little.
        return -1;

    // Begin our quest from the Kernel Highlands and dive deep into the murky
    // depths of the Userland River. We'll take with us our current stack, a
    // map to our destination, and some additional information that may come
    // in handy when we arrive.
    Processor::jumpUser(0, EVENT_HANDLER_TRAMPOLINE2, stack, reinterpret_cast<uintptr_t>(entry), reinterpret_cast<uintptr_t>(new_args), 0, 0);

    // If we get here, we probably got catapaulted many miles back to the
    // Kernel Highlands. That's probably not a good thing. They won't really
    // accept us here anymore :(
    FATAL("I'm not supposed to be in the Kernel Highlands!");
    return 0;
}

/**
 * posix_pthread_create: POSIX thread creation
 *
 * This function will create a new execution thread in userspace.
 */
int posix_pthread_create(pthread_t *thread, const pthread_attr_t *attr, pthreadfn start_addr, void *arg)
{
    PT_NOTICE("pthread_create");

    // Build some defaults for the attributes, if needed
    size_t stackSize = 0;
    if(attr)
    {
        if(attr->stackSize >= PTHREAD_STACK_MIN)
            stackSize = attr->stackSize;
    }

    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Build the information structure to pass to the pthread entry function
    pthreadInfoBlock *dat = new pthreadInfoBlock;
    dat->entry = reinterpret_cast<void*>(start_addr);
    dat->arg = arg;

    // Allocate a stack for this thread
    void *stack = Processor::information().getVirtualAddressSpace().allocateStack(stackSize);
    if(!stack)
    {
        ERROR("posix_pthread_create: couldn't get a stack!");
        SYSCALL_ERROR(OutOfMemory);
        return -1;
    }

    // Create the thread
    Thread *pThread = new Thread(pProcess, pthread_kernel_enter, reinterpret_cast<void*>(dat), stack, true);

    // Create our information structure, shove in the initial elements
    PosixSubsystem::PosixThread *p = new PosixSubsystem::PosixThread;
    p->pThread = pThread;
    p->returnValue = 0;

    // Take information from the attributes
    if(attr)
    {
        p->isDetached = (attr->detachState == PTHREAD_CREATE_DETACHED);
    }

    // Insert the thread
    pSubsystem->insertThread(pThread->getId(), p);
    *thread = static_cast<pthread_t>(pThread->getId());

    // All done!
    return 0;
}

/**
 * posix_pthread_join: POSIX thread join
 *
 * Joining a thread basically means the caller of pthread_join will wait until
 * the passed thread completes execution. The return value from the thread is
 * stored in value_ptr.
 */
int posix_pthread_join(pthread_t thread, void **value_ptr)
{
    PT_NOTICE("pthread_join");

    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab the thread information structure and verify it
    PosixSubsystem::PosixThread *p = pSubsystem->getThread(thread);
    if(!p)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Check that we can join
    if(p->isDetached)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Now that we have it, wait for the thread and then store the return value
    p->isRunning.acquire();
    if(value_ptr)
        *value_ptr = p->returnValue;

    // Clean up - we're never going to use this again
    pSubsystem->removeThread(thread);
    delete p;

    // Success!
    return 0;
}

/**
 * posix_pthread_detach: POSIX thread detach
 *
 * If the thread has completed, clean up its information. Otherwise signal that
 * its thread information will not be used, and can be cleaned up.
 *
 * Similar to join, but without caring about the return value.
 */
int posix_pthread_detach(pthread_t thread)
{
    PT_NOTICE("pthread_join");

    // Grab the subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab the thread information structure and verify it
    PosixSubsystem::PosixThread *p = pSubsystem->getThread(thread);
    if(!p)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Check that we can detach
    if(p->isDetached)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Now that we have it, wait for the thread and then store the return value
    if(p->isRunning.tryAcquire())
    {
        // Clean up - we're never going to use this again
        pSubsystem->removeThread(thread);
        delete p;
    }
    else
        p->canReclaim = true;

    // Success!
    return 0;
}

/**
 * posix_pthread_self: Returns the thread ID for the current thread.
 */
pthread_t posix_pthread_self()
{
    Thread *pThread = Processor::information().getCurrentThread();
    return pThread->getId();
}

/**
 * posix_pthread_enter: Called by userspace when entering the new thread.
 *
 * At this stage, this function does not do anything. In future, this may
 * change to support adding new state information to the thread lists, or
 * to set some kernel-side state.
 */
int posix_pthread_enter(uintptr_t arg)
{
    return 0;
}

/**
 * posix_pthread_exit
 *
 * This function is called either when the thread function returns, or
 * when pthread_exit is called. The single parameter specifies a return
 * value from the thread.
 */
void posix_pthread_exit(void *ret)
{
    PT_NOTICE("pthread_exit");

    // Grab the subsystem and unlock any waiting threads
    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (pSubsystem)
    {
        PosixSubsystem::PosixThread *p = pSubsystem->getThread(pThread->getId());
        if(p)
        {
            p->returnValue = ret;
            p->isRunning.release();

            if(p->canReclaim)
            {
                // Clean up...
                pSubsystem->removeThread(pThread->getId());
                delete p;
            }
        }
    }

    // Kill the thread
    Processor::information().getScheduler().killCurrentThread();

    while(1);
}

/**
 * pedigree_init_pthreads
 *
 * This function copies the user mode thread wrapper from the kernel to a known
 * user mode location. The location is already mapped by pedigree_init_signals
 * which must be called before this function.
 */
void pedigree_init_pthreads()
{
    PT_NOTICE("init_pthreads");
    memcpy(reinterpret_cast<void*>(EVENT_HANDLER_TRAMPOLINE2), reinterpret_cast<void*>(pthread_stub), (reinterpret_cast<uintptr_t>(&pthread_stub_end) - reinterpret_cast<uintptr_t>(pthread_stub)));
}
