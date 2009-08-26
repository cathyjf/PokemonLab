/*
 * File:   network.cpp
 * Author: Catherine
 *
 * Created on April 9, 2009, 4:58 PM
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
 * In Shoddy Battle 2, a "message" consists of a five byte header followed by
 * a message body. The first byte identifies the type of message. The next
 * four bytes are an int in network byte order specifying the length of the
 * message body.
 */

#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include <deque>
#include <set>
#include <bitset>
#include <map>
#include <cstring>
#include "network.h"
#include "Channel.h"
#include "NetworkBattle.h"
#include "../database/DatabaseRegistry.h"
#include "../text/Text.h"
#include "../shoddybattle/Pokemon.h"
#include "../moves/PokemonMove.h"
#include "../shoddybattle/PokemonSpecies.h"
#include "../mechanics/PokemonNature.h"
#include "../scripting/ScriptMachine.h"
#include "../matchmaking/MetagameList.h"

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;
namespace fs = boost::filesystem;

namespace shoddybattle { namespace network {

class ClientImpl;
class ServerImpl;
class Channel;
typedef shared_ptr<ClientImpl> ClientImplPtr;
typedef shared_ptr<ServerImpl> ServerImplPtr;

typedef set<ClientImplPtr> CLIENT_LIST;
typedef set<ChannelPtr> CHANNEL_LIST;
typedef set<NetworkBattle::PTR> BATTLE_LIST;

void OutMessage::finalise() {
    // insert the size into the data
    *reinterpret_cast<int32_t *>(&m_data[1]) =
            htonl(m_data.size() - HEADER_SIZE);
}

const vector<unsigned char> &OutMessage::operator()() const {
    return m_data;
}

OutMessage &OutMessage::operator<<(const int16_t i) {
    const int pos = m_data.size();
    m_data.resize(pos + sizeof(int16_t), 0);
    unsigned char *p = &m_data[pos];
    *reinterpret_cast<int16_t *>(p) = htons(i);
    return *this;
}

OutMessage &OutMessage::operator<<(const int32_t i) {
    const int pos = m_data.size();
    m_data.resize(pos + sizeof(int32_t), 0);
    unsigned char *p = &m_data[pos];
    *reinterpret_cast<int32_t *>(p) = htonl(i);
    return *this;
}

OutMessage &OutMessage::operator<<(const unsigned char byte) {
    m_data.push_back(byte);
}

/**
 * Write a string in a format similar to the UTF-8 format used by the
 * Java DataInputStream.
 *
 * The first two bytes are a network byte order unsigned short specifying
 * the number of additional bytes to be written.
 */
OutMessage &OutMessage::operator<<(const string &str) {
    const int pos = m_data.size();
    const int length = str.length();
    m_data.resize(pos + length + sizeof(uint16_t), 0);
    uint16_t l = htons(length);
    unsigned char *p = &m_data[pos];
    *reinterpret_cast<uint16_t *>(p) = l;
    memcpy(p + sizeof(uint16_t), str.c_str(), length);
    return *this;
}

/**
 * A message that the client sends to the server.
 */
class InMessage {
public:
    class InvalidMessage {

    };

    enum TYPE {
        REQUEST_CHALLENGE = 0,
        CHALLENGE_RESPONSE = 1,
        REGISTER_ACCOUNT = 2,
        JOIN_CHANNEL = 3,
        CHANNEL_MESSAGE = 4,
        CHANNEL_MODE = 5,
        OUTGOING_CHALLENGE = 6,
        RESOLVE_CHALLENGE = 7,
        CHALLENGE_TEAM = 8,
        WITHDRAW_CHALLENGE = 9,
        BATTLE_ACTION = 10,
        PART_CHANNEL = 11,
        REQUEST_CHANNEL_LIST = 12,
        QUEUE_TEAM = 13
    };

    InMessage() {
        reset();
    }

    TYPE getType() const {
        return m_type;
    }

    void reset() {
        m_data.resize(HEADER_SIZE, 0);
        m_pos = 0;
    }

    void processHeader() {
        m_type = (TYPE)m_data[0];
        ++m_pos;
        int32_t size;
        *this >> size;
        m_data.resize(size);
        m_pos = 0;
    }

    vector<unsigned char> &operator()() {
        return m_data;
    }

    InMessage &operator>>(int32_t &i) {
        ensureMoreData(sizeof(int32_t));

        int32_t *p = reinterpret_cast<int32_t *>(&m_data[m_pos]);
        i = ntohl(*p);
        m_pos += sizeof(int32_t);
        return *this;
    }

    InMessage &operator>>(unsigned char &c) {
        ensureMoreData(sizeof(unsigned char));

        c = *(unsigned char *)(&m_data[m_pos]);
        m_pos += sizeof(unsigned char);
        return *this;
    }

