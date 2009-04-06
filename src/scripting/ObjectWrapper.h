/* 
 * File:   ObjectWrapper.h
 * Author: Catherine
 *
 * Created on April 5, 2009, 8:45 PM
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

#ifndef _OBJECT_WRAPPER_H_
#define _OBJECT_WRAPPER_H_

namespace shoddybattle {

class ScriptObject;

class ObjectWrapper {
public:
    virtual ScriptObject *getObject() = 0;
};

}

#endif
