/* 
 * File:   Authenticator.h
 * Author: Catherine
 *
 * Created on June 13, 2010, 6:27 PM
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

#ifndef _AUTHENTICATOR_H_
#define _AUTHENTICATOR_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace shoddybattle { namespace database {

typedef std::pair<std::string, std::string> SECRET_PAIR;

class ScopedConnection;

class Authenticator : boost::noncopyable {
public:
    enum {
        SECRET_UNADORNED = 0,   // secret := password
        SECRET_MD5 = 1,         // secret := md5(password)
        SECERT_MD5_SALT = 2     // secret := md5(md5(password) + salt)
    };

    virtual int getSecretStyle() = 0;
    virtual SECRET_PAIR getSecret(ScopedConnection &,
            const std::string &userName,
            const std::string &ipAddress) = 0;
    virtual bool finishAuthentication(ScopedConnection &,
            std::string &, const std::string &, const bool match) {
        return match;
    }
    virtual bool registerUser(ScopedConnection &,
            const std::string &, const std::string &, const std::string &) = 0;
    virtual std::string getLoginInfo() const = 0;
    virtual bool allowsRegistration() const = 0;
    virtual std::string getRegistrationInfo() const = 0;
    virtual ~Authenticator() { }
};

class DefaultAuthenticator : public Authenticator {
public:
    int getSecretStyle() { return SECRET_UNADORNED; }
    SECRET_PAIR getSecret(ScopedConnection &, const std::string &,
            const std::string &);
    bool registerUser(ScopedConnection &,
            const std::string &, const std::string &, const std::string &);
    std::string getLoginInfo() const { return std::string(); }
    bool allowsRegistration() const { return true; }
    std::string getRegistrationInfo() const { return std::string(); }
};

class SaltAuthenticator : public Authenticator {
public:
    SaltAuthenticator();
    int getSecretStyle() { return SECERT_MD5_SALT; }
    SECRET_PAIR getSecret(ScopedConnection &, const std::string &,
            const std::string &);
    bool registerUser(ScopedConnection &,
            const std::string &, const std::string &, const std::string &);
    std::string getLoginInfo() const { return std::string(); }
    bool allowsRegistration() const { return true; }
    std::string getRegistrationInfo() const { return std::string(); }
private:
    class SaltAuthenticatorImpl;
    boost::shared_ptr<SaltAuthenticatorImpl> m_impl;
};

class VBulletinAuthenticator : public Authenticator {
public:
    VBulletinAuthenticator(const std::string &, const std::string &,
            const std::string &database = "vbulletin");
    int getSecretStyle() { return SECERT_MD5_SALT; }
    SECRET_PAIR getSecret(ScopedConnection &, const std::string &,
            const std::string &);
    bool finishAuthentication(ScopedConnection &, std::string &,
            const std::string &, const bool);
    bool registerUser(ScopedConnection &,
            const std::string &, const std::string &, const std::string &) {
        return false;
    }
    std::string getLoginInfo() const { return m_loginInfo; }
    bool allowsRegistration() const { return false; }
    std::string getRegistrationInfo() const { return m_registrationInfo; }
private:
    const std::string m_database, m_loginInfo, m_registrationInfo;
};

}} // namespace shoddybattle::database

#endif
