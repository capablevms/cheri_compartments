#include <assert.h>
#include <cheriintrin.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(__CHERI_PURE_CAPABILITY__) ||!__has_feature(capabilities)
#  error This example must be run on a CHERI hybrid system
#endif

/* #define BASE_OFF (0xfffffff00000) */
/* #define BASE_OFF ((vaddr_t) 1<<48) */
#define STACK_LEN 4096

void ddc_set(void *__capability cap) {
    asm volatile("MSR DDC, %[cap]" : : [cap] "C"(cap) : "memory");
}

void handler(int sig) {
    printf("DDC in signal handler: %#lp\n", cheri_ddc_get());
}

void main2() {
    pid_t self_pid = getpid();

    void *__capability old_ddc = cheri_ddc_get();
    void *__capability new_ddc = cheri_bounds_set(cheri_ddc_get(), (vaddr_t) __builtin_frame_address(0));
    printf("Will restrict DDC to: %#lp\n", new_ddc);
    ddc_set(new_ddc);
    signal(SIGHUP, handler);
    kill(self_pid, SIGHUP);
    ddc_set(old_ddc);
    kill(self_pid, SIGHUP);
    exit(0);
}

int main() {
    // Force these functions to be loaded so that _rtld_bind doesn't bork when
    // we restrict the DDC later.
    signal(SIGHUP, SIG_IGN);
    kill(getpid(), SIGHUP);

    assert(cheri_address_get(cheri_ddc_get()) == 0);
    void *new_stack = malloc(STACK_LEN) + STACK_LEN;
    asm volatile("MOV SP, %[new_stack]" : : [new_stack] "r"(new_stack) : "memory");
    main2();

    return 0;
}
