// utils.cpp
#include "utils.hpp"

#include <iomanip> 
#include <iostream> 
#include <cctype>   
// Function definition (implementation)
void printBufferAsHex(const char* buffer, size_t length) {
    std::cout << "Hex Dump:\n";
    std::cout << std::hex << std::setfill('0');

    for (size_t i = 0; i < length; ++i) {
        if (i % 16 == 0) {
            std::cout << std::setw(4) << i << " | ";
        }

        std::cout << std::setw(2) << static_cast<int>(static_cast<unsigned char>(buffer[i])) << " ";

        if ((i + 1) % 16 == 0) {
            std::cout << "| ";
            for (size_t j = i - 15; j <= i; ++j) {
                char c = buffer[j];
                std::cout << ( (c >= 32 && c <= 126) ? c : '.' );
            }
            std::cout << std::endl;
        }
    }

    if (length % 16 != 0) {
        for (size_t i = 0; i < (16 - (length % 16)); ++i) {
            std::cout << "   ";
        }
        std::cout << "| ";
        for (size_t j = length - (length % 16); j < length; ++j) {
            char c = buffer[j];
            std::cout << ( (c >= 32 && c <= 126) ? c : '.' );
        }
        std::cout << std::endl;
    }

    std::cout << std::dec << std::setfill(' '); // Reset formatting
}