#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void server_setup(struct sockaddr_in *server, unsigned short port_number, int* sockfd)
{
    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(server, 0, sizeof(struct sockaddr_in));
    server->sin_family = AF_INET;

    server->sin_addr.s_addr = htonl(INADDR_ANY);
    server->sin_port = htons(port_number);
    bind(*sockfd, (struct sockaddr *)server, (socklen_t) sizeof(struct sockaddr_in));
}
size_t file_to_buffer(FILE *fp, char* send_buffer){

    int c;
    while(index != 512){
        c = fgetc(fp);
        if (c == EOF) return index;
        send_buffer[index++] = (char)c;
    }

    return index;
}

int main(int argc, char** argv)
{
    struct sockaddr_in server, client;
    char buffer_in[516];
    char buffer_out[516];

    fprintf(stdout, "Starting\n");
    fflush(stdout);

    // Checks if the number of arguments is correct
    if(argc != 3){  
        perror("Wrong numer of arguments");
        exit(1);
    }

    fprintf(stdout, "Arguments okay\n");
    fflush(stdout);

    unsigned short port_number = atoi(argv[1]);  // TODO ERROR HANDLING
    int sockfd = 0;
    
    fprintf(stdout, "Setting up and binding socket\n");
    fflush(stdout);

    server_setup(&server, port_number, &sockfd);

    fprintf(stdout, "Setup and bind complete\n");
    fflush(stdout);

    for (;;) {


        fprintf(stdout, "Starting server loop\n");
        fflush(stdout);

        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);



        fprintf(stdout, "Something received...\n");
        fflush(stdout);

        // char buffer[10] = {0, 1, 0, 0, 'R', 'A', 's', 's', '\0', 12};

        if(message[0] != 0 && message[1] != 1)
        {
            sendto(sockfd, message, (size_t) n, 0, (struct sockaddr *) &client, len);
        }

        char send_buffer[516];
        char delivery[512];

        if(message[1] == 1) // Read request
        {

            fprintf(stdout, "Received RRQ\n");
            fflush(stdout);

            // Extracta filepath/name
            size_t file_address_length = strlen(argv[2]); // lengdin á möppunafni
            char* f_name = message + 2;
            size_t f_size = strlen(f_name); // lengd á cock.txt
            size_t total = f_size + file_address_length + 2;

            char file_path[total]; // tómur array af stærð file og folder
            memset(file_path, 0, total);

            strcat(file_path, argv[2]);
            strcat(file_path, "/");
            strcat(file_path, f_name);
            file_path[total-1] = 0;

            fprintf(stdout, "File asked for: %s\n", file_path);
            fflush(stdout);


            // Opna file
            FILE *fp;
            fp = fopen(file_path, "r");

            send_buffer[0] = 0;
            send_buffer[1] = 3;

            unsigned short jenny_from_the_block_number = 1;
            
            int x = 1;
            //Transfer loop
            while(1){
                //fprintf(stdout, "Iteration %d\n", x++);
                //fflush(stdout);


                send_buffer[2] = (jenny_from_the_block_number >> 8)&0xff;
                send_buffer[3] = jenny_from_the_block_number&0xff;
                
                size_t count_read = file_to_buffer(fp, send_buffer + 4); 

                //fprintf(stdout, "count read = %zu\n", count_read);
                //fflush(stdout);
                
                sendto(sockfd, send_buffer, count_read + 4, 0, 
                    (struct sockaddr *) &client, len);


                //fprintf(stdout, "Message sent\n");
                //fflush(stdout);

                // bíða eftir ackki
                // þyggja ack

                ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);

                //fprintf(stdout, "Message received\n");
                //fflush(stdout);

                if(count_read < 512) break;
                jenny_from_the_block_number++;
            }

            fprintf(stdout, "Transfer Over\n");
            fflush(stdout);
            
            fclose(fp);
        }

        //size_t strlen
        printf("%hu\n", client.sin_port);
        
        
    }
}

/*
 sendto(sockfd, message, (size_t) n, 0,
               (struct sockaddr *) &client, len);
*/