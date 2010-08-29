/* 
 * File:   ObjectTeamFile.cpp
 * Author: Catherine
 *
 * Created on March 28, 2009, 5:36 PM
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

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#include "ObjectTeamFile.h"
#include "../main/Log.h"

using namespace std;

namespace shoddybattle {

/**
 * Specialised implementation of a Java ObjectInputStream capable of reading
 * Shoddy Battle team files.
 *
 * This is based on the grammar provided at
 * http://java.sun.com/j2se/1.3/docs/guide/serialization/spec/protocol.doc5.html
 */
class JavaObjectStream {
public:

    /**
     * Load a Shoddy Battle 1 team file.
     */
    static bool readFile(const string file, vector<POKEMON> &p) {
        return JavaObjectStream().processFile(file, &p);
    }
    

private:
    /** Constants used in the Java file format. **/
    const static short STREAM_MAGIC = (short)0xaced;
    const static short STREAM_VERSION = 5;
    const static char TC_NULL = (char)0x70;
    const static char TC_REFERENCE = (char)0x71;
    const static char TC_CLASSDESC = (char)0x72;
    const static char TC_OBJECT = (char)0x73;
    const static char TC_STRING = (char)0x74;
    const static char TC_ARRAY = (char)0x75;
    const static char TC_CLASS = (char)0x76;
    const static char TC_BLOCKDATA = (char)0x77;
    const static char TC_ENDBLOCKDATA = (char)0x78;
    const static char TC_RESET = (char)0x79;
    const static char TC_BLOCKDATALONG = (char)0x7A;
    const static char TC_EXCEPTION = (char)0x7B;
    const static char TC_LONGSTRING = (char) 0x7C;
    const static char TC_PROXYCLASSDESC = (char) 0x7D;
    const static int baseWireHandle = 0x7E0000;
    const static char SC_WRITE_METHOD = 0x01;
    const static char SC_BLOCK_DATA = 0x08;
    const static char SC_SERIALIZABLE = 0x02;
    const static char SC_EXTERNALIZABLE = 0x04;

    /** Mapping of Shoddy Battle 1 nature IDs to internal nature ID (which are
     *  used exclusively in Shoddy Battle 2). **/
    const static int m_natures[25];

    bool processFile(const string file, vector<POKEMON> *p) {
        resetObjects();
        m_handle = -1;
        m_pokemon = p;
        m_ifs.open(file.c_str(), ios_base::binary);
        short magic = getShort();
        if (magic != STREAM_MAGIC) {
            return false;
        }
        short version = getShort();
        if (version != STREAM_VERSION) {
            return false;
        }
        readObject(); // String UUID
        readObject(); // Pokemon[] team
        return true;
    }

    JavaObjectStream() {

    }

    ~JavaObjectStream() {
        resetObjects();
    }

    struct STREAM_OBJECT {
        int type;
        int handle;
        virtual ~STREAM_OBJECT() { }
    };

    struct FIELD {
        bool endBlockData;
        bool object;
        char typeCode;
        string name;
        string type;
    };

    struct STREAM_CLASS_DESC : public STREAM_OBJECT {
        string name;
        long long uid;
        char flags;
        vector<FIELD> fields;
        STREAM_CLASS_DESC *superclass;
    };

    struct STREAM_ARRAY : public STREAM_OBJECT {
        STREAM_CLASS_DESC *pClassDesc;
        vector<STREAM_OBJECT *> elements;
        vector<int> intElements;
        bool objects;
    };

    struct STREAM_STRING : public STREAM_OBJECT {
        string data;
    };

    struct STREAM_NATURE : public STREAM_OBJECT {
        int nature;
    };

    short getShort() {
        unsigned char bytes[2];
        bytes[0] = m_ifs.get();
        bytes[1] = m_ifs.get();
        return ((bytes[0] << 8) | bytes[1]);
    }

    long long getLong() {
        long long ret = 0;
        for (int shift = 56; shift >= 0; shift -= 8) {
            ret |= ((long long)((unsigned char)m_ifs.get())) << shift;
        }
        return ret;
    }

