#include "shed.h"
#include "uart.h"
#include "mm.h"
#include "entry.h"
#include "irq.h"

static struct task_struct init_task = INIT_TASK;
struct task_struct *current = &(init_task);
struct task_struct *task[NR_TASKS] = {&(init_task), };
extern void fork_child_exit();

int n_tasks = 1;
int n_task_id = 1;


void enable_preempt() {
    current->preempt_count = 1;
}

void disable_preempt() {
    current->preempt_count = 0;
}

void current_task_info() {
    while(1) {
        uart_puts("Task id: ");
        uart_print_int(current->task_id);
        uart_puts("\r\n");
        delay(100000);
    }
}

int num_runnable_tasks() {
    int cnt = 0;
    for(int i=0; i<NR_TASKS; i++) {
        if(task[i]) {
            if(task[i]->state == TASK_RUNNING) {
                cnt++;
            }
        }
    }

    return cnt;
}

int get_taskid() {
    return current->task_id;
}

void privilege_task_create(void (*func)()) {
    _privilege_task_create(func, PF_KTHREAD, n_task_id);
}

void _privilege_task_create(void (*func)(), int clone_flags, unsigned long stack) {
    disable_preempt();
    struct task_struct *p = (struct task_struct *) get_free_page();
    if(!p) {
        uart_puts("Fail to create a new task...\r\n");
        return;
    }

    struct pt_regs *childregs = task_pt_regs(p);
	memzero((unsigned long)childregs, sizeof(struct pt_regs));
	memzero((unsigned long)&p->cpu_context, sizeof(struct cpu_context));

    if (clone_flags & PF_KTHREAD) {
		p->cpu_context.x19 = func;
	} else {
		struct pt_regs * cur_regs = task_pt_regs(current);
		*childregs = *cur_regs;
		childregs->regs[0] = 0;
		childregs->sp = stack + PAGE_SIZE;
		p->stack = stack;
	}

    p->flag = clone_flags;
    p->task_id = n_task_id;
    p->counter = 5;
    p->state = TASK_RUNNING;
    p->preempt_count = 1;
    p->cpu_context.x19 = (unsigned long) func;
    p->cpu_context.pc = (unsigned long) ret_from_fork;
    p->cpu_context.sp = (unsigned long) childregs;
    p->parent_id = current->task_id;
    n_task_id++;

    task[n_tasks] = p;
    n_tasks %= NR_TASKS;
    n_tasks++;

    // print created message
    uart_puts("Create new task: ");
    uart_print_int(p->task_id);
    uart_puts("\r\n");
    enable_preempt();
    return;
}

int move_to_user_mode(unsigned long pc) {
    struct pt_regs *regs = task_pt_regs(current);
    memzero((unsigned long)regs, sizeof(*regs));
    regs->pc = pc;
    regs->pstate = PSR_MODE_EL0t;
    unsigned long stack = get_free_page(); //allocate new user stack
    if (!stack) {
        return -1;
    }
    regs->sp = stack + PAGE_SIZE;
    current->stack = stack;
    return 0;
}

struct pt_regs * task_pt_regs(struct task_struct *tsk){
	unsigned long p = (unsigned long)tsk + THREAD_SIZE - sizeof(struct pt_regs);
	return (struct pt_regs *)p;
}

void switch_to(struct task_struct * next)  {
	if (current == next) 
		return;
	struct task_struct * prev = current;
	current = next;
	cpu_switch_to(prev, next);
}

void _schedule() {
    disable_preempt();
    int next, c;
    struct task_struct *p;

    c = 0;
    next = 0;

    for(int i=1; i<NR_TASKS; i++) {
        p = task[i];
        if(p && p->state == TASK_RUNNING && p->counter > c) {
            c = p->counter;
            next = i;
        }
    }
    uart_puts("switch to pid: ");
    uart_print_int(task[next]->task_id);
    uart_puts("\r\n");
    switch_to(task[next]);
    enable_preempt();
}

void schedule() {
    // current->counter = 0;
    _schedule();
    // timer_tick();
}

void schedule_tail() {
    enable_preempt();
}

struct task_struct *get_current_task() {
    return current;
}

void timer_tick() {
    // uart_print_int(current->counter);
    // uart_puts("\r\n");
	current->counter--;
	if (current->counter > 0 || current->preempt_count  == 0) {
		return;
	}
	current->counter=0;
	enable_irq();
    uart_puts("Rescheduling...\r\n");
	_schedule();
	disable_irq();
}

void _do_exec(void(*func)()) {
    struct pt_regs *regs = task_pt_regs(current);
    memzero((unsigned long)regs, sizeof(*regs));
    regs->pc = (unsigned long)func;
    regs->pstate = PSR_MODE_EL0t;
    unsigned long stack = get_free_page(); //allocate new user stack
    if (!stack) {
        return -1;
    }
    regs->sp = stack + PAGE_SIZE;
    current->stack = stack;
    uart_puts("Task ");
    uart_print_int(current->task_id);
    uart_puts(" do_exec done.\r\n");
}

void _do_exit() {
    current->state = TASK_ZOMBIE;
    current->counter = 0;
    schedule();
}

int _do_fork() {
    disable_preempt();
    struct task_struct *p = (struct task_struct *) get_free_page();
    if(!p) {
        uart_puts("Fail to create a new task...\r\n");
        return;
    }

    struct pt_regs *childregs = task_pt_regs(p);
	memzero((unsigned long)childregs, sizeof(struct pt_regs));
	memzero((unsigned long)&p->cpu_context, sizeof(struct cpu_context));

    p->cpu_context.x19 = current->cpu_context.x19;
    p->cpu_context.x20 = current->cpu_context.x20;
    p->cpu_context.x21 = current->cpu_context.x21;
    p->cpu_context.x22 = current->cpu_context.x22;
    p->cpu_context.x23 = current->cpu_context.x23;
    p->cpu_context.x24 = current->cpu_context.x24;
    p->cpu_context.x25 = current->cpu_context.x25;
    p->cpu_context.x26 = current->cpu_context.x26;
    p->cpu_context.x27 = current->cpu_context.x27;
    p->cpu_context.x28 = current->cpu_context.x28;

    p->task_id = n_task_id;
    p->counter = 3;
    p->state = TASK_RUNNING;
    p->preempt_count = 1;
    p->cpu_context.pc = fork_child_exit;
    p->cpu_context.sp = (unsigned long) childregs;
    p->parent_id = current->task_id;
    n_task_id++;

    task[n_tasks] = p;
    n_tasks %= NR_TASKS;
    n_tasks++;

    struct pt_regs *cur_regs = task_pt_regs(current);
    struct pt_regs *regs = task_pt_regs(p);

    memzero((unsigned long)regs, sizeof(*regs));
    regs->pc = cur_regs->pc;
    regs->pstate = cur_regs->pstate;
    unsigned long stack = get_free_page(); //allocate new user stack
    if (!stack) {
        return -1;
    }
    regs->sp = stack + PAGE_SIZE;
    p->stack = stack;

    // print created message
    uart_puts("===========================================\r\n");
    uart_puts("Task ");
    uart_print_int(current->task_id);
    uart_puts(" called fork\r\n");

    uart_puts("Create new task: ");
    uart_print_int(p->task_id);
    uart_puts(" Current run: ");
    uart_print_int(n_task_id);
    uart_puts("\r\n");
    uart_puts("===========================================\r\n");
    
    enable_preempt();
    return p->task_id;
}
