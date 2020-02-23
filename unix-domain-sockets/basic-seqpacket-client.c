/**
 * Example code for a simple client using Unix Domain Sockets.
 * 
 * This client will connect to an echo server using SOCK_SEQPACKET protocol 
 * through Unix Domain Sockets.
 * 
 * Sources: Linux manual pages
 */

#include <stdio.h>  // printf, fgets
#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE
#include <sys/types.h>  // socket, connect, recv, send
#include <sys/socket.h> // socket, connect, recv, send
#include <sys/un.h> // sockaddr_un
#include <string.h> // memset, strlen
#include <unistd.h> // close

#define SERVER_SOCKET_FILE "/tmp/unix-domain-socket-example"

int main() {
    int ret = EXIT_FAILURE;

    // Step 1: Create a socket
    //  We have to create a socket in the exact same way as we did for the 
    //  listener socket in the server.
    int s = socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (s < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // Step 2: Connect the socket to the server's socket file.
    //  We create the sockaddr structure in the same way as the server
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_SOCKET_FILE); // On Linux no longer than 107 characters.
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        goto cleanup;
    }

    // Step 3: Start sending and receiving data!
    int len = 0;
    char buffer[8192];
    printf("Enter a line to send to the server\n");
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        // Send data to the server. Since this is a datagram socket, we can 
        // ensure that the whole packet will be sent at once.
        // However, datagrams have a length limit so we cannot send packets of 
        // arbitrary size. 
        // Since this is a connection oriented socket we don't need to specify
        // the address so we use send instead of sendto.
        if (send(s, buffer, strlen(buffer), 0) < 0) {
            perror("send");
            goto cleanup;
        }

        // Wait for a response.
        // Again, since this is a connection oriented socket we use recv instead
        // of recvfrom.
        if ((len = recv(s, buffer, sizeof(buffer) - 1, 0)) < 0) {
            perror("recv");
            goto cleanup;
        }

        buffer[len] = '\0';

        printf("%d bytes received\n", len);
        printf("%s\n", buffer);
        printf("--------------------------\n");        
    }

    ret = EXIT_SUCCESS;

cleanup:
    close(s);
    return ret;
}
