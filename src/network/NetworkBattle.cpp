/* 
 * File:   NetworkBattle.cpp
 * Author: Catherine
 *
 * Created on April 19, 2009, 1:18 AM
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

#include "NetworkBattle.h"
#include "network.h"
#include "../mechanics/JewelMechanics.h"
#include <vector>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>

using namespace std;
using namespace shoddybattle::network;

namespace shoddybattle {

template <class T>
class ThreadedQueue {
public:
    typedef boost::function<void (T &)> DELEGATE;

    ThreadedQueue(DELEGATE delegate):
            m_delegate(delegate),
            m_empty(true),
            m_terminated(false),
            m_thread(boost::bind(&ThreadedQueue::process, this)) { }

    void post(T elem) {
        boost::unique_lock<boost::mutex> lock(m_mutex);
        while (!m_empty) {
            m_condition.wait(lock);
        }
        m_item = elem;
        m_empty = false;
        lock.unlock();
        m_condition.notify_one();
    }

    void terminate() {
        boost::unique_lock<boost::mutex> lock(m_mutex);
        if (!m_terminated) {
            while (!m_empty) {
                m_condition.wait(lock);
            }
            m_thread.interrupt();
            m_terminated = true;
        }
    }

    ~ThreadedQueue() {
        terminate();
    }

private:

    void process() {
        boost::unique_lock<boost::mutex> lock(m_mutex);
        while (true) {
            while (m_empty) {
                m_condition.wait(lock);
            }
            m_delegate(m_item);
            m_empty = true;
            m_condition.notify_one();
        }
    }

    bool m_terminated;
    bool m_empty;
    DELEGATE m_delegate;
    T m_item;
    boost::mutex m_mutex;
    boost::condition_variable m_condition;
    boost::thread m_thread;
};

typedef vector<PokemonTurn> PARTY_TURN;
typedef vector<int> PARTY_REQUEST;
typedef boost::shared_ptr<vector<PokemonTurn> > TURN_PTR;

struct NetworkBattleImpl {
    JewelMechanics mech;
    NetworkBattle *field;
    vector<Client::PTR> clients;
    vector<PARTY_TURN> turns;
    vector<PARTY_REQUEST> requests;
    ThreadedQueue<TURN_PTR> queue;
    boost::mutex mutex;
    bool replacement;

    NetworkBattleImpl():
            queue(boost::bind(&NetworkBattleImpl::executeTurn,
                    this, _1)),
            replacement(false) { }

    ~NetworkBattleImpl() {
        // we need to make sure the queue gets deconstructed first, since
        // its thread can reference this object
        queue.terminate();
    }
    
    void executeTurn(TURN_PTR &ptr) {
        if (replacement) {
            field->processReplacements(*ptr);
        } else {
            field->processTurn(*ptr);
        }
        if (!requestReplacements()) {
            requestMoves();
        }
    }

    void cancelAction(const int party) {
        PARTY_TURN &turn = turns[party];
        if (requests[party].size() == turn.size()) {
            // too late to cancel
            return;
        }

        turn.pop_back();
        requestAction(party);
    }

    /**
     * BATTLE_BEGIN
     *
     * int32 : field id
     * string : opponent
     * byte : party
     */
    void sendBattleBegin(const int party) {
        const int32_t id = field->getId();
        const string opponent = clients[1 - party]->getName();

        OutMessage msg(OutMessage::BATTLE_BEGIN);
        msg << id << opponent << ((unsigned char)party);
        msg.finalise();
        clients[party]->sendMessage(msg);
    }

    /**
     * BATTLE_POKEMON
     *
     * int32 : field id
     * for 0...1:
     *     for 0...n-1:
     *         int16 : species id
     *         if id != -1:
     *             byte : gender
     *             byte : whether the pokemon is shiny
     */
    void updateBattlePokemon() {
        OutMessage msg(OutMessage::BATTLE_POKEMON);
        msg << (int32_t)field->getId();

        const int size = field->getPartySize();
        boost::shared_ptr<PokemonParty> *active = field->getActivePokemon();
        for (int i = 0; i < TEAM_COUNT; ++i) {
            PokemonParty &party = *active[i];
            for (int j = 0; j < size; ++j) {
                Pokemon::PTR p = party[j].pokemon;
                if (p && !p->isFainted()) {
                    int16_t species = (int16_t)p->getSpeciesId();
                    msg << species;
                    msg << (unsigned char)p->getGender();
                    msg << (unsigned char)p->isShiny();
                } else {
                    msg << ((int16_t)-1);
                }
            }
        }
        msg.finalise();

        boost::lock_guard<boost::mutex> lock(mutex);
        vector<Client::PTR>::iterator i = clients.begin();
        for (; i != clients.end(); ++i) {
            (*i)->sendMessage(msg);
        }
    }

    /**
     * REQUEST_ACTION
     *
     * int32 : field id
     * byte  : slot of relevant pokemon
     * byte  : position of relevant pokemon
     * byte  : whether this is a replacement
     * int32 : number of pokemon
     * for each pokemon:
     *      byte : whether it is legal to switch to this pokemon
     * if not replacement:
     *      byte : whether switching is legal
     *      byte : whether there is a forced move
     *      if not forced:
     *          int32 : total number of moves
     *          for each move:
     *              byte : whether the move is legal
     */
    void requestAction(const int party) {
        const int slot = requests[party][turns[party].size()];
        Pokemon::PTR p = field->getActivePokemon(party, slot);
        
        OutMessage msg(OutMessage::REQUEST_ACTION);
        msg << field->getId();
        msg << (unsigned char)(p->getSlot());
        msg << (unsigned char)(p->getPosition());
        msg << (unsigned char)replacement;

        vector<bool> switches;
        field->getLegalSwitches(p.get(), switches);
        const int switchSize = switches.size();

        msg << switchSize;
        for (int i = 0; i < switchSize; ++i) {
            msg << ((unsigned char)switches[i]);
        }

        if (!replacement) {
            msg << (unsigned char)p->isSwitchLegal();

            const bool forced = (p->getForcedTurn() != NULL);
            msg << (unsigned char)forced;

            if (!forced) {
                const int moveCount = p->getMoveCount();
                msg << moveCount;
                for (int i = 0; i < moveCount; ++i) {
                    msg << (unsigned char)p->isMoveLegal(i);
                }
            }
        }
        msg.finalise();

        Client::PTR client = clients[party];
        client->sendMessage(msg);
    }

    bool requestReplacements() {
        boost::lock_guard<boost::mutex> lock(mutex);

        Pokemon::ARRAY pokemon;
        field->getFaintedPokemon(pokemon);
        if (pokemon.empty()) {
            return false;
        }
        // todo: check if there are actually enough pokemon to replace all
        // of the fainted pokemon
        replacement = true;
        for (Pokemon::ARRAY::const_iterator i = pokemon.begin();
                i != pokemon.end(); ++i) {
            const int party = (*i)->getParty();
            const int slot = (*i)->getSlot();
            requests[party].push_back(slot);
        }
        for (int i = 0; i < TEAM_COUNT; ++i) {
            if (!requests[i].empty()) {
                requestAction(i);
            }
        }
        return true;
    }

    void requestMoves() {
        boost::lock_guard<boost::mutex> lock(mutex);

        replacement = false;
        Pokemon::ARRAY pokemon;
        field->getActivePokemon(pokemon);
        for (Pokemon::ARRAY::const_iterator i = pokemon.begin();
                i != pokemon.end(); ++i) {
            const int party = (*i)->getParty();
            const int slot = (*i)->getSlot();
            (*i)->determineLegalActions();
            requests[party].push_back(slot);
        }
        for (int i = 0; i < TEAM_COUNT; ++i) {
            requestAction(i);
        }
    }

    void maybeExecuteTurn() {
        const int count = requests.size();
        for (int i = 0; i < count; ++i) {
            if (requests[i].size() != turns[i].size()) {
                return;
            }
        }

        TURN_PTR turn(new vector<PokemonTurn>());
        for (int i = 0; i < count; ++i) {
            PARTY_TURN &v = turns[i];
            turn->insert(turn->end(), v.begin(), v.end());
            v.clear();
            requests[i].clear();
        }
        
        queue.post(turn);
    }
};

