/*
 * Header file for 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <utmp.h>
#include <unistd.h>

#ifndef __Stats_header
#define __Stats_header

/** @brief Move the cursor up.
 *  @param lines The number of lines the cursor moves.
 *  @return Void.
 */
void move_up(int lines);

/** @brief Move the cursor down.
 *  @param lines The number of lines the cursor moves.
 *  @return Void.
 */
void move_down(int lines);

/** @brief Print the memory usage of the current process in C (in kilobytes).
 *  @return Void.
 */
void show_runtime_info();

/** @brief Virtualize the physical memory usage difference.
 *
 *  ::::::@ denoted that the total relative change is negative.
 *  ######* denoted that the total relative change is positive.
 *  |o denoted the total relative change is positive infinitesimal
 *  |@ denoted the total relative change is negative infinitesimal
 *
 *  @param curr_use Current-used physical memory.
 *  @param previous_use Previous-used phisical memory.
 *  @return Void.
 */
void show_memory_graph(double curr_use, double previous_use);

/** @brief Prints the physical and virsual memory usage compared to total (in gigabytes).
 *
 *  Read memory information from the Linux file "/proc/meminfo",
 *  and calculate the memory usage based on following equations:
 *      Total Physical Memory = MemTotal
 *      Used Physical Memory = MemTotal - MemFree - (Buffers + Cached Memory)
 *      Total Virtual Memory = MemTotal + SwapTotal
 *      Used Virtual Memory = Total Virtual Memory - SwapFree - MemFree - (Buffers + Cached Memory)
 *  where,
 *      Cached memory = Cached + SReclaimable
 *  If the "--graphics" argument is used during compiling, then
 *  print the phisical memory usage difference between the current
 *  and previous stage.
 *
 *  @param previous_use The physical memory used from the previous iteration.
 *  @param graph_flag An interger indicating whether "--graphics" argument is used during compiling.
 *  @return The physical memory used at current stage.
 */
long get_memory_info(long previous_use, int graph_flag);

/** @brief Calculate CPU usage (in percentage) in real-time.
 * 
 *  Read the CPU information from Linux file "/proc/stat", and
 *  calculate the CPU usage percentage based on following equations:
 *      CPU (%) = (use_diff / total_diff) * 100
 *      use_diff = use_curr - use_prev
 *      total_diff = total_curr - total_prev
 *  where,
 *      use = user + nice + system + irq + softirq
 *      total = user + nice + system + idle + iowait + irq + softirq
 *
 *  @param tdelay The period of time system between each time reading CPU info.
 *  @return A double of the current CPU usage (in percentage).
 */
double calculate_cpu_use(int tdelay);

/** @brief Virtualize the CPU usage (in percentage).
 *
 *  Use '|' to denote the positive percentage increase.
 *  The number of '|' is in proportion to the CPU usage percentage.
 *
 *  @param percent A double representing CPU usage at current stage.
 *  @return Void.
 */
void show_cpu_graph(double percent);

/** @brief Prints the number of CPU cores and CPU usage percentage.
 *  @return Void.
 */
double show_cpu_info(int tdelay);

/** @brief Prints User Usage information.
 *
 *  Use getutent() function from <utmp.h> library to get the user usage.
 *  Print user's name, type of the terminal device, and their remote IP
 *  address iff the username exists.
 *
 *  @return Void.
 */
int show_session_user();

/** @brief Prints system information.
 *
 *  Use uname from <sys/utsname.h> library to get the system information.
 *  Print the OS name, release information, architecture, and version of OS.
 *
 *  @return Void.
 */
void show_sys_info();


/** @brief Validate the command line arguments user gived.
 *
 *  Use flags to indicate whether an argument is been called.
 *  Set the flag variables to 1 if does, and 0 if not.
 *  If any argument call violates the assumptions (see README.txt
 *  part c "Assumptions made:"), report an error.
 *  Note that "--samples=N" and "--tdelay=T" can be considered as
 *  positional arguments if they are not flagged.
 *
 *  @param argc Number of ommand line arguments.
 *  @param argv The array of strings storing command line arguments.
 *  @param sample Number of times the statistics are going to be collected.
 *  @param tdelay Period of time the statistics refresh (in seconds).
 *  @param sys_flag Point to an integer to indicate if "--system" is been called
 *  @param user_flag Point to an integer to indicate if "--user" is been called
 *  @param sequential_flag Point to an integer to indicate if "--sequential" is been called
 *  @param graph_flag Point to an integer to indicate if "--graphics" is been called
 *  @param sample_flag Point to an integer to indicate if "--sample=N" is been called
 *  @param tdelay_flag Point to an integer to indicate if "--tdelay=T" is been called
 *  @return Void.
 */
void vertify_arg(int argc, char *argv[], int *sample, int *tdelay, int *sys_flag,
    int *user_flag, int *sequential_flag, int *graph_flag, int *sample_flag, int *tdelay_flag);


#endif