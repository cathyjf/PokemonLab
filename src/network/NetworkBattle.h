/* 
 * File:   NetworkBattle.h
 * Author: Catherine
 *
 * Created on April 19, 2009, 1:24 AM
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

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include "../shoddybattle/BattleField.h"

namespace shoddybattle {

namespace network { class Client; }
    
class NetworkBattleImpl;

class NetworkBattle : public BattleField {
public:
    typedef boost::shared_ptr<NetworkBattle> PTR;
    
    NetworkBattle(ScriptMachine *machine,
            boost::shared_ptr<network::Client> *clients,
            Pokemon::ARRAY *teams,
            const GENERATION generation,
            const int partySize);

    int getParty(boost::shared_ptr<network::Client> client) const;

    void beginBattle();

    void handleTurn(const int party, const PokemonTurn &turn);
    void handleCancelTurn(const int party);

    void print(const TextMessage &msg);
    void informVictory(const int);
    void informUseMove(Pokemon *, MoveObject *);
    void informWithdraw(Pokemon *);
    void informSendOut(Pokemon *);
    void informHealthChange(Pokemon *, const int);
    void informFainted(Pokemon *);

    int32_t getId() const { return (int32_t)this; }

private:
    boost::shared_ptr<NetworkBattleImpl> m_impl;
};

}

