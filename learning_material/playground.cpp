#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

// Helper function to convert a single hex character to its integer value
int hexCharToInt(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    throw std::invalid_argument("Invalid hex character");
}

// Function to convert hex string to ASCII string
std::string hexToAscii(const std::string& hex) {
    if (hex.length() % 2 != 0) {
        throw std::invalid_argument("Hex string length must be even");
    }

    std::string ascii;
    ascii.reserve(hex.length() / 2);

    for (size_t i = 0; i < hex.length(); i += 2) {
        char high = hexCharToInt(hex[i]);
        char low = hexCharToInt(hex[i + 1]);
        ascii.push_back((high << 4) | low);
    }

    return ascii;
}

int main() {
    std::string hexString = "747970652d6c6f63616c2d693332";
    try {
        std::string asciiString = hexToAscii(hexString);
        std::cout << "ASCII string: " << asciiString << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}