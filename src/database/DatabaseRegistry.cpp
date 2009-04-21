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
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/random.hpp>
#include <mysql++.h>
#include <ssqls.h>
#include <string>
#include <sstream>
#include "sha2.h"
#include "rijndael.h"
#include "DatabaseRegistry.h"

using namespace std;
using namespace mysqlpp;
using namespace boost;

namespace shoddybattle { namespace database {

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

/** RAII-style Connection **/
class ScopedConnection {
public:
    ScopedConnection(ShoddyConnectionPool &pool):
            m_pool(pool) {
        m_conn = m_pool.grab();
    }
    Connection *operator->() {
        return m_conn;
    }
    ~ScopedConnection() {
        m_pool.release(m_conn);
    }
private:
    Connection *m_conn;
    ShoddyConnectionPool &m_pool;

    ScopedConnection(const ScopedConnection &);
    ScopedConnection &operator=(const ScopedConnection &);
};

struct DatabaseRegistryImpl {
    typedef variate_generator<mt11213b &,
            uniform_int<> > GENERATOR;
    ShoddyConnectionPool pool;
    mt11213b rand;     // used for challenge-response
    mutex randMutex;
    DatabaseRegistryImpl() {
        rand = mt11213b(time(NULL));
    }
    int getChallenge() {
        lock_guard<mutex> lock(randMutex);
        boost::uniform_int<> range(1, 2147483646); // = 2^31-2
        GENERATOR r(rand, range);
        return r();
    }
};

DatabaseRegistry::DatabaseRegistry() {
    m_impl = new DatabaseRegistryImpl();
}

DatabaseRegistry::~DatabaseRegistry() {
    delete m_impl;
}

bool DatabaseRegistry::startThread() {
    return Connection::thread_start();
}

static const char *HEX_TABLE = "0123456789ABCDEF";

static string getHexHash(const string message) {
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

int DatabaseRegistry::getUserFlags(const int channel, const int idx) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select flags from channel");
    query << channel << " where user = " << idx;
    query.parse();
    StoreQueryResult result = query.store();
    if (result.empty())
        return 0;
    return result[0][0];
}

DatabaseRegistry::AUTH_PAIR DatabaseRegistry::isResponseValid(const string name,
        const int challenge,
        const unsigned char *response) {
    ScopedConnection conn(m_impl->pool);
    Query query =
            conn->query("select id, password from users where name = %0q");
    query.parse();
    StoreQueryResult result = query.store(name);
    if (result.empty())
        return DatabaseRegistry::AUTH_PAIR(false, -1);

    const int responseInt = challenge + 1;
    string password;
    const int id = result[0][0];
    result[0][1].to_string(password);
    const string key = fromHex(password);

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
    return DatabaseRegistry::AUTH_PAIR(match, id);
}

int DatabaseRegistry::getAuthChallenge(const string name,
        unsigned char *challenge) {
    ScopedConnection conn(m_impl->pool);
    Query query = conn->query("select password from users where name = %0q");
    query.parse();
    StoreQueryResult result = query.store(name);
    if (result.empty())
        return 0;

    string password;
    result[0][0].to_string(password);
    const string key = fromHex(password);
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

    return challengeInt;
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

}} // namespace shoddybattle::database

