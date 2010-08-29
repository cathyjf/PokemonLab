/*
 * File:   LogFile.h
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

#ifndef _MAIN_LOG_FILE_H
#define _MAIN_LOG_FILE_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace shoddybattle {

class LogFile : boost::noncopyable {
public:

    LogFile();
    virtual ~LogFile() { }

    /**
     * Write text to the appropriate log for today's date. Adds a prefix for
     * the time, but does not write a newline.
     * This function is thread safe.
     */
    void write(const std::string &);

    /**
     * Get the name of the log file for the current date.
     */
    std::string getCurrentLogFileName() const;

    /**
     * Close a log file if one is open.
     */
    void close();

protected:

    /**
     * This function is called to determined the file name for the log for a
     * particular day.
     */
    virtual std::string getLogFileName(
            const boost::gregorian::date &) const = 0;

private:
    class LogFileImpl;
    boost::shared_ptr<LogFileImpl> m_impl;
};

}

#endif
