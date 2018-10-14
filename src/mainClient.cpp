#include <iostream>
#include <string>

#include "Client.hpp"

#define PORT 4000

int main(void){
    dropbox::Client client("username", "localhost", PORT);
    std::string dummy;

    client.start();

    sleep(10);
    client.upload("oi.md");
    //client.download("alo.txt");

    std::cin >> dummy;


    client.stop();

    return 0;
}
