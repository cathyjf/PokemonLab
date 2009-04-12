/* 
 * File:   network.cpp
 * Author: Catherine
 *
 * Created on April 9, 2009, 4:58 PM
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

/**
 * In Shoddy Battle 2, a "message" consists of a five byte header followed by
 * a message body. The first byte identifies the type of message. The next
 * four bytes are an int in network byte order specifying the length of the
 * message body.
 */

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <vector>
#include <deque>
#include <set>
#include "network.h"
#include "../database/DatabaseRegistry.h"

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

namespace shoddybattle { namespace network {

class ClientImpl;
class ServerImpl;
typedef shared_ptr<ClientImpl> ClientImplPtr;
typedef shared_ptr<ServerImpl> ServerImplPtr;

typedef set<ClientImplPtr> CLIENT_LIST;

void OutMessage::finalise() {
    // insert the size into the data
    *reinterpret_cast<int32_t *>(&m_data[1]) =
            htonl(m_data.size() - HEADER_SIZE);
}

const std::vector<unsigned char> &OutMessage::operator()() const {
    return m_data;
}

OutMessage &OutMessage::operator<<(const int32_t i) {
    const int pos = m_data.size();
    m_data.resize(pos + sizeof(int32_t), 0);
    unsigned char *p = &m_data[pos];
    *reinterpret_cast<int32_t *>(p) = htonl(i);
    return *this;
}

OutMessage &OutMessage::operator<<(const unsigned char byte) {
    m_data.push_back(byte);
}

/**
 * Write a string in a format similar to the UTF-8 format used by the
 * Java DataInputStream.
 *
 * The first two bytes are an network byte order unsigned short specifying
 * the number of additional bytes to be written.
 */
OutMessage &OutMessage::operator<<(const std::string &str) {
    const int pos = m_data.size();
    const int length = str.length();
    m_data.resize(pos + length + sizeof(uint16_t), 0);
    uint16_t l = htons(length);
    unsigned char *p = &m_data[pos];
    *reinterpret_cast<uint16_t *>(p) = l;
    memcpy(p + sizeof(uint16_t), str.c_str(), length);
    return *this;
}

/**
 * A message that the client sends to the server.
 */
class InMessage {
public:
    class InvalidMessage {
        
    };

    enum TYPE {
        REQUEST_CHALLENGE = 0,
        CHALLENGE_RESPONSE = 1,
    };

    InMessage() {
        reset();
    }

    TYPE getType() const {
        return m_type;
    }

    void reset() {
        m_data.resize(HEADER_SIZE, 0);
        m_pos = 0;
    }

    void processHeader() {
        m_type = (TYPE)m_data[0];
        ++m_pos;
        int32_t size;
        *this >> size;
        m_data.resize(size);
        m_pos = 0;
    }

    vector<unsigned char> &operator()() {
        return m_data;
    }

    InMessage &operator>>(int32_t &i) {
        if (!hasMoreData(sizeof(int32_t)))
            throw InvalidMessage();
        int32_t *p = reinterpret_cast<int32_t *>(&m_data[m_pos]);
        i = ntohl(*p);
        m_pos += sizeof(int32_t);
        return *this;
    }

    InMessage &operator>>(unsigned char &c) {
        if (!hasMoreData(sizeof(unsigned char)))
            throw InvalidMessage();
        c = *(unsigned char *)(&m_data[m_pos]);
        m_pos += sizeof(unsigned char);
        return *this;
    }
    
    InMessage &operator>>(string &str) {
        if (!hasMoreData(sizeof(uint16_t)))
            throw InvalidMessage();

        uint16_t *p = reinterpret_cast<uint16_t *>(&m_data[m_pos]);
        uint16_t length = ntohs(*p);

        if (!hasMoreData(sizeof(uint16_t) + length))
            throw InvalidMessage();

        str = string((char *)&m_data[m_pos + sizeof(uint16_t)], length);
        m_pos += length + sizeof(int16_t);
        return *this;
    }

    virtual ~InMessage() { }

private:
    bool hasMoreData(const int count) const {
        const int length = m_data.size();
        return (length - m_pos) >= count;
    }

