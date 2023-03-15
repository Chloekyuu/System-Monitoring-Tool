/** @file mySystemStats.c
 *  @brief A report different metrics of the utilization of a system
 *
 *  This file can help user to get basic system information, including
 *  system's memory usage, users' usage, CPU usage and OS information.
 *  You can display the information in the way you like, refreshing N
 *  times or print out sequentially, showing system or user usage only,
 *  or with graphics that can virtualize the system usage change.
 *  Also you are free to decide how many times the statistics will be
 *  displayed and interval between each time the information prints.
 *
 *  @author Huang Xinzi
 *  @bug No known bugs.
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

/** @brief Handle errors.
 * 
 *  Display error message and then terminate the program.
 * 
 *  @return Void.
 */
void handle_error(char *message) {
    printf("%s\n", message);
    exit(0);
}

/** @brief Move the cursor up.
 *  @param lines The number of lines the cursor moves.
 *  @return Void.
 */
void move_up(int lines){
   printf("\x1b[%dF", lines);
}

/** @brief Move the cursor down.
 *  @param lines The number of lines the cursor moves.
 *  @return Void.
 */
void move_down(int lines){
   printf("\x1b[%dE", lines);
}

/** @brief Print the memory usage of the current process in C (in kilobytes).
 *  @return Void.
 */
void show_runtime_info() {
    struct rusage r_usage; // A variable to store resource usage section
    if (getrusage(RUSAGE_SELF,&r_usage) < 0) { // Get the resource usage statistics
        perror("rusage"); // If fail to get, report the error
    } else {
        // Print the maximum resident set size used (in kilobytes).
        printf(" Memory usage: %ld kilobytes\n", r_usage.ru_maxrss);
    }
}

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
void show_memory_graph(double curr_use, double previous_use) {
    // Get the difference between current-used and previous-used phisical memory.
    double diff = curr_use - previous_use;

    printf("  |"); // A sign indicating the start of our graph

    if ((diff < 0.01 && diff >= 0.0) || previous_use == -1) {
        // If the difference is a positive infinitesimal or it's the first iteration
        printf("o 0.00 (%.2f)\n", curr_use);
    } else if (diff <= 0.0 && diff > -0.01) {
        // If the difference is a negative infinitesimal
        printf("@ 0.00 (%.2f)\n", curr_use);
    } else if (diff >= 0.01) {
        // If the difference is positive, print '#' in proportion
        for (int i = 0; i < diff * 100; i ++) {
            printf("#");
        }
        // Print an asterisk to indicate the graph is end
        printf("* %.2f (%.2f)\n", diff, curr_use);
    } else {
        // If the difference is positive, print ':' in proportion
        for (int i = 0; i > diff * 100; i --) {
            printf(":");
        }
        // Print '@' to indicate the graph is end
        printf("@ %.2f (%.2f)\n", diff, curr_use);
    }
}

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
long get_memory_info(long previous_use, int graph_flag) {
    FILE *meminfo;   // A file pointer pointing to "/proc/meminfo"
    char line[200];  // A string storing each line of the file
    long totalram, freeram, bufferram, cachedram, totalswap, freeswap, sreclaimable;
    long phys_used, total_phys, virtual_used, total_virtual;

    // If cannot open the file, report an error
    if((meminfo = fopen("/proc/meminfo", "r")) == NULL) {
        perror("fopen");
    }

    // Loop the file util read all the information we need
    while(fgets(line, sizeof(line), meminfo)) {
        if(sscanf(line, "MemTotal: %ld kB", &totalram) == 1) {
            totalram *= 1024;       // Convert into kilobytes
            total_phys = totalram;  // Total Physical Memory = MemTotal
        } else if (sscanf(line, "MemFree: %ld kB", &freeram) == 1){
            freeram *= 1024;
        } else if (sscanf(line, "Buffers: %ld kB", &bufferram) == 1) {
            bufferram *= 1024;
        } else if (sscanf(line, "Cached: %ld kB", &cachedram) == 1) {
            cachedram *= 1024;
        } else if (sscanf(line, "SwapTotal: %ld kB", &totalswap) == 1) {
            totalswap *= 1024;
            total_virtual = totalram + totalswap;  // Total Virtual Memory = MemTotal + SwapTotal
        } else if (sscanf(line, "SwapFree: %ld kB", &freeswap) == 1) {
            freeswap *= 1024;
        } else if (sscanf(line, "SReclaimable: %ld kB", &sreclaimable) == 1) {
            sreclaimable *= 1024;
            cachedram += sreclaimable;  // Cached memory = Cached + SReclaimable
            // Used Physical Memory = MemTotal - MemFree - (Buffers + Cached Memory)
            phys_used = (totalram - freeram) - (bufferram + cachedram);
            // Used Virtual Memory = MemTotal + SwapTotal - SwapFree - MemFree - (Buffers + Cached Memory)
            virtual_used = phys_used + totalswap - freeswap;

            // After reading the last argument, close the file.
            // (Assume the informtion in "/proc/meminfo" is in default order)
            // If fails, report an error.
            if (fclose(meminfo) != 0) {
                perror("fclose");
            }
        }
    }
    // Display the memory usage information
    printf("%.2f GB / %.2f GB  -- %.2f GB / %.2f GB", phys_used * 1e-9,
        total_phys * 1e-9, virtual_used * 1e-9, total_virtual * 1e-9);

    // If the "--graphics" argument is used during compiling,
    // virtualize the physical memory usage difference.
    if (graph_flag == 1) {
        show_memory_graph(phys_used * 1e-9, previous_use * 1e-9);
    } else {
        printf("\n");
    }

    // Return the physical memory usage at current stage.
    return phys_used;
}

