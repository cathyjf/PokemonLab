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
    int m_generation;
    int m_partySize;
    int m_maxTeamLength;
    set<unsigned int> m_banList;
    vector<string> m_clauses;
    network::TimerOptions m_timerOptions;

    void getMetagame(SpeciesDatabase *, DOMElement *);
};

int getIntNodeValue(DOMNode *node);
string getStringNodeValue(DOMNode *node, bool text = false);
string getTextFromElement(DOMElement *element, bool text = false);

void Metagame::MetagameImpl::getMetagame(SpeciesDatabase *species,
        DOMElement *node) {
    XMLCh tempStr[20];

    XMLString::transcode("name", tempStr, 19);
    DOMNodeList *list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        m_name = getTextFromElement((DOMElement *)list->item(0));
    }

    XMLString::transcode("id", tempStr, 19);
    list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        m_id = getTextFromElement((DOMElement *)list->item(0));
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
            const PokemonSpecies *p = species->getSpecies(txt);
            if (!p) {
                Log::out() << "Unknown species: " << txt << endl;
            } else {
                m_banList.insert(p->getSpeciesId());
            }
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

    m_generation = GEN_PLATINUM; // TODO
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

int Metagame::getGeneration() const {
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

class MetagameErrorHandler : public HandlerBase {
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

Metagame::Metagame():
        m_impl(boost::shared_ptr<MetagameImpl>(new MetagameImpl())) { }

void Metagame::readMetagames(const string &file, SpeciesDatabase *species,
        vector<MetagamePtr> &metagames) {
    XMLPlatformUtils::Initialize();
    XercesDOMParser parser;

    MetagameErrorHandler handler;
    parser.setErrorHandler(&handler);
    parser.setEntityResolver(&handler);

    parser.parse(file.c_str());

    DOMDocument *doc = parser.getDocument();
    DOMElement *root = doc->getDocumentElement();

    XMLCh tempStr[12];
    XMLString::transcode("metagame", tempStr, 11);
    DOMNodeList *list = root->getElementsByTagName(tempStr);

    int length = list->getLength();
    for (int i = 0; i < length; ++i) {
        DOMElement *item = (DOMElement *)list->item(i);
        MetagamePtr metagame(new Metagame());
        metagame.get()->m_impl->getMetagame(species, item);
        metagame.get()->m_impl->m_idx = i;
        metagames.push_back(metagame);
    }
}

}

