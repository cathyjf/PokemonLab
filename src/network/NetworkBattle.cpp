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

#include <vector>
#include <cmath>
#include <ctime>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "NetworkBattle.h"
#include "ThreadedQueue.h"
#include "network.h"
#include "Channel.h"
#include "../mechanics/JewelMechanics.h"
#include "../scripting/ScriptMachine.h"

using namespace std;

namespace shoddybattle { namespace network {

class NetworkBattleImpl;
class BattleChannel;

typedef boost::shared_ptr<BattleChannel> BattleChannelPtr;

/**
 * In Shoddy Battle 2, every battle is also a channel. The participants in
 * the battle are initially granted +o. Anybody with +o or higher on the main
 * chat is granted +ao in every battle.
 *
 * The initial participants of a battle join the battle directly, but all
 * spectators join the channel rather than the underlying battle. When the
 * NetworkBattle broadcasts a message, it is sent to everybody in the channel.
 * Indeed, most features related to battles are actually associated to the
 * channel.
 *
 * The name of the channel contains the participants in the battle; the topic
 * encodes the ladder, if any, on which the battle is taking place and
 * possibly other metadata.
 *
 * When the channel becomes completely empty, or if a set amount of time
 * passes without another message being posted, the channel is destroyed.
 */
class BattleChannel : public Channel {
public:

    static BattleChannelPtr createChannel(Server *server,
            NetworkBattleImpl *field) {
        BattleChannelPtr p(new BattleChannel(server, field,
                string(), string(),
                CHANNEL_FLAGS()));
        p->setName(boost::lexical_cast<string>(p->getId()));
        return p;
    }

    Type::TYPE getChannelType() const {
        return Type::BATTLE;
    }

    void commitStatusFlags(ClientPtr /*client*/, FLAGS /*flags*/) {
        // does nothing in a BattleChannel
    }

    bool join(ClientPtr client);

    FLAGS handleJoin(ClientPtr client);

    void handlePart(ClientPtr client);

    bool handleBan() const {
        // Banned users cannot join a battle.
        return true;
    }

    void handleFinalise() {
        m_server->removeChannel(shared_from_this());
    }

    void informBattleTerminated() {
        m_field = NULL;
    }

    boost::recursive_mutex &getMutex() {
        return m_mutex;
    }

private:
    BattleChannel(Server *server,
            NetworkBattleImpl *field,
            const string &name,
            const string &topic,
            CHANNEL_FLAGS flags):
                Channel(server, name, topic, flags),
                m_server(server),
                m_field(field) { }

    Server *m_server;
    NetworkBattleImpl *m_field;
    boost::recursive_mutex m_mutex;
};

typedef vector<PokemonTurn> PARTY_TURN;
typedef vector<int> PARTY_REQUEST;
typedef boost::shared_ptr<PARTY_TURN> TURN_PTR;


struct PlayerTimer {
    bool stopped;
    int remaining;
    int periods;
    boost::shared_mutex mutex;
};

class Timer {
public:
    Timer() : m_enabled(false) { };
    Timer(const int pool, const int periods, const int periodLength, 
            NetworkBattleImpl *battle) :
            m_enabled(true),
            m_pool(pool),
            m_periods(periods),
            m_periodLength(periodLength),
            m_battle(battle) { 
                m_lastTime = time(NULL);
                for (int i = 0; i < 2; ++i) {
                    m_players[i].remaining = pool;
                    m_players[i].periods = periods;
                    m_players[i].stopped = false;
                }
            };
    bool isEnabled() { return m_enabled; }
    int getPeriods() { return m_periods; }
    void tick();
    void startTimer(const int party);
    void stopTimer(const int party);
    int getRemaining(const int party) {
        boost::unique_lock<boost::shared_mutex> lock(m_players[party].mutex);
        return m_players[party].remaining;
    }
    int getPeriods(const int party) {
        boost::unique_lock<boost::shared_mutex> lock(m_players[party].mutex);
        return m_players[party].periods;
    }
    void beginTicking() {
        for (int i = 0; i < 2; ++i) {
            boost::unique_lock<boost::shared_mutex> lock(m_players[i].mutex);
            m_players[i].stopped = false;
        }
    }
private:
    bool m_enabled;
    int m_pool;
    int m_periods;
    int m_periodLength;
    time_t m_lastTime;
    NetworkBattleImpl *m_battle;
    PlayerTimer m_players[2];
};

typedef boost::shared_ptr<Timer> TimerPtr;

struct NetworkBattleImpl {
    Server *m_server;
    JewelMechanics m_mech;
    NetworkBattle *m_field;
    BattleChannelPtr m_channel;
    vector<string> m_trainer;
    int m_maxTeamLength;
    int m_metagame;
    bool m_rated;
    vector<ClientPtr> m_clients;
    vector<PARTY_TURN> m_turns;
    vector<PARTY_REQUEST> m_requests;
    ThreadedQueue<TURN_PTR> m_queue;
    boost::recursive_mutex m_mutex;
    bool m_replacement;
    bool m_victory;
    int m_turnCount;
    boost::unique_lock<boost::recursive_mutex> *m_lock;
    bool m_waiting;
    Pokemon *m_selection;
    boost::condition_variable_any m_condition;
    bool m_terminated;
    boost::shared_ptr<Timer> m_timer;
    static list<TimerPtr> m_timerList;
    static boost::recursive_mutex m_timerMutex;
    static boost::thread m_timerThread;

