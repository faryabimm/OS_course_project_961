#include <generated/utsrelease.h>

/* Simply sanity version stamp for modules. */
#ifdef CONFIG_SMP
#define MODULE_VERMAGIC_SMP "SMP "
#else
#define MODULE_VERMAGIC_SMP ""
#endif
#ifdef CONFIG_PREEMPT
#define MODULE_VERMAGIC_PREEMPT "preempt "
#else
#define MODULE_VERMAGIC_PREEMPT ""
#endif
#ifdef CONFIG_MODULE_UNLOAD
#define MODULE_VERMAGIC_MODULE_UNLOAD "mod_unload "
#else
#define MODULE_VERMAGIC_MODULE_UNLOAD ""
#endif
#ifdef CONFIG_MODVERSIONS
#define MODULE_VERMAGIC_MODVERSIONS "modversions "
#else
#define MODULE_VERMAGIC_MODVERSIONS ""
#endif
#ifndef MODULE_ARCH_VERMAGIC
#define MODULE_ARCH_VERMAGIC ""
#endif

#ifdef CONFIG_PAX_REFCOUNT
#define MODULE_PAX_REFCOUNT "REFCOUNT "
#else
#define MODULE_PAX_REFCOUNT ""
#endif

#ifdef CONSTIFY_PLUGIN
#define MODULE_CONSTIFY_PLUGIN "CONSTIFY_PLUGIN "
#else
#define MODULE_CONSTIFY_PLUGIN ""
#endif

#ifdef STACKLEAK_PLUGIN
#define MODULE_STACKLEAK_PLUGIN "STACKLEAK_PLUGIN "
#else
#define MODULE_STACKLEAK_PLUGIN ""
#endif

#ifdef RANDSTRUCT_PLUGIN
#include <generated/randomize_layout_hash.h>
#define MODULE_RANDSTRUCT_PLUGIN "RANDSTRUCT_PLUGIN_" RANDSTRUCT_HASHED_SEED
#else
#define MODULE_RANDSTRUCT_PLUGIN
#endif

#ifdef CONFIG_GRKERNSEC
#define MODULE_GRSEC "GRSEC "
#else
#define MODULE_GRSEC ""
#endif

#define VERMAGIC_STRING 						\
	UTS_RELEASE " "							\
	MODULE_VERMAGIC_SMP MODULE_VERMAGIC_PREEMPT 			\
	MODULE_VERMAGIC_MODULE_UNLOAD MODULE_VERMAGIC_MODVERSIONS	\
	MODULE_ARCH_VERMAGIC						\
	MODULE_PAX_REFCOUNT MODULE_CONSTIFY_PLUGIN MODULE_STACKLEAK_PLUGIN \
	MODULE_GRSEC MODULE_RANDSTRUCT_PLUGIN
