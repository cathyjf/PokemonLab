/* 
 * File:   Channel.cpp
 * Author: Catherine
 *
 * Created on May 3, 2009, 2:41 AM
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

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/algorithm/string.hpp>
#include "Channel.h"
#include "network.h"
#include "../database/DatabaseRegistry.h"

using namespace std;
using namespace boost;

namespace shoddybattle { namespace network {

struct Channel::ChannelImpl {
    Server *server;
    int id;       // channel id
    CLIENT_MAP clients;
    string name;  // name of this channel (e.g. #main)
    string topic; // channel topic
    CHANNEL_FLAGS flags;
    shared_mutex mutex;
};

class Channel::ChannelInfo : public OutMessage {
public:
    ChannelInfo(ChannelPtr channel): OutMessage(CHANNEL_INFO) {
        // channel id
        *this << channel->getId();

        // channel type
        *this << (unsigned char)channel->getChannelType();

        // channel name, topic, and flags
        *this << channel->m_impl->name;
        *this << channel->m_impl->topic;
        *this << (int)channel->m_impl->flags.to_ulong();

        // list of clients
        CLIENT_MAP &clients = channel->m_impl->clients;
        *this << (int)clients.size();
        CLIENT_MAP::iterator i = clients.begin();
        for (; i != clients.end(); ++i) {
            *this << i->first->getName();
            *this << (int)i->second.to_ulong();
        }

        finalise();
    }
};

class Channel::ChannelStatus : public OutMessage {
public:
    ChannelStatus(ChannelPtr channel, const string &user,
            ClientPtr client, int flags):
            OutMessage(CHANNEL_STATUS) {
        *this << channel->getId();
        *this << user;
        *this << client->getName();
        *this << flags;
        finalise();
    }
};

class Channel::ChannelMessage : public OutMessage {
public:
    ChannelMessage(ChannelPtr channel, ClientPtr client,
                const string &msg):
            OutMessage(CHANNEL_MESSAGE) {
        *this << channel->getId();
        *this << client->getName();
        *this << msg;
        finalise();
    }
};

class Channel::ChannelJoinPart : public OutMessage {
public:
    ChannelJoinPart(ChannelPtr channel,
            ClientPtr client, bool join):
            OutMessage(CHANNEL_JOIN_PART) {
        *this << channel->getId();
        *this << client->getName();
        *this << ((unsigned char)join);
        finalise();
    }
};

Channel::Channel(Server *server,
        const std::string &name,
        const std::string &topic,
        Channel::CHANNEL_FLAGS flags,
        const int id) {
    m_impl = shared_ptr<ChannelImpl>(new ChannelImpl());
    m_impl->server = server;
    m_impl->name = name;
    m_impl->topic = topic;
    m_impl->flags = flags;
    m_impl->id = id;
}

Channel::FLAGS Channel::getStatusFlags(ClientPtr client) {
    shared_lock<shared_mutex> lock(m_impl->mutex);
    CLIENT_MAP::iterator i = m_impl->clients.find(client);
    if (i == m_impl->clients.end()) {
        return FLAGS();
    }
    return i->second;
}

void Channel::setStatusFlags(const string &name, ClientPtr client,
        Channel::FLAGS flags) {
    {
        shared_lock<shared_mutex> lock(m_impl->mutex);
        CLIENT_MAP::iterator i = m_impl->clients.find(client);
        if (i == m_impl->clients.end()) {
            return;
        }
        i->second = flags;
        commitStatusFlags(client, flags);
    }
    broadcast(ChannelStatus(shared_from_this(), name,
            client, flags.to_ulong()));
}

Channel::CLIENT_MAP::value_type Channel::getClient(const string &name) {
    shared_lock<shared_mutex> lock(m_impl->mutex);
    CLIENT_MAP::iterator i = m_impl->clients.begin();
    for (; i != m_impl->clients.end(); ++i) {
        // todo: maybe make this lowercase?
        if (name == (*i).first->getName()) {
            return *i;
        }
    }
    return CLIENT_MAP::value_type();
}

string Channel::getName() {
    shared_lock<shared_mutex> lock(m_impl->mutex);
    return m_impl->name;
}

string Channel::getTopic() {
    shared_lock<shared_mutex> lock(m_impl->mutex);
    return m_impl->topic;
}

int32_t Channel::getId() const {
    // assume that pointers are 32-bit
    // todo: use something less hacky than this for ids
    return (int32_t)this;
}

bool Channel::join(ClientPtr client) {
    shared_lock<shared_mutex> lock(m_impl->mutex, boost::defer_lock_t());
    lock.lock();
    if (m_impl->clients.find(client) != m_impl->clients.end()) {
        // already in channel
        return false;
    }
    // look up the client's auto flags on this channel
    Channel::FLAGS flags = handleJoin(client);
    if (flags[BAN] && handleBan()) {
        // user is banned from this channel
        return false;
    }
    // tell the client all about the channel
    client->sendMessage(ChannelInfo(shared_from_this()));
    // add the client to the channel
    m_impl->clients.insert(Channel::CLIENT_MAP::value_type(client, flags));
    lock.unlock();
    // inform the channel
    broadcast(ChannelJoinPart(shared_from_this(), client, true));
    broadcast(ChannelStatus(shared_from_this(), string(),
            client, flags.to_ulong()));
    return true;
}

void Channel::part(ClientPtr client) {
    shared_lock<shared_mutex> lock(m_impl->mutex, boost::defer_lock_t());
    lock.lock();
    if (m_impl->clients.find(client) == m_impl->clients.end())
        return;
    m_impl->clients.erase(client);
    handlePart(client);
    lock.unlock();
    broadcast(ChannelJoinPart(shared_from_this(), client, false));
}

void Channel::commitStatusFlags(ClientPtr client, FLAGS flags) {
    m_impl->server->getRegistry()->setUserFlags(m_impl->id, client->getId(),
            flags.to_ulong());
}

Channel::FLAGS Channel::handleJoin(ClientPtr client) {
    return m_impl->server->getRegistry()->getUserFlags(m_impl->id,
            client->getId());
}

void Channel::handlePart(ClientPtr client) {

}

void Channel::sendMessage(const string &message, ClientPtr client) {
    FLAGS flags = getStatusFlags(client);
    if (flags[PROTECTED]
            || flags[OP]
            || (!flags[BAN] && (flags[VOICE] || !m_impl->flags[MODERATED]))) {
        broadcast(ChannelMessage(shared_from_this(), client, message));
    }
}

bool Channel::setMode(ClientPtr setter, const string &user,
        const int mode, const bool enabled) {
    CLIENT_MAP::value_type target = getClient(user);
    if (!target.first) {
        // todo: maybe issue an error message
        return false;
    }
    FLAGS auth = getStatusFlags(setter);
    FLAGS flags = target.second;
    switch (mode) {
        case Mode::A: {
            if (auth[PROTECTED]) {
                // There is no good reason why a user with +a would ever
                // intentionally take away his own PROTECTED bit, so to guard
                // against an accident, we prevent users from taking away
                // their own +a.
                if (!iequals(user, setter->getName())) {
                    flags[PROTECTED] = enabled;
                }
            }
        } break;
        case Mode::O: {
            if (auth[PROTECTED]) {
                flags[OP] = enabled;
            }
        } break;
        case Mode::V: {
            if (auth[OP]) {
                flags[VOICE] = enabled;
            }
        } break;
        case Mode::B: {
            if (auth[OP]) {
                flags[BAN] = enabled;
            }
        } break;
        // TODO: the other modes
    }

    if (flags == target.second)
        return false;

    setStatusFlags(setter->getName(), target.first, flags);
    return true;
}

int Channel::getPopulation() {
    shared_lock<shared_mutex> lock(m_impl->mutex);
    return m_impl->clients.size();
}

void Channel::broadcast(const OutMessage &msg, ClientPtr client) {
    shared_lock<shared_mutex> lock(m_impl->mutex);
    CLIENT_MAP::const_iterator i = m_impl->clients.begin();
    for (; i != m_impl->clients.end(); ++i) {
        ClientPtr p = i->first;
        if (p != client) {
            p->sendMessage(msg);
        }
    }
}

}} // namespace shoddybattle::network


