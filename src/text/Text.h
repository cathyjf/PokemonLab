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

#include <string>
#include <map>

namespace shoddybattle {

enum STRING_CATEGORY {
    SC_NONE = -1,
    SC_TYPE = 0,
    SC_NATURE
};

class SyntaxException {
public:
    SyntaxException(const unsigned int line): m_line(line) { }
    unsigned int getLine() {
        return m_line;
    }
private:
    unsigned int m_line;
};

class StringCategory {
public:
    StringCategory(const STRING_CATEGORY category, std::string name):
        m_category(category), m_name(name) { }
    STRING_CATEGORY getCategory() const {
        return m_category;
    }
    std::string getName() const {
        return m_name;
    }
private:
    STRING_CATEGORY m_category;
    std::string m_name;
};

typedef std::map<int, std::string> INDEX_MAP;
typedef std::map<STRING_CATEGORY, INDEX_MAP> TEXT_MAP;

/**
 * All loading of text is done through this class. Generally speaking, there
 * should not be any string literals floating around the source code.
 */
class Text {
public:

    /**
     * Construct a new Text object by loading a file.
     */
    Text(std::string file) throw(SyntaxException) {
        loadFile(file);
    }

    /**
     * Load text from the string table.
     *
     * The first argument is the type of sting to load (e.g. nature, type,
     * etc.). The second argument is the index of the string of that type.
     * The additional arguments are strings (specified as char *) that
     * parameterise the desired text. The number of additional arguments must
     * be provided (in the 'count' parameter).
     *
     * For example, if "X's attack missed!" has type BATTLE_MESSAGE and
     * an id of MISS_MESSAGE then we might call
     *
     * getText(BATTLE_MESSAGE, MISS_MESSAGE, p.getName().c_str());
     *
     * Notice the c_str() call. The additional arguments must be of type char *
     * not of type std::string.
     *
     */
    std::string getText(const STRING_CATEGORY type,
            const int id, const int count = 0, ...) const;

    /**
     * Populate the string table by reading a file.
     */
    bool loadFile(const std::string file) throw(SyntaxException);

private:
    static STRING_CATEGORY getCategory(const std::string name);

    TEXT_MAP m_text;
    static const StringCategory m_categories[];
};

}

#endif
