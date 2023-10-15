

/* _________________ INCLUDES________________________ */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* _______________ MACROS DEFINITIONS ________________________ */
#define LISTENER_ADDRESS ((const char *)NULL)
#define LISTENER_PORT ((const char *)"9000")
#define MAX_CONNECTION_REQUESTS ((int)20)
#define MAX_OPENED_FD ((int)20)
#define RETURN_OK ((int)0)
#define RETURN_NOT_OK ((int)-1)
#define RECEIVED_DATA_FILE_LOCATION ((const char *)"/var/tmp/aesdsocketdata")

/* _______________ VARS DECs/DEFS ________________________ */

int err;                              // SAVE errno VAR
struct addrinfo hints, *res, *p;      // SERVER ADDRESSES GENERATOR
struct sockaddr_storage clientaddr;   // RETURN OF ACCEPTED CLIENT ADDRESS
int allfd_count;                      // STORE THE COUNT OF ANY OPENED FD
int allfd[MAX_OPENED_FD];             // STORE ANY OPENED FD
char buffer_client_addresss[50];      // RETURN HUMAN RIDEABLE ADDRESS.
int lisnerfd = -1, fdwriterfile = -1; // THE FD OF LISTENER & FD OF FILE TRACKING FOR ALL RECEIVED DATA.
char *sendbuffer = NULL;              // READ BACK THE FILE OF RECEIVED DATA IN THIS FILE.
ssize_t sendbuffer_size;              // @sendbuffer ELEMENT SIZE FOR REALLOCATING
char *sendbufptr;                     // KEEPING TRACKING OF LAST READ ELEMENT IN @sendbuffer

/*______________ FUNCTION DECs ___________________ */

/**
 * @brief loop in all opened fd and close it.
 *
 */
static void closs_all_fd(void);

/**
 * @brief Identify the binary IP address if it v4 Or v6
 *
 * @param a Structure Pointer describing a generic socket address
 * @return void* Return a POINTER TO sockaddr_in6 OR sockaddr_in
 */
static void *getaddinetntop(struct sockaddr *a);

/**
 * @brief Send the receive to specific fd client
 *
 * @param fds fd client
 * @param buf the buffer containing the received data send back to client.
 * @param len the lens of buffer and the remaining lens of
 * @return @param RETURN_NOT_OK if it fails
 *         @param RETURN_OK if it OK.
 */
static int sendall(int fds, char *buf, int *len);
/**
 * @brief signals initializing set the signals handlers
 *
 * @return @param RETURN_NOT_OK if it fails
 *         @param RETURN_OK if it OK.
 *
 */
int signal_initializing(void);

/**
 * @brief initializing the server .. bind and get ready to listen to requests.
 *
 * @return @param RETURN_NOT_OK if it fails
 *         @param RETURN_OK if it OK.
 */
static int listener_socket_initializing(void);

/**
 * @brief accept request, receive the data, write to a file @param RECEIVED_DATA_FILE_LOCATION
 *        the read the file and send it back to the client.
 *
 * @return int
 */
static int super_loop_accept_receive_write_sendback(void);

/**
 * @brief Gracefully exits when SIGINT or
 *        SIGTERM is received , Closing all FDs ,
 *        Deallocating Memory, Terminating
 *        andy waiting zombies.
 *
 * @param signo SIGNAL
 *
 *  */

void sigint_handler(int signoo)
{
    if ((signoo == SIGTERM) || (signoo == SIGINT))
    {
        write(STDOUT_FILENO, (void *)"Caught signal, exiting!\n", 24);
        closs_all_fd();
        syslog(LOG_MAKEPRI(LOG_USER, LOG_INFO), "Caught signal, exiting!");
        closelog();
        free(sendbuffer);
        system("rm -rf /var/tmp/aesdsocketdata");
        fflush(stdout);
        exit(RETURN_OK);
    }
    if (signoo == SIGCHLD)
    {
        while (waitpid(-1, NULL, WNOHANG) > 0); // Caught SIGCHLD, KILL ZOMBIES!
    }
}