    vector<unsigned char> m_data;
    TYPE m_type;
    int m_pos;
};

class WelcomeMessage : public OutMessage {
public:
    WelcomeMessage(const int version, const string name, const string message):
            OutMessage(WELCOME_MESSAGE) {
        *this << version;
        *this << name;
        *this << message;
        finalise();
    }
};

class ChallengeMessage : public OutMessage {
public:
    ChallengeMessage(const unsigned char *challenge):
            OutMessage(PASSWORD_CHALLENGE, 16) {
        for (int i = 0; i < 16; ++i) {
            *this << challenge[i];
        }
    }
};

class ServerImpl {
public:
    ServerImpl(tcp::endpoint &endpoint);
    void run();
    void removeClient(ClientImplPtr client);
    void broadcast(OutMessage &msg);
    database::DatabaseRegistry *getRegistry() { return &m_registry; }
    
private:
    void acceptClient();
    void handleAccept(ClientImplPtr client,
            const boost::system::error_code &error);

    CLIENT_LIST m_clients;
    shared_mutex m_clientMutex;
    io_service m_service;
    tcp::acceptor m_acceptor;
    database::DatabaseRegistry m_registry;
};

class Client {
public:
    virtual void sendMessage(const OutMessage &msg) = 0;
    virtual std::string getName() const = 0;
    virtual std::string getIp() const = 0;

protected:
    Client() { }
    virtual ~Client() { }
};

class ClientImpl : public Client, public enable_shared_from_this<ClientImpl> {
public:
    ClientImpl(io_service &service, ServerImpl *server):
            m_server(server),
            m_service(service),
            m_socket(service),
            m_authenticated(false),
            m_challenge(0) { }

    tcp::socket &getSocket() {
        return m_socket;
    }
    void sendMessage(const OutMessage &msg) {
        lock_guard<mutex> lock(m_queueMutex);
        const bool empty = m_queue.empty();
        m_queue.push_back(msg);
        const OutMessage &post = m_queue.back();
        if (empty) {
            async_write(m_socket, buffer(post()),
                    boost::bind(&ClientImpl::handleWrite,
                    shared_from_this(), placeholders::error));
        }
    }
    void start() {
        m_ip = m_socket.remote_endpoint().address().to_string();

        async_read(m_socket, buffer(m_msg()),
                boost::bind(&ClientImpl::handleReadHeader,
                shared_from_this(), placeholders::error));
    }
    string getIp() const {
        return m_ip;
    }
    string getName() const {
        return m_name;
    }
private:

    /**
     * Handle reading in the header of a message.
     */
    void handleReadHeader(const boost::system::error_code &error) {
        if (error) {
            handleError(error);
            return;
        }

        m_msg.processHeader();
        async_read(m_socket, buffer(m_msg()),
                boost::bind(&ClientImpl::handleReadBody,
                shared_from_this(), placeholders::error));
    }

    void handleReadBody(const boost::system::error_code &error);

    /**
     * Handle the completion of writing a message.
     */
    void handleWrite(const boost::system::error_code &error) {
        if (error) {
            handleError(error);
            return;
        }

        lock_guard<mutex> lock(m_queueMutex);
        m_queue.pop_front();
        if (!m_queue.empty()) {
            async_write(m_socket, buffer(m_queue.front()()),
                    boost::bind(&ClientImpl::handleWrite,
                    shared_from_this(), placeholders::error));
        }
    }

    void handleError(const boost::system::error_code &error) {
        m_server->removeClient(shared_from_this());
    }

    /**
     * string : user name
     */
    void handleRequestChallenge(InMessage &msg) {
        string user;
        msg >> user;
        unsigned char data[16];
        m_challenge = m_server->getRegistry()->getAuthChallenge(user, data);
        if (m_challenge != 0) {
            // user exists
            m_name = user;
            ChallengeMessage msg(data);
            sendMessage(msg);
        } else {
            // TODO: issue an error message
        }
    }

