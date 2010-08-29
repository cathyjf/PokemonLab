/*
 * File:   Log.cpp
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

#include <string>
#include <ostream>
#include <boost/filesystem.hpp>
#include "Log.h"
#include "LogFile.h"

using namespace std;
namespace gc = boost::gregorian; // (gc for gregorian calendar)

namespace shoddybattle {
    
Log Log::out("server", MODE_CONSOLE);

struct Log::LogImpl : LogFile {
    LOG_MODE mode;
    string directory;
    string getLogFileName(const gc::date &d) const {
        return directory + gc::to_iso_extended_string(d);
    }
};

Log::Log(const string &directory, const LOG_MODE mode): m_impl(new LogImpl()) {
    m_impl->mode = mode;
    m_impl->directory = "logs/" + directory + "/";
    boost::filesystem::create_directories(m_impl->directory);
}

LogFile &Log::getLogFile() const {
    return *m_impl;
}

void Log::setMode(const LOG_MODE mode) {
    m_impl->mode = mode;
}

Log::tempstream Log::operator()() {
    return tempstream(*this);
}

class Log::logbuf : public stringbuf {
public:
    logbuf(Log &log): m_log(log) { }
protected:
    int sync() {
        const string text = str();
        if (m_log.m_impl->mode & MODE_FILE) {
            m_log.m_impl->write(text);
        }
        if (m_log.m_impl->mode & MODE_CONSOLE) {
            cout << text << flush;
        }
        setp(NULL, NULL);
        return 0;
    }
private:
    Log &m_log;
};

Log::tempstream::tempstream(Log &log) {
    m_buf = boost::shared_ptr<streambuf>(new logbuf(log));
    m_stream = boost::shared_ptr<ostream>(new ostream(m_buf.get()));
}

}
