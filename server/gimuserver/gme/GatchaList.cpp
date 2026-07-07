#include "App.hpp"
#include "Handlers.hpp"

#include <chrono>
#include <string>
#include <random>
#include <vector>
#include <utility>
#include <thread>

// ============================================================================
// POOL D'INVOCATION : vraies unites (ids verifies avec assets dans game_content)
// extraites de F_UNIT_MST, groupees par rarete. Le client affiche nom/art/element
// depuis sa propre localisation via unit_id. element="fire" ici = sans effet visuel
// (le client utilise ses donnees). Taux de drop ponderes ci-dessous.
// ============================================================================
namespace {
struct PoolUnit { int unit_id; const char* element; };
static const std::vector<PoolUnit> POOL_R3 = { {10012,"fire"}, {10022,"fire"}, {10032,"fire"}, {20012,"water"}, {20022,"water"}, {20032,"water"}, {30012,"earth"}, {30022,"earth"}, {30032,"earth"}, {40012,"thunder"}, {40022,"thunder"}, {40032,"thunder"}, {50012,"light"}, {50022,"light"}, {50032,"light"}, {60012,"dark"}, {60022,"dark"}, {60032,"dark"} };
static const std::vector<PoolUnit> POOL_R4 = { {10013,"fire"}, {10023,"fire"}, {10063,"fire"}, {20013,"water"}, {20023,"water"}, {20063,"water"}, {30013,"earth"}, {30023,"earth"}, {30063,"earth"}, {40013,"thunder"}, {40023,"thunder"}, {40063,"thunder"}, {50013,"light"}, {50023,"light"}, {50063,"light"}, {60013,"dark"}, {60023,"dark"}, {60063,"dark"} };
static const std::vector<PoolUnit> POOL_R5 = { {10014,"fire"}, {10024,"fire"}, {10094,"fire"}, {20014,"water"}, {20024,"water"}, {20094,"water"}, {30014,"earth"}, {30024,"earth"}, {30094,"earth"}, {40014,"thunder"}, {40024,"thunder"}, {40094,"thunder"}, {50014,"light"}, {50024,"light"}, {50084,"light"}, {60014,"dark"}, {60024,"dark"}, {60084,"dark"} };
static const std::vector<PoolUnit> POOL_R6 = { {10015,"fire"}, {10025,"fire"}, {10095,"fire"}, {20015,"water"}, {20025,"water"}, {20095,"water"}, {30015,"earth"}, {30025,"earth"}, {30095,"earth"}, {40015,"thunder"}, {40025,"thunder"}, {40095,"thunder"}, {50015,"light"}, {50025,"light"}, {50085,"light"}, {60015,"dark"}, {60025,"dark"}, {60085,"dark"} };

// Tire une unite selon les taux : 3* 45%, 4* 35%, 5* 15%, 6* 5%.
static PoolUnit drawUnit(int& rarityOut)
{
	static thread_local std::mt19937 rng(std::random_device{}());
	int roll = rng() % 1000;
	const std::vector<PoolUnit>* pool;
	if (roll < 450)      { pool = &POOL_R3; rarityOut = 3; }
	else if (roll < 800) { pool = &POOL_R4; rarityOut = 4; }
	else if (roll < 950) { pool = &POOL_R5; rarityOut = 5; }
	else                 { pool = &POOL_R6; rarityOut = 6; }
	return (*pool)[rng() % pool->size()];
}
} // namespace

// Reponse de GatchaList (commande Uo86DcRh, bouton Summon).
//   1IR86sAv = GachaInfo (bannieres actives)
//   IBs49NiH = GachaCategory (categories d'invocation)  <- requis pour mapper les bannieres
//   6FrKacq7 = SignalKey
//   fEi17cnx = UserTeamInfo
struct GatchaListResp
{
	std::vector<GachaInfoMst> gacha_info;
	std::vector<GachaCategory> gacha_categories;
	SignalKey signal_key;
	UserTeamInfo team_info;
};

template <>
struct glz::meta<GatchaListResp>
{
	using T = GatchaListResp;
	static constexpr auto value = glz::object(
		"1IR86sAv", &T::gacha_info,
		"IBs49NiH", &T::gacha_categories,
		"6FrKacq7", pkg::glaze::single_array<&T::signal_key>(),
		"fEi17cnx", pkg::glaze::single_array<&T::team_info>()
	);
};

