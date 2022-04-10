/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

#include <tilck/common/basic_defs.h>
#include <tilck/common/atomics.h>
#include <tilck/kernel/hal_types.h>
#include <tilck/kernel/list.h>
#include <tilck/kernel/bintree.h>
#include <tilck/kernel/sync.h>
#include <tilck/kernel/worker_thread.h>
#include <tilck/kernel/signal.h>

#include <tilck_gen_headers/config_sched.h>

#define TIME_SLICE_TICKS (TIMER_HZ / 25)

enum task_state {
   TASK_STATE_INVALID   = 0,
   TASK_STATE_RUNNABLE  = 1,
   TASK_STATE_RUNNING   = 2,
   TASK_STATE_SLEEPING  = 3,
   TASK_STATE_ZOMBIE    = 4
};

enum wakeup_reason {
   task_died,
   task_stopped,
   task_continued,
};

struct misc_buf {
   char path_buf[MAX_PATH];
   char unused[1024 - MAX_PATH];
   char execve_ctx[1024];
   char resolve_ctx[2048];
};

struct sched_ticks {

   u32 timeslice;       /* ticks counter for the current time slice */
   u64 total;           /* total life-time ticks */
   u64 total_kernel;    /* total life-time ticks spent in kernel */
   u64 vruntime;        /* a brutal approx. of Linux's vruntime */
};

STATIC_ASSERT(sizeof(enum sig_state) == 1);

struct task {

   union {

      int tid;               /* User/kernel task ID (pid in the Linux kernel) */

      /*
       * For the moment, `tid` has everywhere `int` as type, while the field is
       * used as key with the bintree_*_int functions which use pointer-sized
       * integers. Therefore, in case sizeof(long) > sizeof(int), we need some
       * padding.
       */

      ulong padding_0;
   };

   struct process *pi;

   bool is_main_thread;                      /* value of `tid == pi->pid` */
   bool running_in_kernel;
   bool stopped;
   bool was_stopped;

   volatile ATOMIC(enum task_state) state;   /* see docs/atomics.md */

   regs_t *state_regs;
   regs_t *fault_resume_regs;
   u32 faults_resume_mask;
   void *worker_thread;                      /* only for worker threads */

   struct bintree_node tree_by_tid_node;
   struct list_node runnable_node;
   struct list_node wakeup_timer_node;
   struct list_node siblings_node;    /* nodes in parent's pi's children list */

   struct list tasks_waiting_list;    /* tasks waiting this task to end */

   s32 wstatus;                       /* waitpid's wstatus  */
   struct sched_ticks ticks;          /* scheduler counters */

   void *kernel_stack;
   void *args_copybuf;

   union {
      void *io_copybuf;
      struct misc_buf *misc_buf;
   };

   struct wait_obj wobj;
   u32 ticks_before_wake_up;

   /* List of callbacks to call on exit */
   struct list on_exit;

   /* Temp kernel allocations for user requests */
   struct kernel_alloc *kallocs_tree_root;

   /* This task is stopped because of its vfork-ed child */
   bool vfork_stopped;

   /* Trace the syscalls of this task (requires debugpanel) */
   bool traced;

   /* The task was sleeping on a timer and has just been woken up */
   bool timer_ready;

   /* The current sa_mask has been altered by sigsuspend() */
   bool in_sigsuspend;

   /* Number of nested custom signal handlers (at most 1, at the moment). */
   int nested_sig_handlers;

   /* Kernel thread name, NULL for user tasks */
   const char *kthread_name;

   /* Pending signals bitset */
   ulong sa_pending[K_SIGACTION_MASK_WORDS];

   /* Blocked signals mask, updated by sigprocmask() and sigsuspend() */
   ulong sa_mask[K_SIGACTION_MASK_WORDS];

   /* Old blocked signals mask, saved by sys_rt_sigsuspend() */
   ulong sa_old_mask[K_SIGACTION_MASK_WORDS];

   /* See the comment above struct process' pi_arch */
   char ti_arch[ARCH_TASK_MEMBERS_SIZE] ALIGNED_AT(ARCH_TASK_MEMBERS_ALIGN);
};

extern struct task *kernel_process;
extern struct process *kernel_process_pi;

extern struct list runnable_tasks_list;
extern const char *const task_state_str[5];

#define KTH_ALLOC_BUFS                       (1 << 0)
#define KTH_WORKER_THREAD                    (1 << 1)

#define KERNEL_TID_START                        10000
#define KERNEL_MAX_TID                           1024 /* + KERNEL_TID_START */

