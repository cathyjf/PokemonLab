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
        const string &user) {
    Query query = conn->query("select password from users where name = %0q");
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
        const string &user) {
    Query query = conn->query("select password, salt from " + m_database +
            ".user where username = %0q");
    query.parse();
    StoreQueryResult result = query.store(user);
    if (result.empty())
        return SECRET_PAIR();

    string secret, salt;
    result[0][0].to_string(secret);
    result[0][1].to_string(salt);
    return SECRET_PAIR(DatabaseRegistry::getHexHash(secret), salt);
}

void VBulletinAuthenticator::finishAuthentication(ScopedConnection & /*conn*/,
        const std::string & /*user*/, const bool /*success*/) {
    
}

}} // namespace shoddybattle::database

