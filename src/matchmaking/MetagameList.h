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
#include "../network/network.h"

namespace shoddybattle {

class Generation;
class Metagame;
typedef boost::shared_ptr<Metagame> MetagamePtr;
typedef boost::shared_ptr<Generation> GenerationPtr;

class SpeciesDatabase;

class Metagame {
public:
    Metagame();

    std::string getName() const;
    std::string getId() const;
    int getIdx() const;
    std::string getDescription() const;
    Generation *getGeneration() const;
    int getActivePartySize() const;
    int getMaxTeamLength() const;
    const std::set<unsigned int> &getBanList() const;
    const std::vector<std::string> &getClauses() const;
    const network::TimerOptions &getTimerOptions() const;

    void initialise(SpeciesDatabase *);

private:
    class MetagameImpl;
    boost::shared_ptr<MetagameImpl> m_impl;
    friend class Generation;
};

class Generation {
public:
    static void readGenerations(const std::string &,
            std::vector<GenerationPtr> &);

    Generation();
    
    std::string getId() const;
    std::string getName() const;
    int getIdx() const;
    const std::vector<MetagamePtr> &getMetagames() const;
    void getMetagameClauses(const int idx, std::vector<std::string> &clauses);
    void getMetagameBans(const int idx, std::set<unsigned int> &bans);

    void initialiseMetagames(SpeciesDatabase *);
    
private:
    class GenerationImpl;
    boost::shared_ptr<GenerationImpl> m_impl;
};

} // namespace shoddybattle

#endif

