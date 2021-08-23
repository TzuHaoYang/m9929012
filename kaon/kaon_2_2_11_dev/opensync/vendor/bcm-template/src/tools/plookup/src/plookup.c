/**
 * Simple nslookup like utility designed to handle only IPv4 addresses and
 * lookup queries.
 */
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <resolv.h>
#include <errno.h>


#define DNS_PORT 53


bool lookup_init(char *server)
{
    in_addr_t server_addr;

    server_addr = inet_addr(server);
    if (server_addr == INADDR_NONE)
    {
        printf("Invalid nameserver IP address: %s\n", server);
        return false;
    }

    // Manipulate global "_res" struct to set specific name server address.
    // The same is done in busybox nslookup implementation.
    res_init();
    _res.nscount = 1;
    _res.nsaddr_list[0].sin_family          = AF_INET;  // IPv4 only
    _res.nsaddr_list[0].sin_addr.s_addr     = server_addr;
    _res.nsaddr_list[0].sin_port            = htons(DNS_PORT);

    printf("Using non-default nameserver: %s\n", server);
    return true;
}

int lookup_hostname(char *hostname)
{
    int ret;
    struct addrinfo hints;
    struct addrinfo *res, *p;

    // We will request only IPv4 addresses
    memset(&hints, 0, sizeof(hints));
    hints.ai_family     = AF_INET;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = AI_PASSIVE;

    res = NULL;
    ret = getaddrinfo(hostname, NULL, &hints, &res);
    if (ret != 0)
    {
        printf("Unable to resolve: %s [%s]\n", hostname, strerror(errno));
        goto error;
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        void *addr;
        char addr_str[1024];

        if (p->ai_family == AF_INET)
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
        }
        else
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        inet_ntop(p->ai_family, addr, addr_str, sizeof(addr_str));
        printf("Resolved address: %s\n", addr_str);
    }

error:
    if (res)
        freeaddrinfo(res);

    return ret;
}

int usage()
{
    printf("Usage: plookup HOST [NAMESERVER]\n");
    return 0;
}


int main(int argc, char *argv[])
{
    int ret = 1;

    if (argc == 2 || argc == 3)
    {
        if (argc == 3)
        {
            lookup_init(argv[2]);
        }

        ret = lookup_hostname(argv[1]);
    }
    else
    {
        usage();
    }

    return ret;
}