NetworkBattle::NetworkBattle(ScriptMachine *machine,
        Client::PTR *clients,
        Pokemon::ARRAY *teams,
        const GENERATION generation,
        const int partySize) {
    m_impl = boost::shared_ptr<NetworkBattleImpl>(new NetworkBattleImpl());
    m_impl->field = this;
    m_impl->turns.resize(TEAM_COUNT);
    m_impl->requests.resize(TEAM_COUNT);
    string trainer[TEAM_COUNT];
    for (int i = 0; i < TEAM_COUNT; ++i) {
        Client::PTR client = clients[i];
        trainer[i] = client->getName();
        m_impl->clients.push_back(client);
    }
    BattleField::initialise(&m_impl->mech,
            generation, machine, teams, trainer, partySize);
}

void NetworkBattle::beginBattle() {
    boost::unique_lock<boost::mutex> lock(m_impl->mutex);
    for (int i = 0; i < TEAM_COUNT; ++i) {
        m_impl->sendBattleBegin(i);
    }
    lock.unlock();
    BattleField::beginBattle();
    m_impl->requestMoves();
}

int NetworkBattle::getParty(boost::shared_ptr<network::Client> client) const {
    boost::lock_guard<boost::mutex> lock(m_impl->mutex);
    int count = m_impl->clients.size();
    if (count > 2) {
        count = 2;
    }
    for (int i =  0; i < count; ++i) {
        if (m_impl->clients[i] == client)
            return i;
    }
    return -1;
}