    NetworkBattleImpl(Server *server, NetworkBattle *p, TimerOptions &t):
            m_server(server),
            m_field(p),
            m_channel(BattleChannelPtr(
                BattleChannel::createChannel(server, this))),
            m_queue(boost::bind(&NetworkBattleImpl::executeTurn, this, _1)),
            m_replacement(false),
            m_victory(false),
            m_turnCount(0),
            m_waiting(false),
            m_terminated(false) {
        if (t.enabled) {
            m_timer = TimerPtr(new Timer(t.pool, t.periods, t.periodLength, this));
            addTimer(m_timer);
        } else {
            m_timer = TimerPtr(new Timer());
        }
    }
    
    ~NetworkBattleImpl() {
        if (m_timer->isEnabled()) {
            removeTimer(m_timer);
        }
    }
    
    static void addTimer(TimerPtr t) {
        boost::unique_lock<boost::recursive_mutex> lock(m_timerMutex);
        m_timerList.push_back(t);
    }

    static void removeTimer(TimerPtr t) {
        boost::unique_lock<boost::recursive_mutex> lock(m_timerMutex);
        m_timerList.remove(t);
    }
    
    // Called when player number idx's time runs out - just kick them
    void timeExpired(int idx) {
        m_channel->part(m_clients[idx]);
    }
    
    void beginTurn() {
        ++m_turnCount;
        informBeginTurn();
        requestMoves();
        m_timer->beginTicking();
    }
    
    void executeTurn(TURN_PTR &ptr) {
        // NOTE: This variable 'p' ensures that 'this' object lives at least
        //       until the end of this function.
        NetworkBattle::PTR p = m_field->shared_from_this();

        boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        m_lock = &lock;
        ScriptContextPtr cx = m_field->getContext()->shared_from_this();
        ScriptContextLock cxLock(cx);
        if (m_replacement) {
            m_field->processReplacements(*ptr);
        } else {
            m_field->processTurn(*ptr);
        }
        if (!m_victory && !requestReplacements()) {
            beginTurn();
        }
    }

    Pokemon *requestInactivePokemon(Pokemon *user) {
        const int party = user->getParty();
        m_timer->startTimer(party);
        if (m_field->getAliveCount(party, true) == 0)
            return NULL;
        m_selection = user;
        m_waiting = m_replacement = true;
        m_requests[party].push_back(user->getSlot());
        requestAction(party);

        ScriptContextPtr cx = m_field->getContext()->shared_from_this();
        
        const int depth = cx->clearContextThread();
        while (m_waiting) {
            m_condition.wait(*m_lock);
        }
        cx->setContextThread(depth);

        Pokemon *ret = m_selection;
        m_selection = NULL;
        m_condition.notify_one();
        return ret;
    }

    void informBeginTurn() {
        OutMessage msg(OutMessage::BATTLE_BEGIN_TURN);
        msg << m_field->getId();
        msg << (int16_t)m_turnCount;
        bool timing = m_timer->isEnabled();
        msg << (unsigned char)timing;
        if (timing) {
            for (int i = 0; i < 2; ++i) {
                msg << (int16_t)m_timer->getRemaining(i);
                msg << (unsigned char)m_timer->getPeriods(i);
            }
        }
        msg.finalise();

        broadcast(msg);
    }

