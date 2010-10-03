/*
 * File:   DatabaseRegistry.cpp
 * Author: Catherine
 *
 * Created on April 10, 2009, 9:34 PM
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

#include <netinet/in.h>

#include "Authenticator.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/random.hpp>
#include <mysql++.h>
#include <ssqls.h>
#include <string>
#include <sstream>
#include <ctime>
#include "../matchmaking/glicko2.h"
#include "sha2.h"
#include "rijndael.h"
#include "DatabaseRegistry.h"
#include "../matchmaking/MetagameList.h"

/**
 * Warning: The code in here is terrible and it should be improved at some
 *          point. The database schema is also terrible.
 */

using namespace std;
using namespace mysqlpp;
using namespace boost;

namespace shoddybattle { namespace database {

const double INITIAL_RATING = 1500.0;
const double INITIAL_DEVIATION = 350.0;
const double INITIAL_VOLATILITY = 0.09;
const double SYSTEM_CONSTANT = 1.2;

const char *TABLE_STATS_PREFIX = "ladder_stats_";
const char *TABLE_MATCHES_PREFIX = "ladder_matches_";

// MySQL users table
sql_create_6(users, 1, 6,
        sql_int, id,
        sql_varchar, name,
        sql_varchar, password,
        sql_int, level,
        sql_datetime, activity, // last activity
        sql_varchar, ip);

class ShoddyConnectionPool : public ConnectionPool {
public:
    ShoddyConnectionPool() {

    }
    void connect(const string db, const string server,
            const string user, const string password,
            const unsigned int port) {
        m_db = db;
        m_server = server;
        m_user = user;
        m_password = password;
        m_port = port;
    }
    unsigned int max_idle_time() {
        return 3600;
    }
    ~ShoddyConnectionPool() {
        clear();
    }
protected:
    Connection *create() {
        return new Connection(m_db.c_str(),
                m_server.c_str(),
                m_user.c_str(),
                m_password.c_str(),
                m_port);
    }
    void destroy(Connection *conn) {
        delete conn;
    }
private:
    string m_db;
    string m_server;
    string m_user;
    string m_password;
    unsigned int m_port;

    ShoddyConnectionPool(const ShoddyConnectionPool &);
    ShoddyConnectionPool &operator=(const ShoddyConnectionPool &);
};

ScopedConnection::ScopedConnection(ConnectionPool &pool):
        m_pool(pool) {
    m_conn = m_pool.grab();
}

ScopedConnection::~ScopedConnection() {
    m_pool.release(m_conn);
}

struct DatabaseRegistry::DatabaseRegistryImpl {
    typedef variate_generator<mt11213b &,
            uniform_int<> > GENERATOR;
    ShoddyConnectionPool pool;
    mt11213b rand;     // used for challenge-response
    mutex randMutex;
    shared_ptr<Authenticator> authenticator;
    DatabaseRegistryImpl() {
        rand = mt11213b(time(NULL));
        authenticator = shared_ptr<Authenticator>(new DefaultAuthenticator());
    }
    int getChallenge() {
        lock_guard<mutex> lock(randMutex);
        uniform_int<> range(1, 2147483646); // = 2^31-2
        GENERATOR r(rand, range);
        return r();
    }
};

DatabaseRegistry::DatabaseRegistry():
        m_impl(new DatabaseRegistryImpl()) { }

shared_ptr<Authenticator> DatabaseRegistry::getAuthenticator() const {
    return m_impl->authenticator;
}

void DatabaseRegistry::setAuthenticator(
        shared_ptr<Authenticator> authenticator) {
    m_impl->authenticator = authenticator;
}

bool DatabaseRegistry::startThread() {
    return Connection::thread_start();
}

static const char *HEX_TABLE = "0123456789ABCDEF";

string DatabaseRegistry::getHexHash(const string &message) {
    unsigned char digest[32];
    sha256(reinterpret_cast<const unsigned char *>(message.c_str()),
            message.length(), digest);
    string hex(64, ' ');
    for (int i = 0; i < 32; ++i) {
        const int high = (digest[i] & 0xf0) >> 4;
        const int low = (digest[i] &  0x0f);
        hex[i * 2] = HEX_TABLE[high];
        hex[i * 2 + 1] = HEX_TABLE[low];
    }
    return hex;
}

static string fromHex(const string message) {
    int length = message.length();
    if (length % 2)
        return string();

    string ret(length / 2, ' ');
    int j = 0;
    for (int i = 0; i < length; i += 2) {
        const int high = (int)(strchr(HEX_TABLE, message[i]) - HEX_TABLE);
        const int low = (int)(strchr(HEX_TABLE, message[i + 1]) - HEX_TABLE);
        ret[j++] = (char)(((high << 4) & 0xf0) | (low & 0x0f));
    }
    return ret;
}

bool DatabaseRegistry::userExists(const string name) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select count(id) from users where name = %0q");
    query.parse();
    StoreQueryResult res = query.store(name);
    int count = res[0][0];
    return (count != 0);
}

void DatabaseRegistry::getChannelInfo(DatabaseRegistry::CHANNEL_INFO &info) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select * from channels");
    query.parse();
    StoreQueryResult res = query.store();
    const int count = res.num_rows();
    for (int i = 0; i < count; ++i) {
        string name, topic;
        Row row = res[i];
        const int id = row[0];
        row[1].to_string(name);
        row[2].to_string(topic);
        const int flags = row[3];
        info[id] = INFO_ELEMENT(name, topic, flags);
    }
}
	
