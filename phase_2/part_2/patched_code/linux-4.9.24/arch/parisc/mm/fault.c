/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 *
 * Copyright (C) 1995, 1996, 1997, 1998 by Ralf Baechle
 * Copyright 1999 SuSE GmbH (Philipp Rumpf, prumpf@tux.org)
 * Copyright 1999 Hewlett Packard Co.
 *
 */

#include <linux/mm.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/extable.h>
#include <linux/uaccess.h>
#include <linux/unistd.h>

#include <asm/traps.h>

/* Various important other fields */
#define bit22set(x)		(x & 0x00000200)
#define bits23_25set(x)		(x & 0x000001c0)
#define isGraphicsFlushRead(x)	((x & 0xfc003fdf) == 0x04001a80)
				/* extended opcode is 0x6a */

#define BITSSET		0x1c0	/* for identifying LDCW */


DEFINE_PER_CPU(struct exception_data, exception_data);

int show_unhandled_signals = 1;

/*
 * parisc_acctyp(unsigned int inst) --
 *    Given a PA-RISC memory access instruction, determine if the
 *    the instruction would perform a memory read or memory write
 *    operation.
 *
 *    This function assumes that the given instruction is a memory access
 *    instruction (i.e. you should really only call it if you know that
 *    the instruction has generated some sort of a memory access fault).
 *
 * Returns:
 *   VM_READ  if read operation
 *   VM_WRITE if write operation
 *   VM_EXEC  if execute operation
 */
static unsigned long
parisc_acctyp(unsigned long code, unsigned int inst)
{
	if (code == 6 || code == 7 || code == 16)
	    return VM_EXEC;

	switch (inst & 0xf0000000) {
	case 0x40000000: /* load */
	case 0x50000000: /* new load */
		return VM_READ;

	case 0x60000000: /* store */
	case 0x70000000: /* new store */
		return VM_WRITE;

	case 0x20000000: /* coproc */
	case 0x30000000: /* coproc2 */
		if (bit22set(inst))
			return VM_WRITE;

	case 0x0: /* indexed/memory management */
		if (bit22set(inst)) {
			/*
			 * Check for the 'Graphics Flush Read' instruction.
			 * It resembles an FDC instruction, except for bits
			 * 20 and 21. Any combination other than zero will
			 * utilize the block mover functionality on some
			 * older PA-RISC platforms.  The case where a block
			 * move is performed from VM to graphics IO space
			 * should be treated as a READ.
			 *
			 * The significance of bits 20,21 in the FDC
			 * instruction is:
			 *
			 *   00  Flush data cache (normal instruction behavior)
			 *   01  Graphics flush write  (IO space -> VM)
			 *   10  Graphics flush read   (VM -> IO space)
			 *   11  Graphics flush read/write (VM <-> IO space)
			 */
			if (isGraphicsFlushRead(inst))
				return VM_READ;
			return VM_WRITE;
		} else {
			/*
			 * Check for LDCWX and LDCWS (semaphore instructions).
			 * If bits 23 through 25 are all 1's it is one of
			 * the above two instructions and is a write.
			 *
			 * Note: With the limited bits we are looking at,
			 * this will also catch PROBEW and PROBEWI. However,
			 * these should never get in here because they don't
			 * generate exceptions of the type:
			 *   Data TLB miss fault/data page fault
			 *   Data memory protection trap
			 */
			if (bits23_25set(inst) == BITSSET)
				return VM_WRITE;
		}
		return VM_READ; /* Default */
	}
	return VM_READ; /* Default */
}

#undef bit22set
#undef bits23_25set
#undef isGraphicsFlushRead
#undef BITSSET


#if 0
/* This is the treewalk to find a vma which is the highest that has
 * a start < addr.  We're using find_vma_prev instead right now, but
 * we might want to use this at some point in the future.  Probably
 * not, but I want it committed to CVS so I don't lose it :-)
 */
			while (tree != vm_avl_empty) {
				if (tree->vm_start > addr) {
					tree = tree->vm_avl_left;
				} else {
					prev = tree;
					if (prev->vm_next == NULL)
						break;
					if (prev->vm_next->vm_start > addr)
						break;
					tree = tree->vm_avl_right;
				}
			}
#endif