// Reponse de TIRAGE (F7JvPk5H). La reponse GME est composee de sections typees
// (cf. factory client FUN_00895290). Les unites OBTENUES = section UserUnitInfoResponse
// sous la cle "qC2tJs4E" (l'autre cle UserUnitInfoResponse "4ceMWH6k" = liste box dans UserInfo).
// SummonerOpeResultInfo (classe client AVSummonerOpeResultInfo, objet 0x38).
// Section "s2sZaCgW" (SummonerOpeResultResponse) = LE resultat de l'operation de
// tirage qui declenche la scene resultat (porte). 8 champs INT decouverts via RTTI
// walk du client (desserialiseur FUN_00915cf0) : offsets +0x14..+0x30.
struct SummonerOpeResult
{
	int f14 = 0; // 6hMI5xeF
	int f18 = 0; // 4I57ivB7
	int f1c = 0; // 1ybJVbKt
	int f20 = 0; // ODJl2WpY
	int f24 = 0; // a4XihN8b
	int f28 = 0; // NqVAPbLC
	int f2c = 0; // gfNKNGky
	int f30 = 0; // y3aoq08m
};

template <>
struct glz::meta<SummonerOpeResult>
{
	using T = SummonerOpeResult;
	static constexpr auto value = glz::object(
		"6hMI5xeF", &T::f14,
		"4I57ivB7", &T::f18,
		"1ybJVbKt", &T::f1c,
		"ODJl2WpY", &T::f20,
		"a4XihN8b", &T::f24,
		"NqVAPbLC", &T::f28,
		"gfNKNGky", &T::f2c,
		"y3aoq08m", &T::f30
	);
};

// Section "Km35HAXv" = OpeUserUnitResponse (classe client AVOpeUserUnitResponse,
// vtable RVA 0x14f3f40, parser readParam = FUN_008fa5e0). C'est LA section qui remplit
// le vecteur de RESULTAT de la porte (OpeUserUnitInfoList / DAT_0199772c). Si ce vecteur
// est VIDE, FUN_00c72010 affiche "Summon Gate connection error". Chaque unite-resultat =
// 3 champs string (clés decouvertes via decompile du parser) : edy7fq3L, u0vkt9yH, g30VnzQh.
struct OpeUserUnit
{
	std::string edy7fq3L; // unit_id
	std::string u0vkt9yH; // user_unit_id (instance)
	std::string g30VnzQh; // (flag/extra)
};

template <>
struct glz::meta<OpeUserUnit>
{
	using T = OpeUserUnit;
	static constexpr auto value = glz::object(
		"edy7fq3L", &T::edy7fq3L,
		"u0vkt9yH", &T::u0vkt9yH,
		"g30VnzQh", &T::g30VnzQh
	);
};

struct GatchaActionResp
{
	std::vector<GachaInfoMst> gacha_info;
	std::vector<UserUnitInfo> new_units;
	SignalKey signal_key;
	UserTeamInfo team_info;
	SummonerOpeResult ope_result;
	std::vector<OpeUserUnit> ope_units;
};

template <>
struct glz::meta<GatchaActionResp>
{
	using T = GatchaActionResp;
	static constexpr auto value = glz::object(
		"1IR86sAv", &T::gacha_info,
		"qC2tJs4E", &T::new_units,
		"6FrKacq7", pkg::glaze::single_array<&T::signal_key>(),
		"fEi17cnx", pkg::glaze::single_array<&T::team_info>(),
		"s2sZaCgW", pkg::glaze::single_array<&T::ope_result>(),
		"Km35HAXv", &T::ope_units
	);
};

// Construit la liste gacha_info COMPLETE (toutes les bannieres du MST gacha.json).
// CRITIQUE : utilisee a la fois par GatchaList (menu) ET GatchaExec (tirage). La reponse
// du tirage (1IR86sAv) ECRASE la GachaInfoList du client ; si elle est tronquee, la scene
// resultat de la porte cherche la banniere tiree via GachaInfoList::find (FUN_005096f0) et
// recoit NULL -> deref [obj+0x44] -> crash. Les deux handlers DOIVENT renvoyer la meme liste.
// GachaMst et GachaInfoMst partagent les memes cles -> copie champ a champ. On exclut les
// bannieres "Selector" (layout client manquant). detail_id vide = detail basique.
static std::vector<GachaInfoMst> buildFullGachaInfo()
{
	using namespace std::chrono;
	std::vector<GachaInfoMst> out;
	const std::string pastStr = "2017-01-01 00:00:00";
	const auto future = floor<milliseconds>(system_clock::now() + hours(24 * 365));
	for (const auto& g : theServer()->cache().initializeResp().gachas)
	{
		if (g.name.find("Selector") != std::string::npos) continue;
		if (g.name.find("selector") != std::string::npos) continue;
		GachaInfoMst gi{};
		gi.id = g.id;
		gi.name = g.name;
		gi.type = g.type;
		gi.priority = g.priority;
		gi.start_date = pastStr;
		gi.end_date = future;
		gi.start_hour = g.start_hour;
		gi.end_hour = g.end_hour;
		gi.need_friends_point = g.need_friend_point;
		gi.brave_coin = g.need_gems;          // meme cle 03UGMHxF (cout gems)
		gi.once_day_flag = false;
		gi.background_img = g.bg_img;
		gi.button_img = g.btn_img;
		gi.door_img = g.door_img;
		gi.caption_msg = g.caption_msg;
		gi.detail_msg = g.detail_msg;
		gi.comment = g.comment_msg;
		gi.group_id = g.gatcha_group_id;
		gi.description = g.description;
		gi.detail_id = "";                    // vide -> evite un chargement de detail manquant
		gi.content_banner_img = "";
		out.push_back(gi);
	}
	return out;
}

