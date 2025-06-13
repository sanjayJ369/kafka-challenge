#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

const int supported_versions[] = {0, 1, 2, 3, 4}; 
const int versions = 5;
const int16_t ver_err_code = 35;

int main(int argc, char* argv[]) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket: " << std::endl;
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        std::cerr << "setsockopt failed: " << std::endl;
        return 1;
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9092);

    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        close(server_fd);
        std::cerr << "Failed to bind to port 9092" << std::endl;
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        close(server_fd);
        std::cerr << "listen failed" << std::endl;
        return 1;
    }

    std::cout << "Waiting for a client to connect...\n";

    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!\n";
    

    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
    std::cout << "Client connected\n";
    int32_t req_message_size, correlation_id, res_message_size; 
    int16_t request_api_key, request_api_version, error;
    error = htons(0);
    // read message size
    read(client_fd, &req_message_size, 4); 
    req_message_size = ntohl(req_message_size);

    // read message
    char *buffer = (char*)calloc(req_message_size, sizeof(char));
    read(client_fd, buffer, req_message_size); 

    // parsing header
    memcpy(&correlation_id, buffer+4, sizeof(int32_t));
    memcpy(&request_api_version, buffer+2, sizeof(int16_t)); 


    // check if requested version is supported
    bool ver_err = true;
    for (int i = 0; i < versions; i++) {
        if (ntohs(request_api_version) == supported_versions[i]) {
            ver_err = false;
        }
    }
    if (ver_err) {
        error = htons(ver_err_code);
    }

    res_message_size = htonl(6);
    write(client_fd, &res_message_size, 4);
    write(client_fd, &correlation_id, 4); 
    write(client_fd, &error, 2); 
    close(client_fd);

    close(server_fd);
    return 0;
}