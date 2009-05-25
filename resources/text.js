/*
 * File:   text.js
 * Author: Catherine
 *
 * Created on May 25, 2009, 2:14 AM
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

var Text = { idx : 0 };

function includeText(file) {
    loadText(file, function(name) {
        // JavaScript variable names cannot contain hyphens, so we replace them
        // by underscores before creating the property.
        name = name.replace(/-/g, "_");
        var idx = Text.idx++;
        Text[name] = function(text) {
            var tokens = [idx, text];
            for (var i = 1; i < arguments.length; ++i) {
                tokens[i + 1] = arguments[i];
            }
            tokens.toString = function() {
                return "${" + this[0] + "," + this[1] + "}";
            };
            return tokens;
        };
        Text[name].wrap = function() {
            var base = Text[name].apply(Text, arguments);
            return function() {
                return base.concat.apply(base, arguments);
            };
        };
        return idx;
    });
}
