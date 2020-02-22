/**
 * Example code for a simple echo server using Unix Domain Sockets.
 * 
 * This server uses SOCK_DGRAM protocol for sending and receiving datagrams.
 * 
 * Sources: Linux manual pages
 */

#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE
#include <sys/types.h>  // socket, bind (see man socket), recvfrom, sendto
#include <sys/socket.h> // socket, bind, recvfrom, sendto
#include <sys/un.h> // sockaddr_un
#include <string.h> // memset
#include <unistd.h> // unlink

#define SERVER_SOCKET_FILE "/tmp/unix-domain-socket-example"

int main() {
    int ret = EXIT_FAILURE;

    // Step 1: Create a socket
    //  We can use (man 7 unix):
    //   - SOCK_DGRAM (UDP like: datagrams, no connection),
    //   - SOCK_STREAM (TCP like: byte stream, connected) or 
    //   - SOCK_SEQPACKET (datagrams, connected).
    //  
    //  Keep in mind that, from man 7 unix:
    //    "[...] as on most UNIX implementations, UNIX domain datagram sockets 
    //     are always reliable and don't reorder datagrams."
    //  Since transmission control is not needed, the decision on which protocol
    //  to use depends on what's being sent: datagrams from anyone, a byte 
    //  stream or datagrams from a connected peer.
    int s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // Step 2: Bind the socket to a file so it can start accepting connections
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr)); // Clear the structure
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_SOCKET_FILE); // On Linux no longer than 107 characters.
    unlink(SERVER_SOCKET_FILE); // Ensure that no file exists with that name
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto cleanup;
    }
    
    // Step 3: Start sending and receiving data!
    int len;
    char buffer[8192];
    struct sockaddr_un from;
    socklen_t from_len = sizeof(from);
    while ((len = recvfrom(s, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&from, &from_len)) > 0) {
        buffer[len] = '\0';

        printf("%d bytes received from %s: \n", len, from.sun_path);
        printf("%s\n", buffer);
        printf("--------------------------\n");

        // Echo data back. Since this is a datagram socket, we can ensure that
        // the whole packet will be sent at once.
        if (sendto(s, buffer, len, 0, (struct sockaddr *)&from, from_len) < 0) {
            perror("sendto");
            goto cleanup;
        }

        // This should be the size of the address buffer before each call to
        // recvfrom (see man 2 recvfrom).
        from_len = sizeof(from);
    }

    // Check if we couldn't receive because of an error.
    if (len < 0) {
        perror("recvfrom");
        goto cleanup;
    }

    ret = EXIT_SUCCESS;

cleanup:
    close(s);
    unlink(SERVER_SOCKET_FILE);
    return ret;
}
