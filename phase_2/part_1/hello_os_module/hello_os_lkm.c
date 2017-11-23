/*
 * In the name of Allah
 * Sharif University of Technology
 * Department of Computer Engineering
 * Operating Systems course (40-424)
 * Project # 2
 * Part 1
 * Writing a simple Loadable Kernel Module
 *
 * Mohammadmahdi Faryabi:	93101951
 * Mohammadhosein A'lami:	94104401
 *
 * @file	hello_os_lkm.c
 * @author	Mohammadmahdi Faryabi / Mohammadhosein A'lami
 * @date	Thursday Azar 2 1396
 * @brief	a simple Loadable Kernel Module (LKM) that will write "Hello OS" message
 *  in /var/log/kern.log file upon loading. this file can be read with dmesg bash command
    with super user access privilages
 */

#include <linux/module.h>		// added for creating a kernel module (kernel info macros).
#include <linux/kernel.h>		// added for specifing kernel message severity level.
#include <linux/init.h>			// added for using load and unload macros.

MODULE_LICENSE("GPL");			// GNU public license v2 or later
MODULE_AUTHOR("Mohammadmahdi Faryabi");
MODULE_AUTHOR("Mohammadhosein A'lami");
MODULE_DESCRIPTION("A simple LKM which prints \"Hello OS\" message in "
	"/var/log/kern.log file upon loading. This log file can be accessed "
	"using dmesg bash command with superuser access privilages.");

MODULE_VERSION("1.0");

static int __init hello_os_lkm_start(void) {
	// this function will be called upon loading the module in kernel.
	printk(KERN_INFO "Hello OS\n");
	return 0;
}

/*
 * kernel module severity levels defined in linux/kernel.h:
 * 1 -> KERN_EMERG: used for emergency messages that usually produce a crash.
 * 2 -> KERN_ALERT: a message indicating that immediate action is required.
 * 3 -> KERN_CRIT: critical situation related to hardware/software malfunction.
 * 4 -> KERN_ERR: error conditions. usually device drivers issue this type of message to indicate hardware problems.
 * 5 -> KERN_WARNING: a problematic situation not causing serious system problems.
 * 6 -> KERN_NOTICE: normal situations worth noticing.
 * 7 -> KERN_INFO: information messages about anything important.
 * 8 -> KERN_DEBUG: used for debugging messages.
 */

static void __exit hello_os_lkm_end(void) {
	// cleanup function.
	// this function will be called upon unloading the module from kernel.
}

// assigning the functions to be called upon loading/unloading the module.
module_init(hello_os_lkm_start);
module_exit(hello_os_lkm_end);
