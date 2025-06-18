#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "utils.hpp"

void handle_sock(int client_fd);
const int supported_versions[] = {0, 1, 2, 3, 4};
const int versions = 5;
const int16_t ver_err_code = 35;

struct APIVersion
{
    uint16_t api_key;
    uint16_t max_ver;
    uint16_t min_ver;
};

APIVersion api_versions_api = {
    18,
    4,
    0,
};

APIVersion api_versions_apis[] = {api_versions_api};
const int api_versions_apis_count = 1;

int main(int argc, char *argv[])
{
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Failed to create server socket: " << std::endl;
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        close(server_fd);
        std::cerr << "setsockopt failed: " << std::endl;
        return 1;
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9092);

    if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) != 0)
    {
        close(server_fd);
        std::cerr << "Failed to bind to port 9092" << std::endl;
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0)
    {
        close(server_fd);
        std::cerr << "listen failed" << std::endl;
        return 1;
    }

    std::cout << "Waiting for a client to connect...\n";

    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!\n";

    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len);
    std::cout << "Client connected\n";
    handle_sock(client_fd);

    close(server_fd);
    return 0;
}

void handle_sock(int client_fd)
{
    int32_t req_message_size, correlation_id, res_message_size;
    int16_t request_api_key, request_api_version, error;
    error = htons(0);

    // read message size
    while (read(client_fd, &req_message_size, 4) > 0)
    {
        req_message_size = ntohl(req_message_size);

        // read entire message
        char *buffer = (char *)calloc(req_message_size, sizeof(char));
        read(client_fd, buffer, req_message_size);

        // parsing header
        memcpy(&correlation_id, buffer + 4, sizeof(int32_t));
        memcpy(&request_api_version, buffer + 2, sizeof(int16_t));

        // check if requested version is supported
        bool ver_err = true;
        for (int i = 0; i < versions; i++)
        {
            if (ntohs(request_api_version) == supported_versions[i])
            {
                ver_err = false;
            }
        }

        // construct compact array, 
        uint32_t comp_arr_len = 7 * api_versions_apis_count + (api_versions_apis_count - 1);
        char *comp_arr = (char *)calloc(comp_arr_len, sizeof(char));
        comp_arr[0] = api_versions_apis_count + 1;
        if (!ver_err)
        {
            for (int i = 0; i < api_versions_apis_count; i++)
            {
                APIVersion ver = api_versions_apis[i];
                // array length 1B
                uint8_t start = i * 7, buffer_tag = 0;
                uint8_t arr_len = 8;
                uint16_t api_key = htons(ver.api_key);
                uint16_t min_support = htons(ver.min_ver);
                uint16_t max_support = htons(ver.max_ver);

                memcpy(comp_arr + start + 1, &api_key, 2);
                memcpy(comp_arr + start + 3, &min_support, 2);
                memcpy(comp_arr + start + 5, &max_support, 2);
                if (i != comp_arr_len - 1)
                {
                    memcpy(comp_arr + start + 7, &buffer_tag, 1);
                }
            }
        }
        if (ver_err)
        {
            error = htons(ver_err_code);
            res_message_size = 6;
        }
        else
        {
            res_message_size = 6 + comp_arr_len + 1 + 4 + 1;
        }
        res_message_size = htonl(res_message_size);
        uint8_t tagbuffer = 0;
        uint32_t throttle = 0;

        write(client_fd, &res_message_size, 4);
        write(client_fd, &correlation_id, 4);
        write(client_fd, &error, 2);
        if (!ver_err)
        {
            write(client_fd, comp_arr, comp_arr_len);
            std ::cout << "\nwrote compact array";
            write(client_fd, &tagbuffer, 1);
            write(client_fd, &throttle, 4);
            write(client_fd, &tagbuffer, 1);
        }
    }
    close(client_fd);
}