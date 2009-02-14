/**********************************************************************

  rubysig.h -

  $Author$
  $Date$
  created at: Wed Aug 16 01:15:38 JST 1995

  Copyright (C) 1993-2003 Yukihiro Matsumoto

**********************************************************************/

#ifndef SIG_H
#define SIG_H
#include <errno.h>

#ifdef _WIN32
typedef LONG rb_atomic_t;

# define ATOMIC_TEST(var) InterlockedExchange(&(var), 0)
# define ATOMIC_SET(var, val) InterlockedExchange(&(var), (val))
# define ATOMIC_INC(var) InterlockedIncrement(&(var))
# define ATOMIC_DEC(var) InterlockedDecrement(&(var))

/* Windows doesn't allow interrupt while system calls */
# define TRAP_BEG do {\
    int saved_errno = 0;\
    rb_atomic_t trap_immediate = ATOMIC_SET(rb_trap_immediate, 1)
# define TRAP_END\
    ATOMIC_SET(rb_trap_immediate, trap_immediate);\
    saved_errno = errno;\
    CHECK_INTS;\
    errno = saved_errno;\
} while (0)
# define RUBY_CRITICAL(statements) do {\
    rb_w32_enter_critical();\
    statements;\
    rb_w32_leave_critical();\
} while (0)
#else
typedef int rb_atomic_t;

# define ATOMIC_TEST(var) ((var) ? ((var) = 0, 1) : 0)
# define ATOMIC_SET(var, val) ((var) = (val))
# define ATOMIC_INC(var) (++(var))
# define ATOMIC_DEC(var) (--(var))

# define TRAP_BEG do {\
    int saved_errno = 0;\
    int trap_immediate = rb_trap_immediate;\
    rb_trap_immediate = 1
# define TRAP_END rb_trap_immediate = trap_immediate;\
    saved_errno = errno;\
    CHECK_INTS;\
    errno = saved_errno;\
} while (0)

# define RUBY_CRITICAL(statements) do {\
    int trap_immediate = rb_trap_immediate;\
    rb_trap_immediate = 0;\
    statements;\
    rb_trap_immediate = trap_immediate;\
} while (0)
#endif
RUBY_EXTERN rb_atomic_t rb_trap_immediate;

RUBY_EXTERN int rb_prohibit_interrupt;
#define DEFER_INTS (rb_prohibit_interrupt++)
#define ALLOW_INTS do {\
    rb_prohibit_interrupt--;\
    CHECK_INTS;\
} while (0)
#define ENABLE_INTS (rb_prohibit_interrupt--)

VALUE rb_with_disable_interrupt _((VALUE(*)(ANYARGS),VALUE));

RUBY_EXTERN rb_atomic_t rb_trap_pending;
void rb_trap_restore_mask _((void));

RUBY_EXTERN int rb_thread_critical;
void rb_thread_schedule _((void));
#if defined(HAVE_SETITIMER) || defined(_THREAD_SAFE)
RUBY_EXTERN int rb_thread_pending;

EXTERN size_t rb_gc_malloc_increase;
EXTERN size_t rb_gc_malloc_limit;
EXTERN VALUE *rb_gc_stack_end;
EXTERN int *rb_gc_stack_grow_direction;  /* -1 for down or 1 for up */
#define __stack_zero_up(end,sp)  while (end >= ++sp) *sp=0
#define __stack_grown_up  (rb_gc_stack_end > (VALUE *)alloca(0))
#define __stack_zero_down(end,sp)  while (end <= --sp) *sp=0
#define __stack_grown_down  (rb_gc_stack_end < (VALUE *)alloca(0))

#if STACK_GROW_DIRECTION > 0
#define __stack_zero(end,sp)  __stack_zero_up(end,sp)
#define __stack_grown  __stack_grown_up
#elif STACK_GROW_DIRECTION < 0
#define __stack_zero(end,sp)  __stack_zero_down(end,sp)
#define __stack_grown  __stack_grown_down
#else  /* limp along if stack direction can't be determined at compile time */
#define __stack_zero(end,sp) if (rb_gc_stack_grow_direction<0) \
        __stack_zero_down(end,sp); else __stack_zero_up(end,sp);
#define __stack_grown  \
        (rb_gc_stack_grow_direction<0 ? __stack_grown_down : __stack_grown_up)
#endif
 
/*
  zero the memory that was (recently) part of the stack
  but is no longer.  Invoke when stack is deep to mark its extent
  and when it is shallow to wipe it
*/
#define rb_gc_wipe_stack() {     \
  VALUE *sp = alloca(0);         \
  VALUE *end = rb_gc_stack_end;  \
  rb_gc_stack_end = sp;          \
  __stack_zero(end, sp);   \
}

/*
  Update our record of maximum stack extent without zeroing unused stack
*/
#define rb_gc_update_stack_extent() \
    if __stack_grown rb_gc_stack_end = alloca(0);


# define CHECK_INTS do {\
    rb_gc_wipe_stack(); \
    if (!(rb_prohibit_interrupt || rb_thread_critical)) {\
        if (rb_thread_pending) rb_thread_schedule();\
	if (rb_trap_pending) rb_trap_exec();\
    }\
} while (0)
#else
/* pseudo preemptive thread switching */
RUBY_EXTERN int rb_thread_tick;
#define THREAD_TICK 500
#define CHECK_INTS do {\
    rb_gc_wipe_stack(); \
    if (!(rb_prohibit_interrupt || rb_thread_critical)) {\
	if (rb_thread_tick-- <= 0) {\
	    rb_thread_tick = THREAD_TICK;\
            rb_thread_schedule();\
	}\
    }\
    if (rb_trap_pending) rb_trap_exec();\
} while (0)
#endif

#endif