    InMessage &operator>>(string &str) {
        ensureMoreData(sizeof(uint16_t));

        uint16_t *p = reinterpret_cast<uint16_t *>(&m_data[m_pos]);
        uint16_t length = ntohs(*p);

        ensureMoreData(sizeof(uint16_t) + length);

        str = string((char *)&m_data[m_pos + sizeof(uint16_t)], length);
        m_pos += length + sizeof(int16_t);
        return *this;
    }

    virtual ~InMessage() { }

private:
    void ensureMoreData(const int count) const {
        const int length = m_data.size();
        if ((length - m_pos) < count)
            throw InvalidMessage();
    }

    vector<unsigned char> m_data;
    TYPE m_type;
    int m_pos;
};

class WelcomeMessage : public OutMessage {
public:
    WelcomeMessage(const int version, const string name, const string &message):
            OutMessage(WELCOME_MESSAGE) {
        *this << version;
        *this << name;
        *this << message;
        finalise();
    }
};

class ChallengeMessage : public OutMessage {
public:
    ChallengeMessage(const unsigned char *challenge):
            OutMessage(PASSWORD_CHALLENGE, 16) {
        for (int i = 0; i < 16; ++i) {
            *this << challenge[i];
        }
    }
};

/**
 * byte : type
 * string : details
 */
class RegistryResponse : public OutMessage {
public:
    enum TYPE {
        NAME_UNAVAILABLE = 0,
        REGISTER_SUCCESS = 1,
        ILLEGAL_NAME = 2,
        TOO_LONG_NAME = 3,
        NONEXISTENT_NAME = 4,
        INVALID_RESPONSE = 5,
        USER_BANNED = 6,
        SUCCESSFUL_LOGIN = 7,
        USER_ALREADY_ON = 8
    };
    RegistryResponse(const TYPE type, const string &details = string()):
            OutMessage(REGISTRY_RESPONSE) {
        *this << ((unsigned char)type);
        *this << details;
        finalise();
    }
};

/**
 * Format of one pokemon:
 *
 * int32  : species id
 * string : nickname
 * byte   : whether the pokemon is shiny
 * byte   : gender
 * int32  : level
 * string : item
 * string : ability
 * int32  : nature
 * int32  : move count
 * for each move:
 *      int32 : move id
 *      int32 : pp ups
 * for each stat i in [0, 5]:
 *      int32 : iv
 *      int32 : ev
 */
Pokemon::PTR readPokemon(SpeciesDatabase *speciesData,
        MoveDatabase *moveData,
        InMessage &msg) {
    int speciesId;
    string nickname;
    unsigned char shiny;
    unsigned char gender;
    int level;
    string item;
    string ability;
    int nature;
    int moveCount;
    vector<string> moves;
    vector<int> ppUp;
    int ivs[STAT_COUNT];
    int evs[STAT_COUNT];

    msg >> speciesId >> nickname >> shiny >> gender >> level
            >> item >> ability >> nature >> moveCount;

    nickname = trim(nickname);

    moves.resize(moveCount);
    ppUp.resize(moveCount);
    for (int i = 0; i < moveCount; ++i) {
        int id, pp;
        msg >> id >> pp;
        moves[i] = moveData->getMove(id);
        ppUp[i] = pp;
    }

    for (int i = 0; i < STAT_COUNT; ++i) {
        msg >> ivs[i] >> evs[i];
    }

    const PokemonSpecies *species = speciesData->getSpecies(speciesId);

    return Pokemon::PTR(new Pokemon(species,
            nickname,
            PokemonNature::getNature(nature),
            ability,
            item,
            ivs,
            evs,
            level,
            gender,
            255, // TODO: happiness
            shiny,
            moves,
            ppUp));
}

void readTeam(SpeciesDatabase *speciesData,
        MoveDatabase *moveData,
        InMessage &msg,
        Pokemon::ARRAY &team) {
    int size;
    msg >> size;
    for (int i = 0; i < size; ++i) {
        team.push_back(readPokemon(speciesData, moveData, msg));
    }
}

struct Challenge {
    mutex mx;
    Pokemon::ARRAY teams[TEAM_COUNT];
    GENERATION generation;
    int partySize;
    // todo: clauses and other options

};

typedef shared_ptr<Challenge> ChallengePtr;

class IncomingChallenge : public OutMessage {
public:
    IncomingChallenge(const string &user, const Challenge &data):
            OutMessage(INCOMING_CHALLENGE) {
        *this << user;
        *this << (unsigned char)data.generation;
        *this << data.partySize;
        finalise();
    }
};

class FinaliseChallenge : public OutMessage {
public:
    FinaliseChallenge(const string &user, const bool accept):
            OutMessage(FINALISE_CHALLENGE) {
        *this << user;
        *this << (unsigned char)accept;
        finalise();
    }
};

class MetagameQueue {
public:
    typedef pair<ClientImplPtr, Pokemon::ARRAY> QUEUE_ENTRY;

