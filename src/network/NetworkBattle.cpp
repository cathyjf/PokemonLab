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
#include <fstream>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "NetworkBattle.h"
#include "ThreadedQueue.h"
#include "network.h"
#include "Channel.h"
#include "../mechanics/JewelMechanics.h"
#include "../mechanics/PokemonNature.h"
#include "../scripting/ScriptMachine.h"
#include "../text/Text.h"

using namespace std;
namespace fs = boost::filesystem;
namespace gc = boost::gregorian; // (gc for gregorian calendar)
namespace pt = boost::posix_time;

namespace shoddybattle { namespace network {

class NetworkBattleImpl;
class BattleChannel;

typedef boost::shared_ptr<BattleChannel> BattleChannelPtr;

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
    bool isEnabled() const { return m_enabled; }
    int getPeriods() const { return m_periods; }
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
typedef list<TimerPtr> TimerList;

class BattleLogMessage;

namespace {

// An entity that should be written to the battle log. This class intentionally
// has a very short name. If an entity is not given a name, it will be written
// only to the network, not to the battle log.
template <class T>
class Entity {
public:
    Entity(const T &v, const string &name = string()):
            m_value(v), m_name(name) { }
protected:
    friend class network::BattleLogMessage;
    const T &m_value;
    string m_name;
};

template <class T>
class ArrayEntity : public Entity<T> {
public:
    ArrayEntity(const T &v, const string &name, const int idx):
            Entity<T>(v, name) {
        Entity<T>::m_name += "[" + boost::lexical_cast<string>(idx) + "]";
    }
};

// Encode a value for inclusion in the log.
template <class T>
string getLogValue(const T &v) {
    return boost::lexical_cast<string>(v);
}

template <>
string getLogValue(const unsigned char &v) {
    return boost::lexical_cast<string>(static_cast<int>(v));
}

template <>
string getLogValue(const string &v) {
    boost::regex pattern1("\\\\");  // literal backslash
    boost::regex pattern2("\"");    // literal double quote
    string ret = boost::regex_replace(v, pattern1, "\\\\\\\\",
            boost::format_perl);
    ret = boost::regex_replace(ret, pattern2, "\\\\\"",
            boost::format_perl);
    return "\"" + ret + "\"";
}

}

/**
 * Battle logs will be written in a text format whose structure is similar to
 * the information contained the raw network messages, so we write both the
 * raw network message and the battle log at the same time using this class.
 */
class BattleLogMessage {
public:
    BattleLogMessage(const OutMessage::TYPE &type, const string &name):
            m_msg(type) {
        m_log = "[" + name + "]  ";
    }
    template <class T>
    BattleLogMessage &operator<<(const Entity<T> &val) {
        m_msg << val.m_value;
        if (!val.m_name.empty()) {
            m_log += val.m_name + "=" + getLogValue(val.m_value) + ", ";
        }
        return *this;
    }
    BattleLogMessage &operator<<(const NetworkBattle *p) {
        m_msg << p->getId();
        return *this;
    }
    void finalise() {
        m_msg.finalise();
        m_log = m_log.erase(m_log.size() - 2);
    }
    const OutMessage &getMsg() const {
        return m_msg;
    }
    const string &getLog() const {
        return m_log;
    }
private:
    OutMessage m_msg;
    string m_log;
};

class BattleLog {
public:
    BattleLog() {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        m_date = to_iso_extended_string(gc::day_clock::local_day());
        const string dir = "logs/battles/" + m_date + "/";
        fs::create_directories(dir);
        const pt::time_duration t =
                pt::second_clock::local_time().time_of_day();
        const string prefix = pt::to_simple_string(t);
        int suffix = -1;
        string file;
        do {
            ++suffix;
            m_id = prefix;
            if (suffix != 0) {
                m_id += "-" + boost::lexical_cast<string>(suffix);
            }
            file = dir + m_id;
        } while (fs::exists(file));
        m_file.open(file.c_str());
    }
    const string getId() const {
        return m_date + "-" + m_id;
    }
    void write(const string &str, const bool timestamp = true) {
        if (timestamp) {
            const pt::time_duration t =
                    pt::second_clock::local_time().time_of_day();
            m_file << "(" <<  pt::to_simple_string(t) << ") ";
        }
        m_file << str << flush;
    }
    BattleLog &operator<<(const BattleLogMessage &msg) {
        write(msg.getLog() + "\n");
        return *this;
    }
private:
    string m_date, m_id;
    ofstream m_file;
    static boost::mutex m_mutex;
};

class BattleChannel : public Channel {
public:

