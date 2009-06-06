/*
 * File:   PokemonMove.cpp
 * Author: Catherine
 *
 * Created on April 1, 2009, 1:38 PM
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

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <string>
#include <iostream>
#include <map>
#include <vector>

#include "PokemonMove.h"
#include "../mechanics/PokemonType.h"
#include "../scripting/ScriptMachine.h"

using namespace std;
using namespace xercesc;

namespace shoddybattle {

class MoveTemplateImpl {
public:
    MoveTemplateImpl(): targetClass(targetClass) {
        power = 0;
        moveClass = MC_PHYSICAL;
        pp = 0;
        priority = 0;
        flags = 0;
        accuracy = 1.00;
        type = &PokemonType::NORMAL;
    }
    
    string name;
    int id;
    unsigned int power;
    MOVE_CLASS moveClass;
    TARGET targetClass;
    unsigned int pp;
    int priority;
    bitset<FLAG_COUNT> flags;
    double accuracy;
    const PokemonType *type;

    ScriptFunctionPtr initFunction;
    ScriptFunctionPtr useFunction;
    ScriptFunctionPtr attemptHit;
};

/**
 * Note: Probably should put these in a header.
 */
int getIntNodeValue(DOMNode *node);
string getStringNodeValue(DOMNode *node, bool text = false);
string getTextFromElement(DOMElement *element, bool text = false);

/**
 * Methods to get properties of a MoveTemplate.
 */
string MoveTemplate::getName() const {
    return m_pImpl->name;
}
int MoveTemplate::getId() const {
    return m_pImpl->id;
}
MOVE_CLASS MoveTemplate::getMoveClass() const {
    return m_pImpl->moveClass;
}
TARGET MoveTemplate::getTargetClass() const {
    return m_pImpl->targetClass;
}
unsigned int MoveTemplate::getPp() const {
    return m_pImpl->pp;
}
unsigned int MoveTemplate::getPower() const {
    return m_pImpl->power;
}
int MoveTemplate::getPriority() const {
    return m_pImpl->priority;
}
double MoveTemplate::getAccuracy() const {
    return m_pImpl->accuracy;
}
const PokemonType *MoveTemplate::getType() const {
    return m_pImpl->type;
}
bool MoveTemplate::getFlag(const MOVE_FLAG i) const {
    return m_pImpl->flags[i];
}
ScriptFunctionPtr MoveTemplate::getInitFunction() const {
    return m_pImpl->initFunction;
}
ScriptFunctionPtr MoveTemplate::getUseFunction() const {
    return m_pImpl->useFunction;
}
ScriptFunctionPtr MoveTemplate::getAttemptHitFunction() const {
    return m_pImpl->attemptHit;
}

class SpeciesHandler : public HandlerBase {
    void error(const SAXParseException& e) {
        fatalError(e);
    }
    void fatalError(const SAXParseException& e) {
        const XMLFileLoc line = e.getLineNumber();
        const XMLFileLoc column = e.getColumnNumber();
        cout << "Error at (" << line << "," << column << ")." << endl;
        char *message = XMLString::transcode(e.getMessage());
        cout << message << endl;
        XMLString::release(&message);
    }
};

string getElementText(DOMElement *node, const string element, bool text = false) {
    const int len = element.length();
    XMLCh tempStr[len + 1];
    XMLString::transcode(element.c_str(), tempStr, len);
    DOMNodeList *list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        DOMElement *item = (DOMElement *)list->item(0);
        return getTextFromElement(item, text);
    }
    return string();
}

bool hasChildElement(DOMElement *node, const string child) {
    const int len = child.length();
    XMLCh tempStr[len + 1];
    XMLString::transcode(child.c_str(), tempStr, len);
    DOMNodeList *list = node->getElementsByTagName(tempStr);
    return (list->getLength() != 0);
}

