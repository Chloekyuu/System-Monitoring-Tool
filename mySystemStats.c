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

/** @brief Send ANSI escape sequence to save cursor position.
 *  @return Void.
 */
void save_cursor(void) {
    printf("\033[s");
}

/** @briefSend ANSI escape sequence to restore cursor to the last saved position.
 *  @return Void.
 */
void restore_cursor(void) {
    printf("\033[u");
}

/** @brief Move the cursor up.
 *  @param lines The number of lines the cursor moves.
 *  @return Void.
 */
void move_up(int lines){
   printf("\033[%dF", lines);
}

/** @brief Move the cursor down.
 *  @param lines The number of lines the cursor moves.
 *  @return Void.
 */
void move_down(int lines){
   printf("\033[%dE", lines);
}

void read_memory_info(int *fd, long *prev_used, int graph) {
    if (pipe(fd) == -1) {           // set up pipe for memory utilization
        perror("pipe");
        exit(1);
    }

    int pid;
    if ((pid = fork()) == -1) {     // create a child to report the usage
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        close(fd[STDIN_FILENO]);    // close the reading end in child

        // change the standard out into our pipe writing fd, such that
        // we can output the information in pipe rather than terminal
        if (dup2(fd[STDOUT_FILENO], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        // report memory usage
        *prev_used = get_memory_info(*prev_used, graph);

        close(fd[STDOUT_FILENO]);   // close the writing end in child
        exit(0);                    // terminate the child
    } else {
        close(fd[STDOUT_FILENO]);   // close the writing end in parent
    }
}

void read_cpu_info(int *fd, int tdelay) {
    if (pipe(fd) == -1) {         // set up pipe for CPU utilization
        perror("pipe");
        exit(1);
    }

    int pid;
    if ((pid = fork()) == -1) {     // create a child to report cpu usage
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        close(fd[STDIN_FILENO]);    // close the reading end in child

        // change the standard out into our pipe writing fd
        if (dup2(fd[STDOUT_FILENO], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        calculate_cpu_use(tdelay);  // report the CPU usage (sleep for tdelay)
        close(fd[STDOUT_FILENO]);   // close the writing end in child
        exit(0);                    // terminate the child
    } else {
        close(fd[STDOUT_FILENO]);  // close the writing end in parent
    }
}

void read_user_info(int *fd) {
    if (pipe(fd) == -1) {          // set up pipe for user connection
        perror("pipe");
        exit(1);
    }

    int pid;
    if ((pid = fork()) == -1) {     // create a child to report the usage
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        close(fd[STDIN_FILENO]);    // close the reading end in child

        // change the standard out into our pipe writing fd
        if (dup2(fd[STDOUT_FILENO], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        show_session_user();        // report user connection
        close(fd[STDOUT_FILENO]);   // close the writing end in child
        exit(0);
    } else {
        close(fd[STDOUT_FILENO]);   // close the writing end in parent
    }
}


void show_sys_usage(int sample, int tdelay, int sys, int user, int graph, int sequential) {
    // set up three pipes, fd[0] for communication of memory use,
    // fd[1] for connected users, and fd[2] for CPU utilization
    int fd[3][2];

    long prev_used = -1e9;          // to store current memory usage
    double cpu_use;
    char mem_info[512], user_info[512];
    FILE *mem_file, *user_file;

    for (int i = 0; i < sample; i ++) {
        if (sys == 1) {
            // create child processes to report system usage
            read_memory_info(fd[0], &prev_used, graph);
            read_cpu_info(fd[2], tdelay);

            if ((mem_file = fdopen(fd[0][STDIN_FILENO], "r")) == NULL) {
                perror("fdopen");
                exit(1);
            }
        }

        if (user == 1) {
            read_user_info(fd[1]);

            if ((user_file = mem_file = fdopen(fd[1][STDIN_FILENO], "r")) == NULL) {
                perror("fdopen");
                exit(1);
            }
        }

        if (sequential == 1) {
            printf(">>> iteration %d\n", i + 1);
            show_runtime_info();
        } else if (i == 0) {
            show_runtime_info();
        }

        if (sys == 1 && (sequential == 1 || i == 0)) {
            printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot) \n");
            // print empty lines reserving space for memory usage
            for (int j = 0; j < i; j ++) {
                printf("\n");
            }
        }

        if (sys == 1) {
            // Read first from child 1
            if (fgets(mem_info, 512, mem_file) == NULL) {
                perror("fgets");
                exit(1);
            } else {
                if (sequential != 1 && i != 0) restore_cursor();
                printf("%s", mem_info);
                if (sequential != 1) save_cursor();
            }

            for (int j = 1; j < sample - i; j ++) {
                printf("\n");
            }
            close(fd[0][STDIN_FILENO]);  // close the reading end in parent
            printf("---------------------------------------\n");
        }

        if (user == 1) {
            if (sys == 0 && sequential == 0 && i != 0) restore_cursor();
            if (sys == 0 && sequential == 0) save_cursor();
            // Now read from child 2
            while (fgets(user_info, 512, user_file) != NULL) {
                printf("%s", user_info);
            }
            close(fd[1][STDIN_FILENO]);  // close the reading end in parent
            if (sys == 0) sleep(tdelay);
        }

        if (sys == 1) {
            // Now read from child 3
            if (read(fd[2][STDIN_FILENO], &cpu_use, sizeof(double)) < 0) {
                perror("read");
                exit(1);
            } else {
                show_cpu_info(cpu_use);
                if (sequential == 1 && graph == 1) {
                    show_cpu_graph(cpu_use);
                } else if (graph == 1) {
                    move_down(i);
                    show_cpu_graph(cpu_use);
                }
            }
            close(fd[2][STDIN_FILENO]);  // close the reading end in parent
            printf("---------------------------------------\n");
        }
    }
    show_sys_info();
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
// void show_usage(int sample, int tdelay, int user_flag, int graph_flag) {

//     printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
    
//     // print empty lines reserving space for memory usage
//     for (int i = 0; i < sample; i ++) {
//         printf("\n");
//     }
//     printf("---------------------------------------\n");
//     move_up(sample + 1);  // then move the cursor up to memory section

//     long men_use = - 1e9; // to store current memory usage
//     int n = 0;            // to store the current user number

//     for (int i = sample - 1; i >= 0; i --) {
//         // print the current memory use and store it in mem_use
//         men_use = get_memory_info(men_use, graph_flag);
//         // go to next section
//         move_down(i + 1);

//         if (user_flag == 1) {
//             n = show_session_user();  // show user section
//         }

//         // print the current CPU use and store it in cpu_use
//         double cpu_use = show_cpu_info(tdelay);

//         // if the "--grphics" being called, show CPU graphics
//         if (graph_flag == 1) {
//             move_down(sample - i - 1);
//             show_cpu_graph(cpu_use);
//             move_up(sample - i);
//         }

//         // move the cursor up to memory section for next memory usage
//         if (user_flag == 1) {
//             move_up(n + 2);
//         }
//         move_up(i + 3);
//     }
//     // move the cursor down to print next section
//     move_down(3);
//     if (user_flag == 1) {
//         move_down(n + 2);
//     }
//     if (graph_flag == 1) {
//         move_down(sample);
//     }
//     printf("---------------------------------------\n");
// }

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
// void show_usage_sequential(int sample, int tdelay, int sys, int user, int graph) {
//     long men_use = - 1e9;   // to store current memory usage
//     int n = 0;              // to store the current user number

//     for (int i = sample; i > 0; i --) {
//         printf(">>> iteration %d\n", sample - i);
//         if (sys == 1) {               // show system memory usage section
//             show_runtime_info();
//             printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");

//             // print empty lines reserving space for memory usage
//             for (int i = 0; i < sample; i ++) {
//                 printf("\n");
//             }
//             printf("---------------------------------------\n");

//             // print the current memory use and store it in mem_use
//             move_up(i + 1);     // find the place for the ith iteration
//             men_use = get_memory_info(men_use, graph);
//             move_down(i);       // go to next section
//         }

//         if (user == 1) {
//             n = show_session_user();  // show user section
//         }

//         if (sys == 1) {               // show cpu usage section
//             double cpu_use = show_cpu_info(tdelay); // show CPU usage

//             // if "--grphics" being called, show CPU graphics
//             if (graph == 1) {
//                 show_cpu_graph(cpu_use);
//             }
//         }
//     }
// }

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
            fprintf(stderr, "Invalid arguments: \"%s\"\n", argv[i]);
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

    // set defalut behaviour: if no "--system" or "--user" called,
    // display the usage for both
    if (sys_flag == 0 && user_flag == 0) {
        sys_flag = 1;
        user_flag = 1;
    }

    // Display system (Memory + User + CPU) usage information only
    show_sys_usage(sample, tdelay, sys_flag, user_flag, graph_flag, sequential_flag);
    
    return 0;
}