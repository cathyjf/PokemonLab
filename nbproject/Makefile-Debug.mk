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
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/network/network.o \
	${OBJECTDIR}/src/shoddybattle/BattleField.o \
	${OBJECTDIR}/src/text/Text.o \
	${OBJECTDIR}/src/scripting/StatusObject.o \
	${OBJECTDIR}/src/matchmaking/MetagameList.o \
	${OBJECTDIR}/src/moves/PokemonMove.o \
	${OBJECTDIR}/src/shoddybattle/Team.o \
	${OBJECTDIR}/src/scripting/FieldObject.o \
	${OBJECTDIR}/src/network/Channel.o \
	${OBJECTDIR}/src/database/sha2.o \
	${OBJECTDIR}/src/scripting/PokemonObject.o \
	${OBJECTDIR}/src/shoddybattle/PokemonSpecies.o \
	${OBJECTDIR}/src/shoddybattle/Pokemon.o \
	${OBJECTDIR}/src/main/Log.o \
	${OBJECTDIR}/src/database/md5.o \
	${OBJECTDIR}/src/mechanics/PokemonType.o \
	${OBJECTDIR}/src/database/rijndael.o \
	${OBJECTDIR}/src/mechanics/JewelMechanics.o \
	${OBJECTDIR}/src/main/LogFile.o \
	${OBJECTDIR}/src/database/DatabaseRegistry.o \
	${OBJECTDIR}/src/mechanics/stat.o \
	${OBJECTDIR}/src/mechanics/PokemonNature.o \
	${OBJECTDIR}/src/main/main.o \
	${OBJECTDIR}/src/database/Authenticator.o \
	${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.o \
	${OBJECTDIR}/src/network/NetworkBattle.o \
	${OBJECTDIR}/src/matchmaking/glicko2.o \
	${OBJECTDIR}/src/scripting/ScriptMachine.o \
	${OBJECTDIR}/src/scripting/MoveObject.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-Wall -Wextra -lmozjs -lnspr4 -lxerces-c -lmysqlclient -lmysqlpp -lboost_thread-mt -lboost_regex-mt -lboost_system-mt -lboost_filesystem -lboost_date_time -lboost_program_options-mt -ldaemon
CXXFLAGS=-Wall -Wextra -lmozjs -lnspr4 -lxerces-c -lmysqlclient -lmysqlpp -lboost_thread-mt -lboost_regex-mt -lboost_system-mt -lboost_filesystem -lboost_date_time -lboost_program_options-mt -ldaemon

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/lib/mysql -L/usr/local/lib -L/usr/lib

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/server

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/server: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/server ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/src/network/network.o: src/network/network.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/network/network.o src/network/network.cpp

${OBJECTDIR}/src/shoddybattle/BattleField.o: src/shoddybattle/BattleField.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/BattleField.o src/shoddybattle/BattleField.cpp

${OBJECTDIR}/src/text/Text.o: src/text/Text.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/text
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/text/Text.o src/text/Text.cpp

${OBJECTDIR}/src/scripting/StatusObject.o: src/scripting/StatusObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/StatusObject.o src/scripting/StatusObject.cpp

${OBJECTDIR}/src/matchmaking/MetagameList.o: src/matchmaking/MetagameList.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/matchmaking
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/matchmaking/MetagameList.o src/matchmaking/MetagameList.cpp

${OBJECTDIR}/src/moves/PokemonMove.o: src/moves/PokemonMove.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/moves
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/moves/PokemonMove.o src/moves/PokemonMove.cpp

${OBJECTDIR}/src/shoddybattle/Team.o: src/shoddybattle/Team.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/Team.o src/shoddybattle/Team.cpp

${OBJECTDIR}/src/scripting/FieldObject.o: src/scripting/FieldObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/FieldObject.o src/scripting/FieldObject.cpp

${OBJECTDIR}/src/network/Channel.o: src/network/Channel.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/network/Channel.o src/network/Channel.cpp

${OBJECTDIR}/src/database/sha2.o: src/database/sha2.c 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/sha2.o src/database/sha2.c

${OBJECTDIR}/src/scripting/PokemonObject.o: src/scripting/PokemonObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/PokemonObject.o src/scripting/PokemonObject.cpp

${OBJECTDIR}/src/shoddybattle/PokemonSpecies.o: src/shoddybattle/PokemonSpecies.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/PokemonSpecies.o src/shoddybattle/PokemonSpecies.cpp

${OBJECTDIR}/src/shoddybattle/Pokemon.o: src/shoddybattle/Pokemon.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/Pokemon.o src/shoddybattle/Pokemon.cpp

${OBJECTDIR}/src/main/Log.o: src/main/Log.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/main
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/main/Log.o src/main/Log.cpp

${OBJECTDIR}/src/database/md5.o: src/database/md5.c 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/md5.o src/database/md5.c

${OBJECTDIR}/src/mechanics/PokemonType.o: src/mechanics/PokemonType.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/PokemonType.o src/mechanics/PokemonType.cpp

${OBJECTDIR}/src/database/rijndael.o: src/database/rijndael.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/rijndael.o src/database/rijndael.cpp

${OBJECTDIR}/src/mechanics/JewelMechanics.o: src/mechanics/JewelMechanics.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/JewelMechanics.o src/mechanics/JewelMechanics.cpp

${OBJECTDIR}/src/main/LogFile.o: src/main/LogFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/main
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/main/LogFile.o src/main/LogFile.cpp

${OBJECTDIR}/src/database/DatabaseRegistry.o: src/database/DatabaseRegistry.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/DatabaseRegistry.o src/database/DatabaseRegistry.cpp

${OBJECTDIR}/src/mechanics/stat.o: src/mechanics/stat.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/stat.o src/mechanics/stat.cpp

${OBJECTDIR}/src/mechanics/PokemonNature.o: src/mechanics/PokemonNature.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/PokemonNature.o src/mechanics/PokemonNature.cpp

${OBJECTDIR}/src/main/main.o: src/main/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/main
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/main/main.o src/main/main.cpp

${OBJECTDIR}/src/database/Authenticator.o: src/database/Authenticator.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/database
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/database/Authenticator.o src/database/Authenticator.cpp

${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.o: src/shoddybattle/ObjectTeamFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.o src/shoddybattle/ObjectTeamFile.cpp

${OBJECTDIR}/src/network/NetworkBattle.o: src/network/NetworkBattle.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/network
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/network/NetworkBattle.o src/network/NetworkBattle.cpp

${OBJECTDIR}/src/matchmaking/glicko2.o: src/matchmaking/glicko2.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/matchmaking
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/matchmaking/glicko2.o src/matchmaking/glicko2.cpp

${OBJECTDIR}/src/scripting/ScriptMachine.o: src/scripting/ScriptMachine.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/ScriptMachine.o src/scripting/ScriptMachine.cpp

${OBJECTDIR}/src/scripting/MoveObject.o: src/scripting/MoveObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -DDEBUG -DJS_NO_JSVAL_JSID_STRUCT_TYPES -I/usr/local/include/mysql++ -I/usr/include/mysql -I/usr/include/mozjs -I/usr/include/mozjs/mozilla -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scripting/MoveObject.o src/scripting/MoveObject.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/server

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