void DatabaseRegistry::setChannelFlags(const int channel, const int flags) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("update channels set flags=");
    query << flags << " where id=" << channel;
    query.execute();
}

void DatabaseRegistry::setUserFlags(const int channel, const int idx,
        const int flags) {
    ScopedConnection conn(m_impl->pool);
    {
        Query query = conn->query("delete from channel");
        query << channel << " where id = " << idx;
        query.execute();
    }
    {
        Query query = conn->query("insert into channel");
        query << channel << " values (" << idx << "," << flags << ")";
        query.execute();
    }
}

int DatabaseRegistry::getUserFlags(const int channel, const string &user) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select flags from channel");
    query << channel << " join users on channel" << channel;
    query << ".id=users.id where name=%0q";
    query.parse();
    StoreQueryResult result = query.store(user);
    if (result.empty())
        return 0;
    return result[0][0];
}

int DatabaseRegistry::getUserFlags(const int channel, const int idx) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select flags from channel");
    query << channel << " where id = " << idx;
    query.parse();
    StoreQueryResult result = query.store();
    if (result.empty())
        return 0;
    return result[0][0];
}

double DatabaseRegistry::getRatingEstimate(const int id, const std::string &ladder) {
    const string table = TABLE_STATS_PREFIX + ladder;
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select estimate from %0 where user = %1");
    query.parse();
    StoreQueryResult result = query.store(table, id);
    if (result.empty())
        return -1;

    return result[0][0];
}

DatabaseRegistry::ESTIMATE_LIST DatabaseRegistry::getEstimates(const int id,
        const std::vector<MetagamePtr> &metagames) {
    ESTIMATE_LIST list;
    vector<MetagamePtr>::const_iterator i = metagames.begin();
    for(; i != metagames.end(); ++i) {
        MetagamePtr p = *i;
        double estimate = getRatingEstimate(id, p->getId());
        list.push_back(ESTIMATE_ELEMENT(p->getIdx(), estimate));
    }
    return list;
}

