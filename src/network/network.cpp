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

#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost-1_38/boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <vector>
#include <deque>
#include <set>
#include <bitset>
#include <map>
#include "network.h"
#include "../database/DatabaseRegistry.h"
#include "../text/Text.h"

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

namespace shoddybattle { namespace network {

class ClientImpl;
class ServerImpl;
class Channel;
typedef shared_ptr<ClientImpl> ClientImplPtr;
typedef shared_ptr<ServerImpl> ServerImplPtr;
typedef shared_ptr<Channel> ChannelPtr;

typedef set<ClientImplPtr> CLIENT_LIST;
typedef set<ChannelPtr> CHANNEL_LIST;

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
        REGISTER_ACCOUNT = 2,
        JOIN_CHANNEL = 3,
        CHANNEL_MESSAGE = 4,
        CHANNEL_MODE = 5
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
        ensureMoreData(sizeof(int32_t));

        int32_t *p = reinterpret_cast<int32_t *>(&m_data[m_pos]);
        i = ntohl(*p);
        m_pos += sizeof(int32_t);
        return *this;
    }

    InMessage &operator>>(unsigned char &c) {
        ensureMoreData(sizeof(unsigned char));
        
        c = *(unsigned char *)(&m_data[m_pos]);
        m_pos += sizeof(unsigned char);
        return *this;
    }
    
    InMessage &operator>>(string &str) {
        ensureMoreData(sizeof(uint16_t));

        uint16_t *p = reinterpret_cast<uint16_t *>(&m_data[m_pos]);
        uint16_t length = ntohs(*p);

        ensureMoreData(sizeof(uint16_t) + length);

        str = string((char *)&m_data[m_pos + sizeof(uint16_t)], length);
        m_pos += length + sizeof(int16_t);
        return *this;
    }

    virtual ~InMessage() { }

private:
    void ensureMoreData(const int count) const {
        const int length = m_data.size();
        if ((length - m_pos) < count)
            throw InvalidMessage();
    }

    vector<unsigned char> m_data;
    TYPE m_type;
    int m_pos;
};

class WelcomeMessage : public OutMessage {
public:
    WelcomeMessage(const int version, const string name, const string &message):
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

/**
 * byte : type
 * string : details
 */
class RegistryResponse : public OutMessage {
public:
    enum TYPE {
        NAME_UNAVAILABLE = 0,
        REGISTER_SUCCESS = 1,
        ILLEGAL_NAME = 2,
        TOO_LONG_NAME = 3,
        NONEXISTENT_NAME = 4,
        INVALID_RESPONSE = 5,
        USER_BANNED = 6,
        SUCCESSFUL_LOGIN = 7
    };
    RegistryResponse(const TYPE type, const string &details = string()):
            OutMessage(REGISTRY_RESPONSE) {
        *this << ((unsigned char)type);
        *this << details;
        finalise();
    }
};

class ChannelJoinPart : public OutMessage {
public:
    ChannelJoinPart(ChannelPtr, ClientImplPtr, bool);
};

class ServerImpl {
public:
    ServerImpl(tcp::endpoint &endpoint);
    void run();
    void removeClient(ClientImplPtr client);
    void broadcast(OutMessage &msg);
    void initialiseChannels();
    database::DatabaseRegistry *getRegistry() { return &m_registry; }
    ChannelPtr getMainChannel() const { return m_mainChannel; }
    void sendChannelList(ClientImplPtr client);
    ChannelPtr getChannel(const string &);
    
private:
    void acceptClient();
    void handleAccept(ClientImplPtr client,
            const boost::system::error_code &error);

    class ChannelList : public OutMessage {
    public:
        ChannelList(ServerImpl *);
    };

    CHANNEL_LIST m_channels;
    shared_mutex m_channelMutex;
    ChannelPtr m_mainChannel;
    CLIENT_LIST m_clients;
    shared_mutex m_clientMutex;
    io_service m_service;
    tcp::acceptor m_acceptor;
    database::DatabaseRegistry m_registry;
};

Server::Server(const int port) {
    tcp::endpoint endpoint(tcp::v4(), 8446);
    m_impl = new ServerImpl(endpoint);
}

void Server::run() {
    m_impl->run();
}

database::DatabaseRegistry *Server::getRegistry() {
    return m_impl->getRegistry();
}

Server::~Server() {
    delete m_impl;
}

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
    int getId() const {
        return m_id;
    }
    void disconnect();
private:

