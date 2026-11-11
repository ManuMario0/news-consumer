/*
* app.hpp - App life cycle.
*
* It uses the different modules to handle:
* - Client connection ;
* - Authentication ;
* - Last known state recovery ;
* - Poll event loop.
*/

#include <poll.h>
#include <string>

#include "client.hpp"
#include "client_data.hpp"
#include "transport.hpp"
#include "uid.hpp"

namespace nc {
    class App {
        public:
            App(const std::string& recovery_path, const uint32_t port);

            void run();

        private:
            Client clients[MAX_CLIENTS];
            struct pollfd client_polls[MAX_CLIENTS + 1];
            TcpServer server;
            ClientStorage storage;

            void insert_new_client(TcpClientHandle client);
    };
}
