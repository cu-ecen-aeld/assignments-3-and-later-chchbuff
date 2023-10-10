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
 * @file aesdsocket.c
 * @brief This file contains functionality of socket application.
 *
 * To compile: make
 *
 * @author Chandana Challa
 * @date Oct 7 2023
 * @version 1.0
 * @resources AESD Course Slides
 */

/* Header files */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Macro definitions */
#define ARG_COUNT    (2)
#define SUCCESS      (0)
#define FAILURE      (-1)
#define ERROR        (-1)
#define PORT         "9000"
#define MAX_CONNECTIONS_ALLOWED   (10)
#define FILENAME      "/var/tmp/aesdsocketdata"
#define MAX_BUFF_LEN   (1024)

/* Global definitions */
static volatile sig_atomic_t exit_condition = 0;
static int socket_fd = 0;
static int file_fd = 0;
static int connection_fd = 0;

/* Function Prototypes */
static int start_daemon(void);
static void close_app(void);
void signal_handler(int signo);

/* Function definitions */
/**
 * @brief Starts Daemon by creating a child process
 *
 * @param void
 *
 * @return int
 */
static int start_daemon(void)
{
    fflush(stdout);
    pid_t pid = fork();
    if (FAILURE == pid)
    {
        syslog(LOG_PERROR, "creating a child process:%s\n", strerror(errno));
        return FAILURE;
    }
    else if (0 == pid)
    {
        printf("Child process created successfully\n");
        umask(0);
        pid_t id = setsid();
        if (FAILURE == id)
        {
            syslog(LOG_PERROR, "setsid:%s\n", strerror(errno));
            return FAILURE; 
        }
        /* change current directory to root */
        if (FAILURE == chdir("/"))
        {
            syslog(LOG_PERROR, "chdir:%s\n", strerror(errno));
            return FAILURE;       
        }
        /* close standard files of process */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        /* redirect standard files to /dev/null */
        int fd = open("/dev/null", O_RDWR);
        if (FAILURE == fd)
        {
            syslog(LOG_PERROR, "open:%s\n", strerror(errno));
            return FAILURE;       
        }
        if (FAILURE == dup2(fd, STDIN_FILENO))
        {
            syslog(LOG_PERROR, "dup2:%s\n", strerror(errno));
            return FAILURE;    
        }
        if (FAILURE == dup2(fd, STDOUT_FILENO))
        {
            syslog(LOG_PERROR, "dup2:%s\n", strerror(errno));
            return FAILURE;    
        }
        if (FAILURE == dup2(fd, STDERR_FILENO))
        {
            syslog(LOG_PERROR, "dup2:%s\n", strerror(errno));
            return FAILURE;    
        }
        close(fd);
    }
    else
    {
        syslog(LOG_PERROR, "Terminating Parent process");
        exit(0);
    }
    return SUCCESS;
}

/**
 * @brief Performs closing steps of the application
 *
 * @param void
 *
 * @return void
 */
static void close_app(void)
{
    close(file_fd);
    close(connection_fd);
    /* deletes the file */
    if (FAILURE == unlink(FILENAME))
    {
       syslog(LOG_PERROR, "unlink %s: %s", FILENAME, strerror(errno));
    }
    close(socket_fd);
    if (FAILURE == shutdown(socket_fd, 2))
    {
        syslog(LOG_PERROR, "shutdown: %s", strerror(errno));
    }

    /* close the connection to the syslog utility */
    closelog();
}

/**
 * @brief Handles signals SIGINT and SIGTERM
 *
 * @param int signo - number of signal received
 *
 * @return void
 */
void signal_handler(int signo)
{
    if ((SIGINT == signo) || (SIGTERM == signo))
    {
        syslog(LOG_DEBUG, "Caught signal:%d, exiting", signo);
        exit_condition = 1;
        close_app();
    }
}

/**
 * @brief Main function to write a string to the file
 *
 * Creates socket and wait for client connections. Receives data from client and
 * write to a file when new line is found and sends back data to client.
 *
 * @param argc number of arguments
 *
 * @param argv array of command line arguments
 *
 * @return int - -1 on error, 0 on success.
 */
