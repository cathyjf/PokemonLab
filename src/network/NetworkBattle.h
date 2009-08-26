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
    class Pokemon;
} // namespace shoddybattle

namespace shoddybattle { namespace network {

class Server;
class Client;

class NetworkBattleImpl;

class NetworkBattle : public BattleField,
        public boost::enable_shared_from_this<NetworkBattle> {
public:
    typedef boost::shared_ptr<NetworkBattle> PTR;
    
    NetworkBattle(Server *server,
            boost::shared_ptr<network::Client> *clients,
            Pokemon::ARRAY *teams,
            const GENERATION generation,
            const int partySize,
            const int maxTeamLength,
            const int metagame,
            const bool rated);

    int getParty(boost::shared_ptr<network::Client> client) const;

    void beginBattle();
    void terminate();

    void handleTurn(const int party, const PokemonTurn &turn);
    void handleCancelTurn(const int party);

    Pokemon *requestInactivePokemon(Pokemon *);
    void print(const TextMessage &msg);
    void informVictory(const int);
    void informUseMove(Pokemon *, MoveObject *);
    void informWithdraw(Pokemon *);
    void informSendOut(Pokemon *);
    void informHealthChange(Pokemon *, const int);
    void informSetPp(Pokemon *, const int, const int);
    void informSetMove(Pokemon *, const int, const int, const int, const int);
    void informFainted(Pokemon *);

    int32_t getId() const;

private:
    boost::shared_ptr<NetworkBattleImpl> m_impl;
};

}} // namespace shoddybattle::network
