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
              https://github.com/stockrt/queue.h/tree/master
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
#include <pthread.h>
#include <time.h>
#include "queue.h"

/* Macro definitions */
#define ARG_COUNT    (2)
#define SUCCESS      (0)
#define FAILURE      (-1)
#define ERROR        (-1)
#define PORT         "9000"
#define MAX_CONNECTIONS_ALLOWED   (10)
#define FILENAME      "/var/tmp/aesdsocketdata"
#define MAX_BUFF_LEN   (1024)
#define TIMER_DELAY_PERIOD   (10)

/* Global definitions */
static volatile sig_atomic_t exit_condition = 0;
static int socket_fd = 0;
static char ClientIpAddr[INET_ADDRSTRLEN];

typedef struct socket_node {
    pthread_t thread_id;
    int connection_fd;
    bool thread_complete_success;
    pthread_mutex_t *thread_mutex;
    SLIST_ENTRY(socket_node) node_count;
}socket_node_t;

/* Function Prototypes */
static int start_daemon(void);
static void close_app(void);
void signal_handler(int signo);
void *recv_and_send_thread(void *thread_node);

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
    /* deletes the file */
    if (FAILURE == unlink(FILENAME))
    {
       syslog(LOG_PERROR, "unlink %s: %s", FILENAME, strerror(errno));
    }
    if (FAILURE == shutdown(socket_fd, SHUT_RDWR))
    {
        syslog(LOG_PERROR, "shutdown: %s", strerror(errno));
    }
    close(socket_fd);
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
    }
}

/**
 * @brief Handles timer thread functionality by writing timestamp for every 10 secs.
 *
 * @param thread_node contains thread data.
 *
 * @return void *
 */
void *start_timer_thread(void *thread_node)
{
    socket_node_t *node = NULL;
    int status = FAILURE;
    int file_fd = -1;
    struct timespec time_period;
    char output[MAX_BUFF_LEN] = {'\0'};
    time_t curr_time;
    struct tm *temp;
    int written_bytes = 0;
    if (NULL == thread_node)
    {
        return NULL;
    }
    node = (socket_node_t *)thread_node;
    
    while (!exit_condition)
    {
        if (SUCCESS != clock_gettime(CLOCK_MONOTONIC, &time_period))
        {
            syslog(LOG_ERR, "clock_gettime: %s", strerror(errno));
            status = FAILURE;
            goto exit;
        
        }
        time_period.tv_sec += TIMER_DELAY_PERIOD;
        
        if (SUCCESS != clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_period, NULL))
        {
            syslog(LOG_ERR, "clock_nanosleep: %s", strerror(errno));
            status = FAILURE;
            goto exit;       
        }
        curr_time = time(NULL);
        if (FAILURE == curr_time)
        {
            syslog(LOG_ERR, "time: %s", strerror(errno));
            status = FAILURE;
            goto exit;      
        }       
        temp = localtime(&curr_time);
        if (NULL == temp)
        {
            syslog(LOG_ERR, "localtime: %s", strerror(errno));
            status = FAILURE;
            goto exit;      
        }
        
        if (0 == strftime(output, sizeof(output), "timestamp: %Y %B %d, %H:%M:%S\n", temp))
        {
            syslog(LOG_ERR, "strftime: %s", strerror(errno));
            status = FAILURE;
            goto exit;        
        }
        /* open file in readwrite mode */
        file_fd = open(FILENAME, O_CREAT|O_RDWR|O_APPEND, 
                       S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
        if (FAILURE == file_fd)
        {
            syslog(LOG_ERR, "Error opening %s file: %s", FILENAME, strerror(errno));
            status = FAILURE;
            goto exit;
        }       
        if (SUCCESS != pthread_mutex_lock(node->thread_mutex))
        {
            syslog(LOG_PERROR, "pthread_mutex_lock: %s", strerror(errno));
            status = FAILURE;
            goto exit;
        }
        /* write the timestamp to the file */
        written_bytes = write(file_fd, output, strlen(output));
        if (written_bytes != strlen(output))
        {
            syslog(LOG_ERR, "Error writing %s to %s file: %s", output, FILENAME,
                       strerror(errno));
            status = FAILURE;
            pthread_mutex_unlock(node->thread_mutex);
            goto exit;
        }
        if (SUCCESS != pthread_mutex_unlock(node->thread_mutex))
        {
            syslog(LOG_PERROR, "pthread_mutex_unlock: %s", strerror(errno));
            status = FAILURE;
            goto exit;
        }
        status = SUCCESS;
        close(file_fd);
    }
exit:
     (status == FAILURE) ? (node->thread_complete_success = false) : 
                           (node->thread_complete_success = true);
     return thread_node;
}

