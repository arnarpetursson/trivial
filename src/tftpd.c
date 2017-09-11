#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

typedef int bool;
#define true 1
#define false 0
// Function that writes out error messages for the server and the client
void error_packet(struct sockaddr_in *client, int error_code, socklen_t len, int* sockfd) {
    char error_msg[169];
    memset(error_msg, 0, 169);
    error_msg[0] = 0;
    error_msg[1] = 5;
    error_msg[2] = 0;
    
    switch(error_code) {
        case 0:
            error_msg[3] = 0;
            perror("Undefined error");
            strcat(error_msg + 4, "Undefined error");
            break;
        case 1:
            error_msg[3] = 1;
            perror("File not found");
            strcat(error_msg + 4, "File not found");
            break;
        case 2:
            error_msg[3] = 2;
            perror("Access violation");
            strcat(error_msg + 4, "Access violation");
            break;
        case 3:
            error_msg[3] = 3;
            perror("Disk full or allocation exceeded.");
            strcat(error_msg + 4, "Disk full or allocation exceeded.");
            break;
        case 4:
            error_msg[3] = 4;
            perror("Illegal TFTP operation.");
            strcat(error_msg + 4, "Illegal TFTP operation.");
            break;
        case 5:
            error_msg[3] = 5;
            perror("Unknown transfer ID.");
            strcat(error_msg + 4, "Unknown transfer ID.");
            break;
    }

    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ssize_t error_msg_length = strlen(error_msg + 4);
    sendto(*sockfd, error_msg, error_msg_length +4, 0, 
        (struct sockaddr *) client, len);
}
// Adds the file address and file name to a single char*
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
// Sets up the server
void server_setup(struct sockaddr_in *server, unsigned short port_number, int* sockfd){
    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(server, 0, sizeof(struct sockaddr_in));
    server->sin_family = AF_INET;

    server->sin_addr.s_addr = htonl(INADDR_ANY);
    server->sin_port = htons(port_number);
    bind(*sockfd, (struct sockaddr *)server, (socklen_t) sizeof(struct sockaddr_in));
}
// Reads the file content into an array at most 512 chars at time
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
    // Server and client variables and buffers to send and receive
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

    unsigned short port_number = atoi(argv[1]);
    // Checks if the port number is legal
    if(port_number < 0 || port_number > 65535){
        perror("Illegal port number");
        exit(1);
    }

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

        fprintf(stdout, "Received an answer from a client\n");
        fflush(stdout);

        unsigned short client_port = client.sin_port;
        unsigned short last_client_port = client_port;

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

            // Get file name with path
            char* f_name = buffer_in + 2;
            char file_path[6969];
            get_filepath(argv[2], f_name, file_path);

            // Get the ip of the client
            char* client_ip = inet_ntoa(client.sin_addr);
            fprintf(stdout, "File %s requested from %s:%d \n", f_name, client_ip, client.sin_port);
            fflush(stdout);

            // Checks if client tries to access something below the shared folder
            if (strstr(file_path, "/../")){
                error_packet(&client, 2, len, &sockfd);
                continue;
            }
            
            // Open file
            FILE *fp;
            fp = fopen(file_path, "r");

            // Checks if the file does not exist 
            if(fp == NULL){
                error_packet(&client, 1, len, &sockfd);
                continue;
            }

            // Setting OP code to 3(data)
            buffer_out[0] = 0;
            buffer_out[1] = 3;

            // Initialing variables for the transferloop
            unsigned short block_number = 1;
            size_t count_read = 512;
            unsigned short last_block_number = -69;
            ssize_t resend = -1;

            //Transfer loop
            while(count_read == 512){
                // Checks if another client is trying to intercept the transfer
                if(client.sin_port != last_client_port){
                    error_packet(&client, 5, len, &sockfd);
                    continue;
                }

                // TODO: blocknumbercheck

                // Converts block_number to bytes
                buffer_out[2] = (block_number >> 8)&0xff;
                buffer_out[3] = block_number&0xff;
                
                // Checks if the server needs to resend the last packet
                if (last_block_number+1 != block_number){
                    while(resend == -1){
                        resend = sendto(sockfd, buffer_out, count_read + 4, 0, 
                        (struct sockaddr *) &client, len);
                    }
                }
            
                // Reads from the file to a buffer
                // returns the number of chars read into the buffer
                count_read = file_to_buffer(fp, buffer_out + 4); 
                
                bool block_number_correct = false;        
                // Send and receive loop
                while(block_number_correct == false){
                    // Sends the buffer
                    ssize_t sendto_validation = sendto(sockfd, buffer_out, count_read + 4, 0, 
                        (struct sockaddr *) &client, len);
                    // receive info from client
                    ssize_t n = recvfrom(sockfd, buffer_in, sizeof(buffer_in) - 1,
                             0, (struct sockaddr *) &client, &len);

                    // TODO: Timeout check

                    if(sendto_validation != -1){
                        block_number_correct = true;
                    }
                }
                // Checks if not ACK
                if(buffer_in[1] != 4){
                    if(buffer_in[1] < 0){
                        error_packet(&client, 0, len, &sockfd);
                        break;
                    }
                    error_packet(&client, 5, len, &sockfd);
                    continue;
                }
        
                // If buffer < 512 == end of file
                if(count_read < 512){
                    fprintf(stdout, "End of file \n");
                    fflush(stdout);
                    break;
                }
                last_block_number = block_number;
                block_number++;
            }

            fprintf(stdout, "Transfer Over\n");
            fflush(stdout);
            fclose(fp);
        }
    }
}