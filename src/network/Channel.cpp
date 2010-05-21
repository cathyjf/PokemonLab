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

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include "Channel.h"
#include "network.h"
#include "../database/DatabaseRegistry.h"

using namespace std;
using namespace boost;
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace shoddybattle { namespace network {

struct Channel::ChannelImpl {
    Server *server;
    int id;       // channel id
    CLIENT_MAP clients;
    string name;  // name of this channel (e.g. #main)
    string topic; // channel topic
    CHANNEL_FLAGS flags;
    shared_mutex mutex;
    date lastDate;
    ofstream log;

    static const char *MODES;
    static const char *CHANNEL_MODES;
};

const char *Channel::ChannelImpl::MODES = "aovbiu";
const char *Channel::ChannelImpl::CHANNEL_MODES = "mi";

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
            const string &subject, int flags):
            OutMessage(CHANNEL_STATUS) {
        *this << channel->getId();
        *this << user;
        *this << subject;
        *this << flags;
        finalise();
    }
};

class Channel::ChannelMessage : public OutMessage {
public:
    ChannelMessage(ChannelPtr channel, const string &name,
                const string &msg):
            OutMessage(CHANNEL_MESSAGE) {
        *this << channel->getId();
        *this << name;
        *this << msg;
        finalise();
    }
};

