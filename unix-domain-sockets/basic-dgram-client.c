/**
 * Example code for a simple datagram client using Unix Domain Sockets.
 * 
 * This client will connect to a server using SOCK_DGRAM protocol through Unix
 * Domain Sockets.
 * 
 * Sources: Linux manual pages
 */

#include <stdio.h>  // printf, fgets
#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE
#include <sys/types.h>  // socket, bind, connect (see man socket), recv, send
#include <sys/socket.h> // socket, bind, connect, recv, send
#include <sys/un.h> // sockaddr_un
#include <string.h> // memset, strlen
#include <unistd.h> // unlink, close

#define SERVER_SOCKET_FILE "/tmp/unix-domain-socket-example"
#define CLIENT_SOCKET_FILE "/tmp/unix-domain-socket-example-client"

int main() {
    int ret = EXIT_FAILURE;

    // Step 1: Create a socket
    // We have to create a socket in the exact same way as we did in the server.
    int s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // [Semi-optional] Step 2: Bind the client socket to a file.
    //  If we want to receive data from the server, we need to have a socket 
    //  file to which the server will send the data back. If we don't need to
    //  hear an answer from the server we could skip this step.
    //  This step is the same as the bind done in the server, but changing the
    //  socket file.
    struct sockaddr_un client_addr;
    memset(&client_addr, 0, sizeof(client_addr)); 
    client_addr.sun_family = AF_UNIX;
    strcpy(client_addr.sun_path, CLIENT_SOCKET_FILE); // On Linux no more than 107 characters.
    unlink(CLIENT_SOCKET_FILE);
    if (bind(s, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("bind");
        goto cleanup;
    }

    // [Optional] Step 3: Connect the socket to the server address
    //  This step sets the defaults for sendto to the server's address so we 
    //  can ignore those parameters when we call them and blocks any incoming 
    //  packet that does not come from that address.
    //  If we skip this step, we'll have to pass the server address in each
    //  sendto/recvfrom call (and check who is sending the packet).
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SERVER_SOCKET_FILE);
    if (connect(s, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        goto cleanup;
    }
    
    // Step 4: Start sending and receiving data!
    int len = 0;
    char buffer[8192];
    printf("Enter a line to send to the server\n");
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        // Send data to the server. Since this is a datagram socket, we can 
        // ensure that the whole packet will be sent at once.
        // However, datagrams have a length limit so we cannot send packets of 
        // arbitrary size. The theoretical maximum for an UDP packet is 65.535 
        // bytes, but this includes header information so the real datagram limit
        // is less than 65.535.
        // This call is equivalent to sendto(s, buffer, strlen(buffer), 0, NULL, 0)
        // (man 2 sendto)
        if (send(s, buffer, strlen(buffer), 0) < 0) {
            perror("send");
            goto cleanup;
        }

        // Wait for a response.
        // This call is equivalent to recvfrom(s, buffer, strlen(buffer), 0, NULL, NULL)
        // (man 2 recvfrom)
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
    unlink(CLIENT_SOCKET_FILE);
    return ret;
}
