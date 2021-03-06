#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=
AS=

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Release
CND_DISTDIR=dist

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/moves/PokemonMove.o \
	${OBJECTDIR}/src/shoddybattle/Pokemon.o \
	${OBJECTDIR}/src/scripting/StatusObject.o \
	${OBJECTDIR}/src/database/md5.o \
	${OBJECTDIR}/src/mechanics/stat.o \
	${OBJECTDIR}/src/scripting/FieldObject.o \
	${OBJECTDIR}/src/main/LogFile.o \
	${OBJECTDIR}/src/shoddybattle/PokemonSpecies.o \
	${OBJECTDIR}/src/shoddybattle/BattleField.o \
	${OBJECTDIR}/src/text/Text.o \
	${OBJECTDIR}/src/main/main.o \
	${OBJECTDIR}/src/main/Log.o \
	${OBJECTDIR}/src/database/rijndael.o \
	${OBJECTDIR}/src/matchmaking/MetagameList.o \
	${OBJECTDIR}/src/network/NetworkBattle.o \
	${OBJECTDIR}/src/scripting/ScriptMachine.o \
	${OBJECTDIR}/src/mechanics/PokemonType.o \
	${OBJECTDIR}/src/matchmaking/glicko2.o \
	${OBJECTDIR}/src/mechanics/JewelMechanics.o \
	${OBJECTDIR}/src/database/DatabaseRegistry.o \
	${OBJECTDIR}/src/mechanics/PokemonNature.o \
	${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.o \
	${OBJECTDIR}/src/network/network.o \
	${OBJECTDIR}/src/scripting/MoveObject.o \
	${OBJECTDIR}/src/database/Authenticator.o \
	${OBJECTDIR}/src/network/Channel.o \
	${OBJECTDIR}/src/scripting/PokemonObject.o \
	${OBJECTDIR}/src/database/sha2.o \
	${OBJECTDIR}/src/shoddybattle/Team.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-Wall -Wextra -lmozjs -lnspr -lxerces-c -lmysqlclient -lmysqlpp -lboost_thread-gcc42-mt -lboost_regex-gcc42-mt -lboost_system-gcc42-mt -lboost_filesystem -lboost_date_time -lboost_program_options-gcc42-mt -ldaemon
CXXFLAGS=-Wall -Wextra -lmozjs -lnspr -lxerces-c -lmysqlclient -lmysqlpp -lboost_thread-gcc42-mt -lboost_regex-gcc42-mt -lboost_system-gcc42-mt -lboost_filesystem -lboost_date_time -lboost_program_options-gcc42-mt -ldaemon

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/lib/mysql -L/usr/lib -L/usr/local/lib

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Release.mk dist/Release/GNU-Linux-x86/shoddybattle2

dist/Release/GNU-Linux-x86/shoddybattle2: ${OBJECTFILES}
	${MKDIR} -p dist/Release/GNU-Linux-x86
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/shoddybattle2 ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/src/database/rijndael.h.gch: nbproject/Makefile-${CND_CONF}.mk src/database/rijndael.h 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/database/rijndael.h

${OBJECTDIR}/src/scripting/ObjectWrapper.h.gch: nbproject/Makefile-${CND_CONF}.mk src/scripting/ObjectWrapper.h 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/scripting/ObjectWrapper.h

${OBJECTDIR}/src/database/DatabaseRegistry.h.gch: nbproject/Makefile-${CND_CONF}.mk src/database/DatabaseRegistry.h 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/database/DatabaseRegistry.h

${OBJECTDIR}/src/scripting/ScriptMachine.h.gch: nbproject/Makefile-${CND_CONF}.mk src/scripting/ScriptMachine.h 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/scripting/ScriptMachine.h

${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.h.gch: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/ObjectTeamFile.h 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/shoddybattle/ObjectTeamFile.h

${OBJECTDIR}/src/network/Channel.h.gch: nbproject/Makefile-${CND_CONF}.mk src/network/Channel.h 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/network/Channel.h

${OBJECTDIR}/src/moves/PokemonMove.o: nbproject/Makefile-${CND_CONF}.mk src/moves/PokemonMove.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/moves
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/moves/PokemonMove.o src/moves/PokemonMove.cpp

${OBJECTDIR}/src/shoddybattle/Pokemon.o: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/Pokemon.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/Pokemon.o src/shoddybattle/Pokemon.cpp

${OBJECTDIR}/src/scripting/StatusObject.o: nbproject/Makefile-${CND_CONF}.mk src/scripting/StatusObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/StatusObject.o src/scripting/StatusObject.cpp

${OBJECTDIR}/src/matchmaking/MetagameList.h.gch: nbproject/Makefile-${CND_CONF}.mk src/matchmaking/MetagameList.h 
	${MKDIR} -p ${OBJECTDIR}/src/matchmaking
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/matchmaking/MetagameList.h

${OBJECTDIR}/src/database/md5.o: nbproject/Makefile-${CND_CONF}.mk src/database/md5.c 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/md5.o src/database/md5.c

${OBJECTDIR}/src/mechanics/stat.o: nbproject/Makefile-${CND_CONF}.mk src/mechanics/stat.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/stat.o src/mechanics/stat.cpp

${OBJECTDIR}/src/scripting/FieldObject.o: nbproject/Makefile-${CND_CONF}.mk src/scripting/FieldObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/FieldObject.o src/scripting/FieldObject.cpp

${OBJECTDIR}/src/main/LogFile.o: nbproject/Makefile-${CND_CONF}.mk src/main/LogFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/main
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/main/LogFile.o src/main/LogFile.cpp

${OBJECTDIR}/src/database/Authenticator.h.gch: nbproject/Makefile-${CND_CONF}.mk src/database/Authenticator.h 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/database/Authenticator.h

${OBJECTDIR}/src/matchmaking/glicko2.h.gch: nbproject/Makefile-${CND_CONF}.mk src/matchmaking/glicko2.h 
	${MKDIR} -p ${OBJECTDIR}/src/matchmaking
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/matchmaking/glicko2.h

${OBJECTDIR}/src/shoddybattle/PokemonSpecies.o: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/PokemonSpecies.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/PokemonSpecies.o src/shoddybattle/PokemonSpecies.cpp

${OBJECTDIR}/src/shoddybattle/BattleField.o: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/BattleField.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/BattleField.o src/shoddybattle/BattleField.cpp

${OBJECTDIR}/src/text/Text.o: nbproject/Makefile-${CND_CONF}.mk src/text/Text.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/text
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/text/Text.o src/text/Text.cpp

${OBJECTDIR}/src/text/Text.h.gch: nbproject/Makefile-${CND_CONF}.mk src/text/Text.h 
	${MKDIR} -p ${OBJECTDIR}/src/text
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/text/Text.h

${OBJECTDIR}/src/shoddybattle/BattleField.h.gch: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/BattleField.h 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/shoddybattle/BattleField.h

${OBJECTDIR}/src/network/NetworkBattle.h.gch: nbproject/Makefile-${CND_CONF}.mk src/network/NetworkBattle.h 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/network/NetworkBattle.h

${OBJECTDIR}/src/main/main.o: nbproject/Makefile-${CND_CONF}.mk src/main/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/main
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/main/main.o src/main/main.cpp

${OBJECTDIR}/src/mechanics/BattleMechanics.h.gch: nbproject/Makefile-${CND_CONF}.mk src/mechanics/BattleMechanics.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/BattleMechanics.h

${OBJECTDIR}/src/moves/PokemonMove.h.gch: nbproject/Makefile-${CND_CONF}.mk src/moves/PokemonMove.h 
	${MKDIR} -p ${OBJECTDIR}/src/moves
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/moves/PokemonMove.h

${OBJECTDIR}/src/main/Log.o: nbproject/Makefile-${CND_CONF}.mk src/main/Log.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/main
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/main/Log.o src/main/Log.cpp

${OBJECTDIR}/src/database/rijndael.o: nbproject/Makefile-${CND_CONF}.mk src/database/rijndael.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/rijndael.o src/database/rijndael.cpp

${OBJECTDIR}/src/matchmaking/MetagameList.o: nbproject/Makefile-${CND_CONF}.mk src/matchmaking/MetagameList.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/matchmaking
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/matchmaking/MetagameList.o src/matchmaking/MetagameList.cpp

${OBJECTDIR}/src/mechanics/stat.h.gch: nbproject/Makefile-${CND_CONF}.mk src/mechanics/stat.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/stat.h

${OBJECTDIR}/src/mechanics/PokemonNature.h.gch: nbproject/Makefile-${CND_CONF}.mk src/mechanics/PokemonNature.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/PokemonNature.h

${OBJECTDIR}/src/network/NetworkBattle.o: nbproject/Makefile-${CND_CONF}.mk src/network/NetworkBattle.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/network/NetworkBattle.o src/network/NetworkBattle.cpp

${OBJECTDIR}/src/scripting/ScriptMachine.o: nbproject/Makefile-${CND_CONF}.mk src/scripting/ScriptMachine.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/ScriptMachine.o src/scripting/ScriptMachine.cpp

${OBJECTDIR}/src/mechanics/PokemonType.o: nbproject/Makefile-${CND_CONF}.mk src/mechanics/PokemonType.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/PokemonType.o src/mechanics/PokemonType.cpp

${OBJECTDIR}/src/matchmaking/glicko2.o: nbproject/Makefile-${CND_CONF}.mk src/matchmaking/glicko2.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/matchmaking
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/matchmaking/glicko2.o src/matchmaking/glicko2.cpp

${OBJECTDIR}/src/mechanics/JewelMechanics.o: nbproject/Makefile-${CND_CONF}.mk src/mechanics/JewelMechanics.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/JewelMechanics.o src/mechanics/JewelMechanics.cpp

${OBJECTDIR}/src/database/DatabaseRegistry.o: nbproject/Makefile-${CND_CONF}.mk src/database/DatabaseRegistry.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/DatabaseRegistry.o src/database/DatabaseRegistry.cpp

${OBJECTDIR}/src/shoddybattle/PokemonSpecies.h.gch: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/PokemonSpecies.h 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/shoddybattle/PokemonSpecies.h

${OBJECTDIR}/src/network/network.h.gch: nbproject/Makefile-${CND_CONF}.mk src/network/network.h 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/network/network.h

${OBJECTDIR}/src/mechanics/PokemonNature.o: nbproject/Makefile-${CND_CONF}.mk src/mechanics/PokemonNature.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/PokemonNature.o src/mechanics/PokemonNature.cpp

${OBJECTDIR}/src/mechanics/JewelMechanics.h.gch: nbproject/Makefile-${CND_CONF}.mk src/mechanics/JewelMechanics.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/JewelMechanics.h

${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.o: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/ObjectTeamFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.o src/shoddybattle/ObjectTeamFile.cpp

${OBJECTDIR}/src/shoddybattle/Pokemon.h.gch: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/Pokemon.h 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/shoddybattle/Pokemon.h

${OBJECTDIR}/src/network/network.o: nbproject/Makefile-${CND_CONF}.mk src/network/network.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/network/network.o src/network/network.cpp

${OBJECTDIR}/src/shoddybattle/Team.h.gch: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/Team.h 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/shoddybattle/Team.h

${OBJECTDIR}/src/scripting/MoveObject.o: nbproject/Makefile-${CND_CONF}.mk src/scripting/MoveObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/MoveObject.o src/scripting/MoveObject.cpp

${OBJECTDIR}/src/database/sha2.h.gch: nbproject/Makefile-${CND_CONF}.mk src/database/sha2.h 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/database/sha2.h

${OBJECTDIR}/src/database/Authenticator.o: nbproject/Makefile-${CND_CONF}.mk src/database/Authenticator.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/Authenticator.o src/database/Authenticator.cpp

${OBJECTDIR}/src/network/Channel.o: nbproject/Makefile-${CND_CONF}.mk src/network/Channel.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/network/Channel.o src/network/Channel.cpp

${OBJECTDIR}/src/scripting/PokemonObject.o: nbproject/Makefile-${CND_CONF}.mk src/scripting/PokemonObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/PokemonObject.o src/scripting/PokemonObject.cpp

${OBJECTDIR}/src/database/sha2.o: nbproject/Makefile-${CND_CONF}.mk src/database/sha2.c 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/sha2.o src/database/sha2.c

${OBJECTDIR}/src/shoddybattle/Team.o: nbproject/Makefile-${CND_CONF}.mk src/shoddybattle/Team.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/Team.o src/shoddybattle/Team.cpp

${OBJECTDIR}/src/mechanics/PokemonType.h.gch: nbproject/Makefile-${CND_CONF}.mk src/mechanics/PokemonType.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/PokemonType.h

${OBJECTDIR}/src/network/ThreadedQueue.h.gch: nbproject/Makefile-${CND_CONF}.mk src/network/ThreadedQueue.h 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -O2 -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/network/ThreadedQueue.h

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Release
	${RM} dist/Release/GNU-Linux-x86/shoddybattle2

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