    void joinChannel(ChannelPtr channel);

    void partChannel(ChannelPtr channel);

    ChannelPtr getChannel(const int id);

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
            sendMessage(RegistryResponse(RegistryResponse::NONEXISTENT_NAME));
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
        database::DatabaseRegistry::AUTH_PAIR auth =
                registry->isResponseValid(m_name, m_challenge, data);
        m_challenge = 0;

        if (!auth.first) {
            sendMessage(RegistryResponse(RegistryResponse::INVALID_RESPONSE));
            return;
        }
        
        sendMessage(RegistryResponse(RegistryResponse::SUCCESSFUL_LOGIN));

        m_authenticated = true;
        m_id = auth.second;
        m_server->sendChannelList(shared_from_this());
        // todo: send list of battles...
    }

    /**
     * string : user name
     * string : password (plaintext)
     */
    void handleRegisterAccount(InMessage &msg) {
        string user, password;
        msg >> user >> password;

        user = trim(user);
        // todo: collapse whitespace everywhere within the name

        if (user.length() > 18) {
            sendMessage(RegistryResponse(RegistryResponse::TOO_LONG_NAME));
            return;
        }

        boost::regex e("[A-Za-z0-9_ \\-\\.]+");
        if (!regex_match(user, e)) {
            sendMessage(RegistryResponse(RegistryResponse::ILLEGAL_NAME));
            return;
        }

        database::DatabaseRegistry *registry = m_server->getRegistry();
        if (!registry->registerUser(user, password, m_ip)) {
            sendMessage(RegistryResponse(RegistryResponse::NAME_UNAVAILABLE));
            return;
        }

        sendMessage(RegistryResponse(RegistryResponse::REGISTER_SUCCESS));
    }

    /**
     * string : channel name
     */
    void handleJoinChannel(InMessage &msg) {
        string channel;
        msg >> channel;
        ChannelPtr p = m_server->getChannel(channel);
        if (p) {
            joinChannel(p);
        }
    }

    /**
     * int32 : channel id
     * string : message
     */
    void handleChannelMessage(InMessage &msg);

    /**
     * int32 : channel id
     * string : user
     * byte : the mode to change
     * byte : whether to enable or disable it (1 or 0)
     */
    void handleModeMessage(InMessage &msg);

    string m_name;
    int m_id;   // user id
    bool m_authenticated;
    int m_challenge; // for challenge-response authentication

    InMessage m_msg;
    deque<OutMessage> m_queue;
    mutex m_queueMutex;
    io_service &m_service;
    tcp::socket m_socket;
    string m_ip;
    ServerImpl *m_server;
    CHANNEL_LIST m_channels;    // channels this user is in
    shared_mutex m_channelMutex;

    typedef void (ClientImpl::*MESSAGE_HANDLER)(InMessage &msg);
    static const MESSAGE_HANDLER m_handlers[];
};