/** @brief Prints the number of CPU cores.
 *  @return Void.
 */
void show_core_info() {
    // Get the number of processors which are currently online.
    int cores = (int) sysconf(_SC_NPROCESSORS_ONLN);

    // If on error (return -1), report the error.
    // Otherwise print the information.
    if (cores < 0) {
        perror("sysconf");
    } else {
        printf("---------------------------------------\n");
        printf("Number of cores: %d\n", cores);
    }
}

/** @brief Prints CPU usage (in percentage) in real-time.
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
double show_cpu_info(int tdelay) {
    FILE * stat; // A file pointer pointing to "/proc/stat"

    // If fail to open the file, report an error
    if((stat = fopen("/proc/stat", "r")) == NULL) {
        perror("fopen");
    }

    // Variables storing the CPU information read from "/proc/stat"
    int user_prev, nice_prev, system_prev, idle_prev, iowait_prev, irq_prev, softirq_prev;
    int user_cur, nice_cur, system_cur, idle_cur, iowait_cur, irq_cur, softirq_cur;
    
    // Read the first line of the file "/proc/stat"
    fscanf(stat, "cpu %d %d %d %d %d %d %d", &user_prev, &nice_prev, &system_prev,
        &idle_prev, &iowait_prev, &irq_prev, &softirq_prev);

    // Wait for a period of time
    sleep(tdelay);

    // Read the file again, store the new CPU info in xxx_cur variables
    rewind(stat);
    fscanf(stat, "cpu %d %d %d %d %d %d %d", &user_cur, &nice_cur, &system_cur,
        &idle_cur, &iowait_cur, &irq_cur, &softirq_cur);

    // Close the file. If fails, report an error.
    if (fclose(stat) != 0) {
        perror("fclose");
    }

    // idle = idle + iowait
    int idle_previous = idle_prev + iowait_prev;
    int idle_current = idle_cur + iowait_cur;

    // use = user + nice + system + irq + softirq
    int use_prev = user_prev + nice_prev + system_prev + irq_prev + softirq_prev;
    int use_cur = user_cur + nice_cur + system_cur + irq_cur + softirq_cur;

    // use_diff = use_curr - use_prev
    int numerator = use_cur - use_prev;
    // total = user + nice + system + idle + iowait + irq + softirq = use + idle
    // total_diff = total_curr - total_prev = use_diff + idle_diff
    int denominator = numerator + idle_current - idle_previous;

    // CPU (%) = (use_diff / total_diff) * 100
    double cpu_use = 100 * (double)numerator / (double)denominator;

    printf(" total cpu use = %.2f%%\n", cpu_use);

    // Return the current CPU usage for graphics
    return cpu_use;
}

/** @brief Virtualize the CPU usage (in percentage).
 *
 *  Use '|' to denote the positive percentage increase.
 *  The number of '|' is in proportion to the CPU usage percentage.
 *
 *  @param percent A double representing CPU usage at current stage.
 *  @return Void.
 */
