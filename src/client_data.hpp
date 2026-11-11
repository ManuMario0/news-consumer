/*
* client_data.hpp - Manage longterm storage of client states.
*
* Handles opening, memory mapping and closing of storage. All resources are
* allocated at creation and freed at drop.
*/

#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "uid.hpp"

namespace nc {
    class ClientStorage {
        public:
            explicit ClientStorage(const std::string& path);
            ~ClientStorage();

            size_t next_sequence_token(const UID client_uid);
            void mark_sequence_token_processed(const UID client_id);
        
        private:
            struct ClientList;

            std::unique_ptr<ClientList> client_list;
    };
}
