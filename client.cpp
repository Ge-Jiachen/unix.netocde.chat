#define _GUN_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <cstdlib>
#include <fcntl.h>

#define BUFFER_SIZE 64

int main( int argc, char ** argv ){

    struct sockaddr_in servaddr; 
    const char * ip;
    int port;
    char read_buf[ BUFFER_SIZE ];

    if( argc <= 2 ){
        printf( "usage: %s ip_address port_number\n", basename( argv[ 0 ] ) );
        return 1;
    }
    ip = argv[ 1 ];
    port = atoi( argv [ 2 ] );// string -> int

    bzero( &servaddr, sizeof( servaddr ) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons( port );
    inet_pton( AF_INET, ip, &servaddr.sin_addr );

    int sockfd = socket( AF_INET, SOCK_STREAM, 0);
    assert( sockfd >= 0 );
    if( connect( sockfd, ( struct sockaddr* )&servaddr, sizeof( servaddr ) ) < 0 ){
        printf( "connection failed\n" );
        close( sockfd );
        return 1;
    }

    pollfd fds[ 2 ];

    fds[ 0 ].fd = 0;// stdin
    fds[ 0 ].events = POLLIN;// input
    fds[ 0 ].revents = 0;// ?

    fds[ 1 ].fd = sockfd;
    fds[ 1 ].events = POLLIN|POLLRDHUP;
    fds[ 1 ].revents = 0;

    int pipefd[ 2 ];
    int ret = pipe( pipefd );//read--pipefd[0] write--pipefd[1]
    assert( ret != -1 );

    for( ; ; ){
        ret = poll( fds, 2, -1 );
        if( ret < 0 ){
            printf( "poll failure\n" );
            break;
        }

        if( fds[ 1 ].revents & POLLRDHUP ){
            printf( "server close the communication\n" );
            break;
        }
        else if( fds[ 1 ].revents & POLLIN){ //sockfd 
            memset( read_buf, '\0', BUFFER_SIZE );
            recv( fds[ 1 ].fd, read_buf, BUFFER_SIZE - 1, 0 );
            printf( "%s\n", read_buf);
        }

        if( fds[ 0 ].revents & POLLIN ){
            // stdin -> pipefd[1] -> pipefd[0] -> sockfd
            ret = splice( 0, NULL, pipefd[ 1 ], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
            ret = splice( pipefd[ 0 ], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
        }
    }
    close ( sockfd );
    return 0;
}
