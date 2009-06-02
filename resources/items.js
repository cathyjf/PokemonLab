/*
 * File:   items.js
 * Author: Catherine
 *
 * Created on June 2, 2009, 12:18 AM
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

function HoldItem(name) {
    this.name = this.id = name;
    HoldItem[name] = this;
}

HoldItem.prototype = new StatusEffect();
HoldItem.prototype.switchOut = function() {
    return false;
};

function makeItem(obj) {
    var item = new HoldItem(obj.name);
    for (var p in obj) {
        item[p] = obj[p];
    }
}

makeItem({
    name : "Leftovers",
    tier : 6,
    subtier : 2,
    tick : function() {
        var max = this.subject.getStat(Stat.HP);
        if (this.subject.hp == max)
            return;
        this.subject.field.print(Text.items_messages(0));
        var delta = Math.floor(max / 16);
        this.subject.hp += delta;
    }
});

makeItem({
    name : "Lagging Tail",
    inherentPriority : function() {
        return -2;
    }
});