void show_cpu_graph(double percent) {
    printf("\t");
    // Print '|' in proportion to the CPU usage percentage
    for (int i = 0; i < (int) percent / 2; i ++) {
        printf("|");
    }
    // Print the CPU usage percentage at the end of the graph
    printf("%f\n", percent);
}

/** @brief Prints System Usage sample times with refreshing in every tdelay secs.
 *
 *  If graph_flag is 1 (i.e. "--graphics" is been called)， print graphics
 *  for memory and CPU usage.
 *
 *  @param sample Number of times the statistics are going to be collected.
 *  @param tdelay Period of time the statistics refresh (in seconds).
 *  @param graph_flag An integer to indicate if "--graphics" is been called
 *  @return Void.
 */
void show_sys_usage(int sample, int tdelay, int graph_flag) {
    printf("---------------------------------------\n"
        "### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
    
    // print empty lines reserving space for memory usage
    for (int i = 0; i < sample; i ++) {
        printf("\n");
    }

    show_core_info();    // show core infomrtaion
    move_up(sample + 2); // then move the cursor up to memory section

    long men_use = - 1e9; // to store current memory usage

    for (int i = sample; i > 0; i --) {
        // print the current memory use and store it in mem_use
        men_use = get_memory_info(men_use, graph_flag);
        // go to CPU section
        move_down(i + 1);
        // print the current CPU use and store it in cpu_use
        double cpu_use = show_cpu_info(tdelay);

        // if the graph flag is 1 ("--grphics" being called),
        // show CPU graphics
        if (graph_flag == 1) {
            move_down(sample - i);
            show_cpu_graph(cpu_use);
            move_up(sample - i + 1);
        }

        // move the cursor up to memory section for next memory usage
        move_up(i + 2);
    }
    move_down(3);
    if (graph_flag == 1) {
        move_down(sample);
    }
}

/** @brief Prints System Usage sample times sequentially without refreshing the screen.
 *
 *  If graph_flag is 1 (i.e. "--graphics" is been called)， print graphics
 *  for memory and CPU usage.
 *
 *  @param sample Number of times the statistics are going to be collected.
 *  @param tdelay Period of time the statistics refresh (in seconds).
 *  @param graph_flag An integer to indicate if "--graphics" is been called
 *  @return Void.
 */
void show_usage_sequential(int sample, int tdelay, int graph_flag) {
    long men_use = - 1e9;  // to store current memory usage

    for (int i = sample; i > 0; i --) {
        printf(">>> iteration %d\n", sample - i);
        show_runtime_info();
        printf("---------------------------------------\n"
            "### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");

        // print empty lines reserving space for memory usage
        for (int i = 0; i < sample; i ++) {
            printf("\n");
        }

        move_up(i);
        // print the current memory use and store it in mem_use
        men_use = get_memory_info(men_use, graph_flag);
        move_down(i - 1);  // go to CPU section
        show_core_info();  // show core infomrtaion
        double cpu_use = show_cpu_info(tdelay); // show CPU usage

        // if the graph flag is 1 ("--grphics" being called),
        // show CPU graphics
        if (graph_flag == 1) {
            show_cpu_graph(cpu_use);
        }
    }
}

/** @brief Prints User Usage information.
 *
 *  Use getutent() function from <utmp.h> library to get the user usage.
 *  Print user's name (if has), type of the terminal device, and their
 *  remote IP address (if has).
 *
 *  @return Void.
 */
void show_session_user() {
    struct utmp *users; // a variable to store user information
    users = getutent(); // get the information about who is currently using the system.

    printf("---------------------------------------\n"
        "### Sessions/users ###\n");

    // while we still have users, print their information
    while(users != NULL) {
        // print username (if exists)
        if(strcmp(users -> ut_user, "") != 0) {
            printf(" %s\t", users -> ut_user);
        } else {
            printf("\t\t");
        }
        // print which type of the terminal device is being used
        printf("%s ", users -> ut_line);
        // print remote IP address (if exists)
        if(strcmp(users -> ut_host, "") != 0) {
            printf("(%s)\n", users -> ut_host);
        } else {
            printf("\n");
        }
        users = getutent(); // get the next user information
    }
}

/** @brief Prints system information.
 *
 *  Use uname from <sys/utsname.h> library to get the system information.
 *  Print the OS name, release information, architecture, and version of OS.
 *
 *  @return Void.
 */
void show_sys_info() {
    struct utsname uts;       // A variable to store system information

    if (uname(&uts) < 0) {    // Get the system information
        perror("Get Uname");  // If fails, report an error
    }

    printf("---------------------------------------\n"
        "### System Information ###\n");
    printf(" System Name = %s\n", uts.sysname);
    printf(" Machine Name = %s\n", uts.nodename);
    printf(" Version = %s\n", uts.version);
    printf(" Release = %s\n", uts.release);
    printf(" Architecture = %s\n", uts.machine);
    printf("---------------------------------------\n");
}

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
    int *user_flag, int *sequential_flag, int *graph_flag, int *sample_flag, int *tdelay_flag) {

    int tmp_sample;  // Store temporary sample size
    int tmp_tdelay;  // Store temporary tdelay secs
    int positional;  // Store temporary positional argument
    int positional_arg = 0;  // Store the number of positional arguments

    for (int i = 1; i < argc; i ++) {
        if (strcmp(argv[i], "--system") == 0) {
            if (*user_flag == 0) {
                // If the "--user" command is not called, it's ok to have "--system" command
                *sys_flag = 1;
            } else {
                handle_error("Arguments \"--system\" and \"--user\" cannot be used together!");
            }
        } else if (strcmp(argv[i], "--user") == 0) {
            if (*sys_flag == 0) {
                // If the "--system" command is not called, it's ok to have "--user" command
                *user_flag = 1;
            } else {
                handle_error("Arguments \"--system\" and \"--user\" cannot be used together!");
            }
        } else if (strcmp(argv[i], "--graphics") == 0) {
            *graph_flag = 1; // set the flag to 1
        } else if (strcmp(argv[i], "--sequential") == 0) {
            *sequential_flag = 1; // set the flag to 1
        } else if (sscanf(argv[i], "--samples=%d", &tmp_sample) == 1) {
            if (*sample_flag == 0) {
                // If this is the first "--samples=N" argument called,
                // set the sample value to the input sample value
                // and label sample_flag to 1.
                *sample = tmp_sample;
                *sample_flag = 1;
            } else if (*sample != tmp_sample) {
                // If this value does not corresponds to other sample size value,
                // print an error message.
                handle_error("The value given to \"--samples=N\" should be consistent!");
            }
            if (*sample <= 0) {
                // If the value is negative, print an error messgae
                handle_error("The value given to \"--samples=N\" should be an positive integer!");
            }
        } else if (sscanf(argv[i], "--tdelay=%d", &tmp_tdelay) == 1) {
            if (*tdelay_flag == 0) {
                // If this is the first "--tdelay=T" argument called,
                // set the tdelay value to the input tdelay value
                // and label tdelay_flag to 1.
                *tdelay = tmp_tdelay;
                *tdelay_flag = 1;
            } else if (*tdelay != tmp_tdelay) {
                // If this value does not corresponds to previous tdelay value,
                // print an error message.
                handle_error("The value given to \"--tdelay=T\" should be consistent!");
            }
            if (*tdelay < 0) {
                // If the value is negative, print an error messgae
                handle_error("The value given to \"--tdelay=T\" should be an positive integer");
            }
        } else if (sscanf(argv[i], "%d", &positional) == 1){
            // If the argument is a single integer
            if (positional_arg == 0) {
                // If this is the first integer appeared in the arguments,
                // this would be the sample size value.
                if (*sample_flag == 0 || *sample == positional) {
                    // If it corresponds to other sample size value (if exists),
                    // set sample value to positional and label sample_flag as 1
                    *sample = positional;
                    *sample_flag = 1;
                } else {
                    handle_error("The value given to \"--samples=N\" should be consistent!");
                }
                if (*sample <= 0) {
                    // If the value is negative, print an error messgae
                    handle_error("The value given to \"--samples=N\" should be an positive integer!");
                }
            } else if (positional_arg == 1) {
                // If this is the second integer appeared in the arguments,
                // this would be the tdelay value.
                if (*tdelay_flag == 0 || *tdelay == positional) {
                    // If it corresponds to previous tdelay value (if exists),
                    // set tdelay to positional and label tdelay_flag as 1
                    *tdelay = positional;
                    *tdelay_flag = 1;
                } else {
                    handle_error("The value given to \"--tdelay=T\" should be consistent!");
                }
                if (*sample <= 0) {
                    // If the value is negative, print an error messgae
                    handle_error("The value given to \"--tdelay=T\" should be an positive integer!");
                }
            } else {
                // If we already have 2 single integers as arguments,
                // then print an error message
                handle_error("No more than 2 single integers can be taken as valid arguments.");
            }
            positional_arg ++;
        } else {
            // If the statement is in other format, print an error message
            printf("Invalid arguments: \"%s\"\n", argv[i]);
            exit(0);
        }
    }
}