    int getInt() {
        int ret = 0;
        for (int shift = 24; shift >= 0; shift -= 8) {
            int i = m_ifs.get();
            ret |= ((unsigned char)i) << shift;
        }
        return ret;

    }

    string getUtf() {
        int length = getShort();
        string ret;
        ret.resize(length);
        for (int i = 0; i < length; ++i) {
            ret[i] = m_ifs.get();
        }
        return ret;
    }

    char getChar() {
        return (char)m_ifs.get();
    }

    STREAM_CLASS_DESC *readNewClassDesc() {
        STREAM_CLASS_DESC *desc = new STREAM_CLASS_DESC();
        desc->name = getUtf();
        desc->uid = getLong();
        desc->handle = newHandle();
        desc->flags = getChar();
        int count = getShort();
        for (int i = 0; i < count; ++i) {
            FIELD field;
            field.endBlockData = false;
            field.typeCode = m_ifs.get();
            field.name = getUtf();
            if ((field.typeCode == '[') || (field.typeCode == 'L')) {
                field.object = true;
                STREAM_OBJECT *obj = readObject();
                if (obj->type == TC_STRING) {
                    field.type = ((STREAM_STRING *)obj)->data;
                }
            } else {
                field.object = false;
            }
            desc->fields.push_back(field);
        }
        // Assume no annotations.
        getChar(); // TC_ENDBLOCKDATA

        // Read the superclass, if any.
        desc->superclass = readClassDesc();

        if (desc->superclass != NULL) {
            // We need to add the superclasses's fields before ours.
            vector<FIELD> fields = desc->superclass->fields;
            vector<FIELD>::iterator i = desc->fields.begin();
            for (; i != desc->fields.end(); ++i) {
                fields.push_back(*i);
            }
            desc->fields = fields;
        }

        if (desc->flags & SC_WRITE_METHOD) {
            FIELD field;
            field.endBlockData = true;
            desc->fields.push_back(field);
        }

        return (STREAM_CLASS_DESC *)storeObject(desc);
    }

    STREAM_CLASS_DESC *readClassDesc() {
        char flag = getChar();
        if (flag == TC_CLASSDESC) {
            // read new class desc
            return readNewClassDesc();
        } else if (flag == TC_PROXYCLASSDESC) {
            // read new proxy class desc - should not get here.
            return NULL;
        } else if (flag == TC_NULL) {
            return NULL;
        } else if (flag == TC_REFERENCE) {
            return (STREAM_CLASS_DESC *)readPrevObject();
        }
        return NULL;
    }

    STREAM_OBJECT *readPrevObject() {
        // In the specification, it says to read an int here, but the first
        // two bytes appear to contain irrelevant numbers.
        getShort();
        short handle = getShort();
        OBJECT_MAP::iterator i = m_objects.find(handle);
        if (i == m_objects.end()) {
            return NULL;
        }
        return i->second;
    }

    static void setStringField(POKEMON *p, string field, string value) {
        if (field == "m_name") {
            p->species = value;
        }
        if (field == "m_abilityName") {
            p->ability = value;
        }
        if (field == "m_itemName") {
            p->item = value;
        }
        if (field == "m_nickname") {
            p->nickname = value;
        }
    }

    static void *getPokemonField(POKEMON *p, string field) {
        if (field == "m_gender")
            return &p->gender;
        if (field == "m_level")
            return &p->level;
        if (field == "m_shiny")
            return &p->shiny;
        if (field == "m_ev")
            return p->ev;
        if (field == "m_iv")
            return p->iv;
        if (field == "m_move")
            return p->moves;
        if (field == "m_nature")
            return &p->nature;
        if (field == "m_ppUp")
            return p->ppUp;
        if (field == "m_species")
            return &p->speciesId;
        if (field == "m_name")
            return &p->species;
        return NULL;
    }

