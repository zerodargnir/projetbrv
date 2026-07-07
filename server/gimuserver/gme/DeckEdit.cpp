#include "App.hpp"
#include "Handlers.hpp"

// DeckEdit (m2Ve9PkJ, AES d7UuQsq8) = sauvegarde de la composition d'equipe (ajout/retrait
// d'unites dans un party deck) depuis l'ecran "equipe / manage unit".
// Le client envoie le deck a sauvegarder ; on accuse reception avec un succes minimal.
HANDLEF(DeckEdit)
{
	LOG_DEBUG << "=== DeckEdit (m2Ve9PkJ) REQUEST: " << json;
	// Succes minimal : accuse reception. Si le client attend des sections precises,
	// on les ajoutera d'apres ce log.
	co_return HandleResult::success("{}");
}
