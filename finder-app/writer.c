#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

#define ARG_COUNT    (3)

void usage(void)
{
    syslog(LOG_DEBUG, "Usage: ./writer <writeDir> <writeStr>");
    syslog(LOG_DEBUG,"Total number of arguments should be 2.");
    syslog(LOG_DEBUG, "The order of the arguments should be:");
    syslog(LOG_DEBUG, "1)Full path to a file.");
    syslog(LOG_DEBUG, "2)Text string to be written to a file.");
}

int main(int argc, char* argv[])
{
    FILE *filePtr = NULL;
    const char *writeDir = NULL;
    const char *writeStr = NULL;
    int return_value = 0;

    openlog(NULL, 0, LOG_USER);

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

    filePtr = fopen(writeDir, "w");
    if (NULL == filePtr)
    {
        syslog(LOG_ERR, "Error opening %s file: %s", writeDir, strerror(errno));
        return 1;
    }

    return_value = fwrite(writeStr, 1, strlen(writeStr), filePtr);
    if (strlen(writeStr) != return_value)
    {
        syslog(LOG_ERR, "Error writing %s to %s file: %s", writeStr, writeDir, strerror(errno));
        return 1;
    }
    else
    {
        syslog(LOG_DEBUG, "Writing %s to %s file success", writeStr, writeDir);
    }
 
    fclose(filePtr);
    closelog();
}
