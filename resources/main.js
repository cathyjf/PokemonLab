/**
 * Main entry point for Shoddy Battle 2.
 */

print("Started ShoddyBattle2 at " + new Date() + ".");

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
        return getText.apply(null, tokens);
    };
    return idx;
});

// Test Text object.
print(Text.status_effects_confusion(1, "Benjamin"));

includeSpecies("resources/species.xml");
includeMoves("resources/moves2.xml");
populateMoveLists();
