/*
 * File:   main.cpp
 * Author: Catherine
 *
 * Created on August 28, 2010, 2:48 PM
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

#if 1

#include <fstream>
#include <vector>
#include <csignal>
#include <boost/program_options.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <libdaemon/daemon.h>
#include "../shoddybattle/PokemonSpecies.h"
#include "../scripting/ScriptMachine.h"
#include "../database/DatabaseRegistry.h"
#include "../database/Authenticator.h"
#include "../network/NetworkBattle.h"
#include "Log.h"
#include "LogFile.h"

using namespace std;
using namespace shoddybattle;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace {

string pidFile;

const char *getPidFileName() {
    return pidFile.c_str();
}

int initialise(int argc, char **argv, bool &daemon) {
    string configFile;
    int port, databasePort, workerThreads, serverUid, userLimit;
    string serverName, welcomeFile, welcomeMessage;
    string databaseName, databaseHost, databaseUser, databasePassword;
    string authParameter, loginParameter, registerParameter;

    po::options_description generic("Options");
    generic.add_options()
            ("help", "show this help message")
            ("server.detach",
                "run the server as a daemon")
            ("server.kill",
                "kill a running server daemon")
            ("server.name",
                po::value<string>(&serverName)->default_value(
                     "Test Server"),
                "server name")
            ("server.welcome",
                po::value<string>(&welcomeFile),
                "welcome message file")
            ("server.log",
                "write server messages to file")
            ("server.port",
                po::value<int>(&port)->default_value(
                     8446),
                "server port")
            ("server.limit",
                po::value<int>(&userLimit)->default_value(
                     20),
                "maximum number of users allowed")
            ("server.threads",
                po::value<int>(&workerThreads)->default_value(
                     20),
                "number of worker threads for network I/O")
            ("server.uid",
                po::value<int>(&serverUid),
                "UID to run the server process as")
            ("auth.salt",
                "use simple salt authentication")
            ("auth.vbulletin",
                po::value<string>(
                    &authParameter)->implicit_value("vbulletin"),
                "use vbulletin authentication")
            ("auth.vbulletin.login",
                po::value<string>(&loginParameter),
                "login HTML message")
            ("auth.vbulletin.register",
                po::value<string>(&registerParameter),
                "registration HTML message")
            ("mysql.name",
                po::value<string>(&databaseName)->default_value(
                    "shoddybattle2"),
                "MySQL database name")
            ("mysql.host",
                po::value<string>(&databaseHost)->default_value(
                    "localhost"),
                "MySQL server host name")
            ("mysql.port",
                po::value<int>(&databasePort)->default_value(
                    3306),
                "MySQL server port")
            ("mysql.user",
                po::value<string>(&databaseUser)->default_value(
                    "root"),
                "MySQL user")
            ("mysql.password",
                po::value<string>(&databasePassword)->default_value(
                    ""),
                "MySQL password")
    ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
            ("config-file", po::value<string>(&configFile)->default_value(
                    "config"))
    ;

    po::options_description desc;
    desc.add(generic).add(hidden);

    po::positional_options_description p;
    p.add("config-file", 1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).
                options(desc).positional(p).run(), vm);
    } catch (po::error &e) {
        Log::out() << "Error reading command line: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    po::notify(vm);

    if (vm.count("help")) {
        Log::out() << "Usage: shoddybattle2 [options] [config-file = config]"
             << endl
             << "   Reads the specified config file first, or the file named"
                " config in the\n   program's directory, and then reads"
                " extra options from the command line." << endl
             << generic << endl;
        return EXIT_SUCCESS;
    }

    if (vm.count("config-file")) {
        ifstream file(configFile.c_str());
        if (!file.is_open()) {
            Log::out() << "Error: Config file " << configFile << " not found."
                 << endl;
            return EXIT_FAILURE;
        }
        try {
            po::store(po::parse_config_file(file, desc), vm);
        } catch (po::error &e) {
            Log::out() << "Error reading config file: " << e.what() << endl;
            return EXIT_FAILURE;
        }
        po::notify(vm);
    }

    if (vm.count("server.uid")) {
        if (seteuid(serverUid)) {
            // Failed to set the UID.
            Log::out() << "Failed to set the effective UID to " << serverUid
                    << ". Try\n\n    sudo shoddybattle2\n\ninstead." << endl;
            return EXIT_FAILURE;
        }
    }

    if (vm.count("server.welcome")) {
        ifstream file(welcomeFile.c_str());
        if (!file.is_open()) {
            Log::out() << "Error: Welcome file " << configFile << " not found."
                 << endl;
            return EXIT_FAILURE;
        }
        file.seekg(0, ios::end);
        const int length = file.tellg();
        file.seekg(0, ios::beg);
        char text[length + 1];
        file.read(text, length);
        text[length] = '\0';
        welcomeMessage = text;
    }

    if (vm.count("server.log")) {
        Log::out.setMode(Log::MODE_BOTH);
    }

    const bool serverDetach = vm.count("server.detach");
    const bool serverKill = vm.count("server.kill");

    if (serverDetach || serverKill) {
        fs::path path = (fs::initial_path() / argv[0]).remove_filename();
        path = path.normalize() / "shoddybattle2.pid";
        pidFile = path.string();
        daemon_pid_file_proc = getPidFileName;
        const string baseDir = path.remove_filename().file_string();
        
        if (serverKill) {
            const int ret = daemon_pid_file_kill_wait(SIGTERM, 10);
            if (ret < 0) {
                Log::out() << "Failed to kill the server daemon."
                     << endl;
                return EXIT_FAILURE;
            }
            Log::out() << "Successfully killed the server daemon." << endl;
            return EXIT_SUCCESS;
        }

        pid_t pid = daemon_pid_file_is_running();
        if (pid >= 0) {
            Log::out() << "The server daemon is already running on PID " << pid
                    << ".\nTry shoddybattle2 --server.kill to kill it."
                    << endl;
            return EXIT_FAILURE;
        }
        if (daemon_retval_init() < 0) {
            Log::out() << "Failed to create pipe." << endl;
            return EXIT_FAILURE;
        }
        if ((pid = fork()) < 0) {
            daemon_retval_done();
            return EXIT_FAILURE;
        }
        if (pid) {
            // This is the parent process.
            const int ret = daemon_retval_wait(20);
            if (ret < 0) {
                Log::out() << "Received no response from the daemon." << endl;
                return EXIT_FAILURE;
            }
            if (ret) {
                Log::out() << "The server encountered an error while trying to "
                        "startup. Check the log file for details on the error."
                     << endl;
                return EXIT_FAILURE;
            }

            Log::out() << "Successfully started the daemon as PID " << pid
                 << "." << endl;
            return EXIT_SUCCESS;
        } else {
            // This is the daemon process.
            fclose(stdin);
            fclose(stdout);
            fclose(stderr);
            
            // This technique isn't advised, but I'm going to do it anyway
            // to allow paths relative to the program's location.
            chdir(baseDir.c_str());

            // Write logs to the log directory when running as a daemon.
            Log::out.setMode(Log::MODE_FILE);
            
            // Set this flag to indicate that we need to send a message back to
            // the parent process.
            daemon = true;
            if (daemon_close_all(-1) < 0) {
                Log::out() << "Failed to close all file descriptors." << endl;
                return EXIT_FAILURE;
            }
            if (daemon_pid_file_create() < 0) {
                Log::out() << "Failed to create a PID file." << endl;
                return EXIT_FAILURE;
            }
        }
    }

    network::Server server(port, userLimit);
    server.installSignalHandlers();

    ScriptMachine *machine = server.getMachine();
    machine->acquireContext()->runFile("resources/main.js");
    machine->finalise();

    database::DatabaseRegistry *registry = server.getRegistry();
    registry->connect(databaseName, databaseHost,
            databaseUser, databasePassword, databasePort);
    registry->startThread();

    if (vm.count("auth.salt")) {
        registry->setAuthenticator(boost::shared_ptr<database::Authenticator>(
                new database::SaltAuthenticator()));
    }

    if (vm.count("auth.vbulletin")) {
        registry->setAuthenticator(boost::shared_ptr<database::Authenticator>(
                new database::VBulletinAuthenticator(loginParameter,
                        registerParameter, authParameter)));
    }

    registry->createDefaultDatabase();

    server.initialiseWelcomeMessage(serverName, welcomeMessage);
    server.initialiseChannels();
    server.initialiseMatchmaking("resources/metagames.xml");
    server.initialiseClauses();

    network::NetworkBattle::startTimerThread();

    vector<boost::shared_ptr<boost::thread> > threads;
    for (int i = 0; i < workerThreads; ++i) {
        threads.push_back(boost::shared_ptr<boost::thread>(new boost::thread(
                boost::bind(&network::Server::run, &server))));
    }

    if (daemon) {
        daemon_retval_send(0); // started up successfully
    }

    // This call will block until the process receives one of SIGTERM and
    // friends, at which point this function will be able to end gracefully.
    server.run();

    for_each(threads.begin(), threads.end(),
            boost::bind(&boost::thread::join, _1));
    return EXIT_SUCCESS;
}

}

int main(int argc, char **argv) {
    bool daemon = false;
    try {
        const int ret = initialise(argc, argv, daemon);
        if (daemon) {
            if (ret) {
                daemon_retval_send(ret);
            }
            daemon_pid_file_remove();
        }
        return ret;
    } catch (...) {
        if (daemon) {
            // Record the exception in the log.
            Log::out() << "Terminating due to an uncaught exception." << endl;
            LogFile &logFile = Log::out.getLogFile();
            logFile.close();
            freopen(logFile.getCurrentLogFileName().c_str(), "a", stderr);
            
            daemon_retval_send(1); // error condition
            daemon_pid_file_remove();
        }
        throw; // rethrow exception
    }
}

#endif
