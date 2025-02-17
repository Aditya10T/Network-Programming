#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())
#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif

#include <stdio.h>
#include <string.h>

#if defined (_WIN32)
#include <conio.h>  // this helps us taking input from terminal with _kbhit() in windows, for linux we can do so with select()
#endif

int main(int argc, char *argv[]){
    #if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
    #endif

    if(argc < 3){
        fprintf(stderr,"usage: tcp_client hostname port\n");
        return 1;
    }

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if(getaddrinfo(argv[1],argv[2],&hints,&peer_address)){
        fprintf(stderr,"getaddrinfo() failed, (%d)\n",GETSOCKETERRNO());
        return 1;
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr,peer_address->ai_addrlen,address_buffer,sizeof(address_buffer),service_buffer,sizeof(service_buffer),NI_NUMERICHOST);
    printf("%s %s\n",address_buffer,service_buffer);

    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,peer_address->ai_socktype,peer_address->ai_protocol);
    if(!ISVALIDSOCKET(socket_peer)){
        fprintf(stderr,"socket() failed. (%d)\n",GETSOCKETERRNO());
        return 1;
    }

    printf("Connecting...\n");
    if(connect(socket_peer,peer_address->ai_addr,peer_address->ai_addrlen)){
        fprintf(stderr, "connect() failed. (%d)\n",GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);

    printf("Connected.\n");
    printf("To send data, enter text followed by enter.\n");

    while(1){
        
        fd_set reads; // to store our socket set
        FD_ZERO(&reads);   // set it to zero
        FD_SET(socket_peer,&reads);  // adding our only socket socket_peer
        
        #if !defined(_WIN32)
            FD_SET(0,&reads);
        #endif

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        
        // using select to check if any socket in our set has some incoming data from server or input data from our terminal
        if(select(socket_peer+1,&reads,0,0,&timeout)<0){
            fprintf(stderr,"select() failed. (%d)\n",GETSOCKETERRNO());
            return 1;
        }
        
        // if any data is recieved from the server
        if(FD_ISSET(socket_peer,&reads)){
            char read[4096];
            int bytes_recieved = recv(socket_peer,read,4096,0);
            if(bytes_recieved < 1){
                printf("Connection closed by peer.\n");
                break;
            }
            printf("Recieved (%d bytes): %.*s", bytes_recieved,bytes_recieved,read);
        }

        // if the data is input from the terminal
            #if defined(_WIN32)
                if(_kbhit()){
            #else
                if(FD_ISSET(0,&reads)){  // for linux systems.. the file descriptor for stding in linux systems is 0
            #endif
                    printf("taking input\n");
                    char read[4096];
                    if(!fgets(read,4096,stdin)) break;
                    printf("Sending: %s",read);
                    int bytes_sent = send(socket_peer,read,strlen(read),0);
                    printf("Sent %d bytes.\n",bytes_sent);
                }
    }
    printf("Closing Socket...\n");
    CLOSESOCKET(socket_peer);

    #if defined(_WIN32)
        WSACleanup();
    #endif

    printf("Finished.\n");
    return 0;
}





