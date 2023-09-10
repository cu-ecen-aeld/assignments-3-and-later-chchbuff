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
 * @file writer.c
 * @brief This file contains functionality to write a string to a file.
 *
 * To compile: make
 *
 * @author Chandana Challa
 * @date Sep 10 2023
 * @version 1.0
 * @resources AESD Course Slides
 */

/* Header files */
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

/* Macro definitions */
#define ARG_COUNT    (3)

/* Function definitions */

/**
 * @brief Prints usage of writer application
 *
 * Command format and the parameter details will be logged
 * to syslog file
 *
 * @param void
 *
 * @return void
 */
void usage(void)
{
    syslog(LOG_DEBUG, "Usage: ./writer <writeDir> <writeStr>");
    syslog(LOG_DEBUG,"Total number of arguments should be 2.");
    syslog(LOG_DEBUG, "The order of the arguments should be:");
    syslog(LOG_DEBUG, "1)Full path to a file.");
    syslog(LOG_DEBUG, "2)Text string to be written to a file.");
}

/**
 * @brief Main function to write a string to the file
 *
 * Given a file path and string to write, it will validate the
 * parameters and writes the string to the file and logs any errors
 * or debug logs to syslog file.
 *
 * @param argc number of arguments
 *
 * @param argv array of command line arguments
 *
 * @return int - 1 on error, 0 on success.
 */
int main(int argc, char* argv[])
{
    FILE *filePtr = NULL;
    const char *writeDir = NULL;
    const char *writeStr = NULL;
    int return_value = 0;

    /* opens a connection to syslog for writing the logs */
    openlog(NULL, 0, LOG_USER);

    /* validate the arguments */
    if (ARG_COUNT != argc)
    {
        syslog(LOG_ERR, "Invalid number of arguments %d", argc);
        usage();
        return 1;
    }
    else
    {
        writeDir = argv[1];
        writeStr = argv[2];
    }

    /* open file in write mode */
    filePtr = fopen(writeDir, "w");
    if (NULL == filePtr)
    {
        syslog(LOG_ERR, "Error opening %s file: %s", writeDir, strerror(errno));
        return 1;
    }

    /* write the string provided to the file */
    return_value = fwrite(writeStr, 1, strlen(writeStr), filePtr);
    if (strlen(writeStr) != return_value)
    {
        syslog(LOG_ERR, "Error writing %s to %s file: %s", writeStr, writeDir,
               strerror(errno));
        return 1;
    }
    else
    {
        syslog(LOG_DEBUG, "Writing %s to %s file success", writeStr, writeDir);
    }
 
    /* close the file */
    fclose(filePtr);

    /* close the connection to the syslog utility */
    closelog();
}