const ClientImpl::MESSAGE_HANDLER ClientImpl::m_handlers[] = {
    &ClientImpl::handleRequestChallenge,
    &ClientImpl::handleChallengeResponse,
    &ClientImpl::handleRegisterAccount,
    &ClientImpl::handleJoinChannel,
    &ClientImpl::handleChannelMessage,
    &ClientImpl::handleModeMessage,
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
    if ((type > 2) && !m_authenticated) {
        m_server->removeClient(shared_from_this());
        return;
    }
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

/**
 * A "channel" encapsulates the notion of chatting in a particular place,
 * rather like an IRC channel. The list of battles, however, is global to
 * the server, not channel-specific.
 */
class Channel : public enable_shared_from_this<Channel> {
public:

    class Mode {
    public:
        enum MODE {
            Q,
            A,
            O,
            H,
            V,
            B,
            M,
            I,
            IDLE
        };
    };

    enum STATUS_FLAGS {
        OWNER,      // +q
        PROTECTED,  // +a
        OP,         // +o
        HALF_OP,    // +h
        VOICE,      // +v
        IDLE,       // inactive
        BUSY        // ("ignoring challenges")
    };
    static const int FLAG_COUNT = 7;
    typedef bitset<FLAG_COUNT> FLAGS;

    enum AUTO_FLAGS {
        SOP,
        AOP,
        HOP,
        VOP,
        AKICK
    };
    static const int AUTO_FLAG_COUNT = 5;
    typedef bitset<AUTO_FLAG_COUNT> AUTO_FLAGS;

    enum CHANNEL_FLAGS {
        MODERATED   // +m
    };
    static const int CHANNEL_FLAG_COUNT = 1;
    typedef bitset<CHANNEL_FLAG_COUNT> CHANNEL_FLAGS;

    typedef pair<FLAGS, AUTO_FLAGS> FLAG_PAIR;
    typedef map<ClientImplPtr, FLAG_PAIR> CLIENT_MAP;

    Channel(ServerImpl *server,
            const int id,
            database::DatabaseRegistry::INFO_ELEMENT &element) {
        m_server = server;
        m_id = id;
        m_name = element.get<0>();
        m_topic = element.get<1>();
        m_flags = element.get<2>();
    }

    FLAGS getStatusFlags(ClientImplPtr client) {
        shared_lock<shared_mutex> lock(m_mutex);
        CLIENT_MAP::iterator i = m_clients.find(client);
        if (i == m_clients.end()) {
            return FLAGS();
        }
        return i->second.first;
    }

    void setStatusFlags(ClientImplPtr client, FLAGS flags) {
        {
            shared_lock<shared_mutex> lock(m_mutex);
            CLIENT_MAP::iterator i = m_clients.find(client);
            if (i == m_clients.end()) {
                return;
            }
            i->second.first = flags;
        }
        broadcast(ChannelStatus(shared_from_this(), client, flags.to_ulong()));
    }

    CLIENT_MAP::value_type getClient(const string &name) {
        shared_lock<shared_mutex> lock(m_mutex);
        CLIENT_MAP::iterator i = m_clients.begin();
        for (; i != m_clients.end(); ++i) {
            // todo: maybe make this lowercase?
            if (name == (*i).first->getName()) {
                return *i;
            }
        }
        return CLIENT_MAP::value_type();
    }

    string getName() {
        shared_lock<shared_mutex> lock(m_mutex);
        return m_name;
    }

    string getTopic() {
        shared_lock<shared_mutex> lock(m_mutex);
        return m_topic;
    }

    int32_t getId() const {
        // assume that pointers are 32-bit
        // todo: use something less hacky than this for ids
        return (int32_t)this;
    }

    bool join(ClientImplPtr client) {
        shared_lock<shared_mutex> lock(m_mutex, boost::defer_lock_t());
        lock.lock();
        if (m_clients.find(client) != m_clients.end()) {
            // already in channel
            return false;
        }
        // look up the client's auto flags on this channel
        AUTO_FLAGS flags = m_server->getRegistry()->getUserFlags(m_id,
                client->getId());
        if (flags[AKICK]) {
            // user is banned from this channel
            return false;
        }
        FLAGS user = 0;
        if (flags[SOP]) {
            user[OWNER] = true;
        }
        if (flags[AOP]) {
            user[OP] = true;
        }
        if (flags[HOP]) {
            user[HALF_OP] = true;
        }
        if (flags[VOP]) {
            user[VOICE] = true;
        }
        // tell the client all about the channel
        client->sendMessage(ChannelInfo(shared_from_this()));
        // add the client to the channel
        m_clients.insert(CLIENT_MAP::value_type(client,
                FLAG_PAIR(user, flags)));
        // inform the channel
        lock.unlock();
        broadcast(ChannelJoinPart(shared_from_this(), client, true));
        broadcast(ChannelStatus(shared_from_this(), client, user.to_ulong()));
        return true;
    }

    void part(ClientImplPtr client) {
        shared_lock<shared_mutex> lock(m_mutex, boost::defer_lock_t());
        lock.lock();
        if (m_clients.find(client) == m_clients.end())
            return;
        m_clients.erase(client);
        lock.unlock();
        broadcast(ChannelJoinPart(shared_from_this(), client, false));
    }

    void sendMessage(const string &message, ClientImplPtr client) {
        broadcast(ChannelMessage(shared_from_this(), client, message));
    }

    int getPopulation() {
        shared_lock<shared_mutex> lock(m_mutex);
        return m_clients.size();
    }

private:
    class ChannelInfo : public OutMessage {
    public:
        ChannelInfo(ChannelPtr channel): OutMessage(CHANNEL_INFO) {
            // channel id
            *this << channel->getId();
            
            // channel name, topic, and flags
            *this << channel->m_name;
            *this << channel->m_topic;
            *this << (int)channel->m_flags.to_ulong();

            // list of clients
            CLIENT_MAP &clients = channel->m_clients;
            *this << (int)clients.size();
            CLIENT_MAP::iterator i = clients.begin();
            for (; i != clients.end(); ++i) {
                *this << i->first->getName();
                *this << (int)i->second.first.to_ulong();
            }
            
            finalise();
        }
    };

    class ChannelStatus : public OutMessage {
    public:
        ChannelStatus(ChannelPtr channel, ClientImplPtr client, int flags):
                OutMessage(CHANNEL_STATUS) {
            *this << channel->getId();
            *this << client->getName();
            *this << flags;
            finalise();
        }
    };

    class ChannelMessage : public OutMessage {
    public:
        ChannelMessage(ChannelPtr channel, ClientImplPtr client,
                    const string &msg):
                OutMessage(CHANNEL_MESSAGE) {
            *this << channel->getId();
            *this << client->getName();
            *this << msg;
            finalise();
        }
    };

    void broadcast(const OutMessage &msg) {
        shared_lock<shared_mutex> lock(m_mutex);
        CLIENT_MAP::const_iterator i = m_clients.begin();
        for (; i != m_clients.end(); ++i) {
            i->first->sendMessage(msg);
        }
    }

    ServerImpl *m_server;
    int m_id;   // channel id
    CLIENT_MAP m_clients;
    string m_name;  // name of this channel (e.g. #main)
    string m_topic; // channel topic
    CHANNEL_FLAGS m_flags;
    shared_mutex m_mutex;
};

void ClientImpl::joinChannel(ChannelPtr channel) {
    if (channel->join(shared_from_this())) {
        lock_guard<shared_mutex> lock(m_channelMutex);
        m_channels.insert(channel);
    }
}

void ClientImpl::partChannel(ChannelPtr channel) {
    lock_guard<shared_mutex> lock(m_channelMutex);
    channel->part(shared_from_this());
    m_channels.erase(channel);
}

ChannelPtr ClientImpl::getChannel(const int id) {
    shared_lock<shared_mutex> lock(m_channelMutex);
    CHANNEL_LIST::iterator i = m_channels.begin();
    for (; i != m_channels.end(); ++i) {
        if ((*i)->getId() == id)
            return *i;
    }
    return ChannelPtr();
}

/**
 * int32 : channel id
 * string : user
 * byte : the mode to change
 * byte : whether to enable or disable it (1 or 0)
 */
void ClientImpl::handleModeMessage(InMessage &msg) {
    int id;
    string user;
    unsigned char mode;
    unsigned char enabled;
    msg >> id >> user >> mode >> enabled;
    ChannelPtr p = getChannel(id);
    if (!p) {
        return;
    }
    Channel::CLIENT_MAP::value_type target = p->getClient(user);
    if (!target.first) {
        // todo: maybe issue an error message
        return;
    }
    Channel::FLAGS auth = p->getStatusFlags(shared_from_this());
    Channel::FLAGS flags = target.second.first;
    switch (mode) {
        case Channel::Mode::Q: {
            if (auth[Channel::OWNER]) {
                flags[Channel::OWNER] = enabled;
            }
        } break;
        case Channel::Mode::A: {
            if (auth[Channel::OWNER]) {
                flags[Channel::PROTECTED] = enabled;
            }
        } break;
        case Channel::Mode::O: {
            if (auth[Channel::OWNER] || auth[Channel::OP]) {
                if (!enabled && flags[Channel::PROTECTED]
                        && !auth[Channel::OWNER]) {
                    // todo: error message
                    break;
                }
                flags[Channel::OP] = enabled;
            }
        } break;
        case Channel::Mode::H: {
            if (auth[Channel::OWNER] || auth[Channel::OP]) {
                flags[Channel::HALF_OP] = enabled;
            }
        } break;
        case Channel::Mode::V: {
            if (auth[Channel::OWNER]
                    || auth[Channel::OP]
                    || auth[Channel::HALF_OP]) {
                flags[Channel::VOICE] = enabled;
            }
        } break;
        // TODO: the other modes
    }

    if (flags != target.second.first) {
        p->setStatusFlags(target.first, flags);
    }
}

/**
 * int32 : channel id
 * string : message
 */
void ClientImpl::handleChannelMessage(InMessage &msg) {
    int id;
    string message;
    msg >> id >> message;
    ChannelPtr p = getChannel(id);
    if (p) {
        p->sendMessage(message, shared_from_this());
    }
}

ChannelJoinPart::ChannelJoinPart(ChannelPtr channel,
        ClientImplPtr client, bool join):
        OutMessage(CHANNEL_JOIN_PART) {
    *this << channel->getId();
    *this << client->getName();
    *this << ((unsigned char)join);
    finalise();
}

void ClientImpl::disconnect() {
    {
        lock_guard<shared_mutex> lock(m_channelMutex);
        CHANNEL_LIST::iterator i = m_channels.begin();
        for (; i != m_channels.end(); ++i) {
            (*i)->part(shared_from_this());
        }
        m_channels.clear();
    }
    // todo: forfeit all battles
}

void ServerImpl::sendChannelList(ClientImplPtr client) {
    client->sendMessage(ChannelList(this));
}

ServerImpl::ChannelList::ChannelList(ServerImpl *server):
        OutMessage(CHANNEL_LIST) {
    shared_lock<shared_mutex> lock(server->m_channelMutex);
    *this << (int)server->m_channels.size();
    CHANNEL_LIST::iterator i = server->m_channels.begin();
    for (; i != server->m_channels.end(); ++i) {
        ChannelPtr p = *i;
        *this << p->getName();
        *this << p->getTopic();
        *this << p->getPopulation();
    }
    finalise();
}

ServerImpl::ServerImpl(tcp::endpoint &endpoint):
        m_acceptor(m_service, endpoint) {
    acceptClient();
}

ChannelPtr ServerImpl::getChannel(const string &name) {
    shared_lock<shared_mutex> lock(m_channelMutex);
    CHANNEL_LIST::iterator i = m_channels.begin();
    for (; i != m_channels.end(); ++i) {
        if ((*i)->getName() == name)
            return *i;
    }
    return ChannelPtr();
}

/**
 * Initialise channels from the database.
 */
void ServerImpl::initialiseChannels() {
    using database::DatabaseRegistry;

    DatabaseRegistry::CHANNEL_INFO info;
    m_registry.getChannelInfo(info);
    DatabaseRegistry::CHANNEL_INFO::iterator i = info.begin();
    for (; i != info.end(); ++i) {
        ChannelPtr p = ChannelPtr(new Channel(this, i->first, i->second));
        if (p->getName() == "main") {
            m_mainChannel = p;
        }
        m_channels.insert(p);
    }

    assert(m_mainChannel);
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
    client->disconnect();
    cout << "Client from " << client->getIp() << " disconnected." << endl;

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

#if 1

#include "../database/DatabaseRegistry.h"

int main() {
    using namespace shoddybattle;

    tcp::endpoint endpoint(tcp::v4(), 8446);
    network::ServerImpl server(endpoint);

    database::DatabaseRegistry *registry = server.getRegistry();
    registry->connect("shoddybattle2", "localhost", "Catherine", "");
    registry->startThread();
    server.initialiseChannels();

    server.run();
}

#endif
