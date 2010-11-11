/* 
 * File:   MetagameList.cpp
 * Author: Catherine
 *
 * Created on August 21, 2009, 3:17 PM
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

#include "MetagameList.h"
#include "../shoddybattle/PokemonSpecies.h"
#include "../shoddybattle/BattleField.h"
#include "../network/network.h"
#include "../main/Log.h"

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <boost/lexical_cast.hpp>

#include <iostream>

using namespace std;
using namespace xercesc;

namespace shoddybattle {

struct Metagame::MetagameImpl {
    string m_name;
    string m_id;
    int m_idx;
    string m_description;
    Generation *m_generation;
    int m_partySize;
    int m_maxTeamLength;
    set<unsigned int> m_banList;
    set<string> m_banListProto;
    vector<string> m_clauses;
    network::TimerOptions m_timerOptions;

    void getMetagame(DOMElement *);

    void initialise(SpeciesDatabase *species) {
        set<string>::iterator i = m_banListProto.begin();
        for (; i != m_banListProto.end(); ++i) {
            string name = *i;
            const PokemonSpecies *p = species->getSpecies(name);
            if (!p) {
                Log::out() << "Unknown species: " << name << endl;
            } else {
                m_banList.insert(p->getSpeciesId());
            }
        }
    }
};

struct Generation::GenerationImpl {
    Generation *m_owner;
    string m_id;
    string m_name;
    int m_idx;
    vector<MetagamePtr> m_metagames;

    void getGeneration(DOMElement *);

    void getMetagameClauses(const int idx, vector<string> &ret) {
        const int metagamesize = m_metagames.size();
        if ((idx < 0) || (idx > metagamesize)) {
            return;
        }

        MetagamePtr p = m_metagames[idx];
        const vector<string> &clauses = p->getClauses();
        const int size = clauses.size();
        for (int i = 0; i < size; ++i) {
            ret.push_back(clauses[i]);
        }
    }
    
    void getMetagameBans(const int idx, set<unsigned int> &ret) {
        const int metagameSize = m_metagames.size();
        if ((idx < 0) || (idx > metagameSize)) {
            return;
        }

        MetagamePtr p = m_metagames[idx];
        const set<unsigned int> &bans = p->getBanList();
        set<unsigned int>::const_iterator i = bans.begin();
        for (; i != bans.end(); ++i) {
            ret.insert(*i);
        }
    }

    void initialiseMetagames(SpeciesDatabase *species) {
        vector<MetagamePtr>::iterator i = m_metagames.begin();
        for (; i != m_metagames.end(); ++i) {
            (*i)->initialise(species);
        }
    }
};

int getIntNodeValue(DOMNode *node);
string getStringNodeValue(DOMNode *node, bool text = false);
string getTextFromElement(DOMElement *element, bool text = false);

void Metagame::MetagameImpl::getMetagame(DOMElement *node) {
    XMLCh tempStr[20];

    XMLString::transcode("id", tempStr, 19);
    DOMNode *p = node->getAttributes()->getNamedItem(tempStr);
    if (p) {
        m_id = getStringNodeValue(p);
    }

    XMLString::transcode("name", tempStr, 19);
    DOMNodeList *list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        m_name = getTextFromElement((DOMElement *)list->item(0));
    }

    XMLString::transcode("description", tempStr, 19);
    list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        m_description = getTextFromElement((DOMElement *)list->item(0));
    }

    XMLString::transcode("party-size", tempStr, 19);
    list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        const string txt = getTextFromElement((DOMElement *)list->item(0));
        m_partySize = boost::lexical_cast<int>(txt);
    }

    XMLString::transcode("team-length", tempStr, 19);
    list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        const string txt = getTextFromElement((DOMElement *)list->item(0));
        m_maxTeamLength = boost::lexical_cast<int>(txt);
    }

    XMLString::transcode("ban-list", tempStr, 19);
    list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        DOMElement *banList = (DOMElement *)list->item(0);
        XMLString::transcode("pokemon", tempStr, 19);
        list = banList->getElementsByTagName(tempStr);
        const int length = list->getLength();
        for (int i = 0; i < length; ++i) {
            DOMElement *pokemon = (DOMElement *)list->item(i);
            string txt = getTextFromElement(pokemon);
            m_banListProto.insert(txt);
        }
    }

    XMLString::transcode("clauses", tempStr, 19);
    list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        DOMElement *clauses = (DOMElement *)list->item(0);
        XMLString::transcode("clause", tempStr, 19);
        list = clauses->getElementsByTagName(tempStr);
        const int length = list->getLength();
        for (int i = 0; i < length; ++i) {
            DOMElement *clause = (DOMElement *)list->item(i);
            string txt = getTextFromElement(clause);
            m_clauses.push_back(txt);
        }
    }
    
    XMLString::transcode("timer", tempStr, 19);
    list = node->getElementsByTagName(tempStr);
    int pool = -1, periods = -1, periodLength = -1; 
    if (list->getLength() != 0) {
        DOMElement *timerOpts = (DOMElement *)list->item(0);
        XMLString::transcode("pool", tempStr, 19);
        list = timerOpts->getElementsByTagName(tempStr);
        if (list->getLength() != 0) {
            const string txt = getTextFromElement((DOMElement *)list->item(0));
            pool = boost::lexical_cast<int>(txt);
        }
        XMLString::transcode("periods", tempStr, 19);
        list = timerOpts->getElementsByTagName(tempStr);
        if (list->getLength() != 0) {
            const string txt = getTextFromElement((DOMElement *)list->item(0));
            periods = boost::lexical_cast<int>(txt);
        }
        XMLString::transcode("periodLength", tempStr, 19);
        list = timerOpts->getElementsByTagName(tempStr);
        if (list->getLength() != 0) {
            const string txt = getTextFromElement((DOMElement *)list->item(0));
            periodLength = boost::lexical_cast<int>(txt);
        }
    }
    if ((pool > 0) && (periods >= 0) && (periodLength > 0)) {
        m_timerOptions.enabled = true;
        m_timerOptions.pool = pool;
        m_timerOptions.periods = periods;
        m_timerOptions.periodLength = periodLength;
    }
}

void Generation::GenerationImpl::getGeneration(DOMElement *node) {
    XMLCh tempStr[20];

    XMLString::transcode("id", tempStr, 19);
    DOMNode *p = node->getAttributes()->getNamedItem(tempStr);
    if (p) {
        m_id = getStringNodeValue(p);
    }

    XMLString::transcode("name", tempStr, 19);
    DOMNodeList *list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        m_name = getTextFromElement((DOMElement *)list->item(0));
    }

    XMLString::transcode("metagames", tempStr, 19);
    list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        DOMElement *metagames = (DOMElement *)list->item(0);
        XMLString::transcode("metagame", tempStr, 19);
        list = metagames->getElementsByTagName(tempStr);
        const int length = list->getLength();
        for (int i = 0; i < length; ++i) {
            DOMElement *item = (DOMElement *)list->item(i);
            MetagamePtr metagame(new Metagame());
            metagame.get()->m_impl->getMetagame(item);
            metagame.get()->m_impl->m_idx = i;
            metagame.get()->m_impl->m_generation = m_owner;
            m_metagames.push_back(metagame);
        }
    }
}

string Metagame::getName() const {
    return m_impl->m_name;
}

string Metagame::getId() const {
    return m_impl->m_id;
}

int Metagame::getIdx() const {
    return m_impl->m_idx;
}

string Metagame::getDescription() const {
    return m_impl->m_description;
}

Generation *Metagame::getGeneration() const {
    return m_impl->m_generation;
}

int Metagame::getActivePartySize() const {
    return m_impl->m_partySize;
}

int Metagame::getMaxTeamLength() const {
    return m_impl->m_maxTeamLength;
}

const set<unsigned int> &Metagame::getBanList() const {
    return m_impl->m_banList;
}

const vector<string> &Metagame::getClauses() const {
    return m_impl->m_clauses;
}

const network::TimerOptions &Metagame::getTimerOptions() const {
    return m_impl->m_timerOptions;
}

void Metagame::initialise(SpeciesDatabase *species) {
    m_impl->initialise(species);
}

Metagame::Metagame():
        m_impl(boost::shared_ptr<MetagameImpl>(new MetagameImpl())) { }

Generation::Generation():
        m_impl(boost::shared_ptr<GenerationImpl>(new GenerationImpl())) { }

class GenerationErrorHandler : public HandlerBase {
    void error(const SAXParseException& e) {
        fatalError(e);
    }
    void fatalError(const SAXParseException& e) {
        const XMLFileLoc line = e.getLineNumber();
        const XMLFileLoc column = e.getColumnNumber();
        Log::out() << "Error at (" << line << "," << column << ")." << endl;
        char *message = XMLString::transcode(e.getMessage());
        Log::out() << message << endl;
        XMLString::release(&message);
    }
};

void Generation::readGenerations(const string &file,
        vector<GenerationPtr> &generations) {
    XMLPlatformUtils::Initialize();
    XercesDOMParser parser;

    GenerationErrorHandler handler;
    parser.setErrorHandler(&handler);
    parser.setEntityResolver(&handler);

    parser.parse(file.c_str());

    DOMDocument *doc = parser.getDocument();
    DOMElement *root = doc->getDocumentElement();

    XMLCh tempStr[12];
    XMLString::transcode("generation", tempStr, 11);
    DOMNodeList *list = root->getElementsByTagName(tempStr);

    int length = list->getLength();
    for (int i = 0; i < length; ++i) {
        DOMElement *item = (DOMElement *)list->item(i);
        GenerationPtr generation(new Generation());
        generation->m_impl->m_owner = generation.get();
        generation->m_impl->getGeneration(item);
        generation->m_impl->m_idx = i;
        generations.push_back(generation);
    }
}

string Generation::getId() const {
    return m_impl->m_id;
}

string Generation::getName() const {
    return m_impl->m_name;
}

int Generation::getIdx() const {
    return m_impl->m_idx;
}

const vector<MetagamePtr> &Generation::getMetagames() const {
    return m_impl->m_metagames;
}

void Generation::getMetagameClauses(const int meta, vector<string> &clauses) {
    m_impl->getMetagameClauses(meta, clauses);
}

void Generation::getMetagameBans(const int metagame, set<unsigned int> &bans) {
    m_impl->getMetagameBans(metagame, bans);
}

void Generation::initialiseMetagames(SpeciesDatabase *species) {
    m_impl->initialiseMetagames(species);
}

}

