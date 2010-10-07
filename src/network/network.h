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

class Client;
class ServerImpl;
class OutMessage;
class Channel;
class NetworkBattle;

typedef boost::shared_ptr<Client> ClientPtr;

class Client {
public:
    virtual void sendMessage(const OutMessage &msg) = 0;
    virtual std::string getName() const = 0;
    virtual std::string getIp() const = 0;
    virtual int getId() const = 0;
    virtual void terminateBattle(boost::shared_ptr<NetworkBattle>,
            ClientPtr) = 0;
    virtual void joinChannel(boost::shared_ptr<Channel>) = 0;
    virtual void partChannel(boost::shared_ptr<Channel>) = 0;
    virtual void informBanned(int date) = 0;

protected:
    Client() { }
    virtual ~Client() { }
};

class Server {
public:
    Server(const int port);
    ~Server();
    void installSignalHandlers();
    void run();
    database::DatabaseRegistry *getRegistry();
    ScriptMachine *getMachine();
    void initialiseWelcomeMessage(const std::string &, const std::string &);
    void initialiseChannels();
    void initialiseMatchmaking(const std::string &);
    void initialiseClauses();
    boost::shared_ptr<Channel> getMainChannel() const;
    void addChannel(boost::shared_ptr<Channel>);
    void removeChannel(boost::shared_ptr<Channel>);
    void postLadderMatch(const int, std::vector<ClientPtr> &, const int);
    bool commitBan(const int, const std::string &, const int, const int);

private:
    ServerImpl *m_impl;
    Server(const Server &);
    Server &operator=(const Server &);
};

class OutMessageBuffer : public std::vector<unsigned char> {
public:
    OutMessageBuffer &operator<<(const int16_t);
    OutMessageBuffer &operator<<(const int32_t);
    OutMessageBuffer &operator<<(const unsigned char);
    OutMessageBuffer &operator<<(const std::string &);
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
        /** Battery of messages related to battles */
        BATTLE_BEGIN = 12,
        REQUEST_ACTION = 13,
        BATTLE_POKEMON = 14,
        BATTLE_PRINT = 15,
        BATTLE_VICTORY = 16,
        BATTLE_USE_MOVE = 17,
        BATTLE_WITHDRAW = 18,
        BATTLE_SEND_OUT = 19,
        BATTLE_HEALTH_CHANGE = 20,
        BATTLE_SET_PP = 21,
        BATTLE_FAINTED = 22,
        BATTLE_BEGIN_TURN = 23,
        SPECTATOR_BEGIN = 24,
        BATTLE_SET_MOVE = 25,
        METAGAME_LIST = 26,
        KICK_BAN_MESSAGE = 27,
        USER_DETAILS = 28,
        USER_MESSAGE = 29,
        BATTLE_STATUS_CHANGE = 30,
        CLAUSE_LIST = 31,
        INVALID_TEAM = 32,
        ERROR_MESSAGE = 33,
        PRIVATE_MESSAGE = 34
    };

    // variable size message
    OutMessage(const TYPE type) {
        m_data.push_back((unsigned char)type);
        // Insert zero size into the header to start off with. The correct size
        // is inserted by calling the finalise() method after writing data
        // to the message.
        m_data.resize(HEADER_SIZE, 0);
    }

    // fixed size message
    OutMessage(const TYPE type, const int size) {
        m_data.push_back((unsigned char)type);
        *this << int32_t(size);
        m_data.reserve(HEADER_SIZE + size);
    }

    void finalise();
    
    const OutMessageBuffer &operator()() const {
        return m_data;
    }

    template <class T>
    OutMessage &operator<<(const T &data) {
        m_data << data;
        return *this;
    }

    virtual ~OutMessage() { }
private:
    OutMessageBuffer m_data;
};

struct TimerOptions {
    bool enabled;
    int pool;
    int periods;
    int periodLength;
    TimerOptions() : enabled(false) { }
};

}} // namespace shoddybattle::network

#endif