    void cancelAction(const int party) {
        PARTY_TURN &turn = m_turns[party];
        if (m_requests[party].size() == turn.size()) {
            // too late to cancel
            return;
        }

        turn.pop_back();
        requestAction(party);
    }

    void broadcast(OutMessage &msg, ClientPtr client = ClientPtr()) {
        m_channel->broadcast(msg, client);
    }

    ClientPtr getClient(const int idx) const {
        const int size = m_clients.size();
        if (size <= idx)
            return ClientPtr();
        return m_clients[idx];
    }

    /**
     * SPECTATOR_BEGIN
     *
     * int32  : field id
     * string : first player
     * string : second player
     * byte   : active party size
     * byte   : maximum party size
     * byte   : number of timer periods or -1 if timers are disabled
     *
     * for 0...1:
     *     byte : party size
     *     for 0...party size:
     *         byte : has the pokemon been revealed
     *         if revealed:
     *             int16  : slot the pokemon is in or -1 if no slot:
     *             string : the nickname of the pokemon
     *             int16  : species id
     *             byte : gender
     *             byte : level
     *             byte : whether the pokemon is shiny
     *             byte : whether the pokemon is fainted
     *             if not fainted:
     *                 byte : present hp in [0, 48]
     *                 byte : number of statuses
     *                 for each status
     *                     string : message
     *                     byte   : effect radius
     *                     
     */
    void prepareSpectator(ClientPtr client) {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        ScriptContext *cx = m_field->getContext();
        ScriptContextLock cxLock(cx->shared_from_this());
        
        OutMessage msg(OutMessage::SPECTATOR_BEGIN);
        msg << m_field->getId();
        msg << m_trainer[0] << m_trainer[1];
        msg << (unsigned char)m_field->getPartySize();
        msg << (unsigned char)m_maxTeamLength;
        msg << (unsigned char)(m_timer->isEnabled() ? m_timer->getPeriods() : -1);

        for (int i = 0; i < TEAM_COUNT; ++i) {
            const Pokemon::ARRAY &team = m_field->getTeam(i);
            const int size = team.size();
            msg << (unsigned char)size;
            for (int j = 0; j < size; ++j) {
                Pokemon::PTR p = team[j];
                msg << (unsigned char)p->isRevealed();
                if (p->isRevealed()) {
                    const int slot = p->getSlot();
                    msg << (int16_t)slot;
                    msg << p->getName();

                    int16_t species = (int16_t)p->getSpeciesId();
                    msg << species;
                    msg << (unsigned char)p->getGender();
                    msg << (unsigned char)p->getLevel();
                    msg << (unsigned char)p->isShiny();
                
                    const bool fainted = p->isFainted();
                    msg << (unsigned char)fainted;
                    if (!fainted) {
                        const int hp = p->getRawStat(S_HP);
                        const int present = p->getHp();
                        const int total =
                                floor(48.0 * (double)present / (double)hp + 0.5);
                        msg << (unsigned char)total;

                        // Not all status get sent, so prepare them first
                        STATUSES statuses = p->getEffects();
                        STATUSES::const_iterator iter = statuses.begin();
                        vector<string> texts;
                        vector<unsigned char> radii;
                        for (; iter != statuses.end(); ++iter) {
                            StatusObjectPtr effect = *iter;
                            if (effect->getType(cx) != StatusObject::TYPE_NORMAL)
                                continue;
                            string text = effect->toString(cx);
                            if (text.empty())
                                continue;

                            texts.push_back(text);
                            radii.push_back((unsigned char)effect->getRadius(cx));
                        }

                        //Now write the statuses
                        const int size = texts.size();
                        msg << (unsigned char)size;
                        for (int k = 0; k < size; k++) {
                            msg << texts[k];
                            msg << radii[k];
                        }
                    }
                }
            }
        }

        msg.finalise();
        client->sendMessage(msg);
    }

    /**
     * BATTLE_BEGIN
     *
     * int32  : field id
     * string : opponent
     * byte   : party
     * int16  : metagame (-1 for a direct challenge)
     * byte   : rated
     */
    void sendBattleBegin(const int party) {
        const int32_t id = m_field->getId();
        const string opponent = m_clients[1 - party]->getName();

        OutMessage msg(OutMessage::BATTLE_BEGIN);
        msg << id << opponent << ((unsigned char)party);
        msg << (int16_t)m_metagame;
        msg << (unsigned char)m_rated;
        msg.finalise();
        m_clients[party]->sendMessage(msg);
    }

