#include <cheriintrin.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(__CHERI_PURE_CAPABILITY__) ||!__has_feature(capabilities)
#  error This example must be run on a CHERI hybrid system
#endif

// This example shows that registering a signal handler allows a DDC-restricted
// compartment to break out of itself: the signal handler is run with whatever
// DDC is present at the time the handler is run, which can be different than
// the DDC the handler was registered with.

#define STACK_LEN 4096

void ddc_set(void *__capability cap) {
    asm volatile("MSR DDC, %[cap]" : : [cap] "C"(cap) : "memory");
}

void handler(int sig) {
    printf("DDC in signal handler: %#lp\n", cheri_ddc_get());
}

void restrict_and_check() {
    pid_t self_pid = getpid();

    void *__capability old_ddc = cheri_ddc_get();
    // The new DDC's bounds will be `0..__builtin_frame_address(0)`.
    void *__capability new_ddc = cheri_bounds_set(cheri_ddc_get(), (vaddr_t) __builtin_frame_address(0));
    printf("Will restrict DDC to: %#lp\n", new_ddc);
    // Restrict the DDC (i.e. move into a restricted compartment).
    ddc_set(new_ddc);
    // Register a signal handler while in the restricted compartment.
    signal(SIGHUP, handler);
    // If a signal is received now, the handler will be run under the restricted DDC...
    kill(self_pid, SIGHUP);
    // ...but if we now restore the DDC to its unrestricted value...
    ddc_set(old_ddc);
    // ...then the handler will be run with the unrestricted DDC, breaking out
    // of the compartment the handler was registered under.
    kill(self_pid, SIGHUP);
}

int main() {
    // The default DDC in CheriBSD spans the entirety of virtual memory, and it
    // puts the stack at such a high virtual address that it's difficult to
    // restrict the DDC and still ensure the program has access to whatever
    // chunk of memory library calls like signal will need and the stack. To
    // make our lives easier we allocate a new stack, which CheriBSD will place
    // at a relatively low virtual memory address, and restrict the DDC to
    // encompass virtual memory up to that address
    void *new_stack = malloc(STACK_LEN) + STACK_LEN;
    void *old_stack;
    asm volatile("MOV %[old_stack], SP" : [old_stack]"+r"(old_stack) : : "memory");
    asm volatile("MOV SP, %[new_stack]" : : [new_stack] "r"(new_stack) : "memory");
    // We've now switched to a new stack. In a real system we should do various
    // clever things at this point to stop us getting into undefined behaviour,
    // but we cheat and do the minimum that tends to work at -O0: we call a new
    // function, which should then work in blissful ignorance that it's not on
    // the same stack as the calling function.
    restrict_and_check();
    asm volatile("MOV SP, %[old_stack]" : : [old_stack] "r"(old_stack) : "memory");

    return 0;
}
