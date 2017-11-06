/*
 * In the name of Allah
 * Sharif University of Technology
 * Department of Computer Engineering
 * Operating Systems course (40-424)
 * Project # 1
 * Part 2
 * Getting CPU utilization info using syscalls in android
 *
 * Mohammadmahdi Faryabi:	93101951
 * Mohammadhosein A'lami:	94104401
 */

#include <stdio.h>		// usage: working with standard output
#include <unistd.h>		// usage: system call function
#include <sys/syscall.h>	// usage: syscall name table
#include <errno.h>		// usage: error description
#include <fcntl.h>		// usage: specifyind file open mode

#define BUFFER_SIZE 1024

int main() {
	// opening /proc/stat file in read-only mode
	char buffer[BUFFER_SIZE];
	int proc_stat_fd = \
	syscall(SYS_open, "/proc/stat", O_RDONLY);

	if (proc_stat_fd == -1) {	// failure scenario
		fprintf(stdout, "PROBLEM ACCESSING FILE: %s", strerror(errno));
	} else {			// success scenario
		// writing content of file into buffer and closing the file
		syscall(SYS_read, proc_stat_fd, buffer, BUFFER_SIZE - 1);
		syscall(SYS_close, proc_stat_fd);
	
		// variables for storing different file information
		int user,	/*time spent in user mode*/
		    nice,	/*time spent in user mode with low priority*/	
		    system,	/*time spent in kernrl mode*/
		    idle,	/*time spent in idle task*/
		    io_wait,	/*time spent in wait for io completion (unreliable)*/
		    irq,	/*time spent in interrupt handlers*/
		    soft_irq,	/*time spent in software interrupt handlers*/
		    steal,	/*time spent in others OSes when running a virtualized env*/
		    guest,	/*time spent in running a virtual CPU for a guest OS*/
		    guest_nice;	/*time spent in running a niced guest*/
	
		// reading file data
		sscanf(buffer, "%*s%d%d%d%d%d%d%d%d%d%d", &user, &nice, &system, &idle,
				&io_wait, &irq, &soft_irq, &steal, &guest, &guest_nice);
		// calculating total cpu time and idle cpu time
		int total_cpu_time = user + nice + system + idle + io_wait + irq \
				     + soft_irq + steal + guest + guest_nice;
		int cpu_idle_time = idle;

		// calculating average cpu utilization percentage
		double average_idle_percentage = cpu_idle_time * 100.0 / total_cpu_time; 
		double average_util_percentage = 100.0 - average_idle_percentage;
		
		// reporting the result
		fprintf(stdout, "AVERAGE CPU UTILIZATION: %lf%c\n",
				average_util_percentage, '%');
	}
	return 0;
}
