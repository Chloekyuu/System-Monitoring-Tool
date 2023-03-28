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

#include "stats_functions.h"

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
void show_usage(int sample, int tdelay, int user_flag, int graph_flag) {

    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
    
    // print empty lines reserving space for memory usage
    for (int i = 0; i < sample; i ++) {
        printf("\n");
    }
    printf("---------------------------------------\n");
    move_up(sample + 1);  // then move the cursor up to memory section

    long men_use = - 1e9; // to store current memory usage
    int n = 0;            // to store the current user number

    for (int i = sample - 1; i >= 0; i --) {
        // print the current memory use and store it in mem_use
        men_use = get_memory_info(men_use, graph_flag);
        // go to next section
        move_down(i + 1);

        if (user_flag == 1) {
            n = show_session_user();  // show user section
        }

        // print the current CPU use and store it in cpu_use
        double cpu_use = show_cpu_info(tdelay);

        // if the "--grphics" being called, show CPU graphics
        if (graph_flag == 1) {
            move_down(sample - i - 1);
            show_cpu_graph(cpu_use);
            move_up(sample - i);
        }

        // move the cursor up to memory section for next memory usage
        if (user_flag == 1) {
            move_up(n + 2);
        }
        move_up(i + 3);
    }
    // move the cursor down to print next section
    move_down(3);
    if (user_flag == 1) {
        move_down(n + 2);
    }
    if (graph_flag == 1) {
        move_down(sample);
    }
    printf("---------------------------------------\n");
}

/** @brief Prints System Usage sample times sequentially without refreshing the screen.
 *
 *  If graph_flag is 1 (i.e. "--graphics" is been called)， print graphics
 *  for memory and CPU usage.
 *
 *  @param sample Number of times the statistics are going to be collected.
 *  @param tdelay Period of time the statistics refresh (in seconds).
 *  @param graph_flag An integer to indicate if "--graphics" is been called
 *  @return int number of user sampled.
 */
void show_usage_sequential(int sample, int tdelay, int sys, int user, int graph) {
    long men_use = - 1e9;   // to store current memory usage
    int n = 0;              // to store the current user number

    for (int i = sample; i > 0; i --) {
        printf(">>> iteration %d\n", sample - i);
        if (sys == 1) {               // show system memory usage section
            show_runtime_info();
            printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");

            // print empty lines reserving space for memory usage
            for (int i = 0; i < sample; i ++) {
                printf("\n");
            }
            printf("---------------------------------------\n");

            // print the current memory use and store it in mem_use
            move_up(i + 1);     // find the place for the ith iteration
            men_use = get_memory_info(men_use, graph);
            move_down(i);       // go to next section
        }

        if (user == 1) {
            n = show_session_user();  // show user section
        }

        if (sys == 1) {               // show cpu usage section
            double cpu_use = show_cpu_info(tdelay); // show CPU usage

            // if "--grphics" being called, show CPU graphics
            if (graph == 1) {
                show_cpu_graph(cpu_use);
            }
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
    printf("---------------------------------------\n");

    // set defalut behaviour: if no "--system" or "--user" called,
    // display the usage for both
    if (sys_flag == 0 && user_flag == 0) {
        sys_flag = 1;
        user_flag = 1;
    }

    if (sequential_flag == 1) {
        show_usage_sequential(sample, tdelay, sys_flag, user_flag, graph_flag);
    } else if (sys_flag == 0 && user_flag == 1) {
        show_runtime_info();
        // Display Users' information only
        show_session_user();
    } else if(sys_flag == 1 && user_flag == 0) {
        show_runtime_info();
        // Display System (Memory + User CPU) Usage information only
        show_usage(sample, tdelay, 0, graph_flag);
    } else {
        // Defalut behaviour
        show_runtime_info();
        // Display system (Memory + User + CPU) usage information only
        show_usage(sample, tdelay, 1, graph_flag); 
    }

    show_sys_info(); // print the OS information
    
    return 0;
}