    static BattleChannelPtr createChannel(Server *server,
            NetworkBattleImpl *field);

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

    Channel::FLAGS getUserFlags(const string &user) {
        CLIENT_MAP::value_type client = getClient(user);
        return client.first ? client.second : 0;
    }

    void writeLog(const string &msg) {
        m_log.write("[chat] " + msg + "\n");
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
    BattleLog m_log;
    boost::recursive_mutex m_mutex;
};

boost::mutex BattleLog::m_mutex;

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
    boost::recursive_mutex m_mutex;
    bool m_replacement;
    bool m_victory;
    int m_turnCount;
    boost::unique_lock<boost::recursive_mutex> *m_lock;
    bool m_waiting;
    Pokemon *m_selection;
    boost::condition_variable_any m_condition;
    bool m_terminated;
    TimerPtr m_timer;
    BattleLog *m_log;
    ThreadedQueue<TURN_PTR> m_queue;

    static TimerList m_timerList;
    static boost::recursive_mutex m_timerMutex;
    static boost::thread m_timerThread;

    NetworkBattleImpl(Server *server, NetworkBattle *p, TimerOptions &t):
            m_server(server),
            m_field(p),
            m_channel(BattleChannelPtr(
                BattleChannel::createChannel(server, this))),
            m_replacement(false),
            m_victory(false),
            m_turnCount(0),
            m_waiting(false),
            m_terminated(false),
            m_queue(boost::bind(&NetworkBattleImpl::executeTurn, this, _1)) {
        if (t.enabled) {
            m_timer = TimerPtr(new Timer(t.pool, t.periods, t.periodLength,
                    this));
            addTimer(m_timer);
        } else {
            m_timer = TimerPtr(new Timer());
        }
    }
    
    ~NetworkBattleImpl() {
        // We join the queue explicitly to avoid any doubt about when its
        // destructor will run.
        m_queue.join();
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
        // Note: 'this' is invalid here. Do not touch 'this'!
    }
    
    void beginTurn() {
        ++m_turnCount;
        informBeginTurn();
        requestMoves();
        m_timer->beginTicking();
    }
    
    void executeTurn(TURN_PTR &ptr) {
        // If this turn results in the end of the battle then this reference to
        // m_field will be the last one, so ~NetworkBattle, and hence
        // ~NetworkBattleImpl, will run at the end of this function.
        NetworkBattle::PTR p = m_field->shared_from_this();

        boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        if (m_terminated) {
            // If ~NetworkBattle runs on another thread, there may still be
            // a turn left to execute, but we don't actually want it to
            // execute.
            return;
        }
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
    } // ~NetworkBattle will run here if the battle ended this turn.

    Pokemon *requestInactivePokemon(Pokemon *user) {
        const int party = user->getParty();
        if (m_field->getAliveCount(party, true) == 0)
            return NULL;
        m_selection = user;
        m_waiting = m_replacement = true;
        m_requests[party].push_back(user->getSlot());
        requestAction(party);
        m_timer->startTimer(party);

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
        BattleLogMessage msg(OutMessage::BATTLE_BEGIN_TURN, "begin_turn");
        msg << m_field;
        msg << Entity<int16_t>(m_turnCount, "turn_count");
        // The timer information isn't worth logging.
        const bool timing = m_timer->isEnabled();
        msg << Entity<unsigned char>(timing);
        if (timing) {
            for (int i = 0; i < 2; ++i) {
                msg << Entity<int16_t>(m_timer->getRemaining(i));
                msg << Entity<unsigned char>(m_timer->getPeriods(i));
            }
        }
        msg.finalise();
        *m_log << msg;

        broadcast(msg.getMsg());
    }

