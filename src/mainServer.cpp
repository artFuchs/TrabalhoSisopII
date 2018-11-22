#include <iostream>
#include <string>

#include "Server.hpp"

#define PORT 4000
#define RMPORT 4001

int main(void){
    dropbox::Server server(PORT, RMPORT);
    std::string dummy;

    server.run();

    // Waits for user input to stop the server
    std::cin >> dummy;

    server.stop();

    return 0;
}