    STREAM_OBJECT *readPokemon(STREAM_CLASS_DESC *desc) {
        POKEMON pokemon;
        vector<FIELD> &fields = desc->fields;
        vector<FIELD>::iterator i = fields.begin();
        for (; i != fields.end(); ++i) {
            if (i->endBlockData) {
                getChar(); // TC_ENDBLOCKDATA
                continue;
            }
            if (!i->object) {
                void *p = getPokemonField(&pokemon, i->name);
                if (p == NULL) {
                    // Should not get here!
                    //return NULL;
                    continue;
                }
                if (i->typeCode == 'I') {
                    *((int *)p) = getInt();
                } else if (i->typeCode == 'Z') {
                    *((bool *)p) = m_ifs.get();
                }
            } else {
                // Read an object from the stream.
                STREAM_OBJECT *obj = readObject();
                if (obj == NULL) {
                    continue;
                }
                if (obj->type == TC_STRING) {
                    string data = ((STREAM_STRING *)obj)->data;
                    setStringField(&pokemon, i->name, data);
                } else if (obj->type == TC_ARRAY) {
                    STREAM_ARRAY *arr = (STREAM_ARRAY *)obj;
                    if (!arr->objects) {
                        // The only possible arrays here are int[].
                        int *p = (int *)getPokemonField(&pokemon, i->name);
                        if (p == NULL) {
                            continue;
                        }
                        int idx = 0;
                        vector<int>::iterator j = arr->intElements.begin();
                        for (; j != arr->intElements.end(); ++j) {
                            if (idx == P_STAT_COUNT)
                                break;
                            p[idx++] = *j;
                        }
                    } else {
                        // This is a String[].
                        int idx = 0;
                        vector<STREAM_OBJECT *>::iterator j = arr->elements.begin();
                        for (; j != arr->elements.end(); ++j) {
                            if (idx == P_MOVE_COUNT)
                                break;
                            STREAM_OBJECT *ptr = *j;
                            if (ptr != NULL) {
                                pokemon.moves[idx++]
                                        = ((STREAM_STRING *)ptr)->data;
                            }
                        }
                    }
                } else if (obj->type == TC_OBJECT) {
                    if (i->name == "m_nature") {
                        pokemon.nature = ((STREAM_NATURE *)obj)->nature;
                    }
                }
            }
        }
        m_pokemon->push_back(pokemon);
        return NULL;
    }

    STREAM_OBJECT *readRandomObject(STREAM_CLASS_DESC * /*desc*/) {
        getLong(); // seed
        getLong(); // nextNextGaussian
        getChar(); // haveNextNextGaussian
        getChar(); // TC_ENDBLOCKDATA
        return NULL;
    }

    STREAM_OBJECT *readNewObject() {
        STREAM_CLASS_DESC *desc = readClassDesc();
        int handle = newHandle();
        if (desc->name == "shoddybattle.Pokemon") {
            readPokemon(desc);
        } else if ((desc->name == "mechanics.AdvanceMechanics")
                || (desc->name == "mechanics.JewelMechanics")) {
            // Only contains a single Random.
            readObject();
        } else if (desc->name == "java.util.Random") {
            readRandomObject(desc);
        } else if (desc->name == "mechanics.moves.MoveListEntry") {
            // name of move
            STREAM_OBJECT *move = readObject();
            STREAM_STRING *ret = new STREAM_STRING();
            ret->handle = handle;
            ret->type = TC_STRING;
            if (move->type == TC_STRING) {
                ret->data = ((STREAM_STRING *)move)->data;
            }
            if (desc->flags & SC_WRITE_METHOD) {
                getChar(); // TC_ENDBLOCKDATA
            }
            return storeObject(ret);
        } else if (desc->name == "mechanics.PokemonNature") {
            int nature = getInt();
            STREAM_NATURE *ret = new STREAM_NATURE();
            ret->handle = handle;
            ret->type = TC_OBJECT;
            ret->nature = m_natures[nature];
            return storeObject(ret);
        }
        STREAM_OBJECT *ret = new STREAM_OBJECT();
        ret->handle = handle;
        ret->type = TC_OBJECT;
        return storeObject(ret);
    }

