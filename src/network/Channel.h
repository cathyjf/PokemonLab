/* 
 * File:   Channel.h
 * Author: Catherine
 *
 * Created on May 3, 2009, 3:06 AM
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

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <boost/enable_shared_from_this.hpp>
#include <bitset>
#include <map>
#include "network.h"

namespace shoddybattle { namespace network {

/**
 * A "channel" encapsulates the notion of chatting in a particular place,
 * rather like an IRC channel. The list of battles, however, is global to
 * the server, not channel-specific.
 */
class Channel : public boost::enable_shared_from_this<Channel> {
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
        BAN,        // +b
        IDLE,       // inactive
        BUSY        // ("ignoring challenges")
    };
    static const int FLAG_COUNT = 8;
    typedef std::bitset<FLAG_COUNT> FLAGS;

    enum CHANNEL_FLAGS {
        MODERATED   // +m
    };
    static const int CHANNEL_FLAG_COUNT = 1;
    typedef std::bitset<CHANNEL_FLAG_COUNT> CHANNEL_FLAGS;

    typedef std::map<ClientPtr, FLAGS> CLIENT_MAP;

    Channel(Server *,
            const std::string &name,
            const std::string &topic,
            CHANNEL_FLAGS,
            int = -1);

    FLAGS getStatusFlags(ClientPtr client);

    void setStatusFlags(ClientPtr client, FLAGS flags);

    CLIENT_MAP::value_type getClient(const std::string &name);

    std::string getName();

    std::string getTopic();

    int32_t getId() const;

    bool join(ClientPtr client);

    void part(ClientPtr client);

    void sendMessage(const std::string &message, ClientPtr client);

    int getPopulation();

    void broadcast(const OutMessage &msg);

    virtual void commitStatusFlags(ClientPtr client, FLAGS flags);

    virtual FLAGS handleJoin(ClientPtr client);

    virtual void handlePart(ClientPtr client);

private:
    class ChannelInfo;
    class ChannelStatus;
    class ChannelMessage;
    class ChannelJoinPart;
    class ChannelImpl;

    boost::shared_ptr<ChannelImpl> m_impl;
};

typedef boost::shared_ptr<Channel> ChannelPtr;

}} // namespace shoddybattle::network

#endif