void DatabaseRegistry::initialiseLadder(const std::string &id) {
    const string table1 = TABLE_STATS_PREFIX + id;
    const string table2 = TABLE_MATCHES_PREFIX + id;
    ScopedConnection conn(m_impl->pool);
    {
        Query query = conn->query("show tables like %0q");
        query.parse();
        StoreQueryResult result = query.store(table1);
        if (!result.empty())
            return;
    }
    {
        Query query = conn->query("create table ");
        query << table1 << " (user int(11) unique, rating double, ";
        query << "deviation double, volatility double, estimate double, ";
        query << "updated_rating double, updated_deviation double, ";
        query << "updated_volatility double)";
        query.parse();
        query.execute();
    }
    {
        Query query = conn->query("create table ");
        query << table2 << " (player1 int(11), player2 int(11), ";
        query << "victor int(4))";
        query.parse();
        query.execute();
    }
}

void DatabaseRegistry::updatePlayerStats(const string &ladder,
        const int id) {
    const string table1 = TABLE_STATS_PREFIX + ladder;
    const string table2 = TABLE_MATCHES_PREFIX + ladder;
    vector<glicko2::MATCH> matches;
    ScopedConnection conn(m_impl->pool);
    StoreQueryResult res;
    {
        Query q = conn->query("select * from ");
        q << table2 << " where player1=" << id << " or player2=" << id;
        q.parse();
        res = q.store();
    }
    StoreQueryResult::iterator i = res.begin();
    for (; i != res.end(); ++i) {
        Row &row = *i;
        const int player0 = row[0];
        const int player1 = row[1];
        const int v = row[2];
        const int score = (v == -1) ? -1 : (int(row[v]) == id);
        const int opponent = (player0 == id) ? player1 : player0;
        Query q = conn->query("select rating, deviation from ");
        q << table1 << " where user=" << opponent;
        q.parse();
        StoreQueryResult res2 = q.store();
        if (!res2.empty()) {
            Row &r2 = res2[0];
            const double rating = r2[0];
            const double deviation = r2[1];
            glicko2::MATCH match = { score, rating, deviation };
            matches.push_back(match);
        }
    }
    {
        Query q = conn->query("select rating, deviation, volatility from ");
        q << table1 << " where user=" << id;
        q.parse();
        res = q.store();
    }
    if (res.empty())
        return;
    Row &r = res[0];
    const double rating = r[0];
    const double deviation = r[1];
    const double volatility = r[2];
    glicko2::PLAYER player = { rating, deviation, volatility };
    glicko2::updatePlayer(player, matches, SYSTEM_CONSTANT);
    const double estimate = glicko2::getRatingEstimate(player.rating,
            player.deviation);
    Query q = conn->query("update ");
    q << table1 << " set updated_rating=" << player.rating
            << ", updated_deviation=" << player.deviation
            << ", updated_volatility=" << player.volatility
            << ", estimate=" << estimate << " where user=" << id;
    q.parse();
    q.execute();
}

void DatabaseRegistry::joinLadder(const string &ladder, const int id) {
    const string table = TABLE_STATS_PREFIX + ladder;
    ScopedConnection conn(m_impl->pool);
    {
        Query query = conn->query("select user from ");
        query << table << " where user=" << id;
        query.parse();
        StoreQueryResult result = query.store();
        if (!result.empty())
            return;
    }
    Query query = conn->query("insert into ");
    query << table << " values (" << id << "," << INITIAL_RATING << ","
            << INITIAL_DEVIATION << "," << INITIAL_VOLATILITY
            << ",0,0,0,0)";
    query.parse();
    query.execute();
}

void DatabaseRegistry::postLadderMatch(const string &ladder,
        const int player0, const int player1, int victor) {
    const string table = TABLE_MATCHES_PREFIX + ladder;
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("insert into ");
    query << table << " values (" << player0 << "," << player1
            << "," << victor << ")";
    query.parse();
    query.execute();
}

