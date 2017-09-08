#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void get_filepath(const char* client_argv, const char* f_name, char* file_path){

    size_t file_address_length = strlen(client_argv);
    
    size_t f_size = strlen(f_name);
    size_t total = f_size + file_address_length + 2;

    memset(file_path, 0, total);

    strcat(file_path, client_argv);
    strcat(file_path, "/");
    strcat(file_path, f_name);
    file_path[total-1] = 0;

}
void server_setup(struct sockaddr_in *server, unsigned short port_number, int* sockfd){
    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(server, 0, sizeof(struct sockaddr_in));
    server->sin_family = AF_INET;

    server->sin_addr.s_addr = htonl(INADDR_ANY);
    server->sin_port = htons(port_number);
    bind(*sockfd, (struct sockaddr *)server, (socklen_t) sizeof(struct sockaddr_in));
}
size_t file_to_buffer(FILE *fp, char* buffer_out){

    int c;
    size_t index = 0;
    while(index != 512){
        c = fgetc(fp);
        if (c == EOF) return index;
        buffer_out[index++] = (char)c;
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


    fprintf(stdout, "Starting server loop\n");
    fflush(stdout);
    for (;;) {

        // Wait to receive info/packet from client
        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, buffer_in, sizeof(buffer_in) - 1,
                             0, (struct sockaddr *) &client, &len);

        fprintf(stdout, "Something received...\n");
        fflush(stdout);

        // Checks the OP code if its not a RRQ
        if(buffer_in[0] != 0 && buffer_in[1] != 1)
        {
            sendto(sockfd, buffer_in, (size_t) n, 0, (struct sockaddr *) &client, len);
        }

        // RRQ
        if(buffer_in[1] == 1)
        {
            fprintf(stdout, "Received RRQ\n");
            fflush(stdout);

            char* f_name = buffer_in + 2;
            char file_path[6969];
            get_filepath(argv[2], f_name, file_path);

            fprintf(stdout, "File asked for: %s\n", file_path);
            fflush(stdout);


            // Opna file
            FILE *fp;
            fp = fopen(file_path, "r");

            buffer_out[0] = 0;
            buffer_out[1] = 3;

            unsigned short jenny_from_the_block_number = 1;
            
            int x = 1;
            //Transfer loop
            while(1){
                //fprintf(stdout, "Iteration %d\n", x++);
                //fflush(stdout);


                buffer_out[2] = (jenny_from_the_block_number >> 8)&0xff;
                buffer_out[3] = jenny_from_the_block_number&0xff;
                
                size_t count_read = file_to_buffer(fp, buffer_out + 4); 

                //fprintf(stdout, "count read = %zu\n", count_read);
                //fflush(stdout);
                
                sendto(sockfd, buffer_out, count_read + 4, 0, 
                    (struct sockaddr *) &client, len);


                //fprintf(stdout, "buffer_in sent\n");
                //fflush(stdout);

                // bíða eftir ackki
                // þyggja ack

                ssize_t n = recvfrom(sockfd, buffer_in, sizeof(buffer_in) - 1,
                             0, (struct sockaddr *) &client, &len);

                //fprintf(stdout, "buffer_in received\n");
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
 sendto(sockfd, buffer_in, (size_t) n, 0,
               (struct sockaddr *) &client, len);
*/