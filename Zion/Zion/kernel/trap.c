#include <include/mmu.h>
#include <include/x86.h>
#include <include/assert.h>

#include <kernel/trap.h>
#include <kernel/console.h>
#include <kernel/monitor.h>

#include <mm/pmap.h>

static struct Taskstate ts;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Falt",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if ((size_t) trapno < sizeof(excnames) / sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";

	return "(unknown trap)";
}


void
idt_init(void)
{
	extern struct Segdesc gdt[];

	// LAB 2: Your code here.
	extern uint32_t _idt_code[];
	extern char system_call[];
	for (uint32_t i = 0; i <= T_SIMDERR; ++i)
	        SETGATE(idt[i], 0, GD_KT, _idt_code[i], 3);

	// Set a gate for the system call interrupt.
	// Hint: Must this gate be accessible from userlevel?
	// LAB 3: Your code here.
	SETGATE(idt[T_SYSCALL], 0, GD_KT, system_call, 3);

	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;

	// Initialize the TSS field of the gdt.
	gdt[GD_TSS >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
					sizeof(struct Taskstate), 0);
	gdt[GD_TSS >> 3].sd_s = 0;

	// Load the TSS
	ltr(GD_TSS);

	// Load the IDT
	asm volatile("lidt idt_pd");
}


void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	cprintf("  err  0x%08x\n", tf->tf_err);
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	cprintf("  esp  0x%08x\n", tf->tf_esp);
	cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

void
print_regs(struct Registers *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp  0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

extern "C" {
void
trap(struct Trapframe *tf)
{
	// Dispatch based on what type of trap occurred
	switch (tf->tf_trapno) {

	// LAB 2: Your code here.
	case T_BRKPT:
	    cprintf("Bp...\n");
		monitor(tf);
		//print_trapframe(tf);
		break;
	case T_PGFLT:
	    cprintf("Page Fault...\n");
		monitor(tf);
		break;
	default:
		// Unexpected trap: The user process or the kernel has a bug.
		cprintf("Default...\n");
		print_trapframe(tf); 
		if (tf->tf_cs == GD_KT)
		    //cprintf("unhandled trap in kernel");
			panic("unhandled trap in kernel");
		else
			panic("unhandled trap in user mode");
	}
}
}
