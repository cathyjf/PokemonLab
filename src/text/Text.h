/* 
 * File:   Text.h
 * Author: Catherine
 *
 * Created on March 28, 2009, 2:23 AM
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

#ifndef _TEXT_H_
#define _TEXT_H_

#include <boost/function.hpp>
#include <string>
#include <map>

namespace shoddybattle {

class SyntaxException {
public:
    SyntaxException(const unsigned int line): m_line(line) { }
    unsigned int getLine() {
        return m_line;
    }
private:
    unsigned int m_line;
};

typedef std::map<int, std::string> INDEX_MAP;
typedef std::map<int, INDEX_MAP> TEXT_MAP;

typedef boost::function<int (std::string)> LOOKUP_FUNCTION;

/**
 * All loading of text is done through this class. Generally speaking, there
 * should not be any string literals floating around the source code.
 */
class Text {
public:
    
    Text() { }

    /**
     * Load text from the string table.
     *
     */
    std::string getText(const int type,
            const int id, const int count, const char **args) const;

    /**
     * Populate the string table by reading a file.
     */
    bool loadFile(const std::string file, LOOKUP_FUNCTION lookup)
            throw(SyntaxException);

private:
    TEXT_MAP m_text;
};

}

#endif