int main(int argc, char *argv[])
{
    if (signal_initializing() == RETURN_NOT_OK)
    {
        fprintf(stderr, "signel_initializing FUNCTION ERROR\n");
        return RETURN_NOT_OK;
    }

    if ((argc > 2) || (argc == 2 && (strcmp(argv[1], "-d") != 0)))
    {
        fprintf(stderr, "ERROR ARGUMENTS ARE NOT SUPPORTED!\n");
        return RETURN_NOT_OK;
    }

    if (listener_socket_initializing() == RETURN_NOT_OK)
    {
        fprintf(stderr, "listener_socket_initializing FUNCTION ERROR\n");
        return RETURN_NOT_OK;
    }

    if ((fdwriterfile = open(RECEIVED_DATA_FILE_LOCATION, O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0664)) == -1)
    {
        err = errno;
        fprintf(stderr, "open ERROR FUNCTION: %s\n", strerror(err));
        return RETURN_NOT_OK;
    }
    allfd[++allfd_count] = fdwriterfile;

    /* Put the program in the background, and dissociate from the controlling
   terminal.  If NOCHDIR is zero, do `chdir ("/")'.  If NOCLOSE is zero,
   redirects stdin, stdout, and stderr to /dev/null.  */

    if (argc > 1 && !(strcmp(argv[1], "-d")))
    {
        if (RETURN_NOT_OK == daemon(0, 1))
        {
            err = errno;
            fprintf(stderr, "daemon ERROR: %s\n", strerror(err));
            return RETURN_NOT_OK;
        }
    }

    if (super_loop_accept_receive_write_sendback() == RETURN_NOT_OK)
    {
        fprintf(stderr, "lsuper_loop_accept_recieve_write_sendback FUNCTION ERROR\n");
        return RETURN_NOT_OK;
    }

    return RETURN_OK;
}

static void *getaddinetntop(struct sockaddr *a)
{
    if (a->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)a)->sin_addr);
    }
    else
        return &(((struct sockaddr_in6 *)a)->sin6_addr);
}

