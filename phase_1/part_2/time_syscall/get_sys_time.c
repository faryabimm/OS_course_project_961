/*
 * In the name of Allah
 * Sharif University of Technology
 * Department of Computer Engineering
 * Operating Systems course (40-424)
 * Project # 1
 * Part 2
 * Getting System Time info using syscalls in android
 *
 * Mohammadmahdi Faryabi:	93101951
 * Mohammadhosein A'lami:	94104401
 */

#include <stdio.h>		// usage: working with standard output
#include <unistd.h>		// usage: system call function
#include <sys/syscall.h>	// usage: syscall name table
#include <errno.h>		// usage: error description
#include <time.h>		// usage: struct tm and  localtime functions

int main() {
	struct timeval system_time_value;	// syscall input arguments
	struct timezone system_time_zone;

	// system call
	int status = \
	syscall(SYS_gettimeofday, &system_time_value, &system_time_zone);
	
	if (status == 0) {	// success scenario
//		fprintf(stdout, "OPERATION WAS SUCCESSFULL!\n");

		// parsing system call output using function and DS in time.h library
		struct tm * detailed_time = localtime(&(system_time_value.tv_sec));
		int year = detailed_time->tm_year + 1900;
		int month = detailed_time->tm_mon + 1;
		int month_day = detailed_time->tm_mday;
		int year_day = detailed_time->tm_yday + 1;
		int week_day = detailed_time->tm_wday + 1;
		int hour = detailed_time->tm_hour; 
		int minute = detailed_time->tm_min;
		int second = detailed_time->tm_sec; 
		int msecond = system_time_value.tv_usec / 1000; 
		int usecond = system_time_value.tv_usec - msecond * 1000;


		// printing results in standart output (stdout)
		fprintf(stdout, "Date and Time:\t\t\t");
		fprintf(stdout, "%02d/%02d/%d  -  %02d:%02d:%02d..%03d.%03d\n", 
				month, month_day, year, hour, minute, second,
				msecond, usecond);
		fprintf(stdout, "Daylight Saving Time:\t\t%s\n", 
				detailed_time->tm_isdst ? "YES" : "NO");
		fprintf(stdout, "DST Correction Type:\t\t%d\n",
				system_time_zone.tz_dsttime);
		fprintf(stdout, "Minutes East of Greenwich:\t%d\n", 
				-(system_time_zone.tz_minuteswest));
	} else {	// failure scenario
		fprintf(stderr, "OPERATION FAILED: %s\n", strerror(errno));
	}
	return 0;
}
