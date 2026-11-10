/*
* client_data.hpp - Manage longterm storage of client states.
*/

#include <cstddef>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <span>

#include "client.hpp"
#include "client_data.hpp"

namespace nc {
    /*
    * Helpers.
    */
    struct Client {
        int sequence_token;
    };

    class File {
        public:
            int fd;

            /*
            * Open/Create - file in write + read mode.
            *
            * # Error
            *
            * Throw a `runtime_error` when the file could not be open/created.
            */
            File(const std::string& path) {
                fd = open(
                    path.c_str(),
                    O_RDWR | O_CREAT,
                    S_IRUSR | S_IWUSR
                );
                if (fd == -1) {
                    throw std::runtime_error(std::string("open failed: ") + strerror(errno));
                }
            };

            ~File() {
                close(fd);
            }

            File(const File&) = delete;
            File& operator=(const File&) = delete;
            File(File&&) = delete;
            File& operator=(File&&) = delete;

            /*
            * Truncates the file to a specific size.
            */
            bool truncate(const size_t size) {
                int ret = ftruncate(fd, size);
                return ret != -1;
            }

            /*
            * Map the file descriptor to memory.
            *
            * The file desciptor can be safely dropped once the memory is mapped.
            * The file is mapped with MAP_SHARED to protect data on process crash.
            *
            * The implementation provides a raw pointer (no RAII) that should be destroyed with unmap.
            */
            void *map_file(const size_t size) {
                return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            }
    };

    /*
    * Implementation.
    */
    struct ClientStorage::ClientList {
        std::span<Client, MAX_CLIENTS> clients;

        static std::span<Client, MAX_CLIENTS> map_clients(File&fd) {
            if (!fd.truncate(sizeof(Client[MAX_CLIENTS]))) {
                throw std::runtime_error(std::string("truncate file failed: ") + strerror(errno));
            }
            void* mem = fd.map_file(sizeof(Client[MAX_CLIENTS]));
            if (mem == MAP_FAILED) {
                throw std::runtime_error(std::string("mmap failed: ") + strerror(errno));
            }
            return std::span<Client, MAX_CLIENTS>(static_cast<Client*>(mem), MAX_CLIENTS);
        }
 
        /*
        * RAII encapsulation of the mapped memory.
        *
        * Throw a `runtime_error` if the map fails.
        */
        ClientList(File& fd): clients(map_clients(fd)) {}

        ~ClientList() {
            munmap(clients.data(), sizeof(Client[MAX_CLIENTS]));
        }

        /*
        * Rule of five: delete all move/copy operators here as they will not be needed.
        */
        ClientList(const ClientList&) = delete;
        ClientList& operator=(const ClientList&) = delete;
        ClientList(ClientList&&) = delete;
        ClientList& operator=(ClientList&&) = delete;

        /*
        * Direct indexing.
        */
        Client& client(int client_uid) {
            return clients[client_uid];
        }
    };

    ClientStorage::ClientStorage(const std::string& path) {
        File fd(path);

        client_list = std::make_unique<ClientList>(fd);
    }

    int ClientStorage::next_sequence_token(UID client_uid) {
        Client& client = client_list->client(client_uid.into_int());

        return client.sequence_token;
    }

    void ClientStorage::mark_sequence_token_processed(UID client_uid) {
        Client& client = client_list->client(client_uid.into_int());

        ++client.sequence_token;
    }
}
