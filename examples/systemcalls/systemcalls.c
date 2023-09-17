/*****************************************************************************
 * Copyright (C) 2023 by Chandana Challa
 *
 * Redistribution, modification or use of this software in source or binary
 * forms is permitted as long as the files maintain this copyright. Users are
 * permitted to modify this and use it to learn about the field of embedded
 * software. Chandana Challa and the University of Colorado are not liable for
 * any misuse of this material.
 *
 *****************************************************************************/
/**
 * @file systemcalls.c
 * @brief This file contains system calls functionality
 *
 * @author Chandana Challa
 * @date Sep 17 2023
 * @version 1.0
 *
 *
 * Citations:
 * Slides from AESD course
 */

// Header files
#include "systemcalls.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

// Macro definitions
#define SUCCESS    (0)
#define FAILURE    (-1)
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int status = -1;
    int return_value = system(cmd);
    if (SUCCESS != return_value)
    {
        printf("ERROR executing system call cmd=%s: %s\n", cmd, strerror(errno));
        return false;
    }
    //check for non-zero return value
    if (WIFEXITED(status))
    {
        if (0 == WEXITSTATUS(status))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    printf("System call executed successfully\n");
    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    int status = -1;
    fflush(stdout);
    pid_t pid = fork();
    if (FAILURE == pid)
    {
        printf("ERROR creating a child process:%s\n", strerror(errno));
        va_end(args);
        return false;
    }
    else if (0 == pid)
    {
        printf("Child process created successfully\n");
        if (FAILURE == execv(command[0], command))
        {
            printf("ERROR executing execv: %s\n", strerror(errno));
            va_end(args);
            exit(FAILURE);
        }
    }
    else
    {
        if (FAILURE == waitpid(pid, &status, WUNTRACED|WCONTINUED))
        {
            printf("ERROR executing wait: %s\n", strerror(errno));
            va_end(args);
            return false;
        }
        //check for non-zero return value
        if (WIFEXITED(status))
        {
            if (0 == WEXITSTATUS(status))
            {
                va_end(args);
                return true;
            }
            else
            {
                va_end(args);
                return false;
            }
        }
    }
    va_end(args);
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    int status = -1;
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (FAILURE == fd)
    {
        printf("ERROR opening file:%s\n", strerror(errno));
        va_end(args);
        return false;
    }
    fflush(stdout);
    pid_t pid = fork();
    if (FAILURE == pid)
    {
        printf("ERROR creating a child process:%s\n", strerror(errno));
        close(fd);
        va_end(args);
        return false;
    }
    else if (0 == pid)
    {
        printf("Child process created successfully\n");
        if (FAILURE == dup2(fd, 1))
        {
            printf("ERROR executing dup2: %s\n", strerror(errno));
            close(fd);
            va_end(args);
            return false;
        }
        if (FAILURE == execv(command[0], command))
        {
            printf("ERROR executing execv: %s\n", strerror(errno));
            close(fd);
            va_end(args);
            return false;
        }
    }
    else
    {
        close(fd);
        if (FAILURE == waitpid(pid, &status, WUNTRACED|WCONTINUED))
        {
            printf("ERROR executing wait: %s\n", strerror(errno));
            va_end(args);
            return false;
        }
        //check for non-zero return value
        if (WIFEXITED(status))
        {
            if (0 == WEXITSTATUS(status))
            {
                va_end(args);
                return true;
            }
            else
            {
                va_end(args);
                return false;
            }
        }
    }
    va_end(args);

    return true;
}
