/* 
 * File:   Text.cpp
 * Author: Catherine
 *
 * Created on March 28, 2009, 3:33 PM
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

#include "Text.h"
#include <fstream>
#include <string>
#include <sstream>
#include <cstdarg>

using namespace std;

namespace shoddybattle {

const StringCategory Text::m_categories[] = {
    StringCategory(SC_TYPE, "types"),
    StringCategory(SC_NATURE, "natures")
};

/** Get a category by name. **/
STRING_CATEGORY Text::getCategory(const string name) {
    int length = sizeof(m_categories) / sizeof(m_categories[0]);
    for (int i = 0; i < length; ++i) {
        const StringCategory &c = m_categories[i];
        if (c.getName() == name) {
            return c.getCategory();
        }
    }
    return SC_NONE;
}

/** Trim a string. **/
string trim(string &s, const string &space = " ") {
    string r = s.erase(s.find_last_not_of(space) + 1);
    return r.erase(0, r.find_first_not_of(space));
}

/**
 * Load a string from the table.
 */
string Text::getText(const STRING_CATEGORY type, const int id,
        const int count, ...) const {
    TEXT_MAP::const_iterator itr = m_text.find(type);
    if (itr == m_text.end()) {
        return string();
    }
    const INDEX_MAP &j = itr->second;
    INDEX_MAP::const_iterator k = j.find(id);
    if (k == j.end()) {
        return string();
    }
    string text = k->second;

    // Replace $n entities in the string.
    va_list list;
    va_start(list, count);
    for (int i = 1; i <= count; ++i) {
        const char *p = va_arg(list, const char *);
        stringstream ss;
        ss << "$" << i;
        string str = ss.str();
        size_t pos = text.find(str);
        if (pos != string::npos) {
            text.replace(pos, str.length(), p);
        }
    }
    va_end(list);

    return text;
}

/**
 * Load a language file.
 */
bool Text::loadFile(const string path) throw(SyntaxException) {
    ifstream ifs(path.c_str());
    if (!ifs.is_open()) {
        return false;
    }
    int lineNumber = 0;
    STRING_CATEGORY category = SC_NONE;
    while (!ifs.eof()) {
        string line;
        getline(ifs, line);
        ++lineNumber;

        /** Check for // comments. **/
        size_t comment = line.find("//");
        if (comment != string::npos) {
            if (comment == 0) {
                // Whole line is a comment.
                continue;
            }
            line = line.substr(0, comment);
        }

        /** Check for empty line. **/
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        /** Check for new section. **/
        if (line[0] == '[') {
            size_t pos = line.find_last_of(']');
            if (pos == string::npos) {
                throw SyntaxException(lineNumber);
            }
            string header = line.substr(1, pos - 1);
            category = getCategory(header);
            if (category == SC_NONE) {
                throw SyntaxException(lineNumber);
            }
            continue;
        }

        if (category == SC_NONE) {
            throw SyntaxException(lineNumber);
        }

        /** There should be a colon on this line. **/
        size_t pos = line.find_first_of(':');
        if (pos == string::npos) {
            throw SyntaxException(lineNumber);
        }

        string index = line.substr(0, pos);
        istringstream iss(index);
        unsigned int idx;
        if ((iss >> dec >> idx).fail()) {
            throw SyntaxException(lineNumber);
        }

        string text = line.substr(pos + 1);
        m_text[category][idx] = trim(text);

    }
}

}