    void writeVisualData(OutMessage &msg, Pokemon::PTR p) {
        if (p && !p->isFainted()) {
            int16_t species = (int16_t)p->getSpeciesId();
            msg << species;
            msg << (unsigned char)p->getGender();
            msg << (unsigned char)p->getLevel();
            msg << (unsigned char)p->isShiny();
        } else {
            msg << ((int16_t)-1);
        }
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
     *             byte : level
     *             byte : whether the pokemon is shiny
     */
    void updateBattlePokemon() {
        OutMessage msg(OutMessage::BATTLE_POKEMON);
        msg << (int32_t)m_field->getId();

        const int size = m_field->getPartySize();
        boost::shared_ptr<PokemonParty> *active = m_field->getActivePokemon();
        for (int i = 0; i < TEAM_COUNT; ++i) {
            PokemonParty &party = *active[i];
            for (int j = 0; j < size; ++j) {
                Pokemon::PTR p = party[j].pokemon;
                writeVisualData(msg, p);
            }
        }
        msg.finalise();

        broadcast(msg);
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
        PARTY_TURN &turn = m_turns[party];
        const int size = turn.size();
        const int slot = m_requests[party][size];
        Pokemon::PTR p = m_field->getActivePokemon(party, slot);
        
        OutMessage msg(OutMessage::REQUEST_ACTION);
        msg << m_field->getId();
        msg << (unsigned char)(p->getSlot());
        msg << (unsigned char)(p->getPosition());
        msg << (unsigned char)m_replacement;

        vector<bool> switches;
        m_field->getLegalSwitches(p.get(), switches);

        for (int i = 0; i < size; ++i) {
            PokemonTurn &t = turn[i];
            if (t.type == TT_SWITCH) {
                switches[t.id] = false;
            }
        }

        const int switchSize = switches.size();
        msg << switchSize;
        for (int i = 0; i < switchSize; ++i) {
            msg << ((unsigned char)switches[i]);
        }

        if (!m_replacement) {
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

        ClientPtr client = getClient(party);
        if (client) {
            client->sendMessage(msg);
        }
    }

    bool requestReplacements() {
        boost::lock_guard<boost::recursive_mutex> lock(m_mutex);

        Pokemon::ARRAY pokemon;
        m_field->getFaintedPokemon(pokemon);
        if (pokemon.empty()) {
            return false;
        }

        m_replacement = true;
        for (Pokemon::ARRAY::const_iterator i = pokemon.begin();
                i != pokemon.end(); ++i) {
            const int party = (*i)->getParty();
            const int slot = (*i)->getSlot();
            m_requests[party].push_back(slot);
        }

        for (int i = 0; i < TEAM_COUNT; ++i) {
            if (!m_requests[i].empty()) {
                requestAction(i);
                m_timer->startTimer(i);
            }
        }
        return true;
    }

    void requestMoves() {
        boost::lock_guard<boost::recursive_mutex> lock(m_mutex);

        m_replacement = false;
        Pokemon::ARRAY pokemon;
        m_field->getActivePokemon(pokemon);
        for (Pokemon::ARRAY::const_iterator i = pokemon.begin();
                i != pokemon.end(); ++i) {
            const int party = (*i)->getParty();
            const int slot = (*i)->getSlot();
            (*i)->determineLegalActions();
            m_requests[party].push_back(slot);
        }
        for (int i = 0; i < TEAM_COUNT; ++i) {
            requestAction(i);
        }
    }

    void handleForfeit(const int party) {
        boost::unique_lock<boost::recursive_mutex>
                lock(m_mutex, boost::adopt_lock);

        if (m_waiting) {
            // One client is in the middle of selecting a pokemon for a move
            // like U-turn or Baton Pass. To allow the battle to exit nicely,
            // we have to pick a pokemon for the client.
            const int party = m_selection->getParty();
            m_selection = m_field->getRandomInactivePokemon(m_selection);
            m_waiting = false;
            m_requests[party].clear();
            m_turns[party].clear();
            m_condition.notify_one();
            while (m_selection) {
                m_condition.wait(lock);
            }
        }

        ScriptContextPtr cx = m_field->getContext()->shared_from_this();
        ScriptContextLock cxLock(cx);
        m_field->informVictory(1 - party);
    }

    void maybeExecuteTurn() {
        const int count = m_requests.size();
        for (int i = 0; i < count; ++i) {
            if (m_requests[i].size() != m_turns[i].size()) {
                return;
            }
        }

        TURN_PTR turn(new PARTY_TURN());
        for (int i = 0; i < count; ++i) {
            PARTY_TURN &v = m_turns[i];
            turn->insert(turn->end(), v.begin(), v.end());
            v.clear();
            m_requests[i].clear();
        }
        
        m_queue.post(turn);
    }
};

// static member declarations
list<TimerPtr> NetworkBattleImpl::m_timerList;
boost::recursive_mutex NetworkBattleImpl::m_timerMutex;
boost::thread NetworkBattleImpl::m_timerThread;


Channel::FLAGS BattleChannel::handleJoin(ClientPtr client) {
    FLAGS ret;
    ChannelPtr p = m_server->getMainChannel();
    if (p) {
        FLAGS flags = p->getStatusFlags(client);
        if (flags[OP]) {
            // This user is a main chat op, so the user gets +ao (protected ops).
            ret[PROTECTED] = true;
            ret[OP] = true;
        }
    }
    if (m_field && (m_field->m_field->getParty(client) != -1)) {
        // This user is a participant in the battle, so the user gets +o.
        ret[OP] = true;
    }
    // TODO: bans
    return ret;
}

bool BattleChannel::join(ClientPtr client) {
    boost::lock_guard<boost::recursive_mutex> lock(m_mutex);

    // If the battle has ended then more clients may not join.
    if (!m_field)
        return false;

    if (!Channel::join(client))
        return false;
    if (m_field->m_field->getParty(client) == -1) {
        m_field->prepareSpectator(client);
    }
    return true;
}

void BattleChannel::handlePart(ClientPtr client) {
    NetworkBattle::PTR p;
    NetworkBattleImpl *impl = NULL;
    boost::recursive_mutex *m = NULL;
    {
        boost::lock_guard<boost::recursive_mutex> lock(m_mutex);
        if (!m_field)
            return;
        impl = m_field;
        p = impl->m_field->shared_from_this();
        m = &m_field->m_mutex;
    }
    // Simultaneously lock both the channel mutex and the battle mutex.
    boost::lock(m_mutex, *m);
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex, boost::adopt_lock),
            lock2(*m, boost::adopt_lock);
    if (impl->m_terminated)
        return;
    int party = -1;
    if ((party = p->getParty(client)) != -1) {
        // User was a participant in the battle, so we need to end the battle.
        impl->handleForfeit(party);
        // We need to break the association between lock2 and *m because
        // handleForfeit() has already unlocked *m.
        lock2.release();
    }
}

void NetworkBattle::terminate() {
    NetworkBattle::PTR p = shared_from_this();
    // Simultaneously lock both the channel mutex and the battle mutex.
    boost::recursive_mutex &m = m_impl->m_channel->getMutex();
    boost::lock(m, m_impl->m_mutex);
    boost::unique_lock<boost::recursive_mutex> lock(m, boost::adopt_lock),
            lock2(m_impl->m_mutex, boost::adopt_lock);
    m_impl->m_terminated = true;
    m_impl->m_channel->informBattleTerminated();
    // There will always be two clients in the vector at this point.
    m_impl->m_clients[0]->terminateBattle(p, m_impl->m_clients[1]);
    BattleField::terminate();
    // TODO: Maybe a better way to collect garbage here.
    m_impl->m_server->getMachine()->acquireContext()->maybeGc();
}

NetworkBattle::NetworkBattle(Server *server,
        ClientPtr *clients,
        Pokemon::ARRAY *teams,
        const GENERATION generation,
        const int partySize,
        const int maxTeamLength,
        vector<StatusObject> &clauses,
        network::TimerOptions t,
        const int metagame,
        const bool rated) {
    m_impl = boost::shared_ptr<NetworkBattleImpl>(
            new NetworkBattleImpl(server, this, t));
    m_impl->m_maxTeamLength = maxTeamLength;
    m_impl->m_metagame = metagame;
    m_impl->m_rated = rated;
    m_impl->m_turns.resize(TEAM_COUNT);
    m_impl->m_requests.resize(TEAM_COUNT);
    for (int i = 0; i < TEAM_COUNT; ++i) {
        ClientPtr client = clients[i];
        m_impl->m_trainer.push_back(client->getName());
        m_impl->m_clients.push_back(client);
        client->joinChannel(m_impl->m_channel);
    }

    // Encode some metadata into the channel topic.
    // TODO: Add ladder etc.
    const string topic = m_impl->m_trainer[0] + ","
            + m_impl->m_trainer[1] + ","
            + boost::lexical_cast<string>(generation) + ","
            + boost::lexical_cast<string>(partySize) + ","
            + boost::lexical_cast<string>(maxTeamLength) + ","
            + boost::lexical_cast<string>(metagame) + ","
            + boost::lexical_cast<string>(int(rated));
    m_impl->m_channel->setTopic(topic);

    initialise(&m_impl->m_mech, generation, server->getMachine(),
            teams, &m_impl->m_trainer[0], partySize, clauses);
}

int32_t NetworkBattle::getId() const {
    return m_impl->m_channel->getId();
}

void NetworkBattle::beginBattle() {
    boost::unique_lock<boost::recursive_mutex> lock(m_impl->m_mutex);
    for (int i = 0; i < TEAM_COUNT; ++i) {
        m_impl->sendBattleBegin(i);
    }
    BattleField::beginBattle();
    m_impl->beginTurn();
    m_impl->m_server->addChannel(m_impl->m_channel);
    getContext()->clearContextThread();
}

int NetworkBattle::getParty(boost::shared_ptr<network::Client> client) const {
    boost::lock_guard<boost::recursive_mutex> lock(m_impl->m_mutex);
    int count = m_impl->m_clients.size();
    if (count > 2) {
        count = 2;
    }
    for (int i =  0; i < count; ++i) {
        if (m_impl->m_clients[i] == client)
            return i;
    }
    return -1;
}

void NetworkBattle::handleCancelTurn(const int party) {
    boost::lock_guard<boost::recursive_mutex> lock(m_impl->m_mutex);
    m_impl->cancelAction(party);
}

void NetworkBattle::handleTurn(const int party, const PokemonTurn &turn) {
    boost::unique_lock<boost::recursive_mutex> lock(m_impl->m_mutex);

    PARTY_REQUEST &req = m_impl->m_requests[party];
    PARTY_TURN &pturn = m_impl->m_turns[party];
    const int max = req.size();
    const int present = pturn.size();
    if (present == max) {
        return;
    }
    const int slot = req[present];
    Pokemon *pokemon = getActivePokemon(party, slot).get();
    if (!isTurnLegal(pokemon, &turn, m_impl->m_replacement)) {
        // todo: inform illegal move?
        m_impl->requestAction(party);
        return;
    }
    if (turn.type == TT_SWITCH) {
        for (int i = 0; i < present; ++i) {
            PokemonTurn &t = pturn[i];
            if ((t.type == TT_SWITCH) && (t.id == turn.id)) {
                // inform illegal move
                m_impl->requestAction(party);
                return;
            }
        }
    }
    pturn.push_back(turn);

    m_impl->m_timer->stopTimer(party);

    const int size = pturn.size();
    if (size < max) {
        m_impl->requestAction(party);
    } else if (m_impl->m_waiting) {
        // Client has sent in an inactive pokemon for U-turn and friends.
        m_impl->m_waiting = false;
        m_impl->m_selection = getTeam(party)[turn.id].get();
        req.clear();
        pturn.clear();
        lock.unlock();
        m_impl->m_condition.notify_one();
    } else {
        m_impl->maybeExecuteTurn();
    }
}

Pokemon *NetworkBattle::requestInactivePokemon(Pokemon *pokemon) {
    return m_impl->requestInactivePokemon(pokemon);
}

/**
 * BATTLE_PRINT
 *
 * int32 : field id
 * byte  : category
 * int16 : message id
 * byte  : number of arguments
 * for each argument:
 *     string : value of the argument
 */
void NetworkBattle::print(const TextMessage &text) {
    if (!isNarrationEnabled())
        return;
    
    OutMessage msg(OutMessage::BATTLE_PRINT);
    msg << getId();
    msg << (unsigned char)text.getCategory();
    msg << (int16_t)text.getMessage();
    const vector<string> &args = text.getArgs();
    const int count = args.size();
    msg << (unsigned char)count;
    for (int i = 0; i < count; ++i) {
        msg << args[i];
    }
    msg.finalise();

    m_impl->broadcast(msg);
}

/**
 * BATTLE_VICTORY
 *
 * int32 : field id
 * int16 : party id (or -1 for a draw)
 */
void NetworkBattle::informVictory(const int party) {
    m_impl->m_victory = true;

    OutMessage msg(OutMessage::BATTLE_VICTORY);
    msg << getId();
    msg << (int16_t)party;
    msg.finalise();

    m_impl->broadcast(msg);

    if (m_impl->m_rated) {
        m_impl->m_server->postLadderMatch(m_impl->m_metagame,
                m_impl->m_clients, party);
    }

    // terminate this battle
    terminate();
}

/**
 * BATTLE_USE_MOVE
 *
 * int32 : field id
 * byte : party
 * byte : slot
 * string : user [nick]name
 * int16 : move id
 */
void NetworkBattle::informUseMove(Pokemon *pokemon, MoveObject *move) {
    OutMessage msg(OutMessage::BATTLE_USE_MOVE);
    msg << getId();
    msg << (unsigned char)pokemon->getParty();
    msg << (unsigned char)pokemon->getSlot();
    msg << pokemon->getName();
    msg << (int16_t)move->getTemplate(getContext())->getId();
    msg.finalise();

    m_impl->broadcast(msg);
}

/**
 * BATTLE_WITHDRAW
 *
 * int32 : field id
 * byte : party
 * byte : slot
 * string : pokemon [nick]name
 */
void NetworkBattle::informWithdraw(Pokemon *pokemon) {
    OutMessage msg(OutMessage::BATTLE_WITHDRAW);
    msg << getId();
    msg << (unsigned char)pokemon->getParty();
    msg << (unsigned char)pokemon->getSlot();
    msg << pokemon->getName();
    msg.finalise();

    m_impl->broadcast(msg);
}

/**
 * BATTLE_SEND_OUT
 *
 * int32  : field id
 * byte   : party
 * byte   : slot
 * byte   : index
 * string : pokemon [nick]name
 * int16  : species id
 * byte   : gender
 * byte   : level
 */
void NetworkBattle::informSendOut(Pokemon *pokemon) {
    OutMessage msg(OutMessage::BATTLE_SEND_OUT);
    msg << getId();
    msg << (unsigned char)pokemon->getParty();
    msg << (unsigned char)pokemon->getSlot();
    msg << (unsigned char)pokemon->getPosition();
    msg << pokemon->getName();
    msg << (int16_t)pokemon->getSpeciesId();
    msg << (unsigned char)pokemon->getGender();
    msg << (unsigned char)pokemon->getLevel();
    msg.finalise();

    m_impl->broadcast(msg);
    m_impl->updateBattlePokemon();
}

/**
 * BATTLE_HEALTH_CHANGE
 *
 * int32  : field id
 * byte   : party
 * byte   : slot
 * int16  : delta health in [0, 48]
 * int16  : new total health [0, 48]
 * int16  : denominator
 */
class BattleHealthChange : public OutMessage {
public:
    BattleHealthChange(const int fid, const int party, const int slot,
            const int delta, const int total, const int denominator):
                OutMessage(BATTLE_HEALTH_CHANGE) {
        *this << (int32_t)fid;
        *this << (unsigned char)party;
        *this << (unsigned char)slot;
        *this << (int16_t)delta;
        *this << (int16_t)total;
        *this << (int16_t)denominator;
        finalise();
    }
};

void NetworkBattle::informHealthChange(Pokemon *pokemon, const int raw) {
    const int hp = pokemon->getRawStat(S_HP);
    const int present = pokemon->getHp();
    const int delta = floor(48.0 * (double)raw / (double)hp + 0.5);
    const int total = floor(48.0 * (double)present / (double)hp + 0.5);
    const int party = pokemon->getParty();
    const int slot = pokemon->getSlot();

    ClientPtr client = m_impl->m_clients[party];

    // Send the approximate health change to everybody except the person who
    // controls the pokemon.
    BattleHealthChange msg(getId(), party, slot, delta, total, 48);
    m_impl->broadcast(msg, client);

    // Send the owner of the pokemon exact health change information.
    BattleHealthChange msg2(getId(), party, slot, raw, present, hp);
    client->sendMessage(msg2);
}

/**
 * BATTLE_SET_PP
 *
 * int32 : field id
 * byte  : pokemon index
 * byte  : move index
 * byte  : new pp value
 */
void NetworkBattle::informSetPp(Pokemon *pokemon,
        const int move, const int pp) {
    OutMessage msg(OutMessage::BATTLE_SET_PP);
    msg << getId();
    msg << (unsigned char)pokemon->getPosition();
    msg << (unsigned char)move;
    msg << (unsigned char)pp;
    msg.finalise();

    ClientPtr client = m_impl->m_clients[pokemon->getParty()];
    client->sendMessage(msg);
}

/**
 * BATTLE_SET_MOVE
 *
 * int32 : field id
 * byte  : pokemon index
 * byte  : move index
 * int16 : new move (move list index)
 * byte  : pp of new move
 * byte  : max pp of new move
 */
void NetworkBattle::informSetMove(Pokemon *pokemon, const int idx,
        const int move, const int pp, const int maxPp) {
    OutMessage msg(OutMessage::BATTLE_SET_MOVE);
    msg << getId();
    msg << (unsigned char)pokemon->getPosition();
    msg << (unsigned char)idx;
    msg << (int16_t)move;
    msg << (unsigned char)pp;
    msg << (unsigned char)maxPp;
    msg.finalise();

    ClientPtr client = m_impl->m_clients[pokemon->getParty()];
    client->sendMessage(msg);
}

/**
 * BATTLE_FAINTED
 *
 * int32 : field id
 * byte : party
 * byte : slot
 * string : pokemon [nick]name
 */
void NetworkBattle::informFainted(Pokemon *pokemon) {
    OutMessage msg(OutMessage::BATTLE_FAINTED);
    msg << getId();
    msg << (unsigned char)pokemon->getParty();
    msg << (unsigned char)pokemon->getSlot();
    msg << pokemon->getName();
    msg.finalise();

    m_impl->broadcast(msg);
    m_impl->updateBattlePokemon();
}

/**
 * BATTLE_STATUS_CHANGE
 *
 * int32 : field id
 * byte : party
 * byte : index
 * byte : effect radius
 * string : message
 * byte : if this status is being applied
 */
void NetworkBattle::informStatusChange(Pokemon *p, StatusObject *effect,
        const bool applied) {
    ScriptContext *cx = getContext();
    if (effect->getType(cx) != StatusObject::TYPE_NORMAL)
        return;
    string text = effect->toString(cx);
    if (text.empty()) {
        return;
    }
    OutMessage msg(OutMessage::BATTLE_STATUS_CHANGE);
    msg << getId();
    msg << (unsigned char)p->getParty();
    msg << (unsigned char)p->getPosition();
    msg << (unsigned char)effect->getRadius(cx);
    msg << text;
    msg << (unsigned char)applied;
    msg.finalise();
    m_impl->broadcast(msg);
}

namespace {

void handleTiming() {
    while (true) {
        boost::this_thread::sleep(boost::posix_time::seconds(3));
        boost::unique_lock<boost::recursive_mutex> lock(NetworkBattleImpl::m_timerMutex);
        list<TimerPtr>::iterator i = NetworkBattleImpl::m_timerList.begin();
        for (; i != NetworkBattleImpl::m_timerList.end(); ++i) {
            (*i)->tick();
        }
    }
}

} // anonymous namespace

void NetworkBattle::startTimerThread() {
    NetworkBattleImpl::m_timerThread = boost::thread(boost::bind(&handleTiming));
}

void Timer::tick() {
    time_t now = time(NULL);
    int delta = now - m_lastTime;
    m_lastTime = now;
    for (int i = 0; i < 2; ++i) {
        PlayerTimer &pt = m_players[i];
        boost::unique_lock<boost::shared_mutex> lock(pt.mutex);
        if (pt.stopped) continue;
        int diff = pt.remaining - delta;
        if (diff <= 0) {
            if (pt.periods-- < 0) {
                m_battle->timeExpired(i);
                return;
            }
            pt.remaining = m_periodLength + diff;
        } else {
            pt.remaining = diff;
        }
    }
}

void Timer::startTimer(const int party) {
    if (!m_enabled) return;
    PlayerTimer &pt = m_players[party];
    boost::unique_lock<boost::shared_mutex> lock(pt.mutex);
    pt.stopped = false;
}

void Timer::stopTimer(const int party) {
    if (!m_enabled) return;
    tick();
    PlayerTimer &pt = m_players[party];
    boost::unique_lock<boost::shared_mutex> lock(pt.mutex);
    pt.stopped = true;
    // if our pool is expired then we refill the current period
    if (pt.periods != m_periods) {
        pt.remaining = m_periodLength;
    }
}


}} // namespace shoddybattle::network