#ifdef CONFIG_PAX_PAGEEXEC
/*
 * PaX: decide what to do with offenders (instruction_pointer(regs) = fault address)
 *
 * returns 1 when task should be killed
 *         2 when rt_sigreturn trampoline was detected
 *         3 when unpatched PLT trampoline was detected
 */
static int pax_handle_fetch_fault(struct pt_regs *regs)
{

#ifdef CONFIG_PAX_EMUPLT
	int err;

	do { /* PaX: unpatched PLT emulation */
		unsigned int bl, depwi;

		err = get_user(bl, (unsigned int *)instruction_pointer(regs));
		err |= get_user(depwi, (unsigned int *)(instruction_pointer(regs)+4));

		if (err)
			break;

		if (bl == 0xEA9F1FDDU && depwi == 0xD6801C1EU) {
			unsigned int ldw, bv, ldw2, addr = instruction_pointer(regs)-12;

			err = get_user(ldw, (unsigned int *)addr);
			err |= get_user(bv, (unsigned int *)(addr+4));
			err |= get_user(ldw2, (unsigned int *)(addr+8));

			if (err)
				break;

			if (ldw == 0x0E801096U &&
			    bv == 0xEAC0C000U &&
			    ldw2 == 0x0E881095U)
			{
				unsigned int resolver, map;

				err = get_user(resolver, (unsigned int *)(instruction_pointer(regs)+8));
				err |= get_user(map, (unsigned int *)(instruction_pointer(regs)+12));
				if (err)
					break;

				regs->gr[20] = instruction_pointer(regs)+8;
				regs->gr[21] = map;
				regs->gr[22] = resolver;
				regs->iaoq[0] = resolver | 3UL;
				regs->iaoq[1] = regs->iaoq[0] + 4;
				return 3;
			}
		}
	} while (0);
#endif

#ifdef CONFIG_PAX_EMUTRAMP

#ifndef CONFIG_PAX_EMUSIGRT
	if (!(current->mm->pax_flags & MF_PAX_EMUTRAMP))
		return 1;
#endif

	do { /* PaX: rt_sigreturn emulation */
		unsigned int ldi1, ldi2, bel, nop;

		err = get_user(ldi1, (unsigned int *)instruction_pointer(regs));
		err |= get_user(ldi2, (unsigned int *)(instruction_pointer(regs)+4));
		err |= get_user(bel, (unsigned int *)(instruction_pointer(regs)+8));
		err |= get_user(nop, (unsigned int *)(instruction_pointer(regs)+12));

		if (err)
			break;

		if ((ldi1 == 0x34190000U || ldi1 == 0x34190002U) &&
		    ldi2 == 0x3414015AU &&
		    bel == 0xE4008200U &&
		    nop == 0x08000240U)
		{
			regs->gr[25] = (ldi1 & 2) >> 1;
			regs->gr[20] = __NR_rt_sigreturn;
			regs->gr[31] = regs->iaoq[1] + 16;
			regs->sr[0] = regs->iasq[1];
			regs->iaoq[0] = 0x100UL;
			regs->iaoq[1] = regs->iaoq[0] + 4;
			regs->iasq[0] = regs->sr[2];
			regs->iasq[1] = regs->sr[2];
			return 2;
		}
	} while (0);
#endif

	return 1;
}

void pax_report_insns(struct pt_regs *regs, void *pc, void *sp)
{
	unsigned long i;

	printk(KERN_ERR "PAX: bytes at PC: ");
	for (i = 0; i < 5; i++) {
		unsigned int c;
		if (get_user(c, (unsigned int *)pc+i))
			printk(KERN_CONT "???????? ");
		else
			printk(KERN_CONT "%08x ", c);
	}
	printk("\n");
}
#endif