/**
 * @brief Handles socket recv and send data.
 *
 * @param thread_node contains thread data.
 *
 * @return void *
 */
void *recv_and_send_thread(void *thread_node)
{
    int recv_bytes = 0;
    char buffer[MAX_BUFF_LEN] = {'\0'};
    bool packet_complete = false;
    int written_bytes = 0;
    socket_node_t *node = NULL;
    int status = FAILURE;
    int file_fd = -1;
    if (NULL == thread_node)
    {
        return NULL;
    }
    else
    {
        node = (socket_node_t *)thread_node;
        /* open file in readwrite mode */
        file_fd = open(FILENAME, O_CREAT|O_RDWR|O_APPEND, 
                       S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
        if (FAILURE == file_fd)
        {
            syslog(LOG_ERR, "Error opening %s file: %s", FILENAME, strerror(errno));
            status = FAILURE;
            goto exit;
        }

        /* loop to receive data until new line is found */
        do
        {
            memset(buffer, 0, MAX_BUFF_LEN);
            /* recv data from client */
            recv_bytes = recv(node->connection_fd, buffer, MAX_BUFF_LEN, 0);
            if (FAILURE == recv_bytes)
            {
                syslog(LOG_PERROR, "recv: %s", strerror(errno));
                status = FAILURE;
                goto exit;
            }
            
            if (SUCCESS != pthread_mutex_lock(node->thread_mutex))
            {
                syslog(LOG_PERROR, "pthread_mutex_lock: %s", strerror(errno));
                status = FAILURE;
                goto exit;
            }
            /* write the string received to the file */
            written_bytes = write(file_fd, buffer, recv_bytes);
            if (written_bytes != recv_bytes)
            {
                syslog(LOG_ERR, "Error writing %s to %s file: %s", buffer, FILENAME,
                       strerror(errno));
                status = FAILURE;
                pthread_mutex_unlock(node->thread_mutex);
                goto exit;
            }
            if (SUCCESS != pthread_mutex_unlock(node->thread_mutex))
            {
                syslog(LOG_PERROR, "pthread_mutex_unlock: %s", strerror(errno));
                status = FAILURE;
                goto exit;
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
            status = FAILURE;
            goto exit;
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
                send_bytes = send(node->connection_fd, buffer, read_bytes, 0);
                if (send_bytes != read_bytes)
                {
                    syslog(LOG_PERROR, "send: %s", strerror(errno));
                    status = FAILURE;
                    goto exit;
                }
                status = SUCCESS;
            }
        } while (read_bytes > 0);
    }
exit:
    if (file_fd != -1)
    {
        close(file_fd);
    }
    if (SUCCESS == close(node->connection_fd))
    {
        syslog(LOG_INFO, "Closed connection from %s", ClientIpAddr);
    }
    (status == FAILURE) ? (node->thread_complete_success = false) : 
                           (node->thread_complete_success = true);
    return thread_node;
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
    int status = SUCCESS;
    bool start_as_daemon = false;
    struct addrinfo hints;
    struct addrinfo *serverInfo = NULL;
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(struct sockaddr);
    
    const int enable_reuse = 1;
    socket_node_t *data_ptr = NULL;
    socket_node_t *data_ptr_temp = NULL;
    pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
    /* opens a connection to syslog for writing the logs */
    openlog(NULL, 0, LOG_USER);

    /* check the arguments */
    if ((ARG_COUNT == argc) && (strcmp(argv[1], "-d") == 0))
    {
        syslog(LOG_INFO, "Starting aesdsocket as a daemon");
        start_as_daemon = true;
    }
    
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = signal_handler;
    
    /* register signal handling for SIGINT and SIGTERM */
    if (SUCCESS != sigaction(SIGINT, &sa, NULL))
    {
        syslog(LOG_PERROR, "sigaction SIGINT: %s", strerror(errno));
        return FAILURE;
    }
    if (SUCCESS != sigaction(SIGTERM, &sa, NULL))
    {
        syslog(LOG_PERROR, "sigaction SIGTERM: %s", strerror(errno));
        return FAILURE;
    }
    
    SLIST_HEAD(socket_head, socket_node) head;
    SLIST_INIT(&head);
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
        status = FAILURE;
        goto exit;
    }
    
    if (SUCCESS != setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable_reuse,sizeof(enable_reuse)))
    {
        syslog(LOG_PERROR, "setsockopt: %s", strerror(errno));
        if (NULL != serverInfo)
        {
            freeaddrinfo(serverInfo);
        }
        status = FAILURE;
        goto exit;
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
        status = FAILURE;
        goto exit;
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
            status = FAILURE;
            goto exit;
        }
    }

    /* listen for connection on the socket */
    if (SUCCESS != listen(socket_fd, MAX_CONNECTIONS_ALLOWED))
    {
        syslog(LOG_PERROR, "listen: %s", strerror(errno));
        status = FAILURE;
        goto exit;
    }

    /* create node for timer thread */
    data_ptr = (socket_node_t *)malloc(sizeof(socket_node_t));
    if (NULL == data_ptr)
    {
        syslog(LOG_PERROR, "malloc: %s", strerror(errno));
        status = FAILURE;
        goto exit;

    }

    data_ptr->thread_complete_success = false;
    data_ptr->thread_mutex = &thread_mutex;
    /* create thread for timer */
    if (SUCCESS != pthread_create(&data_ptr->thread_id, NULL, start_timer_thread, data_ptr))
    {
        syslog(LOG_PERROR, "pthread_create: %s", strerror(errno));
        free(data_ptr);
        data_ptr = NULL;
        status = FAILURE;
        goto exit;
    } 
    SLIST_INSERT_HEAD(&head, data_ptr, node_count);

    /* exit accepting connections once signal is received */
    while (!exit_condition)
    {
        /* accept the connection on the socket */
        int connection_fd = accept(socket_fd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (FAILURE == connection_fd)
        {
            syslog(LOG_PERROR, "accept: %s", strerror(errno));
        }
        else
        {
            /* converts binary ip address to string format */
            if (NULL == inet_ntop(AF_INET, &(clientAddr.sin_addr), ClientIpAddr, INET_ADDRSTRLEN))
            {
                syslog(LOG_PERROR, "inet_ntop: %s", strerror(errno));
            }
            syslog(LOG_INFO, "Accepted connection from %s", ClientIpAddr);

            /* create socket node for each connection */
            data_ptr = (socket_node_t *)malloc(sizeof(socket_node_t));
            if (NULL == data_ptr)
            {
                syslog(LOG_PERROR, "malloc: %s", strerror(errno));
                status = FAILURE;
                goto exit;
            }

            data_ptr->connection_fd = connection_fd;
            data_ptr->thread_complete_success = false;
            data_ptr->thread_mutex = &thread_mutex;
            /* create thread for each connection */
            if (SUCCESS != pthread_create(&data_ptr->thread_id, NULL, recv_and_send_thread, data_ptr))
            {
                syslog(LOG_PERROR, "pthread_create: %s", strerror(errno));
                free(data_ptr);
                data_ptr = NULL;
                status = FAILURE;
                goto exit;
            } 
            SLIST_INSERT_HEAD(&head, data_ptr, node_count);
        }
        /* check whether thread exited if yes, join thread and remove from socket list */
        data_ptr = NULL;
        SLIST_FOREACH_SAFE(data_ptr, &head, node_count, data_ptr_temp)
        {
            if (data_ptr->thread_complete_success == true)
            {
                syslog(LOG_INFO, "1 Joined thread id: %ld", data_ptr->thread_id);
                pthread_join(data_ptr->thread_id, NULL);
                SLIST_REMOVE(&head, data_ptr, socket_node, node_count);
                free(data_ptr);
                data_ptr = NULL;
            }
        }
    }

exit:
    close_app();
    /* destroy mutex */
    pthread_mutex_destroy(&thread_mutex);
    /* delete timer node from socket list */
    while (!SLIST_EMPTY(&head))
    {
        data_ptr = SLIST_FIRST(&head);
        SLIST_REMOVE_HEAD(&head, node_count);
        /* join timer thread */
        pthread_join(data_ptr->thread_id, NULL);
        free(data_ptr);
        data_ptr = NULL;
    }
    return status;
}

