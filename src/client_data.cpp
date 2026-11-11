/*
* client_data.cpp - Manage longterm storage of client states.
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
        size_t sequence_token;
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
            explicit File(const std::string& path) {
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
            * Returns the underlying native handle.
            */
            int native_handle() const {
                return fd;
            }
    };

    /*
    * Implementation.
    */
    struct ClientStorage::ClientList {
        std::span<Client, MAX_CLIENTS> clients;

        static const size_t CLIENT_LIST_SIZE = sizeof(Client[MAX_CLIENTS]);

        static std::span<Client, MAX_CLIENTS> map_clients(File& fd) {
            truncate_fd(fd);
            int native_fd = fd.native_handle();
            void* mem = map_file(native_fd, CLIENT_LIST_SIZE);
            return cast_as_span(mem);
        }

        /*
        * Map the file descriptor to memory.
        *
        * The file desciptor can be safely dropped once the memory is mapped.
        * The file is mapped with MAP_SHARED to protect data on process crash.
        *
        * The implementation provides a raw pointer (no RAII) that should be destroyed with unmap.
        *
        * Throw `runtime_error` if the file could not be mapped properly.
        */
        static void *map_file(const int fd, const size_t size) {
            void* mem = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (mem == MAP_FAILED) {
                throw std::runtime_error(std::string("mmap failed: ") + strerror(errno));
            }
            return mem;
        }

        static std::span<Client, MAX_CLIENTS> cast_as_span(void* mem) {
            return std::span<Client, MAX_CLIENTS>(static_cast<Client*>(mem), MAX_CLIENTS);
        }

        static void truncate_fd(File& fd) {
            if (!fd.truncate(CLIENT_LIST_SIZE)) {
                throw std::runtime_error(std::string("truncate file failed: ") + strerror(errno));
            }
        }
 
        /*
        * RAII encapsulation of the mapped memory.
        *
        * Throw a `runtime_error` if the map fails.
        */
        explicit ClientList(File& fd): clients(map_clients(fd)) {}

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
        Client& client(const int client_uid) {
            return clients[client_uid];
        }
    };

    ClientStorage::ClientStorage(const std::string& path) {
        File fd(path);

        client_list = std::make_unique<ClientList>(fd);
    }

    size_t ClientStorage::next_sequence_token(const UID client_uid) {
        Client& client = client_list->client(client_uid.into_int());

        return client.sequence_token;
    }

    void ClientStorage::mark_sequence_token_processed(const UID client_uid) {
        Client& client = client_list->client(client_uid.into_int());

        ++client.sequence_token;
    }
}