void getMove(DOMElement *node, MoveTemplateImpl *pMove,
        ScriptContextPtr cx) {
    DOMNamedNodeMap *attributes = node->getAttributes();
    XMLCh tempStr[20];

    // name
    XMLString::transcode("name", tempStr, 19);
    DOMNode *p = attributes->getNamedItem(tempStr);
    if (p) {
        pMove->name = getStringNodeValue(p);
    }

    // id
    XMLString::transcode("id", tempStr, 19);
    p = attributes->getNamedItem(tempStr);
    if (p) {
        pMove->id = getIntNodeValue(p);
    }

    // type
    string type = getElementText(node, "type");
    if (!type.empty()) {
        pMove->type = PokemonType::getByCanonicalName(type);
    }

    // class
    string cls = getElementText(node, "class");
    if (!cls.empty()) {
        MOVE_CLASS mc;
        if (cls == "Special") {
            mc = MC_SPECIAL;
        } else if (cls == "Physical") {
            mc = MC_PHYSICAL;
        } else if (cls == "Other") {
            mc = MC_OTHER;
        }
        pMove->moveClass = mc;
    }

    // flags
    XMLString::transcode("flags", tempStr, 19);
    DOMNodeList *list = node->getElementsByTagName(tempStr);
    if (list->getLength() != 0) {
        pMove->flags[F_CONTACT] = hasChildElement(node, "contact");
        pMove->flags[F_PROTECT] = hasChildElement(node, "protect");
        pMove->flags[F_FLINCH] = hasChildElement(node, "flinch");
        pMove->flags[F_REFLECT] = hasChildElement(node, "reflect");
        pMove->flags[F_SNATCH] = hasChildElement(node, "snatch");
        pMove->flags[F_MEMORABLE] = hasChildElement(node, "memorable");
        pMove->flags[F_HIGH_CRITICAL] = hasChildElement(node, "high-critical");
        pMove->flags[F_UNIMPLEMENTED] = hasChildElement(node, "unimplemented");
        pMove->flags[F_INTERNAL] = hasChildElement(node, "internal");
        pMove->flags[F_NO_CRITICAL] = hasChildElement(node, "no-critical");
    }

    // power
    string strPower = getElementText(node, "power");
    if (!strPower.empty()) {
        int i = atoi(strPower.c_str());
        pMove->power = i;
    }

    // pp
    string strPp = getElementText(node, "pp");
    if (!strPp.empty()) {
        int i = atoi(strPp.c_str());
        pMove->pp = i;
    }

    // priority
    string strPriority = getElementText(node, "priority");
    if (!strPriority.empty()) {
        int i = atoi(strPriority.c_str());
        pMove->priority = i;
    } else {
        pMove->priority = 0;
    }

    // accuracy
    string strAccuracy = getElementText(node, "accuracy");
    if (!strAccuracy.empty()) {
        double acc = atof(strAccuracy.c_str());
        pMove->accuracy = acc;
    }

    // target
    string strTarget = getElementText(node, "target");
    TARGET tc = T_SINGLE;
    if (strTarget == "Enemy") {
        tc = T_SINGLE;
    } else if (strTarget == "Enemies") {
        tc = T_ENEMIES;
    } else if (strTarget == "User") {
        tc = T_USER;
    } else if (strTarget == "Randenemy") {
        tc = T_RANDOM_ENEMY;
    } else if (strTarget == "Allies") {
        tc = T_ALLIES;
    } else if (strTarget == "Others") {
        tc = T_OTHERS;
    } else if (strTarget == "Lastenemy") {
        tc = T_LAST_ENEMY;
    } else if (strTarget == "All") {
        tc = T_ALL;
    } else if (strTarget == "Enemyfield") {
        tc = T_ENEMY_FIELD;
    } else if (strTarget == "Ally") {
        tc = T_ALLY;
    } else {
        cout << "Unknown target class: " << strTarget << endl;
    }
    pMove->targetClass = tc;

    // init function
    string body = getElementText(node, "init", true);
    if (!body.empty()) {
        string error = pMove->name + "::init";
        vector<string> args;
        pMove->initFunction = cx->compileFunction(args, body, error, 1);
    }

    // use function
    body = getElementText(node, "use", true);
    if (!body.empty()) {
        string error = pMove->name + "::use";
        vector<string> args;
        args.push_back("field");
        args.push_back("user");
        args.push_back("target");
        args.push_back("targets");  // number of targets remaining alive
        pMove->useFunction = cx->compileFunction(args, body, error, 1);
    }

    // attemptHit function
    body = getElementText(node, "attemptHit", true);
    if (!body.empty()) {
        string error = pMove->name + "::attemptHit";
        vector<string> args;
        args.push_back("field");
        args.push_back("user");
        args.push_back("target");
        pMove->attemptHit = cx->compileFunction(args, body, error, 1);
    }

}

MoveTemplate::MoveTemplate(MoveTemplateImpl *p) {
    m_pImpl = p;
}

MoveTemplate::~MoveTemplate() {
    delete m_pImpl;
}