HANDLEF(GatchaList)
{
	using namespace std::chrono;

	GatchaListResp resp{};
	// gacha_info COMPLETE (toutes les bannieres) -> menu + clic detail + tirage.
	resp.gacha_info = buildFullGachaInfo();

	resp.team_info = theServer()->cache().userInfoResp().team_info;
	// FIX crash Summon : reinforcement_deck vide ("") fait crasher le client.
	// Vraie valeur serveur = "-1,-99,-99" (slots vides).
	resp.team_info.reinforcement_deck = {-1, -99, -99};
	// Honor Points (friend_point, cle J3stQ7jd) : le client bloque le tirage Honor
	// Summon cote client si insuffisant (cout 200). On en donne plein pour tester.
	resp.team_info.friend_point = 99999;
	// Capacite d'unites (sinon 0 -> pas de place pour les unites invoquees).
	resp.team_info.max_unit_count = 100;
	resp.team_info.add_unit_count = 100;
	resp.team_info.warehouse_count = 100;
	resp.team_info.free_gems = 9999; resp.team_info.paid_gems = 9999;
	resp.team_info.zel = 99999999; resp.team_info.karma = 99999999;
	resp.team_info.brave_coin = 99999; resp.team_info.rainbow_coin = 9999;
	resp.team_info.level = 1;
	// Energie (quetes) + cost (capacite equipe) : coherent avec UserInfo.
	resp.team_info.max_action_point = 999;
	resp.team_info.action_point = 999;
	resp.team_info.action_rest_timer = 0;
	resp.team_info.deck_cost = 100;
	resp.signal_key.key = "5EdKHavF";

	// Une categorie englobant toutes les bannieres (le client mappe les
	// bannieres via les categories ; sans categorie -> deref null cote client).
	GachaCategory cat{};
	cat.id = 1;
	cat.disp_order = 1;
	cat.img = "";
	cat.start_date = floor<milliseconds>(system_clock::now() - hours(24 * 365));
	cat.end_date = floor<milliseconds>(system_clock::now() + hours(24 * 365));
	std::string ids;
	for (const auto& g : resp.gacha_info)
	{
		if (!ids.empty()) ids += ",";
		ids += std::to_string(g.id);
	}
	cat.gatcha_id_list = ids;
	// TEST DECISIF: ne PAS envoyer de categorie (IBs49NiH=[]). Si CAT_COUNT passe
	// a 0 cote client -> categories = reseau ; si reste a 1 -> categories = MST.
	// resp.gacha_categories.push_back(cat);
	(void)cat;

	std::string buffer{};
	const auto& ec = glz::write_json(resp, buffer);
	if (ec)
	{
		const auto& glze = glz::format_error(ec, buffer);
		LOG_DEBUG << "Gme GatchaList Error during JSON writing: " << glze;
		co_return HandleResult::error("Serialization error", glze);
	}

	co_return HandleResult::success(buffer);
}