static int sendall(int fds, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while (total < *len)
    {
        n = send(fds, buf + total, bytesleft, 0);
        if (n == RETURN_NOT_OK)
        {
            err = errno;
            if (err == EINTR)
                continue;
            fprintf(stderr, "sendall ERROR FUNCTION: %s\n", strerror(err));
            return RETURN_NOT_OK;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total; // return number actually sent here
    if (n == RETURN_NOT_OK)
    {
        return RETURN_NOT_OK;
    }
    return RETURN_OK;
}

static void closs_all_fd(void)
{
    for (; allfd_count != 0; allfd_count--)
    {
        close(allfd[allfd_count]);
    }
}
int signal_initializing(void)
{
    struct sigaction sact;
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;

    sact.sa_handler = &sigint_handler;

    if (sigaction(SIGINT, &sact, NULL) == RETURN_NOT_OK)
    {
        err = errno;
        fprintf(stderr, "signal ERROR FUNCTION: %s\n", strerror(err));
        return RETURN_NOT_OK;
    }

    if (sigaction(SIGTERM, &sact, NULL) == RETURN_NOT_OK)
    {
        err = errno;
        fprintf(stderr, "signal ERROR FUNCTION: %s\n", strerror(err));
        return RETURN_NOT_OK;
    }
    if (sigaction(SIGCHLD, &sact, NULL) == RETURN_NOT_OK)
    {
        err = errno;
        fprintf(stderr, "signal ERROR FUNCTION: %s\n", strerror(err));
        return RETURN_NOT_OK;
    }
    return RETURN_OK;
}

static int listener_socket_initializing(void)
{
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(LISTENER_ADDRESS, LISTENER_PORT, &hints, &res))
    {
        err = errno;
        fprintf(stderr, "getaddrinfo ERROR FUNCTION: %s\n", strerror(err));
        return RETURN_NOT_OK;
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        if ((lisnerfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {

            continue;
        }
        break;
    }

    if (lisnerfd == -1)
    {
        fprintf(stderr, "socket ERROR FUNCTION: %s\n", strerror(err));
        return RETURN_NOT_OK;
    }
    allfd[++allfd_count] = lisnerfd;

    int yes = 1;

    if (setsockopt(lisnerfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        err = errno;
        fprintf(stderr, "setsockopt ERROR FUNCTION: %s\n", strerror(err));
        close(lisnerfd);
        return RETURN_NOT_OK;
    }

    if (bind(lisnerfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        err = errno;
        fprintf(stderr, "bind ERROR FUNCTION: %s\n", strerror(err));
        close(lisnerfd);
        return RETURN_NOT_OK;
    }

    if (listen(lisnerfd, MAX_CONNECTION_REQUESTS) == RETURN_NOT_OK)
    {
        err = errno;
        fprintf(stderr, "listen ERROR FUNCTION: %s\n", strerror(err));
        close(lisnerfd);
        return RETURN_NOT_OK;
    }

    freeaddrinfo(res);

    return RETURN_OK;
}

static int super_loop_accept_receive_write_sendback(void)
{
    int fdclient = RETURN_NOT_OK;
    char rcvbuf[100000];

    socklen_t clientaddr_len = sizeof(clientaddr);
    ssize_t len_buffer = 0;
    ssize_t ret = 0;
    int numbytes = 0;

    while (1)
    {
        if ((fdclient = accept(lisnerfd, (struct sockaddr *)&clientaddr, &clientaddr_len)) == RETURN_NOT_OK)
        {
            err = errno;
            fprintf(stderr, "accept ERROR FUNCTION: %s\n", strerror(err));
            return RETURN_NOT_OK;
        }
        allfd[++allfd_count] = fdclient;

        inet_ntop(clientaddr.ss_family, getaddinetntop((struct sockaddr *)&clientaddr), buffer_client_addresss, 50);
        syslog(LOG_MAKEPRI(LOG_USER, LOG_INFO), "Accepted connection from %s\n", buffer_client_addresss);
        memset(buffer_client_addresss, 0, 50);

        if ((numbytes = recv(fdclient, rcvbuf, 100000, 0)) == -1)
        {
            err = errno;
            fprintf(stderr, "recv ERROR FUNCTION: %s\n", strerror(err));
            return RETURN_NOT_OK;
            ;
        }
        rcvbuf[numbytes] = '\n';

        char *rcvbufptr = rcvbuf;
        len_buffer = numbytes;
        while (len_buffer != 0 && (ret = write(fdwriterfile, rcvbufptr, len_buffer)) != 0)
        {
            if (ret == -1)
            {
                if (errno == EINTR)
                    continue;
                err = errno;
                fprintf(stderr, "write ERROR FUNCTION: %s\n", strerror(err));
                return RETURN_NOT_OK;
                break;
            }
            len_buffer -= ret;
            rcvbufptr += ret;
        }

        sendbuffer_size += numbytes;
        sendbuffer = (char *)realloc((void *)sendbuffer, sendbuffer_size);
        if (sendbuffer == NULL)
        {
            err = errno;
            fprintf(stderr, "realloc ERROR FUNCTION: %s\n", strerror(err));
            return RETURN_NOT_OK;
        }

        if (lseek(fdwriterfile, (off_t)0, SEEK_SET) == -1)
        {
            err = errno;
            fprintf(stderr, "lseek Read ERROR FUNCTION: %s\n", strerror(err));
            return RETURN_NOT_OK;
        }

        len_buffer = sendbuffer_size;
        sendbufptr = sendbuffer;
        while (len_buffer != 0 && (ret = read(fdwriterfile, sendbufptr, len_buffer)) != 0)
        {
            if (ret == RETURN_NOT_OK)
            {
                err = errno;
                if (errno == EINTR)
                    continue;
                fprintf(stderr, "Read sendbuffer ERROR FUNCTION: %s\n", strerror(err));
                return -1;
            }
            len_buffer -= ret;
            sendbufptr += ret;
        }

        if (sendall(fdclient, sendbuffer, (int *)&sendbuffer_size) == -1)
        {
            return RETURN_NOT_OK;
        }

        if (lseek(fdwriterfile, (off_t)0, SEEK_END) == -1)
        {
            err = errno;
            fprintf(stderr, "lseek Read ERROR FUNCTION: %s\n", strerror(err));
            return RETURN_NOT_OK;
        }
    }
    return RETURN_NOT_OK;
}