void MoveDatabase::loadMoves(const string file) {
    XMLPlatformUtils::Initialize();
    XercesDOMParser parser;
    //parser.setDoSchema(true);
    //parser.setValidationScheme(AbstractDOMParser::Val_Always);

    SpeciesHandler handler;
    parser.setErrorHandler(&handler);
    parser.setEntityResolver(&handler);

    parser.parse(file.c_str());

    DOMDocument *doc = parser.getDocument();
    DOMElement *root = doc->getDocumentElement();

    XMLCh tempStr[12];
    XMLString::transcode("move", tempStr, 11);
    DOMNodeList *list = root->getElementsByTagName(tempStr);

    ScriptContextPtr cx = m_machine.acquireContext();

    int implemented = 0;
    XMLSize_t length = list->getLength();
    for (int i = 0; i < length; ++i) {
        DOMElement *item = (DOMElement *)list->item(i);
        MoveTemplateImpl *move = new MoveTemplateImpl();
        shoddybattle::getMove(item, move, cx);

        if (!move->flags[F_UNIMPLEMENTED]) {
            ++implemented;
        }

        MoveTemplate *pMove = new MoveTemplate(move);
        m_data.insert(MOVE_DATABASE::value_type(move->name, pMove));
        m_intData.insert(INT_MOVE_DATABASE::value_type(move->id, move->name));
    }
    cout << implemented << " / " << length << " moves implemented." << endl;
}

MoveDatabase::~MoveDatabase() {
    ScriptContextPtr cx = m_machine.acquireContext();
    MOVE_DATABASE::iterator i = m_data.begin();
    for (; i != m_data.end(); ++i) {
        MoveTemplate *move = i->second;
        if (move == NULL) {
            //cout << "Missing move: " << i->first << endl;
            continue;
        }
        delete move;
    }
}

} // namespace shoddybattle

#if 0

#include "../shoddybattle/PokemonSpecies.h"
#include "../shoddybattle/Pokemon.h"
#include "../shoddybattle/Team.h"
#include "../shoddybattle/BattleField.h"
#include "../mechanics/PokemonNature.h"
#include "../mechanics/JewelMechanics.h"

using namespace shoddybattle;

int main() {
    ScriptMachine machine;
    machine.acquireContext()->runFile("resources/main.js");

    SpeciesDatabase *species = machine.getSpeciesDatabase();
    MoveDatabase *moves = machine.getMoveDatabase();

    Pokemon::ARRAY team[2];
    loadTeam("/home/Catherine/randomteam", *species, team[0]);
    loadTeam("/home/Catherine/toxicorb", *species, team[1]);

    const string trainer[] = { "Catherine", "bearzly" };

    BattleField field;
    JewelMechanics mechanics;
    field.initialise(&mechanics, GEN_PLATINUM, &machine, team, trainer, 2);

    field.getActivePokemon(0, 1)->setMove(0, "False Swipe", 5);
    //field.getActivePokemon(0, 1)->setActed();
    field.getActivePokemon(0, 1)->setMove(1, "Mirror Move", 5);
    //field.getActivePokemon(0, 0)->setMove(0, "Last Resort", 5);
    field.getActivePokemon(0, 0)->setMove(0, "Twister", 5);
    field.getActivePokemon(1, 0)->setMove(0, "Copycat", 5);
    field.getActivePokemon(1, 1)->setMove(0, "Mirror Move", 5);

    field.getActivePokemon(0, 0)->setItem("Leftovers");
    field.getActivePokemon(0, 1)->setItem("Leftovers");
    field.getActivePokemon(1, 0)->setItem("Leftovers");
    field.getActivePokemon(1, 1)->setItem("Leftovers");

    field.getActivePokemon(0, 0)->setAbility("Anger Point");
    field.getActivePokemon(0, 1)->setAbility("Anger Point");
    field.getActivePokemon(1, 0)->setAbility("Anger Point");
    field.getActivePokemon(1, 1)->setAbility("Anger Point");

    field.beginBattle();

    vector<PokemonTurn> turns;
    //turns.push_back(PokemonTurn(TT_SWITCH, 5));
    turns.push_back(PokemonTurn(TT_MOVE, 0, 1));
    turns.push_back(PokemonTurn(TT_MOVE, 0, 3));
    turns.push_back(PokemonTurn(TT_MOVE, 0, 0));
    turns.push_back(PokemonTurn(TT_MOVE, 0, 1));

    time_t initial = clock();
    field.processTurn(turns);
    //turns[1] = PokemonTurn(TT_MOVE, 1, 2);
    /**field.processTurn(turns);
    field.processTurn(turns);
    field.processTurn(turns);
    field.processTurn(turns);
    field.processTurn(turns);
    field.processTurn(turns);**/
    time_t final = clock();
    double delta = (double)(final - initial) / (double)CLOCKS_PER_SEC;
    cout << delta << " seconds to process the turn." << endl;

    field.getContext()->gc();
}

#endif

