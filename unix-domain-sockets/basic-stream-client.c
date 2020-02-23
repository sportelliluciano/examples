/**
 * Example code for a simple client using Unix Domain Sockets.
 * 
 * This client will connect to an echo server using SOCK_STREAM protocol 
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
    int s = socket(PF_UNIX, SOCK_STREAM, 0);
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
        // Send data to the server. Since this is a stream socket, we can send
        // an arbitrary number of bytes but we have to deal with the posibility
        // of the data being sent partially.
        len = strlen(buffer);
        int data_sent = 0;
        while (data_sent < len) {
            // Since this is a connection oriented socket, we don't need
            // to specify the address.
            int sent = send(s, buffer + data_sent, len - data_sent, 0);
            
            if (sent < 0) {
                perror("sendto");
                goto cleanup;
            }

            data_sent += sent;
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
