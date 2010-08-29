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
    string configFile, logFile;
    int port, databasePort, workerThreads;
    string databaseName, databaseHost, databaseUser, databasePassword;
    string authParameter;

    po::options_description generic("Options");
    generic.add_options()
            ("help", "show this help message")
            ("server.detach",
                "run the server as a daemon")
            ("server.kill",
                "kill a running server daemon")
            ("server.log",
                po::value<string>(&logFile),
                "log file for server output")
            ("server.port",
                po::value<int>(&port)->default_value(
                     8446),
                "server port")
            ("server.threads",
                po::value<int>(&workerThreads)->default_value(
                     20),
                "number of worker threads for network I/O")
            ("auth.vbulletin",
                po::value<string>(
                    &authParameter)->implicit_value("vbulletin"),
                "use vbulletin authentication")
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
        cout << "Error reading command line: " << e.what() << endl;
        return 1;
    }
    po::notify(vm);

    if (vm.count("help")) {
        cout << "Usage: shoddybattle2 [options] [config-file = config]" << endl
             << "   Reads the specified config file first, or the file named"
                " config in the\n   program's directory, and then reads"
                " extra options from the command line." << endl
             << generic << endl;
        return EXIT_SUCCESS;
    }

    if (vm.count("config-file")) {
        ifstream file(configFile.c_str());
        if (!file.is_open()) {
            cout << "Error: Config file " << configFile << " not found.\n";
            return 1;
        }
        try {
            po::store(po::parse_config_file(file, desc), vm);
        } catch (po::error &e) {
            cout << "Error reading config file: " << e.what() << endl;
            return 1;
        }
        po::notify(vm);
    }

    const bool serverDetach = vm.count("server.detach");
    const bool serverKill = vm.count("server.kill");

    if (serverDetach || serverKill) {
        fs::path path = (fs::initial_path() / argv[0]).remove_filename();
        path = path.normalize() / "shoddybattle.pid";
        pidFile = path.string();
        daemon_pid_file_proc = getPidFileName;
        const string baseDir = path.remove_filename().file_string();
        
        if (serverKill) {
            const int ret = daemon_pid_file_kill_wait(SIGTERM, 10);
            if (ret < 0) {
                cout << "Failed to kill the server daemon."
                     << endl;
                return 1;
            }
            return EXIT_SUCCESS;
        }

        pid_t pid = daemon_pid_file_is_running();
        if (pid >= 0) {
            cout << "The server daemon is already running on PID " << pid
                    << ".\nTry shoddybattle2 --server.kill to kill it.\n";
            return 1;
        }
        if (daemon_retval_init() < 0) {
            cout << "Failed to create pipe." << endl;
            return 1;
        }
        if (!vm.count("server.log")) {
            cout << "Warning: Since you have not specified a log file, you "
                    "will not be able to see the output of the server daemon."
                 << endl;
        }
        if ((pid = daemon_fork()) < 0) {
            daemon_retval_done();
            return 1;
        }
        if (pid) {
            // This is the parent process.
            const int ret = daemon_retval_wait(20);
            if (ret < 0) {
                cout << "Received no response from the daemon." << endl;
                return 1;
            }
            if (ret == 2) {
                cout << "Failed to open the specified log for write access."
                        << endl;
                return 1;
            }
            if (ret) {
                cout << "The server encountered an error while trying to "
                        "startup. Check the log file for details on the error."
                     << endl;
                return 1;
            }

            cout << "Successfully started the daemon as PID " << pid
                 << "." << endl;
            return 0;
        } else {
            // This is the daemon process.
            
            // This technique isn't advised, but I'm going to do it anyway
            // to allow paths relative to the program's location.
            chdir(baseDir.c_str());
            // Set this flag to indicate that we need to send a message back to
            // the parent process.
            daemon = true;
            if (!logFile.empty()) {
                if (!freopen(logFile.c_str(), "w", stdout)) {
                    return 2;
                }
                if (!freopen(logFile.c_str(), "w", stderr)) {
                    return 2;
                }
            }
            if (daemon_close_all(-1) < 0) {
                cout << "Failed to close all file descriptors." << endl;
                return 1;
            }
            if (daemon_pid_file_create() < 0) {
                cout << "Failed to create a PID file." << endl;
                return 1;
            }
        }
    }

    network::Server server(port);
    server.installSignalHandlers();

    ScriptMachine *machine = server.getMachine();
    machine->acquireContext()->runFile("resources/main.js");
    machine->getSpeciesDatabase()->verifyAbilities(machine);

    database::DatabaseRegistry *registry = server.getRegistry();
    registry->connect(databaseName, databaseHost,
            databaseUser, databasePassword, databasePort);
    registry->startThread();

    if (vm.count("auth.vbulletin")) {
        registry->setAuthenticator(boost::shared_ptr<database::Authenticator>(
                new database::VBulletinAuthenticator(authParameter)));
    }

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

    server.run(); // block
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
            daemon_retval_send(1); // error condition
            daemon_pid_file_remove();
        }
        throw; // rethrow exception
    }
}

#endif