DatabaseRegistry::AUTH_PAIR DatabaseRegistry::isResponseValid(
        const string &name,
        const string &ip,
        const int challenge,
        const unsigned char *response) {
    const int responseInt = challenge + 1;

    ScopedConnection conn(m_impl->pool);
    const SECRET_PAIR secret = m_impl->authenticator->getSecret(conn, name, ip);
    const string key = fromHex(secret.first);

    unsigned char data[16];
    unsigned char middle[16];
    unsigned char correct[16];
    *(int32_t *)data = htonl(responseInt);
    memset(data + 4, 0, 12); // zero out the rest of the block

    // first pass
    rijndael_ctx ctx;
    rijndael_set_key(&ctx, (const unsigned char *)key.c_str(), 128);
    rijndael_encrypt(&ctx, data, middle);

    // second pass
    rijndael_set_key(&ctx, (const unsigned char *)key.c_str() + 16, 128);
    rijndael_encrypt(&ctx, middle, correct);

    const bool match = (memcmp(response, correct, 16) == 0);
    if (!m_impl->authenticator->finishAuthentication(conn, name, ip, match)) {
        return DatabaseRegistry::AUTH_PAIR(false, -1);
    }

    int id = -1;
    if (match) {
        Query query = conn->query("select id from users where name = %0q");
        query.parse();
        StoreQueryResult result = query.store(name);
        if (result.empty()) {
            // It should be impossible to get here.
            return DatabaseRegistry::AUTH_PAIR(false, -1);
        }
        id = result[0][0];
    }
    
    return DatabaseRegistry::AUTH_PAIR(match, id);
}

DatabaseRegistry::CHALLENGE_INFO DatabaseRegistry::getAuthChallenge(
        const string &name, const string &ip, unsigned char *challenge) {
    ScopedConnection conn(m_impl->pool);
    const SECRET_PAIR secret = m_impl->authenticator->getSecret(conn, name, ip);
    if (secret.first.empty())
        return CHALLENGE_INFO();

    const string key = fromHex(secret.first);
    const int32_t challengeInt = m_impl->getChallenge();
    unsigned char data[16];
    unsigned char part[16];
    *(int32_t *)data = htonl(challengeInt);
    memset(data + 4, 0, 12); // zero out the rest of the block
    
    // first pass
    rijndael_ctx ctx;
    rijndael_set_key(&ctx, (const unsigned char *)key.c_str(), 128);
    rijndael_encrypt(&ctx, data, part);

    // second pass
    rijndael_set_key(&ctx, (const unsigned char *)key.c_str() + 16, 128);
    rijndael_encrypt(&ctx, part, challenge);

    return CHALLENGE_INFO(challengeInt,
            m_impl->authenticator->getSecretStyle(), secret.second);
}

void DatabaseRegistry::connect(const string db, const string server,
        const string user, const string password, const unsigned int port) {
    m_impl->pool.connect(db, server, user, password, port);
}

bool DatabaseRegistry::registerUser(const string name,
        const string password, const string ip) {
    if (userExists(name))
        return false;
    const string hex = getHexHash(password);
    users row(0, name, hex, 0, DateTime(time(NULL)), ip);
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query();
    query.insert(row);
    query.execute();
    return true;
}

int DatabaseRegistry::getMaxLevel(const int channel, const string &user) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select flags from channel");
    query << channel << " join users on channel" << channel;
    query << ".id=users.id where ip=(select ip from users where name= %0q )";
    query.parse();
    StoreQueryResult res = query.store(user);

    if (res.empty()) {
        return 0;
    } else {
        int max = 0;
        int length = res.num_rows();
        for (int i = 0; i < length; ++i) {
            if ((int)res[i][0] > max) {
                max = (int)res[i][0];
            }
        }
        return max;
    }
}