    void cancelAction(const int party) {
        PARTY_TURN &turn = m_turns[party];
        if (m_requests[party].size() == turn.size()) {
            // too late to cancel
            return;
        }

        if (!turn.empty()) {
            turn.pop_back();
            requestAction(party);
        }
    }

    void broadcast(const OutMessage &msg, ClientPtr client = ClientPtr()) {
        m_channel->broadcast(msg, client);
    }

    ClientPtr getClient(const int idx) const {
        const int size = m_clients.size();
        if (size <= idx)
            return ClientPtr();
        return m_clients[idx];
    }

    void writeLogPokemon(ostringstream &ss, const Pokemon::PTR p) {
        ss << p->getName() << endl;
        ss << p->getSpeciesId() << " ("
                << getLogValue(p->getSpeciesName()) << ")" << endl;
        ss << p->getAbilityName() << endl;
        ss << p->getItemName() << endl;
        const PokemonNature *nature = p->getNature();
        const Text *text = m_field->getScriptMachine()->getText();
        const int natureId = nature->getInternalValue();
        ss << natureId << " ("
                << getLogValue(text->getText(1, natureId, 0, NULL))
                << ")" << endl;
        ss << p->getGender() << endl;
        for (int i = 0; i < STAT_COUNT; ++i) {
            ss << p->getIv(static_cast<STAT>(i)) << "/";
        }
        ss << endl;
        for (int i = 0; i < STAT_COUNT; ++i) {
            ss << p->getEv(static_cast<STAT>(i)) << "/";
        }
        ss << endl;
        const int moves = p->getMoveCount();
        for (int i = 0; i < moves; ++i) {
            MoveObjectPtr move = p->getMove(i);
            const MoveTemplate *tpl = move->getTemplate();
            if (tpl) {
                ss << tpl->getId() << " ("
                        << getLogValue(tpl->getName()) << ")" << endl;
            }
        }
    }

    void writeLogHeader() {
        ostringstream ret;
        string trainer0 = m_trainer[0];
        string trainer1 = m_trainer[1];
        if (trainer0 > trainer1) {
            // Show the users in alphabetical order on the first line.
            swap(trainer0, trainer1);
        }
        ret << getLogValue(trainer0) << " v. " << getLogValue(trainer1) << endl;
        ret << "Battle ID: " << m_log->getId() << endl << endl;
        for (int i = 0; i < TEAM_COUNT; ++i) {
            ClientPtr client = m_clients[i];
            ret << "Player " << i << ": "
                    << getLogValue(m_trainer[i]) << ", "
                    << client->getId() << ", "
                    << client->getIp() << endl;
        }
        ret << endl;
        for (int i = 0; i < TEAM_COUNT; ++i) {
            Pokemon::ARRAY team = m_field->getTeam(i);
            const int size = team.size();
            for (int j = 0; j < size; ++j) {
                ret << "[pokemon] party=" << i << ", idx=" << j << endl;
                writeLogPokemon(ret, team[j]);
                ret << endl;
            }
        }
        ret << "~~~~~~~~~~" << endl << endl;
        m_log->write(ret.str(), false);
    }