    STREAM_OBJECT *readObject() {
        char flag = getChar();

        switch (flag) {
            case TC_OBJECT: { // newObject
                return readNewObject();
            } break;

            case TC_CLASS: { // newClass

            } break;

            case TC_ARRAY: { // newArray
                STREAM_CLASS_DESC *desc = readClassDesc();
                STREAM_ARRAY *ret = new STREAM_ARRAY();
                ret->type = TC_ARRAY;
                ret->handle = newHandle();
                int size = getInt();
                char code = desc->name[1];
                ret->objects = (code == 'L');
                for (int i = 0; i < size; ++i) {
                    if (ret->objects) {
                        STREAM_OBJECT *p = readObject();
                        ret->elements.push_back(p);
                    } else if (code == 'I') {
                        int datum = getInt();
                        ret->intElements.push_back(datum);
                    }
                }
                return storeObject(ret);
            } break;

            case TC_STRING: { // newString
                STREAM_STRING *ret = new STREAM_STRING();
                ret->type = TC_STRING;
                ret->handle = newHandle();
                ret->data = getUtf();
                return storeObject(ret);
            }

            case TC_CLASSDESC: {
                return readNewClassDesc();
            } break;

            case TC_PROXYCLASSDESC: {

            } break;

            case TC_REFERENCE: {
                return readPrevObject();
            } break;

            case TC_NULL: {
                return NULL;
            } break;

            case TC_BLOCKDATA: {
                int size = m_ifs.get();
                for (int i = 0; i < size; ++i) {
                    m_ifs.get();
                }
            } break;

            case TC_BLOCKDATALONG: {

            } break;
        }
        return NULL;
    }

    STREAM_OBJECT *storeObject(STREAM_OBJECT *obj) {
        m_objects[obj->handle] = obj;
        return obj;
    }

    int newHandle() {
        ++m_handle;
        return m_handle;
    }

    void resetObjects() {
        OBJECT_MAP::iterator i = m_objects.begin();
        for (; i != m_objects.end(); ++i) {
            delete i->second;
        }
        m_objects.clear();
    }

    typedef map<int, STREAM_OBJECT *> OBJECT_MAP;

    vector<POKEMON> *m_pokemon;
    ifstream m_ifs;
    int m_handle;
    OBJECT_MAP m_objects;

    JavaObjectStream(const JavaObjectStream &);
    JavaObjectStream &operator=(const JavaObjectStream &);
};

const int JavaObjectStream::m_natures[25] = { 1, 2, 3, 4, 5, 7, 8, 9, 10, 11,
    13, 14, 15, 16, 17, 19, 20, 21, 22, 23, 24, 0, 12, 18, 6 };

/**
 * Load a Shoddy Battle 1 team file ("Object Team File") and populate the
 * vector with the pokemon from the file.
 */
bool readObjectTeamFile(const string file, vector<POKEMON> &p) {
    return JavaObjectStream::readFile(file, p);
}

}

/**using namespace shoddybattle;

#include "../text/Text.h"

int main(int argc, char **argv) {
    Text text("languages/english.lang"); // for nature names

    vector<POKEMON> pokemon;
    if (JavaObjectStream::readFile("/home/Catherine/arbitraryteam", pokemon)) {
        vector<POKEMON>::iterator i = pokemon.begin();
        for (; i != pokemon.end(); ++i) {
            POKEMON &p = *i;
            Log::out() << "Level " << p.level << " " << p.species
                    << " (#" << p.speciesId << ")" << endl;
            Log::out() << "Nickname: " << p.nickname << endl;
            Log::out() << "Nature: " << text.getText(SC_NATURE, p.nature) << endl;
            Log::out() << "Ability: " << p.ability << endl;
            Log::out() << "Item: " << p.item << endl;
            Log::out() << "IVs: ";
            for (int i = 0; i < P_STAT_COUNT; ++i) {
                Log::out() << p.iv[i] << ", ";
            }
            Log::out() << endl;
            Log::out() << "EVs: ";
            for (int i = 0; i < P_STAT_COUNT; ++i) {
                Log::out() << p.ev[i] << ", ";
            }
            Log::out() << endl;

            Log::out() << "Moves:" << endl;
            for (int i = 0; i < P_MOVE_COUNT; ++i) {
                Log::out() << "   " << p.moves[i] << " (" << p.ppUp[i] << " pp ups)"
                        << endl;
            }

            Log::out() << endl;
            Log::out() << endl;
        }
    }
    return 0;
}**/