    MetagameQueue(MetagamePtr metagame, bool rated, ServerImpl *server):
            m_metagame(metagame),
            m_rated(rated),
            m_server(server) { }
            
    bool queueClient(ClientImplPtr, Pokemon::ARRAY &);

    void startMatches();

private:
    MetagamePtr m_metagame;
    bool m_rated;
    CLIENT_LIST m_clients;
    vector<QUEUE_ENTRY> m_queue;
    map<ClientImplPtr, pair<ClientImplPtr, int> > m_generations;
    mutex m_mutex;
    ServerImpl *m_server;
};

typedef shared_ptr<MetagameQueue> MetagameQueuePtr;
typedef pair<int, bool> METAGAME_PAIR;

class ServerImpl {
public:
    ServerImpl(Server *, tcp::endpoint &);
    Server *getServer() const { return m_server; }
    void run();
    void removeClient(ClientImplPtr client);
    void broadcast(OutMessage &msg);
    void initialiseChannels();
    void initialiseMatchmaking(const string &);
    database::DatabaseRegistry *getRegistry() { return &m_registry; }
    ScriptMachine *getMachine() { return &m_machine; }
    ChannelPtr getMainChannel() const { return m_mainChannel; }
    void sendChannelList(ClientImplPtr client);
    void sendMetagameList(ClientImplPtr client);
    ChannelPtr getChannel(const string &);
    ClientImplPtr getClient(const string &);
    bool authenticateClient(ClientImplPtr client);
    void addChannel(ChannelPtr);
    void removeChannel(ChannelPtr);
    void handleMatchmaking();
    MetagameQueuePtr getMetagameQueue(const int metagame, const bool rated) {
        return m_queues[METAGAME_PAIR(metagame, rated)];
    }

private:
    void acceptClient();
    void handleAccept(ClientImplPtr client,
            const boost::system::error_code &error);

    class ChannelList : public OutMessage {
    public:
        ChannelList(ServerImpl *);
    };

    class MetagameList : public OutMessage {
    public:
        MetagameList(const vector<MetagamePtr> &);
    };

    CHANNEL_LIST m_channels;
    shared_mutex m_channelMutex;
    ChannelPtr m_mainChannel;
    CLIENT_LIST m_clients;
    shared_mutex m_clientMutex;
    io_service m_service;
    tcp::acceptor m_acceptor;
    database::DatabaseRegistry m_registry;
    ScriptMachine m_machine;
    vector<MetagamePtr> m_metagames;
    map<METAGAME_PAIR, MetagameQueuePtr> m_queues;
    thread m_matchmaking;
    shared_ptr<MetagameList> m_metagameList;
    Server *m_server;
};

Server::Server(const int port) {
    tcp::endpoint endpoint(tcp::v4(), 8446);
    m_impl = new ServerImpl(this, endpoint);
}

void Server::run() {
    m_impl->run();
}

database::DatabaseRegistry *Server::getRegistry() {
    return m_impl->getRegistry();
}

ScriptMachine *Server::getMachine() {
    return m_impl->getMachine();
}

void Server::initialiseChannels() {
    m_impl->initialiseChannels();
}

void Server::initialiseMatchmaking(const string &file) {
    m_impl->initialiseMatchmaking(file);
}

ChannelPtr Server::getMainChannel() const {
    return m_impl->getMainChannel();
}

Server::~Server() {
    delete m_impl;
}

class ClientImpl : public Client, public enable_shared_from_this<ClientImpl> {
public:
    ClientImpl(io_service &service, ServerImpl *server):
            m_server(server),
            m_service(service),
            m_socket(service),
            m_authenticated(false),
            m_challenge(0) { }

    tcp::socket &getSocket() {
        return m_socket;
    }
    void sendMessage(const OutMessage &msg) {
        lock_guard<mutex> lock(m_queueMutex);
        const bool empty = m_queue.empty();
        m_queue.push_back(msg);
        const OutMessage &post = m_queue.back();
        if (empty) {
            async_write(m_socket, buffer(post()),
                    boost::bind(&ClientImpl::handleWrite,
                    shared_from_this(), placeholders::error));
        }
    }
    void start() {
        m_ip = m_socket.remote_endpoint().address().to_string();

        async_read(m_socket, buffer(m_msg()),
                boost::bind(&ClientImpl::handleReadHeader,
                shared_from_this(), placeholders::error));
    }
    string getIp() const {
        return m_ip;
    }
    string getName() const {
        return m_name;
    }
    int getId() const {
        return m_id;
    }
    bool isAuthenticated() const {
        return m_authenticated;
    }
    void setAuthenticated(bool auth) {
        m_authenticated = auth;
    }
    /**
     * This method frees a battle by simultaneously removing the battle field
     * from both of the participants' battle lists.
     */
    void terminateBattle(boost::shared_ptr<NetworkBattle> p, ClientPtr client) {
        // dynamic downcast...
        ClientImpl *impl = dynamic_cast<ClientImpl *>(client.get());
        if (impl) {
            lock_guard<mutex> lock(impl->m_battleMutex);
            impl->m_battles.erase(p);
        }
        {
            lock_guard<mutex> lock(m_battleMutex);
            m_battles.erase(p);
        }
    }
    void disconnect();
    void joinChannel(ChannelPtr channel);
    void partChannel(ChannelPtr channel);
    void insertBattle(NetworkBattle::PTR battle) {
        lock_guard<mutex> lock(m_battleMutex);
        m_battles.insert(battle);
    }

private:

    ChannelPtr getChannel(const int id);

    ChallengePtr getChallenge(const string &name) {
        lock_guard<mutex> lock(m_challengeMutex);

        map<string, ChallengePtr>::iterator i = m_challenges.find(name);
        if (i == m_challenges.end())
            return ChallengePtr();
        return i->second;
    }

    // to be called from within a critical section on m_battleMutex
    NetworkBattle::PTR getBattle(const int id) {
        BATTLE_LIST::iterator i = m_battles.begin();
        for (; i != m_battles.end(); ++i) {
            if ((*i)->getId() == id)
                return *i;
        }
        return NetworkBattle::PTR();
    }

    void rejectChallenge(const string &name) {
        lock_guard<mutex> lock(m_challengeMutex);

        map<string, ChallengePtr>::iterator i = m_challenges.find(name);
        if (i == m_challenges.end())
            return;

        m_challenges.erase(i);

        sendMessage(FinaliseChallenge(name, false));
    }

    /**
     * Handle reading in the header of a message.
     */
    void handleReadHeader(const boost::system::error_code &error) {
        if (error) {
            handleError(error);
            return;
        }

        m_msg.processHeader();
        async_read(m_socket, buffer(m_msg()),
                boost::bind(&ClientImpl::handleReadBody,
                shared_from_this(), placeholders::error));
    }

    void handleReadBody(const boost::system::error_code &error);

    /**
     * Handle the completion of writing a message.
     */
    void handleWrite(const boost::system::error_code &error) {
        if (error) {
            handleError(error);
            return;
        }

        lock_guard<mutex> lock(m_queueMutex);
        m_queue.pop_front();
        if (!m_queue.empty()) {
            async_write(m_socket, buffer(m_queue.front()()),
                    boost::bind(&ClientImpl::handleWrite,
                    shared_from_this(), placeholders::error));
        }
    }

    void handleError(const boost::system::error_code &error) {
        m_server->removeClient(shared_from_this());
    }

    /**
     * string : user name
     */
    void handleRequestChallenge(InMessage &msg) {
        string user;
        msg >> user;
        unsigned char data[16];
        m_challenge = m_server->getRegistry()->getAuthChallenge(user, data);
        if (m_challenge != 0) {
            // user exists

            if (m_server->getClient(user)) {
                // user is already online
                sendMessage(RegistryResponse(
                        RegistryResponse::USER_ALREADY_ON));
                return;
            }

            m_name = user;
            ChallengeMessage msg(data);
            sendMessage(msg);
        } else {
            sendMessage(RegistryResponse(RegistryResponse::NONEXISTENT_NAME));
        }
    }

    /**
     * byte[16] : challenge response
     */
    void handleChallengeResponse(InMessage &msg) {
        if (m_challenge == 0) {
            // no challenge in progress
            return;
        }

        unsigned char data[16];
        for (int i = 0; i < 16; ++i) {
            msg >> data[i];
        }
        database::DatabaseRegistry *registry = m_server->getRegistry();
        database::DatabaseRegistry::AUTH_PAIR auth =
                registry->isResponseValid(m_name, m_challenge, data);
        m_challenge = 0;

        if (!auth.first) {
            sendMessage(RegistryResponse(RegistryResponse::INVALID_RESPONSE));
            return;
        }

        if (!m_server->authenticateClient(shared_from_this())) {
            // user is already online
            sendMessage(RegistryResponse(
                    RegistryResponse::USER_ALREADY_ON));
            return;
        }

        m_id = auth.second;

        sendMessage(RegistryResponse(RegistryResponse::SUCCESSFUL_LOGIN));
        m_server->sendMetagameList(shared_from_this());
    }

    /**
     * string : user name
     * string : password (plaintext)
     */
    void handleRegisterAccount(InMessage &msg) {
        string user, password;
        msg >> user >> password;

        user = trim(user);
        // todo: collapse whitespace everywhere within the name

        if (user.length() > 18) {
            sendMessage(RegistryResponse(RegistryResponse::TOO_LONG_NAME));
            return;
        }

        boost::regex e("[A-Za-z0-9_ \\-\\.]+");
        if (!regex_match(user, e)) {
            sendMessage(RegistryResponse(RegistryResponse::ILLEGAL_NAME));
            return;
        }

        database::DatabaseRegistry *registry = m_server->getRegistry();
        if (!registry->registerUser(user, password, m_ip)) {
            sendMessage(RegistryResponse(RegistryResponse::NAME_UNAVAILABLE));
            return;
        }

        sendMessage(RegistryResponse(RegistryResponse::REGISTER_SUCCESS));
    }

