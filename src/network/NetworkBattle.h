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

#ifndef _NETWORK_BATTLE_H_
#define _NETWORK_BATTLE_H_

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <list>
#include "../shoddybattle/BattleField.h"
#include "../network/network.h"

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
    
    static void startTimerThread();
    
    NetworkBattle(Server *server,
            boost::shared_ptr<network::Client> *clients,
            Pokemon::ARRAY *teams,
            Generation *generation,
            const int partySize,
            const int maxTeamLength,
            std::vector<StatusObject> &clauses,
            TimerOptions t,
            const int metagame,
            const bool rated,
            boost::shared_ptr<void> &monitor);

    int32_t getId() const;
    int getParty(boost::shared_ptr<network::Client> client) const;
    void beginBattle();
    void handleTurn(const int party, const PokemonTurn &turn);
    void handleCancelTurn(const int party);
    
private:
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
    void informStatusChange(Pokemon *, StatusObject *, const bool);
    void terminate();
    
    friend class NetworkBattleImpl;
    boost::shared_ptr<NetworkBattleImpl> m_impl;
};

}} // namespace shoddybattle::network

#endif