STATIC_ASSERT(MAX_PID < KERNEL_TID_START);

void init_sched(void);
struct task *get_task(int tid);
struct process *get_process(int pid);
void task_change_state(struct task *ti, enum task_state new_state);
void task_change_state_idempotent(struct task *ti, enum task_state new_state);
bool save_regs_and_schedule(bool skip_disable_preempt);

static ALWAYS_INLINE void sched_set_need_resched(void)
{
   extern ATOMIC(int) __need_resched; /* see docs/atomics.md */
   atomic_store_explicit(&__need_resched, 1, mo_relaxed);
}

static ALWAYS_INLINE void sched_clear_need_resched(void)
{
   extern ATOMIC(int) __need_resched; /* see docs/atomics.md */
   atomic_store_explicit(&__need_resched, 0, mo_relaxed);
}

static ALWAYS_INLINE bool need_reschedule(void)
{
   extern ATOMIC(int) __need_resched; /* see docs/atomics.md */
   return (bool) atomic_load_explicit(&__need_resched, mo_relaxed);
}

static ALWAYS_INLINE void disable_preemption(void)
{
   extern ATOMIC(int) __disable_preempt; /* see docs/atomics.md */
   atomic_fetch_add_explicit(&__disable_preempt, 1, mo_relaxed);
}

static ALWAYS_INLINE void enable_preemption_nosched(void)
{
   extern ATOMIC(int) __disable_preempt; /* see docs/atomics.md */
   atomic_fetch_sub_explicit(&__disable_preempt, 1, mo_relaxed);
}

void enable_preemption(void);

/*
 * WARNING: this function is dangerous and should NEVER be used it for anything
 * other than special self-test code paths. See selftest_kmutex_ord_med().
 */
static ALWAYS_INLINE void force_enable_preemption(void)
{
   extern ATOMIC(int) __disable_preempt; /* see docs/atomics.md */
   atomic_store_explicit(&__disable_preempt, 0, mo_relaxed);
}

static ALWAYS_INLINE int get_preempt_disable_count(void)
{
   extern ATOMIC(int) __disable_preempt; /* see docs/atomics.md */
   return atomic_load_explicit(&__disable_preempt, mo_relaxed);
}

static ALWAYS_INLINE bool is_preemption_enabled(void)
{
   return !get_preempt_disable_count();
}

static ALWAYS_INLINE bool running_in_kernel(struct task *t)
{
   return t->running_in_kernel;
}

static ALWAYS_INLINE bool is_kernel_thread(struct task *ti)
{
   return ti->pi == kernel_process_pi;
}

static ALWAYS_INLINE bool is_main_thread(struct task *ti)
{
   return ti->is_main_thread;
}

static ALWAYS_INLINE bool is_worker_thread(struct task *ti)
{
   return ti->worker_thread != NULL;
}

/*
 * Default yield function
 *
 * Saves the current state and calls the scheduler. Expects the preemption to be
 * enabled. Returns true if a context switch occurred, false otherwise.
 */
static ALWAYS_INLINE bool kernel_yield(void)
{
   sched_set_need_resched();
   return save_regs_and_schedule(false);
}

/*
 * Special yield function to use when we disabled the preemption just *ONCE*
 * and want to yield without wasting a whole enable/disable preemption cycle.
 *
 * WARNING: this function excepts to be called with __disable_preempt == 1 while
 * it will always return with __disable_preempt == 0. It is asymmetric but
 * that's the same as do_schedule(): we want to call it with preemption disabled
 * in order to safely do stuff before calling it, but we EXPECT that calling it
 * WILL very likely "preempt" us and do a context switch, so we clearly expect
 * preemption to be enabled when it returns.
 */
static ALWAYS_INLINE bool kernel_yield_preempt_disabled(void)
{
   sched_set_need_resched();
   return save_regs_and_schedule(true);
}

/*
 * Just save the state and call the scheduler, without forcing a yield.
 */
static ALWAYS_INLINE bool schedule(void)
{
   return save_regs_and_schedule(false);
}

/*
 * Just save the state and call the scheduler, without forcing a yield.
 * Expects the preemption to be already disabled.
 */
static ALWAYS_INLINE bool schedule_preempt_disabled(void)
{
   return save_regs_and_schedule(true);
}