    /**
     * string : channel name
     */
    void handleJoinChannel(InMessage &msg) {
        string channel;
        msg >> channel;
        ChannelPtr p = m_server->getChannel(channel);
        if (p) {
            joinChannel(p);
        }
    }

    /**
     * int32 : channel id
     * string : message
     */
    void handleChannelMessage(InMessage &msg);

    /**
     * int32 : channel id
     * string : user
     * byte : the mode to change
     * byte : whether to enable or disable it (1 or 0)
     */
    void handleModeMessage(InMessage &msg);

    /**
     * string : opponent
     * ... [ TODO: other data ] ...
     */
    void handleOutgoingChallenge(InMessage &msg) {
        ChallengePtr challenge(new Challenge());
        string opponent;
        unsigned char generation;
        int partySize;
        msg >> opponent >> generation >> partySize;
        // todo: read in other challenge details

        challenge->generation = (GENERATION)generation;
        challenge->partySize = partySize;

        lock_guard<mutex> lock(m_challengeMutex);

        if (m_challenges.count(opponent) != 0)
            return;

        ClientImplPtr client = m_server->getClient(opponent);
        if (!client) {
            return;
        }

        m_challenges[opponent] = challenge;
        client->sendMessage(IncomingChallenge(m_name, *challenge));
    }

    void handleResolveChallenge(InMessage &msg) {
        string opponent;
        unsigned char accept;
        msg >> opponent >> accept;

        ClientImplPtr client = m_server->getClient(opponent);
        if (!client) {
            return;
        }

        if (!accept) {
            client->rejectChallenge(m_name);
            return;
        }

        ChallengePtr challenge = client->getChallenge(m_name);
        if (!challenge) {
            return;
        }

        Pokemon::ARRAY team;
        ScriptMachine *machine = m_server->getMachine();
        readTeam(machine->getSpeciesDatabase(),
                machine->getMoveDatabase(),
                msg,
                team);
        // todo: verify legality of team
        unique_lock<mutex> lock(challenge->mx);
        challenge->teams[1] = team;
        lock.unlock();

        client->sendMessage(FinaliseChallenge(m_name, true));
    }

    void handleChallengeTeam(InMessage &msg) {
        unique_lock<mutex> lock(m_challengeMutex);

        string opponent;
        msg >> opponent;

        if (m_challenges.count(opponent) == 0)
            return;

        ClientImplPtr client = m_server->getClient(opponent);
        if (!client) {
            return;
        }

        ChallengePtr challenge = m_challenges[opponent];

        unique_lock<mutex> lock2(challenge->mx);
        if (challenge->teams[1].empty()) {
            return;
        }

        m_challenges.erase(opponent);
        lock.unlock();

        ScriptMachine *machine = m_server->getMachine();
        readTeam(machine->getSpeciesDatabase(),
                machine->getMoveDatabase(),
                msg,
                challenge->teams[0]);
        // todo: verify legality of team

        ClientPtr clients[] = { shared_from_this(), client };
        NetworkBattle::PTR field(new NetworkBattle(m_server->getServer(),
                clients,
                challenge->teams,
                challenge->generation,
                challenge->partySize,
                6, // TODO: Do not hardcode max team length.
                -1,
                false));

        lock2.unlock();

        // todo: apply clauses here

        field->beginBattle();
        insertBattle(field);
        client->insertBattle(field);
    }

    void handleWithdrawChallenge(InMessage &msg) {
        // todo
    }

    /**
     * int32 : field id
     * byte : turn type
     * byte : index
     * byte : target
     */
    void handleBattleAction(InMessage &msg) {
        int field;
        unsigned char tt, idx, target;
        msg >> field >> tt >> idx >> target;
        PokemonTurn turn((TURN_TYPE)tt, idx, target);

        lock_guard<mutex> lock(m_battleMutex);
        NetworkBattle::PTR p = getBattle(field);
        if (p) {
            const int party = p->getParty(shared_from_this());
            if (party != -1) {
                p->handleTurn(party, turn);
            }
        }
    }

    /**
     * int32 : channel id
     */
    void handlePartChannel(InMessage &msg) {
        int32_t id;
        msg >> id;
        ChannelPtr p = getChannel(id);
        if (p) {
            partChannel(p);
        }
    }

    void handleRequestChannelList(InMessage &) {
        m_server->sendChannelList(shared_from_this());
    }

    /**
     * byte : metagame id
     * byte : rated
     * team : the pokemon team to queue
     */
    void handleQueueTeam(InMessage &msg) {
        unsigned char metagame;
        unsigned char rated;
        msg >> metagame >> rated;
        MetagameQueuePtr queue = m_server->getMetagameQueue(metagame, rated);
        if (!queue) {
            return;
        }
        Pokemon::ARRAY team;
        ScriptMachine *machine = m_server->getMachine();
        readTeam(machine->getSpeciesDatabase(),
                machine->getMoveDatabase(),
                msg,
                team);
        // TODO: Verify legality of team.
        queue->queueClient(shared_from_this(), team);
    }