void NetworkBattle::handleCancelTurn(const int party) {
    boost::lock_guard<boost::mutex> lock(m_impl->mutex);
    m_impl->cancelAction(party);
}

void NetworkBattle::handleTurn(const int party, const PokemonTurn &turn) {
    boost::lock_guard<boost::mutex> lock(m_impl->mutex);
    
    PARTY_REQUEST &req = m_impl->requests[party];
    PARTY_TURN &pturn = m_impl->turns[party];
    const int max = req.size();
    const int present = pturn.size();
    if (present == max) {
        return;
    }
    const int slot = req[present];
    Pokemon *pokemon = getActivePokemon(party, slot).get();
    if (!isTurnLegal(pokemon, &turn)) {
        // todo: inform illegal move?
        m_impl->requestAction(party);
        return;
    }
    pturn.push_back(turn);
    if (pturn.size() < max) {
        m_impl->requestAction(party);
    } else {
        m_impl->maybeExecuteTurn();
    }
}

void NetworkBattle::print(const TextMessage &msg) {
    BattleField::print(msg);
}

void NetworkBattle::informVictory(const int party) {
    BattleField::informVictory(party);
}

void NetworkBattle::informUseMove(Pokemon *pokemon, MoveObject *move) {
    BattleField::informUseMove(pokemon, move);
}

void NetworkBattle::informWithdraw(Pokemon *pokemon) {
    BattleField::informWithdraw(pokemon);
}

void NetworkBattle::informSendOut(Pokemon *pokemon) {
    BattleField::informSendOut(pokemon);
    m_impl->updateBattlePokemon();
}

void NetworkBattle::informHealthChange(Pokemon *pokemon, const int delta) {
    BattleField::informHealthChange(pokemon, delta);
}

void NetworkBattle::informFainted(Pokemon *pokemon) {
    BattleField::informFainted(pokemon);
    m_impl->updateBattlePokemon();
}

}

