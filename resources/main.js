/**
 * Main entry point for Shoddy Battle 2.
 */

var Text = { idx : 0 };

loadText("languages/english.lang", function(name) {
    // JavaScript variable names cannot contain hyphens, so we replace them
    // by underscores before creating the property.
    name = name.replace(/-/g, "_");
    var idx = Text.idx++;
    Text[name] = function(text) {
        var tokens = new Array(idx, text);
        for (var i = 1; i < arguments.length; ++i) {
            tokens[i + 1] = arguments[i];
        }
        tokens.toString = function() {
            return "${" + this[0] + "," + this[1] + "}";
        };
        return tokens;
    };
    return idx;
});

includeSpecies("resources/species.xml");
includeMoves("resources/moves.xml");
populateMoveLists();

include("resources/moves.js");
include("resources/constants.js");
include("resources/StatusEffect.js");
include("resources/statuses.js");
include("resources/abilities.js");


