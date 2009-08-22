/**
 * Main entry point for Shoddy Battle 2.
 */

include("resources/text.js");
includeText("languages/english.lang");

includeSpecies("resources/species.xml");
includeMoves("resources/moves.xml");
populateMoveLists();

include("resources/moves.js");
include("resources/constants.js");
include("resources/StatusEffect.js");
include("resources/statuses.js");
include("resources/GlobalEffect.js");
include("resources/hazards.js");
include("resources/abilities.js");
include("resources/items.js");
include("resources/clauses.js");