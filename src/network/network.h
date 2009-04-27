/* 
 * File:   network.h
 * Author: Catherine
 *
 * Created on April 11, 2009, 5:32 AM
 *
 * This file is a part of Shoddy Battle.
 * Copyright (C) 2009  Catherine Fitzpatrick and Benjamin Gwin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, visit the Free Software Foundation, Inc.
 * online at http://gnu.org.
 */

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>

namespace shoddybattle {
    class ScriptMachine;
} // namespace shoddybattle

namespace shoddybattle { namespace database {
    class DatabaseRegistry;
}} // namespace shoddybattle::database

namespace shoddybattle { namespace network {

const int HEADER_SIZE = sizeof(char) + sizeof(int32_t);

class ServerImpl;
class OutMessage;

class Client {
public:
    typedef boost::shared_ptr<Client> PTR;

    virtual void sendMessage(const OutMessage &msg) = 0;
    virtual std::string getName() const = 0;
    virtual std::string getIp() const = 0;

protected:
    Client() { }
    virtual ~Client() { }
};

class Server {
public:
    Server(const int port);
    ~Server();
    void run();
    database::DatabaseRegistry *getRegistry();
    ScriptMachine *getMachine();
    void initialiseChannels();

private:
    ServerImpl *m_impl;
    Server(const Server &);
    Server &operator=(const Server &);
};

/**
 * A message that the server sends to a client.
 */
class OutMessage {
public:
    enum TYPE {
        WELCOME_MESSAGE = 0,
        PASSWORD_CHALLENGE = 1,
        REGISTRY_RESPONSE = 2,
        SERVER_INFO = 3,
        CHANNEL_INFO = 4,
        CHANNEL_JOIN_PART = 5,
        CHANNEL_STATUS = 6,
        CHANNEL_LIST = 7,
        CHANNEL_MESSAGE = 8,
        INCOMING_CHALLENGE = 9,
        FINALISE_CHALLENGE = 10,
        CHALLENGE_WITHDRAWN = 11,
        BATTLE_BEGIN = 12,
        REQUEST_ACTION = 13,
        BATTLE_POKEMON = 14,
        BATTLE_HEALTH_CHANGE = 15,
    };

    // variable size message
    OutMessage(const TYPE type) {
        m_data.push_back((unsigned char)type);
        m_data.resize(HEADER_SIZE, 0);    // insert in 0 size for now
    }

    // fixed size message
    OutMessage(const TYPE type, const int size) {
        m_data.push_back((unsigned char)type);
        *this << int32_t(size);
        m_data.reserve(HEADER_SIZE + size);
    }

    void finalise();
    
    const std::vector<unsigned char> &operator()() const;

    OutMessage &operator<<(const int16_t);
    OutMessage &operator<<(const int32_t);
    OutMessage &operator<<(const unsigned char);
    OutMessage &operator<<(const std::string &);

    virtual ~OutMessage() { }
private:
    std::vector<unsigned char> m_data;
};

}} // namespace shoddybattle::network

#endif