int fixup_exception(struct pt_regs *regs)
{
	const struct exception_table_entry *fix;

	fix = search_exception_tables(regs->iaoq[0]);
	if (fix) {
		struct exception_data *d;
		d = this_cpu_ptr(&exception_data);
		d->fault_ip = regs->iaoq[0];
		d->fault_gp = regs->gr[27];
		d->fault_space = regs->isr;
		d->fault_addr = regs->ior;

		/*
		 * Fix up get_user() and put_user().
		 * ASM_EXCEPTIONTABLE_ENTRY_EFAULT() sets the least-significant
		 * bit in the relative address of the fixup routine to indicate
		 * that %r8 should be loaded with -EFAULT to report a userspace
		 * access error.
		 */
		if (fix->fixup & 1) {
			regs->gr[8] = -EFAULT;

			/* zero target register for get_user() */
			if (parisc_acctyp(0, regs->iir) == VM_READ) {
				int treg = regs->iir & 0x1f;
				regs->gr[treg] = 0;
			}
		}

		regs->iaoq[0] = (unsigned long)&fix->fixup + fix->fixup;
		regs->iaoq[0] &= ~3;
		/*
		 * NOTE: In some cases the faulting instruction
		 * may be in the delay slot of a branch. We
		 * don't want to take the branch, so we don't
		 * increment iaoq[1], instead we set it to be
		 * iaoq[0]+4, and clear the B bit in the PSW
		 */
		regs->iaoq[1] = regs->iaoq[0] + 4;
		regs->gr[0] &= ~PSW_B; /* IPSW in gr[0] */

		return 1;
	}

	return 0;
}

/*
 * parisc hardware trap list
 *
 * Documented in section 3 "Addressing and Access Control" of the
 * "PA-RISC 1.1 Architecture and Instruction Set Reference Manual"
 * https://parisc.wiki.kernel.org/index.php/File:Pa11_acd.pdf
 *
 * For implementation see handle_interruption() in traps.c
 */
static const char * const trap_description[] = {
	[1] "High-priority machine check (HPMC)",
	[2] "Power failure interrupt",
	[3] "Recovery counter trap",
	[5] "Low-priority machine check",
	[6] "Instruction TLB miss fault",
	[7] "Instruction access rights / protection trap",
	[8] "Illegal instruction trap",
	[9] "Break instruction trap",
	[10] "Privileged operation trap",
	[11] "Privileged register trap",
	[12] "Overflow trap",
	[13] "Conditional trap",
	[14] "FP Assist Exception trap",
	[15] "Data TLB miss fault",
	[16] "Non-access ITLB miss fault",
	[17] "Non-access DTLB miss fault",
	[18] "Data memory protection/unaligned access trap",
	[19] "Data memory break trap",
	[20] "TLB dirty bit trap",
	[21] "Page reference trap",
	[22] "Assist emulation trap",
	[25] "Taken branch trap",
	[26] "Data memory access rights trap",
	[27] "Data memory protection ID trap",
	[28] "Unaligned data reference trap",
};

const char *trap_name(unsigned long code)
{
	const char *t = NULL;

	if (code < ARRAY_SIZE(trap_description))
		t = trap_description[code];

	return t ? t : "Unknown trap";
}

/*
 * Print out info about fatal segfaults, if the show_unhandled_signals
 * sysctl is set:
 */
static inline void
show_signal_msg(struct pt_regs *regs, unsigned long code,
		unsigned long address, struct task_struct *tsk,
		struct vm_area_struct *vma)
{
	if (!unhandled_signal(tsk, SIGSEGV))
		return;

	if (!printk_ratelimit())
		return;

	pr_warn("\n");
	pr_warn("do_page_fault() command='%s' type=%lu address=0x%08lx",
	    tsk->comm, code, address);
	print_vma_addr(KERN_CONT " in ", regs->iaoq[0]);

	pr_cont("\ntrap #%lu: %s%c", code, trap_name(code),
		vma ? ',':'\n');

	if (vma)
		pr_warn(KERN_CONT " vm_start = 0x%08lx, vm_end = 0x%08lx\n",
				vma->vm_start, vma->vm_end);

	show_regs(regs);
}

