#include "App.hpp"
#include "Handlers.hpp"

// BjAt1D6b : commande envoyee en entrant dans un gate de quete (ex Grand Gaia), apres la
// cinematique d'intro. Cle AES = "k5EiNe9x" (8 chars, zero-paddee a 16 par BfCrypt).
// STUB : on logge la requete dechiffree pour decouvrir sa structure, puis on renvoie un
// objet vide le temps d'observer la reaction du client.
HANDLEF(QuestGateEnter)
{
	LOG_DEBUG << "=== QuestGateEnter (BjAt1D6b) REQUEST: " << json;
	co_return HandleResult::success("{}");
}
