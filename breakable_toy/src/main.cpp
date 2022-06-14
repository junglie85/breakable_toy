#include <bt/welcome.hpp>

#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << bt::greet() << std::endl;

    return EXIT_SUCCESS;
}
