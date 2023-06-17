#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


const char *HOSTNAME = "telehack.com";
const char *SERVICE  = "telnet";


char *create_query_str(const char *font, const char *text)
{
    const char *template = "figlet /%s %s\r\n";
    size_t len = snprintf(NULL, 0, template, font, text);
    char *query = (char*)malloc(len);
    if (!query) {
        fprintf(stderr, "malloc error\n");
        exit(EXIT_FAILURE);
    }
    snprintf(query, len, template, font, text);
    return query;
}

int create_connect() 
{
    struct addrinfo hints = {0};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = 0;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *res;
    if (getaddrinfo(HOSTNAME, SERVICE, &hints, &res) != 0) {
        fprintf(stderr, "getaddrinfo error\n");
        exit(EXIT_FAILURE);
    }

    int fd = -1;
    for (struct addrinfo *it_addr = res; it_addr != NULL; it_addr = it_addr->ai_next) {
        fd = socket(it_addr->ai_family, it_addr->ai_socktype, it_addr->ai_protocol);
        if (fd == -1) {
            continue;
        }
        if (connect(fd, it_addr->ai_addr, it_addr->ai_addrlen) != -1) {
            break;
        }
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return fd;
}

void close_connect(int fd)
{
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void print_hex(const char *buff)
{
    for (size_t i=0; i<strlen(buff); ++i) {
        printf("0x%02x ", buff[i]);
    }
    printf("\n");
}

int receive_data(int fd, char **msg) 
{
    char *recv_data = NULL;
    char buff[4096] = { 0 };
    
    size_t len = 0;
    ssize_t r = 0;
    while ((r = recv(fd, &buff, sizeof(buff), 0)) > 0) {
        {
            size_t size = len + r;
            recv_data = recv_data ? realloc(recv_data, size + 1)
                                  : malloc(size + 1);
            if (!recv_data) { 
                free(recv_data);
                fprintf(stderr, "realloc error\n");
                exit(EXIT_FAILURE);
            }
            recv_data[size] = 0;
        }
        memcpy(recv_data + len, buff, r);
        len += r;

        //print_hex(&recv_data[len-2]);
        if (strncmp(&recv_data[len-2], "\n.", 2) == 0) {
            break;
        }
    }
    if (msg) {
        *msg = recv_data;
    }
    else {
        free(recv_data);
    }
    return len;
}


int main (int argc, char **argv)
{
    char *font = NULL;
    char *text = NULL;
    {
        if (argc != 3) {
            printf("USAGE: %s <font> <text>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        font = argv[1];
        text = argv[2];
    }

    int fd = create_connect();
    if (fd < 0) {
        fprintf(stderr, "connect error\n");
        exit(EXIT_FAILURE);
    }

    {
        if (receive_data(fd, NULL) < 1) {
            close_connect(fd);
            fprintf(stderr, "no invitation\n");
            exit(EXIT_FAILURE);
        }
    }

    {
        char *query = create_query_str(font, text);
        if (send(fd, query, strlen(query), 0) < 0) {
            free(query);
            close_connect(fd);
            fprintf(stderr, "send error\n");
            exit(EXIT_FAILURE);
        }
        free(query);
    }

    {
        char *message = NULL;
        if (receive_data(fd, &message) < 1) {
            close_connect(fd);
            exit(EXIT_FAILURE);
        }
        puts(message);
        free(message);
    }

    close_connect(fd);
    return 0;
}
