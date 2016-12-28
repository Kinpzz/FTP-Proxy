#include <string.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>

int main() {
    char buffer[] = "abc\r\n\0";
    int byte_num = 6;
    std::string buffer_str;
    buffer_str = buffer;
    std::cout << buffer_str << std::endl;
    return 0;
}