int main(int argc, char* argv[])
{
    bool start_as_daemon = false;
    struct addrinfo hints;
    struct addrinfo *serverInfo = NULL;
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(struct sockaddr);
    char ClientIpAddr[INET_ADDRSTRLEN];
    int recv_bytes = 0;
    char buffer[MAX_BUFF_LEN] = {'\0'};
    bool packet_complete = false;
    int written_bytes = 0;
    const int enable_reuse = 1;
    /* opens a connection to syslog for writing the logs */
    openlog(NULL, 0, LOG_USER);

    /* check the arguments */
    if ((ARG_COUNT == argc) && (strcmp(argv[1], "-d") == 0))
    {
        syslog(LOG_INFO, "Starting aesdsocket as a daemon");
        start_as_daemon = true;
    }
    
    /* register signal handling for SIGINT and SIGTERM */
    if (SIG_ERR == signal(SIGINT, signal_handler))
    {
        syslog(LOG_PERROR, "signal SIGINT: %s", strerror(errno));
    }
    if (SIG_ERR == signal(SIGTERM, signal_handler))
    {
        syslog(LOG_PERROR, "signal SIGTERM: %s", strerror(errno));
    }
    
    /* create socket */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (FAILURE == socket_fd)
    {
        syslog(LOG_PERROR, "Creating socket: %s", strerror(errno));
        return FAILURE;
    }
    /* getaddress info */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (SUCCESS != getaddrinfo(NULL, PORT, &hints, &serverInfo))
    {
        syslog(LOG_PERROR, "getaddrinfo: %s", strerror(errno));
        if (NULL != serverInfo)
        {
            freeaddrinfo(serverInfo);
        }
        close(socket_fd);
        return FAILURE;
    }
    
    if (SUCCESS != setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable_reuse,sizeof(enable_reuse)))
    {
        syslog(LOG_PERROR, "setsockopt: %s", strerror(errno));
        if (NULL != serverInfo)
        {
            freeaddrinfo(serverInfo);
        }
        close(socket_fd);
        return FAILURE;
    }
    /* bind the socket to port */
    if (SUCCESS != bind(socket_fd, serverInfo->ai_addr,
                        serverInfo->ai_addrlen))
    {
        syslog(LOG_PERROR, "bind: %s", strerror(errno));
        if (NULL != serverInfo)
        {
            freeaddrinfo(serverInfo);
        }
        close(socket_fd);
        return FAILURE;
    }
    
    /* free serverinfo after bind */
    if (NULL != serverInfo)
    {
        freeaddrinfo(serverInfo);
    }
    
    /* start daemon if flag is enabled */
    if (start_as_daemon)
    {
        if (FAILURE == start_daemon())
        {
            close(socket_fd);
            return FAILURE;
        }
    }

    /* listen for connection on the socket */
    if (SUCCESS != listen(socket_fd, MAX_CONNECTIONS_ALLOWED))
    {
        syslog(LOG_PERROR, "listen: %s", strerror(errno));
        close(socket_fd);
        return FAILURE;
    }    
    

    /* exit accepting connections once signal is received */
    while (!exit_condition)
    {
        /* accept the connection on the socket */
        connection_fd = accept(socket_fd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (FAILURE == connection_fd)
        {
            syslog(LOG_PERROR, "accept: %s", strerror(errno));
            close(socket_fd);
            return FAILURE;
        }  
    
        /* converts binary ip address to string format */
        if (NULL == inet_ntop(AF_INET, &(clientAddr.sin_addr), ClientIpAddr, INET_ADDRSTRLEN))
        {
            syslog(LOG_PERROR, "inet_ntop: %s", strerror(errno));
        }
        syslog(LOG_INFO, "Accepted connection from %s", ClientIpAddr);
    
        /* open file in readwrite mode */
        file_fd = open(FILENAME, O_CREAT|O_RDWR|O_APPEND, 
                       S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
        if (FAILURE == file_fd)
        {
            syslog(LOG_ERR, "Error opening %s file: %s", FILENAME, strerror(errno));
            close(connection_fd);
            close(socket_fd);
            return FAILURE;
        }

        /* loop to receive data until new line is found */
        do
        {
            memset(buffer, 0, MAX_BUFF_LEN);
            /* recv data from client */
            recv_bytes = recv(connection_fd, buffer, MAX_BUFF_LEN, 0);
            if (FAILURE == recv_bytes)
            {
                syslog(LOG_PERROR, "recv: %s", strerror(errno));
                close(file_fd);
                close(connection_fd);
                close(socket_fd);
                return FAILURE;
            }
            /* write the string received to the file */
            written_bytes = write(file_fd, buffer, recv_bytes);
            if (written_bytes != recv_bytes)
            {
                syslog(LOG_ERR, "Error writing %s to %s file: %s", buffer, FILENAME,
                       strerror(errno));
                close(file_fd);
                close(connection_fd);
                close(socket_fd);
                return FAILURE;
            }
            /* check for new line */
            if (NULL != (memchr(buffer, '\n', recv_bytes)))
            {
                packet_complete = true;
            }
        } while (!packet_complete);

        packet_complete = false;

        /* seek the file fd to start of file to read contents */
        off_t offset = lseek(file_fd, 0, SEEK_SET);
        if (FAILURE == offset)
        {
            syslog(LOG_PERROR, "lseek: %s", strerror(errno));
            close(file_fd);
            close(connection_fd);
            close(socket_fd);
            return FAILURE;
        }
        /* read file contents till EOF */
        int read_bytes = 0;
        int send_bytes = 0;
        do
        {
            memset(buffer, 0, MAX_BUFF_LEN);
            read_bytes = read(file_fd, buffer, MAX_BUFF_LEN);
            if (read_bytes > 0)
            {
                /* send file data to client */
                send_bytes = send(connection_fd, buffer, read_bytes, 0);
                if (send_bytes != read_bytes)
                {
                    syslog(LOG_PERROR, "send: %s", strerror(errno));
                    close(file_fd);
                    close(connection_fd);
                    close(socket_fd);
                    return FAILURE;
                }
            }
        } while (read_bytes > 0);

        close(file_fd);
        if (SUCCESS == close(connection_fd))
        {
            syslog(LOG_INFO, "Closed connection from %s", ClientIpAddr);
        }
    }
    close_app();
}

