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

namespace shoddybattle { namespace database {

class DatabaseRegistryImpl;

class DatabaseRegistry {
public:
    DatabaseRegistry();
    ~DatabaseRegistry();

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
    int getAuthChallenge(const std::string name, unsigned char *challenge);

    /**
     * Determine whether the response to the challenge is valid.
     */
    bool isResponseValid(const std::string name,
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
    
private:
    DatabaseRegistryImpl *m_impl;
    DatabaseRegistry(const DatabaseRegistry &);
    DatabaseRegistry &operator=(const DatabaseRegistry &);
};

}} // namespace shoddybattle::database

#endif