static ALWAYS_INLINE struct task *get_curr_task(void)
{
   extern struct task *__current;

   /*
    * Access to `__current` DOES NOT need to be atomic (not even relaxed) even
    * on architectures (!= x86) where loading/storing a pointer-size integer
    * requires more than one instruction, for the following reasons:
    *
    *    - While ANY given task is running, `__current` is always set and valid.
    *      That is true even if the task is preempted after reading for example
    *      only half of its value and than its execution resumed back, because
    *      during the task switch the older value of `__current` will be
    *      restored.
    *
    *    - The `__current` variable is set only in three cases:
    *       - during initialization [create_kernel_process()]
    *       - in switch_to_task() [with interrupts disabled]
    *       - in kthread_exit() [with interrupts disabled]
    */
   return __current;
}

/* Hack: it works only if the C file includes process.h, but that's fine. */
#define get_curr_proc() (get_curr_task()->pi)

static ALWAYS_INLINE enum task_state
get_curr_task_state(void)
{
   STATIC_ASSERT(sizeof(get_curr_task()->state) == 4);

   /*
    * Casting `state` to u32 and back to `enum task_state` to avoid compiler
    * errors in some weird configurations.
    */

   return (enum task_state) atomic_load_explicit(
      (ATOMIC(u32)*)&get_curr_task()->state,
      mo_relaxed
   );
}

#if DEBUG_CHECKS

   #define ASSERT_CURR_TASK_STATE(exp)                            \
      do {                                                        \
         if (get_curr_task_state() != (exp))                      \
            panic("Curr task state (%s) != expected (%s)",        \
                  task_state_str[get_curr_task_state()],          \
                  task_state_str[exp]);                           \
      } while (0)

   #define ASSERT_TASK_STATE(s, exp)                              \
      do {                                                        \
         if ((s) != (exp))                                        \
            panic("Task state (%s) != expected (%s)",             \
                  task_state_str[s],                              \
                  task_state_str[exp]);                           \
      } while (0)

#else

   #define ASSERT_CURR_TASK_STATE(exp)
   #define ASSERT_TASK_STATE(s, exp)

#endif

static ALWAYS_INLINE void
enter_sleep_wait_state(void)
{
   ASSERT(!is_preemption_enabled());

   /* Cannot call enter_sleep_wait_state() before setting a wait obj */
   ASSERT(get_curr_task()->wobj.type != WOBJ_NONE);

   if (get_curr_task_state() != TASK_STATE_SLEEPING) {

      /*
       * Corner case: we set a timer, then we called prepare_to_wait_on() which
       * set task's wobj and changed task's state to SLEEPING, but the timer
       * fired before we called this function. Timer_ready MUST BE set.
       *
       * Otherwise, that's simply a logic error: we called this function without
       * calling first prepare_to_wait_on().
       */

      ASSERT(get_curr_task()->timer_ready);
   }

   schedule_preempt_disabled();
}


NORETURN void switch_to_task(struct task *ti);

/*
 * Run the scheduler directly without saving the current state.
 * Expects preemption to be disabled.
 */
void do_schedule(void);


bool pending_signals(void);
int get_curr_tid(void);
int get_curr_pid(void);
void save_current_task_state(regs_t *);
void sched_account_ticks(void);
int create_new_pid(void);
int create_new_kernel_tid(void);
void task_info_reset_kernel_stack(struct task *ti);
void add_task(struct task *ti);
void remove_task(struct task *ti);
void init_task_lists(struct task *ti);

// It is called when each kernel thread returns. May be called explicitly too.
void kthread_exit(void);

int kthread_join(int tid, bool ignore_signals);
int kthread_join_all(const int *tids, size_t n, bool ignore_signals);

void task_set_wakeup_timer(struct task *task, u32 ticks);
void task_update_wakeup_timer_if_any(struct task *ti, u32 new_ticks);
u32 task_cancel_wakeup_timer(struct task *ti);

typedef void (*kthread_func_ptr)();

NODISCARD int kthread_create2(kthread_func_ptr func,
                              const char *name,
                              int fl,
                              void *arg);

#define kthread_create(func, fl, arg)  \
   kthread_create2(func, #func, (fl), (arg))

int iterate_over_tasks(bintree_visit_cb func, void *arg);
int sched_count_proc_in_group(int pgid);
int sched_get_session_of_group(int pgid);

struct process *task_get_pi_opaque(struct task *ti);
void process_set_tty(struct process *pi, void *t);
bool in_currently_dying_task(void);

void set_current_task_in_kernel(void);
void set_current_task_in_user_mode(void);
void *task_temp_kernel_alloc(size_t size);
void task_temp_kernel_free(void *ptr);
int register_on_task_exit_cb(void (*cb)(struct task *));
int unregister_on_task_exit_cb(void (*cb)(struct task *));