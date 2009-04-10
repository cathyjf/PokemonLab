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
 * a message body.  The first byte identifies the type of message. The next
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

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

namespace shoddybattle {

const int HEADER_SIZE = sizeof(char) + sizeof(int32_t);

class ClientImpl;
class ServerImpl;
typedef shared_ptr<ClientImpl> ClientImplPtr;
typedef shared_ptr<ServerImpl> ServerImplPtr;

typedef set<ClientImplPtr> CLIENT_LIST;

/**
 * A message that the client sends to the server.
 */
class InMessage {
public:
    InMessage() {
        reset();
    }

    char getType() const {
        return m_type;
    }

    void reset() {
        m_data.resize(HEADER_SIZE, 0);
        m_pos = 0;
    }

    void processHeader() {
        m_type = m_data[0];
        ++m_pos;
        int32_t size;
        *this >> size;
        m_data.resize(size);
        m_pos = 0;
    }

    mutable_buffers_1 operator()() {
        return buffer(m_data);
    }

    InMessage &operator>>(int32_t &i) {
        int32_t *p = reinterpret_cast<int32_t *>(&m_data[m_pos]);
        i = ntohl(*p);
        m_pos += sizeof(int32_t);
        return *this;
    }

    InMessage &operator>>(string &str) {
        int16_t *p = reinterpret_cast<int16_t *>(&m_data[m_pos]);
        int16_t length = ntohs(*p);
        str = string(&m_data[m_pos + sizeof(int16_t)], length);
        m_pos += length + sizeof(int16_t);
        return *this;
    }

private:
    vector<char> m_data;
    char m_type;
    int m_pos;
};

/**
 * A message that the server sends to a client.
 */
class OutMessage {
public:
    OutMessage(const char type = 0) {
        m_data.push_back(type);
        m_data.resize(HEADER_SIZE, 0);    // insert in 0 size for now
    }

    void finalise() {
        // insert the size into the data
        *reinterpret_cast<int32_t *>(&m_data[1]) =
                htonl(m_data.size() - HEADER_SIZE);
    }
    
    const_buffers_1 operator()() const {
        return buffer(m_data);
    }

    OutMessage &operator<<(const int32_t i) {
        const int pos = m_data.size();
        m_data.resize(pos + sizeof(int32_t), 0);
        char *p = &m_data[pos];
        *reinterpret_cast<int32_t *>(p) = htonl(i);
        return *this;
    }

    OutMessage &operator<<(const std::string &str) {
        const int pos = m_data.size();
        const int length = str.length();
        m_data.resize(pos + length + sizeof(int16_t), 0);
        int16_t l = htons(length);
        char *p = &m_data[pos];
        *reinterpret_cast<int16_t *>(p) = l;
        memcpy(p + sizeof(int16_t), str.c_str(), length);
        return *this;
    }

    virtual ~OutMessage() { }
private:
    vector<char> m_data;
};

class ServerImpl {
public:
    ServerImpl(tcp::endpoint &endpoint);
    void run();
    void removeClient(ClientImplPtr client);
    void broadcast(OutMessage &msg);

private:
    void acceptClient();
    void handleAccept(ClientImplPtr client,
            const boost::system::error_code &error);

    CLIENT_LIST m_clients;
    shared_mutex m_clientMutex;
    io_service m_service;
    tcp::acceptor m_acceptor;
};

class ClientImpl : public enable_shared_from_this<ClientImpl> {
public:
    ClientImpl(io_service &service, ServerImpl *server):
            m_server(server),
            m_service(service),
            m_socket(service) { }

    tcp::socket &getSocket() {
        return m_socket;
    }
    void sendMessage(OutMessage &msg) {
        lock_guard<mutex> lock(m_queueMutex);
        const bool empty = m_queue.empty();
        m_queue.push_back(msg);
        if (empty) {
            async_write(m_socket, msg(), boost::bind(&ClientImpl::handleWrite,
                    shared_from_this(), placeholders::error));
        }
    }
    void start() {
        m_ip = m_socket.remote_endpoint().address().to_string();

        async_read(m_socket, m_msg(),
                boost::bind(&ClientImpl::handleReadHeader,
                shared_from_this(), placeholders::error));
    }
    string getIp() const {
        return m_ip;
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
        async_read(m_socket, m_msg(),
                boost::bind(&ClientImpl::handleReadBody,
                shared_from_this(), placeholders::error));
    }

    /**
     * Handle reading in the body of a message.
     */
    void handleReadBody(const boost::system::error_code &error) {
        if (error) {
            handleError(error);
            return;
        }

        // TODO: process the message here

        // Read another message.
        m_msg.reset();
        async_read(m_socket, m_msg(),
                boost::bind(&ClientImpl::handleReadHeader,
                shared_from_this(), placeholders::error));
    }

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
            async_write(m_socket, m_queue.front()(),
                    boost::bind(&ClientImpl::handleWrite,
                    shared_from_this(), placeholders::error));
        }
    }

    void handleError(const boost::system::error_code &error) {
        m_server->removeClient(shared_from_this());
    }

    InMessage m_msg;
    deque<OutMessage> m_queue;
    mutex m_queueMutex;
    io_service &m_service;
    tcp::socket m_socket;
    string m_ip;
    ServerImpl *m_server;
};


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
        OutMessage msg;
        msg << "This is a test!" << 4 << "!";
        msg.finalise();
        client->sendMessage(msg);
    }
}


}

int main() {
    using namespace shoddybattle;
    tcp::endpoint endpoint(tcp::v4(), 8446);
    ServerImpl server(endpoint);
    server.run();
}
