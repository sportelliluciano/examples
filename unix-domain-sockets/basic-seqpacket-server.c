/**
 * Example code for a simple echo server using Unix Domain Sockets.
 * 
 * This server uses SOCK_SEQPACKET protocol for sending and receiving datagrams
 * on a connection oriented socket.
 * 
 * Sources: Linux manual pages
 */

#include <stdio.h>  // printf
#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE
#include <sys/types.h>  // socket, bind (see man socket), recv, send
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
    int listener_socket = socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (listener_socket < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // Step 2: Bind the socket to a file so it can start accepting connections
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_SOCKET_FILE); // On Linux no longer than 107 characters.
    unlink(SERVER_SOCKET_FILE); // Ensure that no file exists with that name
    if (bind(listener_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto cleanup;
    }

    // Step 3: Configure the socket for listening connections
    //  We'll configure the socket to have a backlog of 20 connections. This 
    //  means that we can have up to 20 peers waiting to be accepted by our 
    //  server. While the backlog is full, connections will be rejected.
    if (listen(listener_socket, 20) < 0) {
        perror("listen");
        goto cleanup;
    }

    // Step 4: Wait and accept connections
    while (1) {
        // Here we have 2 sockets:
        //  - The one that will be accepting connections (listener_socket),
        //  - and the connection itself to a peer (data_socket).
        printf("Waiting for connection...\n");
        int data_socket = accept(listener_socket, NULL, NULL);

        if (data_socket < 0) {
            perror("accept");
            goto cleanup;
        }

        printf("New connection\n");

        // Step 5: Start sending and receiving data!
        int len;
        char buffer[8192];
        while ((len = recv(data_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[len] = '\0';

            printf("Data received: \n");
            printf("\tlen: %d\n", len);
            printf("%s\n", buffer);
            printf("--------------------------\n");
            // Echo data back. Since this is a datagram socket, we can ensure 
            // that the whole packet will be sent at once
            if (send(data_socket, buffer, len, 0) < 0) {
                perror("sendto");
                goto cleanup;
            }
        }

        if (len < 0) {
            perror("recv");
            close(data_socket);
            goto cleanup;
        } else {
            printf("Connection closed\n");
            close(data_socket);
        }
    }

    ret = EXIT_SUCCESS;

cleanup:
    close(listener_socket);
    unlink(SERVER_SOCKET_FILE);
    return ret;
}
