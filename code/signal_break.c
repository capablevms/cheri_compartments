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

void call_with_stack(void (*fn)(), void* stack);
asm(".type call_with_stack, @function\n"
    "call_with_stack:\n"
    "    MOV x10, sp\n"
    "    BIC sp, x1, 0xf\n" // Ensure 16-byte stack alignment.
    // Save the return address and old stack, so we can get back.
    "    STP x10, lr, [sp, #-16]!\n"
    // We've now switched to a new stack. We must restore it before the end
    // of the assembly block, because the C compiler is likely to generate
    // stack accesses to access locals. We can, however, call out to another
    // C function, since it shouldn't make any assumptions about its stack
    // on entry and will work in blissful ignorance that it's not on the
    // same stack as the calling function.
    //
    // We've broken an AAPCS64 rule here because we've escaped the stack
    // limit, but in practice this works fine for demonstration purposes.
    "    BLR x0\n"
    // Restore the stack and return.
    "    LDP x10, lr, [sp]\n"
    "    MOV sp, x10\n"
    "    RET lr\n");

int main() {
    // The default DDC in CheriBSD spans the entirety of virtual memory, and it
    // puts the stack at such a high virtual address that it's difficult to
    // restrict the DDC and still ensure the program has access to whatever
    // chunk of memory library calls like signal will need and the stack. To
    // make our lives easier we allocate a new stack, which CheriBSD will place
    // at a relatively low virtual memory address, and restrict the DDC to
    // encompass virtual memory up to that address
    void *new_stack = malloc(STACK_LEN) + STACK_LEN;
    call_with_stack(restrict_and_check, new_stack);

    return 0;
}
