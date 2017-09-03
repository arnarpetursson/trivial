#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

int main(int argc, char** argv)
{

	fprintf(stdout, "%s-%s-%s-%s-%s", argv[0], argv[1], argv[2], argv[3], argv[4]);
	fflush(stdout);

    int sockfd;
    struct sockaddr_in server, client;
    char message[512];






    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;




	server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(6969);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));




    for (;;) {



        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);


        printf("%hu\n", client.sin_port);
        break;
        
    }
}

/*
 sendto(sockfd, message, (size_t) n, 0,
               (struct sockaddr *) &client, len);
*/