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
#include <vector>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

using namespace std;
using namespace shoddybattle::network;

namespace shoddybattle {

typedef vector<PokemonTurn> PARTY_TURN;
typedef vector<int> PARTY_REQUEST;

struct NetworkBattleImpl {
    NetworkBattle *field;
    vector<Client::PTR> clients;
    vector<PARTY_TURN> turns;
    vector<PARTY_REQUEST> requests;
    boost::mutex mutex;

    void maybeExecuteTurn() {
        const int count = requests.size();
        for (int i = 0; i < count; ++i) {
            if (requests[i].size() != turns[i].size()) {
                return;
            }
        }

        boost::shared_ptr<vector<PokemonTurn> > turn(new vector<PokemonTurn>());
        for (int i = 0; i < count; ++i) {
            PARTY_TURN &v = turns[i];
            turn->insert(turn->end(), v.begin(), v.end());
            v.clear();
            requests[i].clear();
        }
        
        // todo: post the turn pointer to a threaded queue
    }
};

NetworkBattle::NetworkBattle() {
    m_impl = boost::shared_ptr<NetworkBattleImpl>(new NetworkBattleImpl());
    m_impl->field = this;
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
        return;
    }
    pturn.push_back(turn);
    if (pturn.size() < max) {
        // todo: request another move from this client
    } else {
        m_impl->maybeExecuteTurn();
    }
}

}