// F7JvPk5H : commande de TIRAGE (summon draw). Cle AES bL9fipzaSy7xN2w1.
// STUB : on log la requete decryptee (pour decouvrir sa structure : quelle gacha,
// combien de tirages) et on renvoie un objet vide le temps d'observer la reaction
// du client. La vraie reponse (unites obtenues) sera construite ensuite.
HANDLEF(GatchaExec)
{
	LOG_DEBUG << "=== GatchaExec (summon draw) REQUEST: " << json;

	GatchaActionResp resp{};

	// Echo du gacha tire (1IR86sAv = GachaInfo). DOIT etre la liste COMPLETE (pas les
	// 6 entrees de gacha_info.json) : cette reponse ECRASE la GachaInfoList du client ;
	// si tronquee, la scene resultat de la porte ne trouve pas la banniere tiree -> crash.
	resp.gacha_info = buildFullGachaInfo();

	// Unite obtenue (test : on renvoie une unite valide connue). Meme structure que
	// les unites de UserInfo, sous la cle "qC2tJs4E" (nouvelles unites du summon).
	UserUnitInfo d = {};
	d.user_id = "0839899613932562";
	// user_unit_id UNIQUE a chaque tirage (sinon collision = pas d'ajout au 2e tirage).
	static int s_nextUserUnitId = 1000;
	d.user_unit_id = ++s_nextUserUnitId;
	int rarity = 3;
	PoolUnit drawn = drawUnit(rarity);
	d.unit_type_id = 1;
	d.element = drawn.element;
	d.base_hp = 1000; d.add_hp = 0; d.ext_hp = 0;
	d.base_def = 1100; d.add_def = 0; d.ext_def = 0;
	d.base_heal = 1200; d.add_heal = 0; d.ext_heal = 0;
	d.base_atk = 1300; d.add_atk = 0; d.ext_atk = 0;
	d.limit_over_atk = 0; d.limit_over_def = 0; d.limit_over_heal = 0; d.limit_over_hp = 0;
	d.unit_lv = 1;
	d.new_flag = 1;
	d.ext_count = 0;
	d.fe_bp = 0; d.fe_used_bp = 0; d.fe_max_usable_bp = 0;
	d.unit_img_type = 0;
	d.exp = 0; d.total_exp = 0;
	d.unit_id = drawn.unit_id; // tire du pool reel selon la rarete
	LOG_DEBUG << "=== GatchaExec: unite tiree id=" << drawn.unit_id << " rarete=" << rarity;
	resp.new_units.push_back(d);

	// Resultat d'operation (section s2sZaCgW) qui declenche la scene resultat (porte).
	// 1ere tentative : unit_id dans le 1er champ, valeurs plausibles ailleurs.
	resp.ope_result.f14 = drawn.unit_id;       // 6hMI5xeF (probable unit_id)
	resp.ope_result.f18 = d.user_unit_id;      // 4I57ivB7 (probable user_unit_id)
	resp.ope_result.f1c = rarity;              // 1ybJVbKt
	resp.ope_result.f20 = 1;                   // ODJl2WpY
	resp.ope_result.f24 = 1;                   // a4XihN8b
	resp.ope_result.f28 = 0;                   // NqVAPbLC
	resp.ope_result.f2c = 0;                   // gfNKNGky
	resp.ope_result.f30 = 0;                   // y3aoq08m

	// Section Km35HAXv (OpeUserUnitResponse) = unites du RESULTAT affichees par la porte.
	// Remplit OpeUserUnitInfoList / le vecteur DAT_0199772c que la scene resultat lit ;
	// si vide -> "Summon Gate connection error" (FUN_00c72010). Une entree = l'unite tiree.
	{
		OpeUserUnit ou;
		ou.edy7fq3L = std::to_string(drawn.unit_id);   // unit_id (art)
		ou.u0vkt9yH = std::to_string(d.user_unit_id);  // user_unit_id (instance)
		ou.g30VnzQh = "1";                              // flag/extra (a affiner)
		resp.ope_units.push_back(ou);
	}

	// Sections standard presentes dans les autres reponses (sinon "connection error").
	resp.signal_key.key = "5EdKHavF";
	resp.team_info = theServer()->cache().userInfoResp().team_info;
	resp.team_info.reinforcement_deck = {-1, -99, -99};
	resp.team_info.friend_point = 99999 - 200; // apres depense d'un tirage Honor
	// CAPACITE d'unites : le cache a max_unit_count=0 -> le client n'a pas de place
	// pour l'unite invoquee -> echec. On donne de la place (comme UserInfo).
	resp.team_info.max_unit_count = 100;
	resp.team_info.add_unit_count = 100;
	resp.team_info.warehouse_count = 100;
	resp.team_info.free_gems = 9999; resp.team_info.paid_gems = 9999;
	resp.team_info.zel = 99999999; resp.team_info.karma = 99999999;
	resp.team_info.brave_coin = 99999; resp.team_info.rainbow_coin = 9999;
	resp.team_info.level = 1;
	// Sans ca, chaque tirage remet energie/cost a 0 (le client applique ce team_info).
	resp.team_info.max_action_point = 999;
	resp.team_info.action_point = 999;
	resp.team_info.action_rest_timer = 0;
	resp.team_info.deck_cost = 100;

	std::string buffer{};
	const auto& ec = glz::write_json(resp, buffer);
	if (ec)
	{
		const auto& glze = glz::format_error(ec, buffer);
		LOG_DEBUG << "GatchaExec serialization error: " << glze;
		co_return HandleResult::error("Serialization error", glze);
	}
	LOG_DEBUG << "=== GatchaExec RESPONSE: " << buffer;
	co_return HandleResult::success(buffer);
}