    static void prepareSpectatorStatuses(const STATUSES &statuses, 
            vector<string> &ids, vector<string> &messages,
            vector<unsigned char> &radii, ScriptContext *cx) {
        STATUSES::const_iterator iter = statuses.begin();
        for (; iter != statuses.end(); ++iter) {
            StatusObjectPtr effect = *iter;
            if (effect->getType(cx) != StatusObject::TYPE_NORMAL) {
                continue;
            }
            string text = effect->toString(cx);
            if (text.empty()) {
                continue;
            }
            ids.push_back(effect->getId(cx));
            messages.push_back(text);
            radii.push_back(effect->getRadius(cx));
        }
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
     *                     string : id
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
        msg << (unsigned char)(m_timer->isEnabled() ?
                m_timer->getPeriods() : -1);

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
                        const int total = floor(48.0 * (double)present /
                                (double)hp + 0.5);
                        msg << (unsigned char)total;

                        // Not all status get sent, so prepare them first
                        const STATUSES &statuses = p->getEffects();
                        vector<string> ids;
                        vector<string> messages;
                        vector<unsigned char> radii;
                        prepareSpectatorStatuses(statuses, ids, messages,
                                radii, cx);

                        // Now write the statuses
                        const int size = ids.size();
                        msg << (unsigned char)size;
                        for (int k = 0; k < size; k++) {
                            msg << ids[k];
                            msg << messages[k];
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
     * string : unique battle ID
     */
    void sendBattleBegin(const int party) {
        const int32_t id = m_field->getId();
        const string opponent = m_clients[1 - party]->getName();

        OutMessage msg(OutMessage::BATTLE_BEGIN);
        msg << id << opponent << ((unsigned char)party);
        msg << (int16_t)m_metagame;
        msg << (unsigned char)m_rated;
        msg << m_log->getId();
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
                Pokemon::PTR p = party[j];
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
TimerList NetworkBattleImpl::m_timerList;
boost::recursive_mutex NetworkBattleImpl::m_timerMutex;
boost::thread NetworkBattleImpl::m_timerThread;

BattleChannelPtr BattleChannel::createChannel(Server *server,
        NetworkBattleImpl *field) {
    BattleChannelPtr p(new BattleChannel(server, field,
            string(), string(),
            CHANNEL_FLAGS()));
    p->setName(boost::lexical_cast<string>(p->getId()));
    field->m_log = &p.get()->m_log;
    return p;
}

Channel::FLAGS BattleChannel::handleJoin(ClientPtr client) {
    FLAGS ret;
    ChannelPtr p = m_server->getMainChannel();
    if (p) {
        FLAGS flags = p->getStatusFlags(client);
        if (flags[OP]) {
            // This user is a main chat op, so the user gets +ao
            // (protected ops).
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
    // We acquire BattleChannel's mutex before Channel's mutex, since we are
    // going to lock both at once.
    boost::lock_guard<boost::recursive_mutex> lock(m_mutex);

    // If the battle has ended then more clients may not join.
    if (!m_field)
        return false;

    if (!Channel::join(client)) // locks Channel's mutex
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
        // Channel's mutex is not locked at this point, so acquiring this lock
        // is safe.
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
} // ~NetworkBattle will run here if the client parting was a participant
  // in the battle.

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
        const bool rated,
        boost::shared_ptr<void> &monitor) {
    m_impl = boost::shared_ptr<NetworkBattleImpl>(
            new NetworkBattleImpl(server, this, t));
    m_impl->m_maxTeamLength = maxTeamLength;
    m_impl->m_metagame = metagame;
    m_impl->m_rated = rated;
    m_impl->m_turns.resize(TEAM_COUNT);
    m_impl->m_requests.resize(TEAM_COUNT);

    // Don't allow clients to part the battle until monitor goes out of scope.
    // Note that the BattleChannel's mutex is always locked before the parent
    // Channel's mutex, if both are going to be locked.
    boost::recursive_mutex &m = m_impl->m_channel->getMutex();
    m.lock();
    monitor = boost::shared_ptr<void>(&m,
            boost::bind(&boost::recursive_mutex::unlock, _1));

    for (int i = 0; i < TEAM_COUNT; ++i) {
        ClientPtr client = clients[i];
        m_impl->m_trainer.push_back(client->getName());
        m_impl->m_clients.push_back(client);
        client->joinChannel(m_impl->m_channel); // locks Channel's mutex
    }

    // Encode some metadata into the channel topic.
    const string topic = m_impl->m_trainer[0] + ","
            + m_impl->m_trainer[1] + ","
            + boost::lexical_cast<string>(generation) + ","
            + boost::lexical_cast<string>(partySize) + ","
            + boost::lexical_cast<string>(maxTeamLength) + ","
            + boost::lexical_cast<string>(metagame) + ","
            + boost::lexical_cast<string>(int(rated));
    m_impl->m_channel->setTopic(topic); // locks Channel's mutex

    BattleField::initialise(&m_impl->m_mech, generation, server->getMachine(),
            teams, &m_impl->m_trainer[0], partySize, clauses);
    m_impl->writeLogHeader();
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
    BattleField::getContext()->clearContextThread();
    m_impl->m_server->addChannel(m_impl->m_channel);
}

int NetworkBattle::getParty(boost::shared_ptr<network::Client> client) const {
    boost::lock_guard<boost::recursive_mutex> lock(m_impl->m_mutex);
    int count = m_impl->m_clients.size();
    if (count > 2) {
        count = 2;
    }
    for (int i =  0; i < count; ++i) {
        if (m_impl->m_clients[i] == client) {
            return i;
        }
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
    Pokemon *pokemon = BattleField::getActivePokemon(party, slot).get();
    if (!BattleField::isTurnLegal(pokemon, &turn, m_impl->m_replacement)) {
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

    BattleLogMessage msg(OutMessage::BATTLE_PRINT, "print");
    msg << this;
    msg << Entity<unsigned char>(text.getCategory(), "category");
    msg << Entity<int16_t>(text.getMessage(), "message");
    const vector<string> &args = text.getArgs();
    const int count = args.size();
    msg << Entity<unsigned char>(count);
    for (int i = 0; i < count; ++i) {
        msg << ArrayEntity<string>(args[i], "token", i);
    }
    msg.finalise();
    *m_impl->m_log << msg;

    m_impl->broadcast(msg.getMsg());
}

/**
 * BATTLE_VICTORY
 *
 * int32 : field id
 * int16 : party id (or -1 for a draw)
 */
void NetworkBattle::informVictory(const int party) {   
    m_impl->m_victory = true;

    BattleLogMessage msg(OutMessage::BATTLE_VICTORY, "victory");
    msg << this;
    msg << Entity<int16_t>(party, "party");
    msg.finalise();
    *m_impl->m_log << msg;

    m_impl->broadcast(msg.getMsg());

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
    BattleLogMessage msg(OutMessage::BATTLE_USE_MOVE, "use_move");
    msg << this;
    msg << Entity<unsigned char>(pokemon->getParty(), "party");
    msg << Entity<unsigned char>(pokemon->getSlot(), "slot");
    msg << Entity<string>(pokemon->getName(), "nickname");
    msg << Entity<int16_t>(move->getTemplate(getContext())->getId(), "move");
    msg.finalise();
    *m_impl->m_log << msg;

    m_impl->broadcast(msg.getMsg());
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
    BattleLogMessage msg(OutMessage::BATTLE_WITHDRAW, "withdraw");
    msg << this;
    msg << Entity<unsigned char>(pokemon->getParty(), "party");
    msg << Entity<unsigned char>(pokemon->getSlot(), "slot");
    msg << Entity<string>(pokemon->getName(), "nickname");
    msg.finalise();
    *m_impl->m_log << msg;

    m_impl->broadcast(msg.getMsg());
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
    BattleLogMessage msg(OutMessage::BATTLE_SEND_OUT, "send_out");
    msg << this;
    msg << Entity<unsigned char>(pokemon->getParty(), "party");
    msg << Entity<unsigned char>(pokemon->getSlot(), "slot");
    msg << Entity<unsigned char>(pokemon->getPosition(), "idx");
    msg << Entity<string>(pokemon->getName(), "nickname");
    msg << Entity<int16_t>(pokemon->getSpeciesId(), "species");
    msg << Entity<unsigned char>(pokemon->getGender(), "gender");
    msg << Entity<unsigned char>(pokemon->getLevel(), "level");
    msg.finalise();
    *m_impl->m_log << msg;

    m_impl->broadcast(msg.getMsg());
    m_impl->updateBattlePokemon();
}

/**
 * BATTLE_HEALTH_CHANGE
 *
 * int32  : field id
 * byte   : party
 * byte   : slot
 * int16  : delta health in [0, denominator]
 * int16  : new total health [0, denominator]
 * int16  : denominator
 */
class BattleHealthChange : public BattleLogMessage {
public:
    BattleHealthChange(const NetworkBattle *p, const int party, const int slot,
            const int delta, const int total, const int denominator):
                BattleLogMessage(OutMessage::BATTLE_HEALTH_CHANGE,
                        "health_change") {
        *this << p;
        *this << Entity<unsigned char>(party, "party");
        *this << Entity<unsigned char>(slot, "slot");
        *this << Entity<int16_t>(delta, "delta");
        *this << Entity<int16_t>(total, "total");
        *this << Entity<int16_t>(denominator, "denominator");
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
    // controls the pokemon. Don't write this message to the log.
    BattleHealthChange msg(this, party, slot, delta, total, 48);
    m_impl->broadcast(msg.getMsg(), client);

    // Send the owner of the pokemon exact health change information.
    BattleHealthChange msg2(this, party, slot, raw, present, hp);
    *m_impl->m_log << msg2;
    client->sendMessage(msg2.getMsg());
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
    // Don't bother logging this message.
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
    // Don't bother logging this message.
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
    BattleLogMessage msg(OutMessage::BATTLE_FAINTED, "fainted");
    msg << this;
    msg << Entity<unsigned char>(pokemon->getParty(), "party");
    msg << Entity<unsigned char>(pokemon->getSlot(), "slot");
    msg << Entity<string>(pokemon->getName(), "nickname");
    msg.finalise();
    *m_impl->m_log << msg;

    m_impl->broadcast(msg.getMsg());
    m_impl->updateBattlePokemon();
}

/**
 * BATTLE_STATUS_CHANGE
 *
 * int32  : field id
 * byte   : party
 * byte   : index
 * byte   : type of status
 * byte   : effect radius
 * string : effect id
 * string : message
 * byte   : if this status is being applied
 *
 * TODO: This function should send { message_category, message_id, ... } for
 *       the effect message rather than a string of the message!
 */
void NetworkBattle::informStatusChange(Pokemon *p, StatusObject *effect,
        const bool applied) {
    ScriptContext *cx = BattleField::getContext();
    string text = effect->toString(cx);
    if (text.empty()) {
        return;
    }
    BattleLogMessage msg(OutMessage::BATTLE_STATUS_CHANGE, "status_change");
    msg << this;
    msg << Entity<unsigned char>(p->getParty(), "party");
    msg << Entity<unsigned char>(p->getPosition(), "idx");
    const int type = effect->getType(cx);
    msg << Entity<unsigned char>(type, "type");
    msg << Entity<unsigned char>(effect->getRadius(cx), "radius");
    msg << Entity<string>(effect->getId(cx), "id");
    msg << Entity<string>(text, "message");
    msg << Entity<unsigned char>(applied, "applied");
    msg.finalise();

    if (type == StatusObject::TYPE_NORMAL) {
        *m_impl->m_log << msg;
        m_impl->broadcast(msg.getMsg());
    } else {
        // Don't write to the log in this case because it's just noise.
        ClientPtr client = m_impl->m_clients[p->getParty()];
        client->sendMessage(msg.getMsg());
    }
}

namespace {

void handleTiming() {
    while (true) {
        boost::this_thread::sleep(pt::seconds(3));
        boost::unique_lock<boost::recursive_mutex> lock(
                NetworkBattleImpl::m_timerMutex);
        TimerList::iterator i = NetworkBattleImpl::m_timerList.begin();
        while (i != NetworkBattleImpl::m_timerList.end()) {
            TimerList::iterator current = i++;
            (*current)->tick(); // This might invalidate 'current'.
        }
    }
}

} // anonymous namespace

void NetworkBattle::startTimerThread() {
    NetworkBattleImpl::m_timerThread = boost::thread(
            boost::bind(&handleTiming));
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
                lock.unlock();
                m_battle->timeExpired(i);
                // Note: 'this' is invalid here! We must return and not
                //       touch 'this'.
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

