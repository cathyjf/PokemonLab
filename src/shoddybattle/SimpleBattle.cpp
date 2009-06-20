/* 
 * File:   SimpleBattle.cpp
 * Author: Catherine
 *
 * Created on April 18, 2009, 4:42 AM
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

/**
 * A simple one player interactive battle implementation, useful for testing.
 */

#include "../moves/PokemonMove.h"
#include "../mechanics/PokemonType.h"
#include "../scripting/ScriptMachine.h"
#include "../shoddybattle/PokemonSpecies.h"
#include "../shoddybattle/Pokemon.h"
#include "../shoddybattle/Team.h"
#include "../shoddybattle/BattleField.h"
#include "../mechanics/PokemonNature.h"
#include "../mechanics/JewelMechanics.h"

using namespace std;
using namespace boost;

namespace shoddybattle {

class SimpleBattle : public BattleField {
public:
    void getTurnInput(vector<PokemonTurn> &turns) {
        ScriptContext *cx = getContext();
        turns.clear();
        vector<Pokemon::PTR> pokemon;
        getActivePokemon(pokemon);
        vector<Pokemon::PTR>::iterator i = pokemon.begin();
        for (; i != pokemon.end(); ++i) {
            Pokemon::PTR p = *i;
            p->determineLegalActions();
            while (true) {
                cout << "What will " << p->getName() << " do?" << endl;
                cout << "    1 - Attack" << endl;
                const bool canSwitch = p->isSwitchLegal();
                if (canSwitch) {
                    cout << "    2 - Switch" << endl;
                }
                int option;
                cin >> option;
                if (option == 1) {
                    cout << "Use which of " << p->getName()
                            << "'s moves?" << endl;
                    const int count = p->getMoveCount();
                    const char *num[] = { "1", "2", "3", "4", "5", "6" };
                    bool legal[] = { false, false, false, false };
                    for (int j = 0; j < count; ++j) {
                        if (legal[j] = p->isMoveLegal(j)) {
                            cout << "    " << num[j] << " - "
                                    << p->getMove(j)->getName(cx)
                                    << endl;
                        }
                    }
                    cin >> option;
                    if (option < 1) option = 1;
                    if (option > 4) option = 4;
                    const int move = option - 1;
                    if (legal[option]) {
                        TARGET t = p->getMove(move)->getTargetClass(cx);
                        int target = -1;
                        if (t == T_NONUSER) {
                            cout << "Target which enemy?" << endl;
                            int pt = 1 - p->getParty();
                            PokemonParty &party = *getActivePokemon()[pt];
                            const int size = party.getSize();
                            for (int k = 0; k < size; ++k) {
                                cout << "    " << num[k] << " - "
                                        << party[k].pokemon->getName()
                                        << endl;
                            }
                            cin >> option;
                            if (option < 1) option = 1;
                            if (option > size) option = size;
                            target = option - 1;
                            if (pt == 1) {
                                target += size;
                            }
                        }
                        turns.push_back(PokemonTurn(TT_MOVE, move, target));
                        break;
                    }
                } else if (canSwitch && (option == 2)) {
                    cout << "Not done yet. Try attack." << endl;
                }
            }
        }
    }
};

}

#if 0

using namespace shoddybattle;

int main() {
    ScriptMachine machine;
    ScriptContext *cx = machine.acquireContext();
    cx->runFile("resources/main.js");
    cx->runFile("resources/moves.js");
    cx->runFile("resources/constants.js");
    cx->runFile("resources/StatusEffect.js");
    cx->runFile("resources/statuses.js");
    cx->runFile("resources/abilities.js");
    machine.releaseContext(cx);

    SpeciesDatabase *species = machine.getSpeciesDatabase();
    MoveDatabase *moves = machine.getMoveDatabase();

    Pokemon::ARRAY team[2];
    loadTeam("/home/Catherine/randomteam", *species, team[0]);
    loadTeam("/home/Catherine/toxicorb", *species, team[1]);

    const string trainer[] = { "Catherine", "bearzly" };

    SimpleBattle field;
    JewelMechanics mechanics;
    field.initialise(&mechanics, GEN_PLATINUM, &machine, team, trainer, 2);

    field.getActivePokemon(0, 1)->setMove(0, "Quick Attack", 5);
    //field.getActivePokemon(0, 0)->setMove(0, "Last Resort", 5);
    field.getActivePokemon(0, 0)->setMove(1, "Mirror Move", 5);
    field.getActivePokemon(1, 0)->setMove(0, "Metal Burst", 5);
    field.getActivePokemon(1, 1)->setMove(0, "Counter", 5);

    field.beginBattle();
    vector<PokemonTurn> turns;
    field.getTurnInput(turns);
    field.processTurn(turns);
}

#endif