    /**
     * byte[16] : challenge response
     */
    void handleChallengeResponse(InMessage &msg) {
        if (m_challenge == 0) {
            // no challenge in progress
            return;
        }

        unsigned char data[16];
        for (int i = 0; i < 16; ++i) {
            msg >> data[i];
        }
        database::DatabaseRegistry *registry = m_server->getRegistry();
        const bool match = registry->isResponseValid(m_name, m_challenge, data);
        cout << match << endl;

        // TODO: inform whether it was a match
    }

    string m_name;
    bool m_authenticated;
    int m_challenge; // for challenge-response authentication

    InMessage m_msg;
    deque<OutMessage> m_queue;
    mutex m_queueMutex;
    io_service &m_service;
    tcp::socket m_socket;
    string m_ip;
    ServerImpl *m_server;

    typedef void (ClientImpl::*MESSAGE_HANDLER)(InMessage &msg);
    static const MESSAGE_HANDLER m_handlers[];
};

const ClientImpl::MESSAGE_HANDLER ClientImpl::m_handlers[] = {
    &ClientImpl::handleRequestChallenge,
    &ClientImpl::handleChallengeResponse
};

/**
 * Handle reading in the body of a message.
 */
void ClientImpl::handleReadBody(const boost::system::error_code &error) {
    if (error) {
        handleError(error);
        return;
    }

    const int count = sizeof(m_handlers) / sizeof(m_handlers[0]);
    const int type = (int)m_msg.getType();
    if (type < count) {
        try {
            // Call the handler for this type of message.
            (this->*m_handlers[type])(m_msg);
        } catch (InMessage::InvalidMessage) {
            // The client sent an invalid message.
            // Disconnect him immediately.
            m_server->removeClient(shared_from_this());
            return;
        }
    }

    // Read another message.
    m_msg.reset();
    async_read(m_socket, buffer(m_msg()),
            boost::bind(&ClientImpl::handleReadHeader,
            shared_from_this(), placeholders::error));
}

ServerImpl::ServerImpl(tcp::endpoint &endpoint):
        m_acceptor(m_service, endpoint) {
    acceptClient();
}

/** Start the server. */
void ServerImpl::run() {
    m_service.run();
}

/**
 * Remove a client.
 */
void ServerImpl::removeClient(ClientImplPtr client) {
    // TODO: other removal logic

    lock_guard<shared_mutex> lock(m_clientMutex);
    m_clients.erase(client);
}

/**
 * Send a message to all clients.
 */
void ServerImpl::broadcast(OutMessage &msg) {
    shared_lock<shared_mutex> lock(m_clientMutex);
    for_each(m_clients.begin(), m_clients.end(),
            boost::bind(&ClientImpl::sendMessage, _1, boost::ref(msg)));
}

void ServerImpl::acceptClient() {
    ClientImplPtr client(new ClientImpl(m_service, this));
    m_acceptor.async_accept(client->getSocket(),
            boost::bind(&ServerImpl::handleAccept, this,
            client, placeholders::error));
}

void ServerImpl::handleAccept(ClientImplPtr client,
        const boost::system::error_code &error) {
    acceptClient();
    if (!error) {
        {
            lock_guard<shared_mutex> lock(m_clientMutex);
            m_clients.insert(client);
        }
        client->start();
        cout << "Accepted client from " << client->getIp() << "." << endl;
        WelcomeMessage msg(2,
                "Official Server",
                "Welcome to Shoddy Battle 2!");
        client->sendMessage(msg);
    }
}


}} // namespace shoddybattle::network

#include "../database/DatabaseRegistry.h"

int main() {
    using namespace shoddybattle;

    tcp::endpoint endpoint(tcp::v4(), 8446);
    network::ServerImpl server(endpoint);

    database::DatabaseRegistry *registry = server.getRegistry();
    registry->connect("shoddybattle2", "localhost", "Catherine", "");
    registry->startThread();
    //registry.registerUser("Catherine", "test", "127.0.0.1");
    //unsigned char challenge[16];
    //int ch = registry.getAuthChallenge("Catherine", challenge);
    //cout << registry.isResponseValid("Catherine", ch, challenge) << endl;

    server.run();
}
