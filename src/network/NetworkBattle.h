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

class Timer {
public:
    Timer() : m_enabled(false) { };
    Timer(const int pool, const int periods, const int periodLength, 
            NetworkBattle *battle) :
            m_enabled(true),
            m_pool(pool),
            m_periods(periods),
            m_periodLength(periodLength),
            m_battle(battle) { };
    void tick();
    bool isEnabled() { return m_enabled; }
private:
    bool m_enabled;
    int m_pool;
    int m_periods;
    int m_periodLength;
    NetworkBattle *m_battle;
};

class NetworkBattle : public BattleField,
        public boost::enable_shared_from_this<NetworkBattle> {
public:
    typedef boost::shared_ptr<NetworkBattle> PTR;
    
    static void startTimerThread();
    static void addTimer(Timer *t);
    static void removeTimer(Timer *t);
    
    NetworkBattle(Server *server,
            boost::shared_ptr<network::Client> *clients,
            Pokemon::ARRAY *teams,
            const GENERATION generation,
            const int partySize,
            const int maxTeamLength,
            std::vector<StatusObject> &clauses,
            TimerOptions t,
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
    void informStatusChange(Pokemon *, StatusObject *, const bool);

    int32_t getId() const;
    
private:
    boost::shared_ptr<NetworkBattleImpl> m_impl;
    static boost::thread m_timerThread;
    static std::list<Timer *> m_timerList;
    static boost::recursive_mutex m_timerMutex;
    static void handleTiming();
};

}} // namespace shoddybattle::network

#endif