    string m_name;
    int m_id;   // user id
    bool m_authenticated;
    int m_challenge; // for challenge-response authentication

    map<string, ChallengePtr> m_challenges;
    mutex m_challengeMutex;

    BATTLE_LIST m_battles;
    mutex m_battleMutex;

    InMessage m_msg;
    deque<OutMessage> m_queue;
    mutex m_queueMutex;
    io_service &m_service;
    tcp::socket m_socket;
    string m_ip;
    ServerImpl *m_server;
    CHANNEL_LIST m_channels;    // channels this user is in
    shared_mutex m_channelMutex;

    typedef void (ClientImpl::*MESSAGE_HANDLER)(InMessage &msg);
    static const MESSAGE_HANDLER m_handlers[];
    static const int MESSAGE_COUNT;
};

const ClientImpl::MESSAGE_HANDLER ClientImpl::m_handlers[] = {
    &ClientImpl::handleRequestChallenge,
    &ClientImpl::handleChallengeResponse,
    &ClientImpl::handleRegisterAccount,
    &ClientImpl::handleJoinChannel,
    &ClientImpl::handleChannelMessage,
    &ClientImpl::handleModeMessage,
    &ClientImpl::handleOutgoingChallenge,
    &ClientImpl::handleResolveChallenge,
    &ClientImpl::handleChallengeTeam,
    &ClientImpl::handleWithdrawChallenge,
    &ClientImpl::handleBattleAction,
    &ClientImpl::handlePartChannel,
    &ClientImpl::handleRequestChannelList,
    &ClientImpl::handleQueueTeam
};

const int ClientImpl::MESSAGE_COUNT =
        sizeof(m_handlers) / sizeof(m_handlers[0]);

/**
 * Handle reading in the body of a message.
 */
void ClientImpl::handleReadBody(const boost::system::error_code &error) {
    if (error) {
        handleError(error);
        return;
    }

    const int type = (int)m_msg.getType();
    if ((type > 2) && !m_authenticated) {
        m_server->removeClient(shared_from_this());
        return;
    }
    if (type < MESSAGE_COUNT) {
        try {
            // Call the handler for this type of message.
            (this->*m_handlers[type])(m_msg);
        } catch (InMessage::InvalidMessage &) {
            // The client sent an invalid message.
            // Disconnect him immediately.
            m_server->removeClient(shared_from_this());
            return;
        }
    }

    // Read another message.
    m_msg.reset();
    async_read(m_socket, buffer(m_msg()),
            boost::bind(&ClientImpl::handleReadHeader,
            shared_from_this(), placeholders::error));
}

void ClientImpl::joinChannel(ChannelPtr channel) {
    if (channel->join(shared_from_this())) {
        lock_guard<shared_mutex> lock(m_channelMutex);
        m_channels.insert(channel);
    }
}

void ClientImpl::partChannel(ChannelPtr channel) {
    lock_guard<shared_mutex> lock(m_channelMutex);
    channel->part(shared_from_this());
    m_channels.erase(channel);
}

ChannelPtr ClientImpl::getChannel(const int id) {
    shared_lock<shared_mutex> lock(m_channelMutex);
    CHANNEL_LIST::iterator i = m_channels.begin();
    for (; i != m_channels.end(); ++i) {
        if ((*i)->getId() == id)
            return *i;
    }
    return ChannelPtr();
}

/**
 * int32 : channel id
 * string : user
 * byte : the mode to change
 * byte : whether to enable or disable it (1 or 0)
 */
void ClientImpl::handleModeMessage(InMessage &msg) {
    int id;
    string user;
    unsigned char mode;
    unsigned char enabled;
    msg >> id >> user >> mode >> enabled;
    ChannelPtr channel = getChannel(id);
    if (!channel) {
        return;
    }
    channel->setMode(shared_from_this(), user, mode, enabled);
}

/**
 * int32 : channel id
 * string : message
 */
void ClientImpl::handleChannelMessage(InMessage &msg) {
    int id;
    string message;
    msg >> id >> message;
    ChannelPtr p = getChannel(id);
    if (p) {
        p->sendMessage(message, shared_from_this());
    }
}

void ClientImpl::disconnect() {
    lock_guard<shared_mutex> lock(m_channelMutex);
    CHANNEL_LIST::iterator i = m_channels.begin();
    for (; i != m_channels.end(); ++i) {
        (*i)->part(shared_from_this());
    }
    m_channels.clear();
}

bool MetagameQueue::queueClient(ClientImplPtr client, Pokemon::ARRAY &team) {
    lock_guard<mutex> lock(m_mutex);
    if (m_clients.find(client) != m_clients.end())
        return false;
    if (team.size() > m_metagame->getMaxTeamLength())
        return false;
    // TODO: validate team with the metagame's other rules
    m_queue.push_back(QUEUE_ENTRY(client, team));
    return true;
}

void MetagameQueue::startMatches() {
    lock_guard<mutex> lock(m_mutex);
    if (m_queue.size() < 2)
        return;
    // TODO: Sort the list of clients.
    // TODO: Use the generations map to prevent rematches.
    QUEUE_ENTRY remainder;
    int size = m_queue.size();
    const bool hasRemainder = size % 2;
    if (hasRemainder) {
        --size;
        remainder = m_queue.back();
    }
    for (int i = 0; i < size; i += 2) {
        QUEUE_ENTRY &q1 = m_queue[i];
        QUEUE_ENTRY &q2 = m_queue[i + 1];
        ClientPtr clients[] = { q1.first, q2.first };
        Pokemon::ARRAY teams[] = { q1.second, q2.second };
        NetworkBattle::PTR field(new NetworkBattle(m_server->getServer(),
                clients,
                teams,
                (GENERATION)m_metagame->getGeneration(),
                m_metagame->getActivePartySize(),
                m_metagame->getMaxTeamLength(),
                m_metagame->getIdx(),
                m_rated));
        // TODO: Apply clauses here.
        field->beginBattle();
        q1.first->insertBattle(field);
        q2.first->insertBattle(field);
    }
    m_clients.clear();
    m_queue.clear();
    if (hasRemainder) {
        m_queue.push_back(remainder);
        m_clients.insert(remainder.first);
    }
}

void ServerImpl::sendChannelList(ClientImplPtr client) {
    client->sendMessage(ChannelList(this));
}

void ServerImpl::sendMetagameList(ClientImplPtr client) {
    client->sendMessage(*m_metagameList);
}

void ServerImpl::addChannel(ChannelPtr p) {
    lock_guard<shared_mutex> lock(m_channelMutex);
    m_channels.insert(p);
}

void Server::addChannel(ChannelPtr p) {
    m_impl->addChannel(p);
}

void ServerImpl::removeChannel(ChannelPtr p) {
    lock_guard<shared_mutex> lock(m_channelMutex);
    m_channels.erase(p);
}

void Server::removeChannel(ChannelPtr p) {
    m_impl->removeChannel(p);
}

ServerImpl::ChannelList::ChannelList(ServerImpl *server):
        OutMessage(CHANNEL_LIST) {
    shared_lock<shared_mutex> lock(server->m_channelMutex);
    *this << (int)server->m_channels.size();
    CHANNEL_LIST::iterator i = server->m_channels.begin();
    for (; i != server->m_channels.end(); ++i) {
        ChannelPtr p = *i;
        *this << p->getName();
        *this << (unsigned char)p->getChannelType();
        *this << p->getTopic();
        *this << p->getPopulation();
    }
    finalise();
}

ServerImpl::MetagameList::MetagameList(const vector<MetagamePtr> &metagames):
        OutMessage(METAGAME_LIST) {
    const int size = metagames.size();
    *this << (int16_t)size;
    for (int i = 0; i < size; ++i) {
        MetagamePtr p = metagames[i];
        *this << (unsigned char)p->getIdx();
        *this << p->getName();
        *this << p->getId();
        *this << p->getDescription();
        *this << (unsigned char)p->getActivePartySize();
        *this << (unsigned char)p->getMaxTeamLength();
        const set<unsigned int> &banList = p->getBanList();
        *this << (int16_t)banList.size();
        set<unsigned int>::const_iterator j = banList.begin();
        for (; j != banList.end(); ++j) {
            *this << (int16_t)(*j);
        }
        const vector<string> &clauses = p->getClauses();
        const int size2 = clauses.size();
        *this << (int16_t)size2;
        for (int k = 0; k < size2; ++k) {
            *this << clauses[k];
        }
    }
    finalise();
}

ServerImpl::ServerImpl(Server *server, tcp::endpoint &endpoint):
        m_server(server),
        m_acceptor(m_service, endpoint) {
    acceptClient();
}

bool ServerImpl::authenticateClient(ClientImplPtr client) {
    lock_guard<shared_mutex> lock(m_clientMutex);
    const string &name = client->getName();
    CLIENT_LIST::iterator i = m_clients.begin();
    for (; i != m_clients.end(); ++i) {
        if ((*i)->isAuthenticated() && iequals((*i)->getName(), name))
            return false;
    }
    client->setAuthenticated(true);
    return true;
}

ClientImplPtr ServerImpl::getClient(const string &name) {
    shared_lock<shared_mutex> lock(m_clientMutex);
    CLIENT_LIST::iterator i = m_clients.begin();
    for (; i != m_clients.end(); ++i) {
        if ((*i)->isAuthenticated() && iequals((*i)->getName(), name))
            return *i;
    }
    return ClientImplPtr();
}

ChannelPtr ServerImpl::getChannel(const string &name) {
    shared_lock<shared_mutex> lock(m_channelMutex);
    CHANNEL_LIST::iterator i = m_channels.begin();
    for (; i != m_channels.end(); ++i) {
        if ((*i)->getName() == name)
            return *i;
    }
    return ChannelPtr();
}

/**
 * Initialise channels from the database.
 */
void ServerImpl::initialiseChannels() {
    using database::DatabaseRegistry;

    fs::create_directory("logs");
    fs::create_directory("logs/chat");

    DatabaseRegistry::CHANNEL_INFO info;
    m_registry.getChannelInfo(info);
    DatabaseRegistry::CHANNEL_INFO::iterator i = info.begin();
    for (; i != info.end(); ++i) {
        database::DatabaseRegistry::INFO_ELEMENT &element = i->second;
        const string name = element.get<0>();
        const string topic = element.get<1>();
        const Channel::CHANNEL_FLAGS flags = element.get<2>();
        ChannelPtr p = ChannelPtr(new Channel(m_server,
                name, topic, flags, i->first));
        if (name == "main") {
            m_mainChannel = p;
        }
        m_channels.insert(p);
        fs::create_directory("logs/chat/" + name);
    }

    assert(m_mainChannel);
}

void ServerImpl::initialiseMatchmaking(const string &file) {
    SpeciesDatabase *species = m_machine.getSpeciesDatabase();
    Metagame::readMetagames(file, species, m_metagames);
    vector<MetagamePtr>::const_iterator i = m_metagames.begin();
    for (; i != m_metagames.end(); ++i) {
        m_registry.initialiseLadder((*i)->getId());
        const int idx = (*i)->getIdx();
        // rated queue
        m_queues[METAGAME_PAIR(idx, true)] =
                MetagameQueuePtr(new MetagameQueue(*i, true, this));
        // unrated queue
        m_queues[METAGAME_PAIR(idx, false)] =
                MetagameQueuePtr(new MetagameQueue(*i, false, this));
    }
    m_metagameList = shared_ptr<MetagameList>(new MetagameList(m_metagames));
    m_matchmaking = thread(boost::bind(&ServerImpl::handleMatchmaking, this));
}

void ServerImpl::handleMatchmaking() {
    while (true) {
        this_thread::sleep(posix_time::seconds(15));
        map<pair<int, bool>, MetagameQueuePtr>::iterator i = m_queues.begin();
        for (; i != m_queues.end(); ++i) {
            i->second->startMatches();
        }
    }
}

/** Start the server. */
void ServerImpl::run() {
    m_registry.startThread();
    m_service.run();
}

/**
 * Remove a client.
 */
void ServerImpl::removeClient(ClientImplPtr client) {
    // TODO: other removal logic
    client->disconnect();
    cout << "Client from " << client->getIp() << " disconnected." << endl;

    lock_guard<shared_mutex> lock(m_clientMutex);
    m_clients.erase(client);
}

/**
 * Send a message to all clients.
 */
void ServerImpl::broadcast(OutMessage &msg) {
    shared_lock<shared_mutex> lock(m_clientMutex);
    for_each(m_clients.begin(), m_clients.end(),
            boost::bind(&ClientImpl::sendMessage, _1, boost::ref(msg)));
}

void ServerImpl::acceptClient() {
    ClientImplPtr client(new ClientImpl(m_service, this));
    m_acceptor.async_accept(client->getSocket(),
            boost::bind(&ServerImpl::handleAccept, this,
            client, placeholders::error));
}

void ServerImpl::handleAccept(ClientImplPtr client,
        const boost::system::error_code &error) {
    acceptClient();
    if (!error) {
        {
            lock_guard<shared_mutex> lock(m_clientMutex);
            m_clients.insert(client);
        }
        client->start();
        cout << "Accepted client from " << client->getIp() << "." << endl;
        WelcomeMessage msg(2,
                "Official Server",
                "Welcome to Shoddy Battle 2!");
        client->sendMessage(msg);
    }
}


}} // namespace shoddybattle::network

#if 1

#include <boost/thread.hpp>

int main() {
    using namespace shoddybattle;

    network::Server server(8446);

    ScriptMachine *machine = server.getMachine();
    machine->acquireContext()->runFile("resources/main.js");

    machine->getSpeciesDatabase()->verifyAbilities(machine);

    database::DatabaseRegistry *registry = server.getRegistry();
    registry->connect("shoddybattle2", "localhost", "Catherine", "");
    registry->startThread();

    server.initialiseChannels();
    server.initialiseMatchmaking("resources/metagames.xml");

    vector<shared_ptr<thread> > threads;
    for (int i = 0; i < 20; ++i) {
        threads.push_back(shared_ptr<thread>(
                new thread(boost::bind(&network::Server::run, &server))));
    }

    server.run(); // block
}

#endif
