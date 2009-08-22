/*
 * File:   MetagameList.h
 * Author: Catherine
 *
 * Created on August 21, 2009, 2:44 PM
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

#ifndef _METAGAMELIST_H_
#define	_METAGAMELIST_H_

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <set>

namespace shoddybattle {

class Metagame;
typedef boost::shared_ptr<Metagame> MetagamePtr;

class SpeciesDatabase;

class Metagame {
public:
    static void readMetagames(const std::string &, SpeciesDatabase *,
            std::vector<MetagamePtr> &);

    Metagame();

    std::string getName() const;
    std::string getId() const;
    std::string getDescription() const;
    const std::set<unsigned int> &getBanList() const;
    const std::vector<std::string> &getClauses() const;

private:
    class MetagameImpl;
    boost::shared_ptr<MetagameImpl> m_impl;
};

} // namespace shoddybattle

#endif

