/* 
 * File:   DatabaseRegistry.h
 * Author: Catherine
 *
 * Created on April 10, 2009, 11:51 PM
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

#ifndef _DATABASE_REGISTRY_H_
#define _DATABASE_REGISTRY_H_

#include <string>
#include <map>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include "../matchmaking/MetagameList.h"

namespace mysqlpp {
class ConnectionPool;
class Connection;
}

namespace shoddybattle { namespace database {

/** RAII-style Connection **/
class ScopedConnection {
public:
    ScopedConnection(mysqlpp::ConnectionPool &);
    ~ScopedConnection();
    mysqlpp::Connection *operator->() {
        return m_conn;
    }
private:
    mysqlpp::Connection *m_conn;
    mysqlpp::ConnectionPool &m_pool;

    ScopedConnection(const ScopedConnection &);
    ScopedConnection &operator=(const ScopedConnection &);
};

class Authenticator;

class DatabaseRegistry {
public:
    typedef boost::tuple<int, int, std::string> CHALLENGE_INFO;
    typedef std::pair<bool, int> AUTH_PAIR;
    typedef boost::tuple<std::string, std::string, int> INFO_ELEMENT;
    typedef std::map<int, INFO_ELEMENT> CHANNEL_INFO;
    typedef boost::tuple<int, std::string, int, bool> BAN_ELEMENT;
    typedef std::vector<BAN_ELEMENT> BAN_LIST;
    typedef boost::tuple<int, double> ESTIMATE_ELEMENT;
    typedef std::vector<ESTIMATE_ELEMENT> ESTIMATE_LIST;

    DatabaseRegistry();

    boost::shared_ptr<Authenticator> getAuthenticator() const;
    void setAuthenticator(boost::shared_ptr<Authenticator>);

    static std::string getShaHexHash(const std::string &);
    static std::string getMd5HexHash(const std::string &);

    /**
     * This needs to be called by every thread that is going to be making
     * use of the DatabaseRegistry.
     */
    static bool startThread();

    /**
     * Connect to a database.
     */
    void connect(const std::string db,
            const std::string server,
            const std::string user,
            const std::string password,
            const unsigned int port = 3306);

    /**
     * Get an authentication challenge for the given user.
     */
    CHALLENGE_INFO getAuthChallenge(const std::string &name,
            const std::string &ip,
            unsigned char *challenge);

    /**
     * Get the information about channels.
     */
    void getChannelInfo(CHANNEL_INFO &);

    /**
     * Determine whether the response to the challenge is valid.
     */
    AUTH_PAIR isResponseValid(std::string &name,
            const std::string &ip,
            const int challenge,
            const unsigned char *response);

    /**
     * Determine whether a particular user name exists.
     */
    bool userExists(const std::string name);
    
    /**
     * Register a new user. Returns false if the user name already exists.
     */
    bool registerUser(const std::string name,
            const std::string password,
            const std::string ip);

    /**
     * Get a user's flags on a channel.
     */
    int getUserFlags(const int channel, const std::string &user);

    /**
     * Get a user's flags on a channel.
     */
    int getUserFlags(const int channel, const int idx);

    /**
     * Set a user's flags on a channel.
     */
    void setUserFlags(const int channel, const int idx, const int flags);

    /**
     * Set the flags for a channel.
     */
    void setChannelFlags(const int channel, const int flags);
    
    /**
     * Gets a user's ban length on a channel and the authority
     * of the setter
     */
    void getBan(const int channel, const std::string &user, int &date,
            int &flags);

    int getGlobalBan(const std::string &user, const std::string &ip);
    
    /**
     * Sets the ban for a user on a channel.
     */
    bool setBan(const int channel, const std::string &user, const int bannerId,
            const long date, const bool ipBan = false);

    /**
     * Remove a ban.
     */
    void removeBan(const int channel, const std::string &user);
    
    /**
     * Gets the maximum level of a user (including their alts)
     */
    int getMaxLevel(const int channel, const std::string &user);
    
    /**
     * Updates the user's ip address when they sign on
     */
    void updateIp(const std::string &user, const std::string &ip);
    
    /**
     * Get's a user's ip address
     */
    std::string getIp(const std::string &user);
    
    /**
     * Get's a user's aliases
     */
    const std::vector<std::string> getAliases(const std::string &user);
    
    /**
     * Get's a user's bans
     */
    const BAN_LIST getBans(const std::string &user);

    /**
     * Get a user's ladder rating
     */
    double getRatingEstimate(const int id, const std::string &ladder);

    /**
     * Get a user's ladder estimates
     */
    ESTIMATE_LIST getEstimates(const int id, 
            const std::vector<MetagamePtr> &metagames);

    /**
     * Initialise the tables required for a ladder.
     */
    void initialiseLadder(const std::string &id);

    void postLadderMatch(const std::string &ladder, const int player0,
            const int player1, int victor);

    void joinLadder(const std::string &, const int);

    void updatePlayerStats(const std::string &ladder, const int id);
    
    void setPersonalMessage(const std::string &user, const std::string &message);
    
    void loadPersonalMessage(const std::string &user, std::string &message);
    
private:
    class DatabaseRegistryImpl;
    boost::shared_ptr<DatabaseRegistryImpl> m_impl;
    DatabaseRegistry(const DatabaseRegistry &);
    DatabaseRegistry &operator=(const DatabaseRegistry &);
};

}} // namespace shoddybattle::database

#endif
