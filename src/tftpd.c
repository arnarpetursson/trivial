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
void transfer_error_msg(char number){
    if(number != 1){
        perror("RRQ not expected\0");
    }
    else if(number != 2){
        perror("WRQ not expected\0");
    }
    else if(number != 3){
        perror("DATA request not expected\0");
    }
    else if(number != 5){
        perror("Error\0");
    }
    else{
        perror("Undefined error");
    }
}

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
    char error_msg[169];
    char opcode[4];

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

        fprintf(stdout, "A undisclosed client appeared\n");
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

            char* f_name = buffer_in + 2;
            char file_path[6969];
            get_filepath(argv[2], f_name, file_path);
            char* client_ip = inet_ntoa(client.sin_addr);
            
            fprintf(stdout, "File %s requested from %s:%d \n", f_name, client_ip, client.sin_port);
            fflush(stdout);

            if (strstr(file_path, "/../")){
                // errorcode 02 access violation
                continue;
            }
            
            // Opna file
            FILE *fp;
            fp = fopen(file_path, "r");

            if(fp == NULL){
                // error code 01 file not found
            }

            // Setting OP code to 3(data)
            buffer_out[0] = 0;
            buffer_out[1] = 3;

            unsigned short block_number = 1;
            size_t count_read = 512;
            unsigned short last_block_number = -69;
            ssize_t resend = -1;

            //Transfer loop
            while(count_read == 512){

                // Checks if another client is trying to intercept the transfer
                if(client.sin_port == last_client_port){
                    // TODO: Skoða með Jonna
                    opcode[0] = 0;
                    opcode[1] = 5;
                    opcode[2] = 0;
                    opcode[3] = 2;
                    char* getout = "get out bastard";
                    memset(error_msg, 0, strlen(getout) + strlen(opcode) + 1);
                    strcat(error_msg, opcode);
                    strcat(error_msg, getout);
                    error_msg[strlen(getout) + strlen(opcode)] = 0;
                    ssize_t getout_length = strlen(error_msg);
                    sendto(sockfd, error_msg, getout_length, 0, 
                        (struct sockaddr *) &client, len);

                    fprintf(stdout, "%s\n", opcode);
                    fflush(stdout);
                }

                // TODO: If not correct client
                buffer_out[2] = (block_number >> 8)&0xff;
                buffer_out[3] = block_number&0xff;
                
                // Checks if the server needs to send the last packet
                if (last_block_number == block_number){
                    fprintf(stdout, "Resend loop started\n");
                    fflush(stdout);
                    while(resend == -1){
                        resend = sendto(sockfd, buffer_out, count_read + 4, 0, 
                        (struct sockaddr *) &client, len);
                    }
                }
                
                // Get byte size of the array
                count_read = file_to_buffer(fp, buffer_out + 4); 
                
                bool block_number_correct = false;        
                // The OP != ACK or wrong blocknumber try to send the request again
                while(block_number_correct == false){
                    // Sends the buffer
                    ssize_t sendto_validation = sendto(sockfd, buffer_out, count_read + 4, 0, 
                        (struct sockaddr *) &client, len);
                    // receive info from client
                    ssize_t n = recvfrom(sockfd, buffer_in, sizeof(buffer_in) - 1,
                             0, (struct sockaddr *) &client, &len);
                    if(sendto_validation != -1){
                        block_number_correct = true;
                    }
                }
                // Checks if not ACK
                if(buffer_in[1] != 4){
                    transfer_error_msg(buffer_in[1]);
                    break;
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