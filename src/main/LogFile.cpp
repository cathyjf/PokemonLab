/*
 * File:   LogFile.cpp
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

#include <fstream>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "LogFile.h"

using namespace std;
namespace gc = boost::gregorian; // (gc for gregorian calendar)
namespace pt = boost::posix_time;

namespace shoddybattle {

struct LogFile::LogFileImpl {
    boost::mutex mutex;
    gc::date lastDate;
    ofstream log;
};

LogFile::LogFile(): m_impl(new LogFileImpl()) { }

void LogFile::close() {
    m_impl->log.close();
}

string LogFile::getCurrentLogFileName() const {
    return getLogFileName(gc::day_clock::local_day());
}

void LogFile::write(const string &line) {
    boost::lock_guard<boost::mutex> lock(m_impl->mutex);
    const gc::date d = gc::day_clock::local_day();
    const bool open = m_impl->log.is_open();
    if (!open || (d.day() != m_impl->lastDate.day())) {
        if (open) {
            m_impl->log.close();
        }
        m_impl->lastDate = d;
        const string file = getLogFileName(d);
        m_impl->log.open(file.c_str(), ios_base::app);
    }
    const pt::time_duration t = pt::second_clock::local_time().time_of_day();
    const string prefix = pt::to_simple_string(t);
    m_impl->log << "(" << prefix << ") " << line << flush;
}

}