void do_page_fault(struct pt_regs *regs, unsigned long code,
			      unsigned long address)
{
	struct vm_area_struct *vma, *prev_vma;
	struct task_struct *tsk;
	struct mm_struct *mm;
	unsigned long acc_type;
	int fault;
	unsigned int flags;

	if (faulthandler_disabled())
		goto no_context;

	tsk = current;
	mm = tsk->mm;
	if (!mm)
		goto no_context;

	flags = FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_KILLABLE;
	if (user_mode(regs))
		flags |= FAULT_FLAG_USER;

	acc_type = parisc_acctyp(code, regs->iir);
	if (acc_type & VM_WRITE)
		flags |= FAULT_FLAG_WRITE;
retry:
	down_read(&mm->mmap_sem);
	vma = find_vma_prev(mm, address, &prev_vma);
	if (!vma || address < vma->vm_start)
		goto check_expansion;
/*
 * Ok, we have a good vm_area for this memory access. We still need to
 * check the access permissions.
 */

good_area:

	if ((vma->vm_flags & acc_type) != acc_type) {

#ifdef CONFIG_PAX_PAGEEXEC
		if ((mm->pax_flags & MF_PAX_PAGEEXEC) && (acc_type & VM_EXEC) &&
		    (address & ~3UL) == instruction_pointer(regs))
		{
			up_read(&mm->mmap_sem);
			switch (pax_handle_fetch_fault(regs)) {

#ifdef CONFIG_PAX_EMUPLT
			case 3:
				return;
#endif

#ifdef CONFIG_PAX_EMUTRAMP
			case 2:
				return;
#endif

			}
			pax_report_fault(regs, (void *)instruction_pointer(regs), (void *)regs->gr[30]);
			do_group_exit(SIGKILL);
		}
#endif

		goto bad_area;
	}

	/*
	 * If for any reason at all we couldn't handle the fault, make
	 * sure we exit gracefully rather than endlessly redo the
	 * fault.
	 */

	fault = handle_mm_fault(vma, address, flags);

	if ((fault & VM_FAULT_RETRY) && fatal_signal_pending(current))
		return;

	if (unlikely(fault & VM_FAULT_ERROR)) {
		/*
		 * We hit a shared mapping outside of the file, or some
		 * other thing happened to us that made us unable to
		 * handle the page fault gracefully.
		 */
		if (fault & VM_FAULT_OOM)
			goto out_of_memory;
		else if (fault & VM_FAULT_SIGSEGV)
			goto bad_area;
		else if (fault & VM_FAULT_SIGBUS)
			goto bad_area;
		BUG();
	}
	if (flags & FAULT_FLAG_ALLOW_RETRY) {
		if (fault & VM_FAULT_MAJOR)
			current->maj_flt++;
		else
			current->min_flt++;
		if (fault & VM_FAULT_RETRY) {
			flags &= ~FAULT_FLAG_ALLOW_RETRY;

			/*
			 * No need to up_read(&mm->mmap_sem) as we would
			 * have already released it in __lock_page_or_retry
			 * in mm/filemap.c.
			 */

			goto retry;
		}
	}
	up_read(&mm->mmap_sem);
	return;

check_expansion:
	vma = prev_vma;
	if (vma && (expand_stack(vma, address) == 0))
		goto good_area;

/*
 * Something tried to access memory that isn't in our memory map..
 */
bad_area:
	up_read(&mm->mmap_sem);

	if (user_mode(regs)) {
		struct siginfo si;

		show_signal_msg(regs, code, address, tsk, vma);

		switch (code) {
		case 15:	/* Data TLB miss fault/Data page fault */
			/* send SIGSEGV when outside of vma */
			if (!vma ||
			    address < vma->vm_start || address > vma->vm_end) {
				si.si_signo = SIGSEGV;
				si.si_code = SEGV_MAPERR;
				break;
			}

			/* send SIGSEGV for wrong permissions */
			if ((vma->vm_flags & acc_type) != acc_type) {
				si.si_signo = SIGSEGV;
				si.si_code = SEGV_ACCERR;
				break;
			}

			/* probably address is outside of mapped file */
			/* fall through */
		case 17:	/* NA data TLB miss / page fault */
		case 18:	/* Unaligned access - PCXS only */
			si.si_signo = SIGBUS;
			si.si_code = (code == 18) ? BUS_ADRALN : BUS_ADRERR;
			break;
		case 16:	/* Non-access instruction TLB miss fault */
		case 26:	/* PCXL: Data memory access rights trap */
		default:
			si.si_signo = SIGSEGV;
			si.si_code = (code == 26) ? SEGV_ACCERR : SEGV_MAPERR;
			break;
		}
		si.si_errno = 0;
		si.si_addr = (void __user *) address;
		force_sig_info(si.si_signo, &si, current);
		return;
	}

no_context:

	if (!user_mode(regs) && fixup_exception(regs)) {
		return;
	}

	parisc_terminate("Bad Address (null pointer deref?)", regs, code, address);

  out_of_memory:
	up_read(&mm->mmap_sem);
	if (!user_mode(regs))
		goto no_context;
	pagefault_out_of_memory();
}