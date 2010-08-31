/* 
 * File:   Authenticator.cpp
 * Author: Catherine
 *
 * Created on June 13, 2010, 6:28 PM
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

#include "Authenticator.h"
#include "DatabaseRegistry.h"
#include <string>
#include <mysql++.h>

using namespace std;
using namespace mysqlpp;

namespace shoddybattle { namespace database {

SECRET_PAIR DefaultAuthenticator::getSecret(ScopedConnection &conn,
        const string &user, const string &) {
    Query query = conn->query("SELECT password FROM users WHERE name = %0q");
    query.parse();
    StoreQueryResult result = query.store(user);
    if (result.empty())
        return SECRET_PAIR();

    string value;
    result[0][0].to_string(value);
    return SECRET_PAIR(value, string());
}

VBulletinAuthenticator::VBulletinAuthenticator(const string &database):
        m_database(database) { }

SECRET_PAIR VBulletinAuthenticator::getSecret(ScopedConnection &conn,
        const string &user, const string &ip) {
    // First check the strike system to make sure the IP is allowed to
    // attempt to log in.
    {
        Query query = conn->query(
                "SELECT count(*) AS strikes,"
                    "unix_timestamp() - MAX(striketime) AS lasttime "
                "FROM " + m_database + ".strikes "
                "WHERE strikeip = %0q "
                    "AND striketime > (unix_timestamp() - 3600)"
            );
        query.parse();
        StoreQueryResult result = query.store(ip);
        const int strikes = result[0][0];
        const int lastTime = result[0][1].is_null() ? 0 : result[0][1];
        // Note: The following arbitrary constants are directly from the
        //       vbulletin source, and are not configurable values in
        //       vbulletin itself.
        if ((strikes >= 5) && (lastTime < 900)) {
            // This IP has attempted to log in too many times.
            return SECRET_PAIR();
        }
    }
    
    Query query = conn->query(
            "SELECT password, salt "
            "FROM " + m_database + ".user "
            "WHERE username = %0q"
        );
    query.parse();
    StoreQueryResult result = query.store(user);
    if (result.empty())
        return SECRET_PAIR();

    string secret, salt;
    result[0][0].to_string(secret);
    result[0][1].to_string(salt);
    return SECRET_PAIR(DatabaseRegistry::getHexHash(secret), salt);
}

bool VBulletinAuthenticator::finishAuthentication(ScopedConnection &conn,
        const std::string &user, const std::string &ip, const bool success) {
    if (!success) {
        // Give a strike to the IP.
        Query query = conn->query(
                "INSERT INTO " + m_database + ".strikes "
                    "(striketime, strikeip, username) "
                 "VALUES (unix_timestamp(), %0q, %1q)"
             );
        query.parse();
        query.execute(ip, user);
        return false;
    }

    Query query = conn->query(
            "SELECT userid FROM " + m_database + ".user WHERE username = %0q"
        );
    query.parse();
    StoreQueryResult result = query.store(user);
    if (result.empty()) {
        // This will happen if the forum account is renamed or deleted at the
        // "right" time.
        return false;
    }
    const int foreignId = result[0][0];
    
    Query q2 = conn->query(
            "INSERT INTO users (name, foreign_id, level, activity, ip) "
            "VALUES (%0q, %1q, 0, now(), %2q) "
            "ON DUPLICATE KEY UPDATE name=%0q, activity=now()"
        );
    q2.parse();
    q2.execute(user, foreignId, ip);
    return true;
}

}} // namespace shoddybattle::database

