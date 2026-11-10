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

#include "client.hpp"

namespace nc {
    class ClientStorage {
        public:
            ClientStorage(const std::string& path);
            ~ClientStorage() = default;

            int next_sequence_token(UID client_uid);
            void mark_sequence_token_processed(UID client_id);
        
        private:
            struct ClientList;

            std::unique_ptr<ClientList> client_list;
    };
}
