/*
 * File:   Log.h
 * Author: Catherine
 *
 * Created on August 29, 2010, 12:02 PM
 *
 * This file is a part of Shoddy Battle.
 * Copyright (C) 2010  Catherine Fitzpatrick and Benjamin Gwin
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

#ifndef _MAIN_LOG_H_
#define _MAIN_LOG_H_

#include <sstream>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace shoddybattle {

class LogFile;

/**
 * This class provides a callable member, Log::out(), which should be used
 * everywhere in place of cout. Writing to this stream is thread safe, unlike
 * cout. Additionally, the messages will be written to dated log files with
 * a timestamp prefix.
 * 
 * Proper use of this class is very similar to using cout. Write endl to the
 * stream to end a message. Do *not* end a message using "\n" rather than endl.
 *
 * Example:
 *     Log::out() << "This is a log message." << endl;
 */
class Log : boost::noncopyable {
public:
    static Log out;

    enum LOG_MODE {
        MODE_CONSOLE = 1,
        MODE_FILE = 2
    };
    
    class tempstream {
    public:
        template <class T> tempstream &operator<<(T t) {
            *m_stream << t;
            return *this;
        }
        // This overload is necessary for writing manipulators to the stream
        // to work, or the compiler won't know which template argument to take
        // for the manipulator.
        tempstream &operator<<(std::ostream & (*p)(std::ostream &)) {
            *m_stream << p;
            return *this;
        }
    private:
        friend class Log;
        tempstream(Log &);
        boost::shared_ptr<std::streambuf> m_buf;
        boost::shared_ptr<std::ostream> m_stream;
    };

    // Get a stream object that log messages can be written to. Write endl at
    // the end of the log message.
    tempstream operator()();

    // Set the logging mode. This function is NOT thread safe. It is an error
    // for two threads to be in this function at once or for one thread to be
    // in this function, and for another thread to be writing to the log.
    void setMode(const LOG_MODE mode);

    // Get the underlying LogFile. This is intended to be used ONLY be the
    // catch (...) handler in main.cpp. Do not call this function from anywhere
    // else.
    LogFile &getLogFile() const;

private:
    class logbuf;
    friend class logbuf;
    Log(const std::string &, const LOG_MODE);
    class LogImpl;
    boost::shared_ptr<LogImpl> m_impl;
};

inline std::ostream &endl(std::ostream &s) {
    return std::endl<char, std::char_traits<char> >(s);
}

}

#endif