class Channel::ChannelJoinPart : public OutMessage {
public:
    ChannelJoinPart(ChannelPtr channel,
            const string &name, bool join):
            OutMessage(CHANNEL_JOIN_PART) {
        *this << channel->getId();
        *this << name;
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

string Channel::getModeText(FLAGS f2, FLAGS f1) {
    string ret;
    int last = -1;
    for (int i = 0; i < FLAG_COUNT; ++i) {
        if (f1[i] == f2[i])
            continue;
        int diff = f1[i];
        if (diff != last) {
            last = diff;
            ret += diff ? "-" : "+";
        }
        ret += ChannelImpl::MODES[i];
    }
    return ret;
}

string Channel::getChannelModeText(CHANNEL_FLAGS f2, CHANNEL_FLAGS f1) {
    string ret;
    int last = -1;
    for (int i = 0; i < CHANNEL_FLAG_COUNT; ++i) {
        if (f1[i] == f2[i])
            continue;
        int diff = f1[i];
        if (diff != last) {
            last = diff;
            ret += diff ? "-" : "+";
        }
        ret += ChannelImpl::CHANNEL_MODES[i];
    }
    return ret;
}

void Channel::writeLog(const string &line) {
    lock_guard<shared_mutex> lock(mutex);
    const date d = day_clock::local_day();
    const bool open = m_impl->log.is_open();
    if (!open || (d.day() != m_impl->lastDate.day())) {
        if (open) {
            m_impl->log.close();
        }
        m_impl->lastDate = d;
        const string file = "logs/chat/" + m_impl->name
                + "/" + to_iso_extended_string(d);
        m_impl->log.open(file.c_str(), ios_base::app);
    }
    const time_duration t = second_clock::local_time().time_of_day();
    const string prefix = to_simple_string(t);
    m_impl->log << "(" << prefix << ") " << line << endl;
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
    FLAGS old;
    {
        shared_lock<shared_mutex> lock(m_impl->mutex);
        CLIENT_MAP::iterator i = m_impl->clients.find(client);
        if (i == m_impl->clients.end()) {
            return;
        }
        old = i->second;
        i->second = flags;
        commitStatusFlags(client, flags);
    }
    const string subject = client->getName();
    broadcast(ChannelStatus(shared_from_this(), name,
            subject, flags.to_ulong()));
    const string message = "[mode] " + getModeText(flags, old)
            + " " + subject + ", " + name;
    writeLog(message);
}

void Channel::setChannelFlags(const string &name, Channel::CHANNEL_FLAGS flags) {
    CHANNEL_FLAGS old = m_impl->flags;
    m_impl->flags = flags;
    commitChannelFlags(flags);
    broadcast(ChannelStatus(shared_from_this(), name,
                            "", flags.to_ulong()));
    const string message = "[mode] " + getChannelModeText(flags, old)
        + ", " + name;
    writeLog(message);
}
    
Channel::CLIENT_MAP::value_type Channel::getClient(const string &name) {
    shared_lock<shared_mutex> lock(m_impl->mutex);
    CLIENT_MAP::iterator i = m_impl->clients.begin();
    for (; i != m_impl->clients.end(); ++i) {
        if (iequals(name, (*i).first->getName())) {
            return *i;
        }
    }
    return CLIENT_MAP::value_type();
}

void Channel::setName(const string &name) {
    unique_lock<shared_mutex> lock(m_impl->mutex);
    m_impl->name = name;
}

string Channel::getName() {
    shared_lock<shared_mutex> lock(m_impl->mutex);
    return m_impl->name;
}

void Channel::setTopic(const string &topic) {
    unique_lock<shared_mutex> lock(m_impl->mutex);
    m_impl->topic = topic;
}

string Channel::getTopic() {
    shared_lock<shared_mutex> lock(m_impl->mutex);
    return m_impl->topic;
}

int32_t Channel::getId() const {
    // assume that pointers are 32-bit
    // todo: use something less hacky than this for ids
	if (getChannelType() == Type::ORDINARY)
        return m_impl->id;
    int64_t point = (int64_t)this;
    int32_t id = (int32_t)(point);
	return id;
}

bool Channel::join(ClientPtr client) {
    upgrade_lock<shared_mutex> lock(m_impl->mutex, boost::defer_lock_t());
    lock.lock();
    if (m_impl->clients.find(client) != m_impl->clients.end()) {
        // already in channel
        return false;
    }
    // look up the client's auto flags on this channel
    Channel::FLAGS flags = handleJoin(client);
    if (handleBan(client)) {
        // user is banned from this channel
        return false;
    }
    // tell the client all about the channel
    client->sendMessage(ChannelInfo(shared_from_this()));
    // upgrade to exclusive ownership of the mutex
    {
        upgrade_to_unique_lock<shared_mutex> exclusive(lock);
        // add the client to the channel
        m_impl->clients.insert(Channel::CLIENT_MAP::value_type(client, flags));
    }
    lock.unlock();
    // inform the channel
    const string name = client->getName();
    broadcast(ChannelJoinPart(shared_from_this(), name, true));
    broadcast(ChannelStatus(shared_from_this(), string(),
            name, flags.to_ulong()));

    string message = "[join] " + name + ", " + client->getIp();
    if (flags.any()) {
        message += ", " + getModeText(flags);
    }
    writeLog(message);

    return true;
}

bool Channel::handleBan(ClientPtr client) {
    int ban, flags;
    m_impl->server->getRegistry()->getBan(m_impl->id, client->getName(), ban ,flags);
    if (ban < time(NULL)) {
        //ban expired; remove it
        m_impl->server->commitBan(m_impl->id, client->getName(), 0, 0);
        return false;
    } else {
        client->informBanned(ban);
        return true;
    }
}

void Channel::part(ClientPtr client) {
    upgrade_lock<shared_mutex> lock(m_impl->mutex, boost::defer_lock_t());
    lock.lock();
    if (m_impl->clients.find(client) == m_impl->clients.end())
        return;
    // upgrade to exclusive ownership
    {
        upgrade_to_unique_lock<shared_mutex> exclusive(lock);
        m_impl->clients.erase(client);
    }
    handlePart(client);
    lock.unlock();
    const string name = client->getName();
    broadcast(ChannelJoinPart(shared_from_this(), name, false));

    const string message = "[part] " + name;
    writeLog(message);

    if (getPopulation() == 0) {
        handleFinalise();
    }
}

void Channel::commitStatusFlags(ClientPtr client, FLAGS flags) {
    m_impl->server->getRegistry()->setUserFlags(m_impl->id, client->getId(),
            flags.to_ulong());
}

void Channel::commitChannelFlags(CHANNEL_FLAGS flags) {
    m_impl->server->getRegistry()->setChannelFlags(m_impl->id, flags.to_ulong());
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
        const string name = client->getName();
        broadcast(ChannelMessage(shared_from_this(), name, message));

        const string log = name + ": " + message;
        writeLog(log);
    }
}

bool Channel::setMode(ClientPtr setter, const string &user,
        const int mode, const bool enabled) {
    FLAGS auth = getStatusFlags(setter);
    CLIENT_MAP::value_type target = getClient(user);
    if (!target.first) {
        // no user means it's a channel flag
		CHANNEL_FLAGS cflags = m_impl->flags;
        switch (mode) {
            case Mode::M: {
                if (auth[OP]) {
                    cflags[MODERATED] = enabled;
                }
            } break;
            case Mode::I: {
                if (auth[OP]) {
                    cflags[INVITE_ONLY] = enabled;
                }
            } break;
        }
        if (cflags == m_impl->flags) return false;
        setChannelFlags(setter->getName(), cflags);
        return true;
    }
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