void DatabaseRegistry::getGlobalBan(const std::string &user,
        const std::string &ip, int &date, int &flags) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query(
            "SELECT expiry, flags "
            "FROM bans "
            "WHERE channel=-1 "
                "AND ("
                    "id=(SELECT id FROM users WHERE name=%0q) "
                    "OR ("
                        "ip_ban AND id IN ("
                            "SELECT id FROM users WHERE ip=%1q)"
                    ")"
                ") "
            "ORDER BY expiry DESC "
            "LIMIT 0, 1");
    query.parse();
    StoreQueryResult res = query.store(user, ip);
    if (res.empty()) {
        date = flags = 0;
        return;
    }
    date = (int)DateTime(res[0][0]);
    flags = (int)res[0][1];
}

void DatabaseRegistry::getBan(const int channel, const string &user,
        int &date, int &flags) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query(
            "SELECT expiry, flags "
            "FROM bans "
            "WHERE channel=%0q "
                "AND id=(SELECT id FROM users WHERE name= %1q )");
    query.parse();
    StoreQueryResult res = query.store(channel, user);
    if (res.empty()) {
        date = flags = 0;
        return;
    }
    date = (int)DateTime(res[0][0]);
    flags = (int)res[0][1];
}

bool DatabaseRegistry::setBan(const int channel, const string &user,
        const int flags, const long date, const bool ipBan) {
    ScopedConnection conn(m_impl->pool);
    {
        Query query = conn->query("delete from bans where channel=");
        query << channel << " and id=(select id from users where name= %0q )";
        query.parse();
        query.execute(user);
    }
    
    if (date < time(NULL)) {
        return true;
    } else {
        Query query = conn->query(
                "INSERT INTO bans "
                    "(channel, id, flags, expiry, ip_ban) "
                "VALUES (%0q, (select id from users where name= %1q ), %2q, "
                    "%3q, %4q)");
        query.parse();
        query.execute(channel, user, flags, DateTime(date), ipBan);
        return false;
    }
}

void DatabaseRegistry::updateIp(const string &user, const string &ip) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("update users set ip= %0q where name= %1q");
    query.parse();
    query.execute(ip, user);
}

string DatabaseRegistry::getIp(const string &user) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select ip from users where name= %0q");
    query.parse();
    StoreQueryResult res = query.store(user);
    if (res.empty()) {
        return string();
    } else {
        string ip;
        res[0][0].to_string(ip);
        return ip;
    }
}

const vector<string> DatabaseRegistry::getAliases(const string &user) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select name from users where ip=");
    query << "(select ip from users where name= %0q)";
    query.parse();
    StoreQueryResult res = query.store(user);
    vector<string> ret;
    const int size = res.num_rows();
    for (int i = 0; i < size; ++i) {
        string alias;
        res[i][0].to_string(alias);
        ret.push_back(alias);
    }
    return ret;
}

const DatabaseRegistry::BAN_LIST DatabaseRegistry::getBans(const string &user) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("SELECT bans.channel, users.name, bans.expiry, "
                "bans.ip_ban "
            "FROM bans ");
    query << "JOIN users ON users.id=bans.id WHERE users.ip=";
    query << "(SELECT ip FROM users WHERE name= %0q )";
    query.parse();
    StoreQueryResult res = query.store(user);
    DatabaseRegistry::BAN_LIST bans;
    const int count = res.num_rows();
    for (int i = 0; i < count; ++i) {
        Row r = res[i];
        const int id = r[0];
        const string name = string(r[1].c_str());
        const int date = (int)DateTime(r[2]);
        const bool ipBan = (bool)r[3];
        bans.push_back(BAN_ELEMENT(id, name, date, ipBan));
    }
    return bans;
}

void DatabaseRegistry::setPersonalMessage(const string &user, const string &msg) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("update users set message= %0q where name= %1q");
    query.parse();
    query.execute(msg, user);
}

void DatabaseRegistry::loadPersonalMessage(const string &user, string &msg) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select message from users where name= %0q");
    query.parse();
    StoreQueryResult res = query.store(user);
    if (res.empty() || res[0][0].is_null()) {
        msg = "";
    } else {
        res[0][0].to_string(msg);
    }
}

}} // namespace shoddybattle::database

