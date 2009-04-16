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

# Macros
PLATFORM=GNU-Linux-x86

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/Debug/${PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/network/network.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/moves/PokemonMove.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/ScriptMachine.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/DatabaseRegistry.o \
	${OBJECTDIR}/src/shoddybattle/Pokemon.o \
	${OBJECTDIR}/src/mechanics/PokemonType.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/MoveObject.o \
	${OBJECTDIR}/src/mechanics/JewelMechanics.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/Team.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/PokemonSpecies.o \
	${OBJECTDIR}/src/mechanics/PokemonNature.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/PokemonObject.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/StatusObject.o \
	${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/FieldObject.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/BattleField.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/sha2.o \
	${OBJECTDIR}/src/text/Text.o \
	${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/rijndael.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-ljs -lboost_thread-mt -lboost_regex -lboost_system-mt -lnspr -lxerces-c -lmysqlpp
CXXFLAGS=-ljs -lboost_thread-mt -lboost_regex -lboost_system-mt -lnspr -lxerces-c -lmysqlpp

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/local/lib

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/Debug/${PLATFORM}/shoddybattle2

dist/Debug/${PLATFORM}/shoddybattle2: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/${PLATFORM}
	${LINK.cc} -o dist/Debug/${PLATFORM}/shoddybattle2 ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/ObjectTeamFile.h.gch: /home/Catherine/ShoddyBattle2/src/shoddybattle/ObjectTeamFile.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/shoddybattle/ObjectTeamFile.h

${OBJECTDIR}/src/mechanics/BattleMechanics.h.gch: src/mechanics/BattleMechanics.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/BattleMechanics.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/network/network.o: /home/Catherine/ShoddyBattle2/src/network/network.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/network
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/network/network.o /home/Catherine/ShoddyBattle2/src/network/network.cpp

${OBJECTDIR}/src/mechanics/stat.h.gch: src/mechanics/stat.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/stat.h

${OBJECTDIR}/src/mechanics/PokemonNature.h.gch: src/mechanics/PokemonNature.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/PokemonNature.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/moves/PokemonMove.o: /home/Catherine/ShoddyBattle2/src/moves/PokemonMove.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/moves
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/moves/PokemonMove.o /home/Catherine/ShoddyBattle2/src/moves/PokemonMove.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/ScriptMachine.h.gch: /home/Catherine/ShoddyBattle2/src/scripting/ScriptMachine.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/scripting/ScriptMachine.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/DatabaseRegistry.h.gch: /home/Catherine/ShoddyBattle2/src/database/DatabaseRegistry.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/database/DatabaseRegistry.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/ScriptMachine.o: /home/Catherine/ShoddyBattle2/src/scripting/ScriptMachine.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/ScriptMachine.o /home/Catherine/ShoddyBattle2/src/scripting/ScriptMachine.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/rijndael.h.gch: /home/Catherine/ShoddyBattle2/src/database/rijndael.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/database/rijndael.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/DatabaseRegistry.o: /home/Catherine/ShoddyBattle2/src/database/DatabaseRegistry.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/DatabaseRegistry.o /home/Catherine/ShoddyBattle2/src/database/DatabaseRegistry.cpp

${OBJECTDIR}/src/shoddybattle/Pokemon.o: src/shoddybattle/Pokemon.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/Pokemon.o src/shoddybattle/Pokemon.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/Team.h.gch: /home/Catherine/ShoddyBattle2/src/shoddybattle/Team.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/shoddybattle/Team.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/BattleField.h.gch: /home/Catherine/ShoddyBattle2/src/shoddybattle/BattleField.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/shoddybattle/BattleField.h

${OBJECTDIR}/src/mechanics/PokemonType.o: src/mechanics/PokemonType.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/PokemonType.o src/mechanics/PokemonType.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/MoveObject.o: /home/Catherine/ShoddyBattle2/src/scripting/MoveObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/MoveObject.o /home/Catherine/ShoddyBattle2/src/scripting/MoveObject.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/ObjectWrapper.h.gch: /home/Catherine/ShoddyBattle2/src/scripting/ObjectWrapper.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/scripting/ObjectWrapper.h

${OBJECTDIR}/src/mechanics/JewelMechanics.o: src/mechanics/JewelMechanics.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/JewelMechanics.o src/mechanics/JewelMechanics.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/network/network.h.gch: /home/Catherine/ShoddyBattle2/src/network/network.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/network
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/network/network.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/Team.o: /home/Catherine/ShoddyBattle2/src/shoddybattle/Team.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/Team.o /home/Catherine/ShoddyBattle2/src/shoddybattle/Team.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/PokemonSpecies.o: /home/Catherine/ShoddyBattle2/src/shoddybattle/PokemonSpecies.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/PokemonSpecies.o /home/Catherine/ShoddyBattle2/src/shoddybattle/PokemonSpecies.cpp

${OBJECTDIR}/src/mechanics/PokemonNature.o: src/mechanics/PokemonNature.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mechanics/PokemonNature.o src/mechanics/PokemonNature.cpp

${OBJECTDIR}/src/mechanics/JewelMechanics.h.gch: src/mechanics/JewelMechanics.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/JewelMechanics.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/PokemonObject.o: /home/Catherine/ShoddyBattle2/src/scripting/PokemonObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/PokemonObject.o /home/Catherine/ShoddyBattle2/src/scripting/PokemonObject.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/StatusObject.o: /home/Catherine/ShoddyBattle2/src/scripting/StatusObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/StatusObject.o /home/Catherine/ShoddyBattle2/src/scripting/StatusObject.cpp

${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.o: src/shoddybattle/ObjectTeamFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/shoddybattle/ObjectTeamFile.o src/shoddybattle/ObjectTeamFile.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/FieldObject.o: /home/Catherine/ShoddyBattle2/src/scripting/FieldObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/scripting/FieldObject.o /home/Catherine/ShoddyBattle2/src/scripting/FieldObject.cpp

${OBJECTDIR}/src/shoddybattle/Pokemon.h.gch: src/shoddybattle/Pokemon.h 
	${MKDIR} -p ${OBJECTDIR}/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/shoddybattle/Pokemon.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/PokemonSpecies.h.gch: /home/Catherine/ShoddyBattle2/src/shoddybattle/PokemonSpecies.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/shoddybattle/PokemonSpecies.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/BattleField.o: /home/Catherine/ShoddyBattle2/src/shoddybattle/BattleField.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/shoddybattle/BattleField.o /home/Catherine/ShoddyBattle2/src/shoddybattle/BattleField.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/sha2.o: /home/Catherine/ShoddyBattle2/src/database/sha2.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database
	${RM} $@.d
	$(COMPILE.c) -g -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/sha2.o /home/Catherine/ShoddyBattle2/src/database/sha2.c

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/moves/PokemonMove.h.gch: /home/Catherine/ShoddyBattle2/src/moves/PokemonMove.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/moves
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/moves/PokemonMove.h

${OBJECTDIR}/src/text/Text.o: src/text/Text.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/text
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/text/Text.o src/text/Text.cpp

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/rijndael.o: /home/Catherine/ShoddyBattle2/src/database/rijndael.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/rijndael.o /home/Catherine/ShoddyBattle2/src/database/rijndael.cpp

${OBJECTDIR}/src/text/Text.h.gch: src/text/Text.h 
	${MKDIR} -p ${OBJECTDIR}/src/text
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/text/Text.h

${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database/sha2.h.gch: /home/Catherine/ShoddyBattle2/src/database/sha2.h 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/Catherine/ShoddyBattle2/src/database
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ /home/Catherine/ShoddyBattle2/src/database/sha2.h

${OBJECTDIR}/src/mechanics/PokemonType.h.gch: src/mechanics/PokemonType.h 
	${MKDIR} -p ${OBJECTDIR}/src/mechanics
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/boost-1_38/ -I/usr/local/include/mysql++ -I/usr/include/mysql -MMD -MP -MF $@.d -o $@ src/mechanics/PokemonType.h

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Debug
	${RM} dist/Debug/${PLATFORM}/shoddybattle2

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
