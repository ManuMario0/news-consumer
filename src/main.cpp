#include <csignal>
#include <cstdio>

#include "client_data.hpp"

int main() {
    std::signal(SIGPIPE, SIG_IGN);

    std::printf("hello world\n");
    return 0;
}
