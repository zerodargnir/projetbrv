#include "App.hpp"
#include "Handlers.hpp"

// HomeInfo (NiYWKdzs) : la SCENE de la porte de tirage (Summon Gate) envoie une
// requete HomeInfo apres le tirage et ATTEND sa reponse pour completer la connexion
// (sinon le connecteur AsyncHttpConnector reste coince -> "Summon Gate connection
// error"). Une reponse "{}" vide ne complete pas le connecteur. On renvoie la vraie
// structure HomeInfo (capturee : gimufrontier/network_json/homecall_resp.json) :
//   Nebq4d8x = infos saison/timings, T_USER_PAYMENT_INFO, 6FrKacq7 = signal key,
//   Pj6zDW3m = config.
HANDLEF(HomeInfo)
{
	co_return HandleResult::success(R"({"Nebq4d8x":{"dk39bDa1":"","81tacsfJ":"","s385qzx9":"","z1I0P1Qk":1650625927,"qS4wLauD":151200,"l4txLZJ1":158400,"WhVJkLa1":187200,"6lldENEG":540000,"7k5fMj3L":"","djaB081u":"SEASON_END","ka8D1i0b":"","d3B6aKl3":""},"T_USER_PAYMENT_INFO":{"P_LAST_PURCHASE":0},"6FrKacq7":[{"Kn51uR4Y":"5EdKHavF"}],"Pj6zDW3m":{"xJNom6i0":"0","jsRoN50z":"http://ios21900.bfww.gumi.sg"}})");
}