/** @brief Report different metrics of the utilization of a given system.
 *
 *  @param argc Number of ommand line arguments.
 *  @param argv The array of strings storing command line arguments.
 *  @return An integer.
 */
int main (int argc, char *argv[]) {
    // initialize sample and tdelay to their defalut value
    int sample = 10;
    int tdelay = 1;
    // create flags for each argument to check if they're called
    int sys_flag = 0;
    int user_flag = 0;
    int sequential_flag = 0;
    int graph_flag = 0;
    int sample_flag = 0;
    int tdelay_flag = 0;

    // validate the arguments
    vertify_arg(argc, argv, &sample, &tdelay, &sys_flag, &user_flag,
        &sequential_flag, &graph_flag, &sample_flag, &tdelay_flag);

    // print the values of sample size and tdelay
    printf("Nbr of samples: %d -- every %d secs\n", sample, tdelay);

    if(sequential_flag == 1 && sys_flag == 1) {
        show_usage_sequential(sample, tdelay, graph_flag);
    } else if(sys_flag == 1) {
        show_runtime_info();
        // display System (Memory + CPU) Usage information only
        show_sys_usage(sample, tdelay, graph_flag);
    } else if (user_flag == 1) {
        show_runtime_info();
        // display Users' information only
        show_session_user();
    } else if (sequential_flag == 1) {
        show_usage_sequential(sample, tdelay, graph_flag);
        show_session_user();
    } else {
        show_runtime_info();
        // display system and user usage
        show_sys_usage(sample, tdelay, graph_flag); 
        show_session_user();
    }

    show_sys_info(); // print the OS information
    
    return 0;
}