#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

int main(int argc, char** argv)
{
	// Checks if the number of arguments is correct
	if(argc != 3){  
		perror("3RR0R");
		exit(1);
	}

	short portNum = atoi(argv[1]);  // TODO ERROR HANDLING
	// argv[2] = file/folder path

    int sockfd;
    struct sockaddr_in server, client;
    char message[512];


    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

	server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(portNum);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    for (;;) {

        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);

        char buffer[10] = {0, 1, 0, 0, 'R', 'A', 's', 's', '\0', 12};

        if(buffer[0] != 0 && buffer[1] != 1)
        {
			sendto(sockfd, buffer, 9, 0, (struct sockaddr *) &client, len);
        }


        printf("%hu\n", client.sin_port);
        break;
        
    }
}


/*
 sendto(sockfd, message, (size_t) n, 0,
               (struct sockaddr *) &client, len);
*/