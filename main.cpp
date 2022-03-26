#include <iostream>

int main() {
    #ifdef _WIN32
    std::cout << "Hello World!" << std::endl;
    #endif
}
