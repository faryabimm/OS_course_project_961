/*
 *  arch/arm/include/asm/domain.h
 *
 *  Copyright (C) 1999 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_PROC_DOMAIN_H
#define __ASM_PROC_DOMAIN_H

#ifndef __ASSEMBLY__
#include <asm/barrier.h>
#include <asm/thread_info.h>
#endif

/*
 * Domain numbers
 *
 *  DOMAIN_IO     - domain 2 includes all IO only
 *  DOMAIN_USER   - domain 1 includes all user memory only
 *  DOMAIN_KERNEL - domain 0 includes all kernel memory only
 *
 * The domain numbering depends on whether we support 36 physical
 * address for I/O or not.  Addresses above the 32 bit boundary can
 * only be mapped using supersections and supersections can only
 * be set for domain 0.  We could just default to DOMAIN_IO as zero,
 * but there may be systems with supersection support and no 36-bit
 * addressing.  In such cases, we want to map system memory with
 * supersections to reduce TLB misses and footprint.
 *
 * 36-bit addressing and supersections are only available on
 * CPUs based on ARMv6+ or the Intel XSC3 core.
 */
#ifndef CONFIG_IO_36
#define DOMAIN_KERNEL	0
#define DOMAIN_USER	1
#define DOMAIN_IO	2
#else
#define DOMAIN_KERNEL	2
#define DOMAIN_USER	1
#define DOMAIN_IO	0
#endif

/*
 * Domain types
 */
#define DOMAIN_NOACCESS	0
#define DOMAIN_CLIENT	1
#ifdef CONFIG_CPU_USE_DOMAINS
#define DOMAIN_MANAGER	3
#define DOMAIN_VECTORS	3
#define DOMAIN_USERCLIENT	DOMAIN_CLIENT
#else

#ifdef CONFIG_PAX_KERNEXEC
#define DOMAIN_MANAGER	1
#define DOMAIN_KERNEXEC	3
#else
#define DOMAIN_MANAGER	1
#endif

#ifdef CONFIG_PAX_MEMORY_UDEREF
#define DOMAIN_USERCLIENT	0
#define DOMAIN_UDEREF		1
#define DOMAIN_VECTORS		DOMAIN_KERNEL
#else
#define DOMAIN_USERCLIENT	1
#define DOMAIN_VECTORS		DOMAIN_USER
#endif

#endif
#define DOMAIN_KERNELCLIENT	1

#define domain_mask(dom)	((3) << (2 * (dom)))
#define domain_val(dom,type)	((type) << (2 * (dom)))

#ifdef CONFIG_CPU_SW_DOMAIN_PAN
#define DACR_INIT \
	(domain_val(DOMAIN_USER, DOMAIN_NOACCESS) | \
	 domain_val(DOMAIN_KERNEL, DOMAIN_MANAGER) | \
	 domain_val(DOMAIN_IO, DOMAIN_KERNELCLIENT) | \
	 domain_val(DOMAIN_VECTORS, DOMAIN_CLIENT))
#elif defined(CONFIG_PAX_MEMORY_UDEREF)
	/* DOMAIN_VECTORS is defined to DOMAIN_KERNEL */
#define DACR_INIT \
	(domain_val(DOMAIN_USER, DOMAIN_USERCLIENT) | \
	 domain_val(DOMAIN_KERNEL, DOMAIN_MANAGER) | \
	 domain_val(DOMAIN_IO, DOMAIN_KERNELCLIENT))
#else
#define DACR_INIT \
	(domain_val(DOMAIN_USER, DOMAIN_USERCLIENT) | \
	 domain_val(DOMAIN_KERNEL, DOMAIN_MANAGER) | \
	 domain_val(DOMAIN_IO, DOMAIN_KERNELCLIENT) | \
	 domain_val(DOMAIN_VECTORS, DOMAIN_CLIENT))
#endif

#define __DACR_DEFAULT \
	domain_val(DOMAIN_KERNEL, DOMAIN_CLIENT) | \
	domain_val(DOMAIN_IO, DOMAIN_CLIENT) | \
	domain_val(DOMAIN_VECTORS, DOMAIN_CLIENT)

#define DACR_UACCESS_DISABLE	\
	(__DACR_DEFAULT | domain_val(DOMAIN_USER, DOMAIN_NOACCESS))
#define DACR_UACCESS_ENABLE	\
	(__DACR_DEFAULT | domain_val(DOMAIN_USER, DOMAIN_CLIENT))

#ifndef __ASSEMBLY__

#ifdef CONFIG_CPU_CP15_MMU
static inline unsigned int get_domain(void)
{
	unsigned int domain;

	asm(
	"mrc	p15, 0, %0, c3, c0	@ get domain"
	 : "=r" (domain)
	 : "m" (current_thread_info()->cpu_domain));

	return domain;
}

static inline void set_domain(unsigned val)
{
	asm volatile(
	"mcr	p15, 0, %0, c3, c0	@ set domain"
	  : : "r" (val) : "memory");
	isb();
}
#else
static inline unsigned int get_domain(void)
{
	return 0;
}

static inline void set_domain(unsigned val)
{
}
#endif

#ifdef CONFIG_CPU_USE_DOMAINS
#define modify_domain(dom,type)					\
	do {							\
		unsigned int domain = get_domain();		\
		domain &= ~domain_mask(dom);			\
		domain = domain | domain_val(dom, type);	\
		set_domain(domain);				\
	} while (0)

#elif defined(CONFIG_PAX_KERNEXEC) || defined(CONFIG_PAX_MEMORY_UDEREF)
#define modify_domain(dom,type)					\
	do {							\
		struct thread_info *thread = current_thread_info(); \
		unsigned int domain = get_domain();		\
		domain &= ~domain_mask(dom);			\
		domain = domain | domain_val(dom, type);	\
		thread->cpu_domain = domain;			\
		set_domain(domain);				\
	} while (0)

#else
static inline void modify_domain(unsigned dom, unsigned type)	{ }
#endif

/*
 * Generate the T (user) versions of the LDR/STR and related
 * instructions (inline assembly)
 */
#ifdef CONFIG_CPU_USE_DOMAINS
#define TUSER(instr)	#instr "t"
#else
#define TUSER(instr)	#instr
#endif

#else /* __ASSEMBLY__ */

/*
 * Generate the T (user) versions of the LDR/STR and related
 * instructions
 */
#ifdef CONFIG_CPU_USE_DOMAINS
#define TUSER(instr)	instr ## t
#else
#define TUSER(instr)	instr
#endif

#endif /* __ASSEMBLY__ */

#endif /* !__ASM_PROC_DOMAIN_H */