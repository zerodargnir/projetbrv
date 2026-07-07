#include "App.hpp"
#include "Handlers.hpp"

#include <glaze/glaze.hpp>
#include <string>
#include <vector>
#include <chrono>
#include <set>
#include <algorithm>

// mission_id de la mission en cours, partage avec MissionEnd.cpp (la requete MissionEnd ne
// contient QUE le serial, pas le mission_id -> on le memorise ici au lancement).
std::string g_lastMissionId = "1";

// MissionStart (jE6Sp0q4) = lancement de combat.
//
// RE (2026-07-01, session 2) : le validateur cote client est MissionStartScene::updateEvent
// (.so @0x1178674) qui lit MissionInfo::shared()->getMissionSerialID() (champ MissionInfo+0x14,
// une std::string). Si vide -> "INVALID_MISSION_SERIAL_ID 0".
//
// Ce champ serial est ecrit par la classe reponse **MissionNumResponse** :
//   - .so : MissionNumResponse::readParam @0xf777f4  (k9cxD7Ba -> MissionInfo serial,
//     nA95Bdj6 -> setBonusType, 5Z1LNoyH -> setBonusRate, LE6JkUp7 -> string)
//   - Windows : readParam FUN_008f5e80, vtable 0x18f3d60, cree par la factory FUN_00895290
//     @0x895b28 pour la **section key "Kz7qfSs5"** (co-apparait avec k9cxD7Ba dans toutes les
//     reponses conteneurs mission : FUN_8b5dd0/8b9a40/8b5a00).
//
// => La reponse a MissionStart doit contenir la section "Kz7qfSs5" avec k9cxD7Ba NON-VIDE.
// L'ancienne section "9Q1Lq5FS" (MissionStartInfo) est conservee (elle remplit d'autres champs
// mais ne pose PAS le serial, prouve par cdb : MissionInfo+0x14 restait vide).
namespace {
// --- section "Kz7qfSs5" = MissionNumResponse : porte le serial de session (LE bon) ---
struct MissionNum
{
	std::string serial;     // k9cxD7Ba -> MissionInfo::setMissionSerialID (valide par MissionStartScene)
	std::string bonus_type; // nA95Bdj6 -> setBonusType (int)
	std::string bonus_rate; // 5Z1LNoyH -> setBonusRate (float)
	std::string f_le;       // LE6JkUp7 -> string
};

// --- section "9Q1Lq5FS" = MissionStartInfo (conservee, champs annexes) ---
struct MissionStartInfo
{
	std::string user_id;    // h7eY3sAK
	std::string f_j3;       // J3stQ7jd
	std::string mission_id; // j28VNcUW
	std::string serial;     // 1VagK32J
	std::string f_z0;       // Z0Y4RoD7
};

// --- section "75t0sx9z" = BattleMonsterGroupMstResponse : composition des vagues d'ennemis ---
// (readParam .so 0xf6eea0 / Win FUN_008ce790 ; remplace toute la BattleMonsterGroupMstList).
// 7 champs -> BattleMonsterGroupMst. Mapping (empirique, ordre de lecture) :
//   Qzhp8B40=groupID, o49dYfpH=monsterID(unit id F_UNIT_MST), hZtF1s8B=position,
//   3g8PW6x0=groupOrder, i30R8TAs=itemDrop, hw3L0uVj=unitDrop, 5wB9SHAV=treasureDrop.
struct MonsterGroup
{
	std::string group_id;      // Qzhp8B40
	std::string monster_id;    // o49dYfpH (unit id ennemi, ex 30030 Mossy)
	std::string position;      // hZtF1s8B
	std::string group_order;   // 3g8PW6x0 (numero de vague)
	std::string item_drop;     // i30R8TAs
	std::string unit_drop;     // hw3L0uVj
	std::string treasure_drop; // 5wB9SHAV
};

// --- section "pj41dy9g" = BattleGroupMstResponse : LIEN mission -> monster groups (les vagues) ---
// (readParam .so 0xf6ecf4 / Win FUN_008ce410). C'est le chainon manquant : sans lui, le client
// charge bien les monster groups mais ne sait pas lesquels appartiennent a la mission.
// DLCManager::requestUnitFilesForMission lit BattleGroupMst::getBattleMonsterGroupID pour ca.
struct BattleGroup
{
	std::string battle_group_id;   // ZSf8e1MG
	std::string monster_group_id;  // Qzhp8B40 (= group_id d'un MonsterGroup)
	std::string mission_id;        // j28VNcUW (LA mission)
	std::string battle_order;      // VETu07N6 (numero de vague) -> setBattleOrder
	std::string boss_flg;          // etM5TCb9 -> setBossFlg
	std::string first_attack_rate; // 5aetPz3C -> setFirstAttackRate (float)
	std::string f_m;               // mFxqsc7Y
};

// --- section "U0v5IeJo" = MonsterMstResponse : DÉFINITION des ennemis (stats/nom/element) ---
// (readParam .so 0xf780ec / Win FUN_008f7360, remplit MonsterMstList = dict string->MonsterMst).
// SANS ça, le combat cherche le monster_id dans MonsterMstList (vide) -> NULL -> CRASH.
// Champs = memes cles que F_MISSION_NPC_UNIT_MST / cheahjs.
// TOUS les champs de MonsterMst (~38). CRITIQUE : le readParam ne zero-init PAS l'objet ->
// tout champ OMIS = memoire garbage -> string corrompue lue par le drop-builder -> CRASH.
// Donc on envoie TOUS les champs (strings="", numeriques init).
struct MonsterDef
{
	std::string id;          // o49dYfpH
	std::string name;        // 3evIn0zZ
	std::string hp;          // e7DK0FQT setHp
	std::string atk;         // 67CApcti setAtk
	std::string def;         // q08xLEsy setDef
	std::string element;     // iNy0ZU5M setElement
	std::string s_eyUo;      // eyUo6a8c
	std::string s_6Aou;      // 6Aou5M9r
	std::string drop_check;  // n9h7p02P setDropCheckCnt
	std::string exp;         // d96tuT2E setExp
	std::string amount;      // Rs7bCE3t setAmount
	std::string s_2EF0;      // 2EF0d6ue
	std::string s_QqfI;      // QqfI9mM4
	std::string base_unit;   // pn16CNah
	std::string s_qp37;      // qp37xTDh
	std::string move_speed;  // J2hPXGo5 setMoveSpeed
	std::string atk_move;    // mv4o39Uz setAttackMoveType
	std::string back_move;   // h2L1YI90 setBackMoveType
	std::string s_i74v;      // i74vGUFa
	std::string after_image; // D4Y5bWK7 setAfterImage
	std::string max_act;     // oMGC3hW0 setMaxActCnt
	std::string min_act;     // m4EK7Gt6 setMinActCnt
	std::string act_rate;    // 6fwL59FT setActRate
	std::string s_F4bQ;      // F4bQ7r8C
	std::string skill_move;  // XE7Yi5c3 setSkillMoveType
	std::string s_CEeq;      // CEeqs63b
	std::string max_zel;     // Najhr8m6 setMaxZelDrop
	std::string zel_cnt;     // 9FN0GAei setZelDropCnt
	std::string max_karma;   // HTVh8a65 setMaxKarmaDrop
	std::string karma_cnt;   // vNk2sI4X setKarmaDropCnt
	std::string s_3BpH;      // 3BpHN6VD
	std::string s_hjAy;      // hjAy9St3
	std::string s_2Smu;      // 2Smu5Mtq
	std::string s_CYk8;      // CYk84E3W
	std::string s_Y6bd;      // Y6bd4fXp
	std::string s_jm6J;      // jm6JSK2D
	std::string wait;        // 9pXSKmn1 setWait
	std::string s_Lkh6;      // Lkh6gYkT
};

struct MissionStartResp
{
	MissionNum num;
	MissionStartInfo info;
	std::vector<MonsterDef> monsters;
	std::vector<BattleGroup> battle_groups;
	std::vector<MonsterGroup> monster_groups;
};
} // namespace

template <>
struct glz::meta<MissionNum>
{
	using T = MissionNum;
	static constexpr auto value = glz::object(
		"k9cxD7Ba", &T::serial,
		"nA95Bdj6", &T::bonus_type,
		"5Z1LNoyH", &T::bonus_rate,
		"LE6JkUp7", &T::f_le
	);
};

template <>
struct glz::meta<MissionStartInfo>
{
	using T = MissionStartInfo;
	static constexpr auto value = glz::object(
		"h7eY3sAK", &T::user_id,
		"J3stQ7jd", &T::f_j3,
		"j28VNcUW", &T::mission_id,
		"1VagK32J", &T::serial,
		"Z0Y4RoD7", &T::f_z0
	);
};

template <>
struct glz::meta<MonsterGroup>
{
	using T = MonsterGroup;
	static constexpr auto value = glz::object(
		"Qzhp8B40", &T::group_id,
		"o49dYfpH", &T::monster_id,
		"hZtF1s8B", &T::position,
		"3g8PW6x0", &T::group_order,
		"i30R8TAs", &T::item_drop,
		"hw3L0uVj", &T::unit_drop,
		"5wB9SHAV", &T::treasure_drop
	);
};

template <>
struct glz::meta<MonsterDef>
{
	using T = MonsterDef;
	static constexpr auto value = glz::object(
		"o49dYfpH", &T::id, "3evIn0zZ", &T::name, "e7DK0FQT", &T::hp,
		"67CApcti", &T::atk, "q08xLEsy", &T::def, "iNy0ZU5M", &T::element,
		"eyUo6a8c", &T::s_eyUo, "6Aou5M9r", &T::s_6Aou, "n9h7p02P", &T::drop_check,
		"d96tuT2E", &T::exp, "Rs7bCE3t", &T::amount, "2EF0d6ue", &T::s_2EF0,
		"QqfI9mM4", &T::s_QqfI, "pn16CNah", &T::base_unit, "qp37xTDh", &T::s_qp37,
		"J2hPXGo5", &T::move_speed, "mv4o39Uz", &T::atk_move, "h2L1YI90", &T::back_move,
		"i74vGUFa", &T::s_i74v, "D4Y5bWK7", &T::after_image, "oMGC3hW0", &T::max_act,
		"m4EK7Gt6", &T::min_act, "6fwL59FT", &T::act_rate, "F4bQ7r8C", &T::s_F4bQ,
		"XE7Yi5c3", &T::skill_move, "CEeqs63b", &T::s_CEeq, "Najhr8m6", &T::max_zel,
		"9FN0GAei", &T::zel_cnt, "HTVh8a65", &T::max_karma, "vNk2sI4X", &T::karma_cnt,
		"3BpHN6VD", &T::s_3BpH, "hjAy9St3", &T::s_hjAy, "2Smu5Mtq", &T::s_2Smu,
		"CYk84E3W", &T::s_CYk8, "Y6bd4fXp", &T::s_Y6bd, "jm6JSK2D", &T::s_jm6J,
		"9pXSKmn1", &T::wait, "Lkh6gYkT", &T::s_Lkh6
	);
};

template <>
struct glz::meta<BattleGroup>
{
	using T = BattleGroup;
	static constexpr auto value = glz::object(
		"ZSf8e1MG", &T::battle_group_id,
		"Qzhp8B40", &T::monster_group_id,
		"j28VNcUW", &T::mission_id,
		"VETu07N6", &T::battle_order,
		"etM5TCb9", &T::boss_flg,
		"5aetPz3C", &T::first_attack_rate,
		"mFxqsc7Y", &T::f_m
	);
};

template <>
struct glz::meta<MissionStartResp>
{
	using T = MissionStartResp;
	static constexpr auto value = glz::object(
		"Kz7qfSs5", pkg::glaze::single_array<&T::num>(),
		"9Q1Lq5FS", pkg::glaze::single_array<&T::info>(),
		"U0v5IeJo", &T::monsters,
		"pj41dy9g", &T::battle_groups,
		"75t0sx9z", &T::monster_groups
	);
};

HANDLEF(MissionStart)
{
	LOG_DEBUG << "=== MissionStart (jE6Sp0q4) REQUEST: " << json;

	// extraire le vrai mission_id : dans la requete il est IMBRIQUE dans la section
	// "9Q1Lq5FS" -> [ { "j28VNcUW": "<mission>" , ... } ] (observe : "10")
	std::string mission_id = "1";
	{
		glz::json_t req{};
		if (!glz::read_json(req, json) && req.is_object())
		{
			auto try_get = [&](const glz::json_t& obj) {
				if (obj.is_object() && obj.contains("j28VNcUW") && obj.at("j28VNcUW").is_string())
				{
					mission_id = obj.at("j28VNcUW").get_string();
					return true;
				}
				return false;
			};
			if (req.contains("9Q1Lq5FS") && req.at("9Q1Lq5FS").is_array() &&
				!req.at("9Q1Lq5FS").get_array().empty())
			{
				try_get(req.at("9Q1Lq5FS").get_array()[0]);
			}
			else
			{
				try_get(req);
			}
		}
	}

	// DIAGNOSTIC (2026-07-01) : le client affichait "12" = la LONGUEUR de l'ancien serial
	// "1_<timestamp>" (12 chars). On teste un serial PUREMENT NUMERIQUE distinctif "70000" :
	//   - si l'erreur affiche "70000" -> le client lit bien la VALEUR (reste a l'enregistrer)
	//   - si elle affiche "5" -> c'est la longueur qui est lue (autre piste)
	const std::string serial = "70000";

	g_lastMissionId = mission_id; // memorise pour MissionEnd (clear + recompenses)

	MissionStartResp resp{};
	// section Kz7qfSs5 (LE serial que MissionStartScene valide)
	resp.num.serial = serial;
	resp.num.bonus_type = "0";
	resp.num.bonus_rate = "0";
	resp.num.f_le = "0";
	// section 9Q1Lq5FS (annexe)
	resp.info.user_id = "0839899613932562";
	resp.info.f_j3 = "1";
	resp.info.mission_id = mission_id;
	resp.info.serial = serial;
	resp.info.f_z0 = "1";

	// TEST mission : plusieurs ennemis DISPERSES dans une meme vague.
	// Depuis le fix du parse de position cote client (mini-parseur d'entier dans libcurl.dll,
	// remplace le stringstream qui crashait), le champ Qzhp8B40 "x:y" est parse de facon FIABLE
	// et donne la position ecran de CHAQUE ennemi (screen = valeur/2). Donc chaque ennemi a sa
	// propre position -> on peut en placer plusieurs a des endroits differents.
	//
	// Modele : 1 MonsterDef par TYPE de monstre (U0v5IeJo, dict par id) + 1 (MonsterGroup +
	// BattleGroup) par INSTANCE d'ennemi. Meme battle_order = meme vague ; Qzhp8B40 unique par
	// ennemi = sa position (et sa cle de lien).
	auto addMonsterDef = [&](const char* monster, const char* name, const char* hp,
							 const char* atk, const char* def, const char* element, const char* exp) {
		MonsterDef m{};
		m.id = monster;
		m.name = name;
		m.hp = hp;
		m.atk = atk;
		m.def = def;
		m.element = element;
		m.drop_check = "2"; // n9h7p02P = nb de checks de drop (BC bleu/HC rouge en combat).
		                    // vraie valeur cheahjs = 2. A "0" -> AUCUN cristal ne tombe -> jauge
		                    // BB ne se remplit pas + pas de soin. C'est la cle des mecaniques combat.
		m.exp = exp;
		m.amount = "1";
		m.base_unit = monster; // rendu = unite du meme id (assets F_UNIT_MST)
		m.move_speed = "8";    // spawn en place (move type 2 = stationnaire, cf Mimic)
		m.atk_move = "2";
		m.back_move = "0";
		m.after_image = "0";
		m.max_act = "1"; // 1 action par tour
		m.min_act = "1";
		m.act_rate = "100";
		m.skill_move = "2";
		m.max_zel = "10"; // drops zel/karma (petits, pour init non-garbage)
		m.zel_cnt = "1";
		m.max_karma = "5";
		m.karma_cnt = "1";
		m.wait = "100";
		m.s_eyUo = m.s_6Aou = m.s_2EF0 = m.s_QqfI = m.s_qp37 = m.s_i74v = "";
		m.s_F4bQ = m.s_CEeq = m.s_3BpH = m.s_hjAy = m.s_2Smu = "";
		m.s_CYk8 = m.s_Y6bd = m.s_jm6J = m.s_Lkh6 = "";
		resp.monsters.push_back(m);
	};

	// MODELE BF pour PLUSIEURS ennemis par vague :
	//   - UN BattleGroup (75t0sx9z->pj41dy9g) par vague : lie un group_id a la mission via
	//     VETu07N6 (battle_order = numero de vague).
	//   - PLUSIEURS MonsterGroup (75t0sx9z) partageant le MEME group_id (Qzhp8B40) = les ennemis
	//     de la vague ; chacun a son monster_id + sa position 3g8PW6x0="x:y" (screen = val/2).
	// RE cdb (2026-07-05) : 3g8PW6x0 est le champ POSITION (copie enemy+0x324, splitte par ':'),
	// PAS Qzhp8B40 (qui n'est QUE la cle de lien). Valeur sans ':' -> crash (element[1] hors limites).
	auto addWave = [&](const char* groupId, const char* order) {
		BattleGroup bg{};
		bg.battle_group_id = groupId;
		bg.monster_group_id = groupId; // lie la vague au group_id
		bg.mission_id = mission_id;
		bg.battle_order = order;        // VETu07N6 = numero de vague
		bg.boss_flg = "0";
		bg.first_attack_rate = "0";
		bg.f_m = "0";
		resp.battle_groups.push_back(bg);
	};
	auto addGroupMonster = [&](const char* groupId, const char* monster, const char* pos) {
		MonsterGroup g{};
		g.group_id = groupId;    // Qzhp8B40 = MEME pour tous les ennemis de la vague
		g.monster_id = monster;
		g.position = "160:280";  // hZtF1s8B (annexe)
		g.group_order = pos;     // 3g8PW6x0 = POSITION "x:y" (unique par ennemi)
		g.item_drop = "";
		g.unit_drop = "";
		g.treasure_drop = "";
		resp.monster_groups.push_back(g);
	};

	// MonsterDef COMPLET a partir d'un objet JSON monstre (missions_data.json = cheahjs
	// pre-compile : missions.json + missions_parsed + info.json).
	auto addMonsterDefJson = [&](const glz::json_t& mon) {
		auto gs = [&](const char* k, const char* dv) -> std::string {
			if (mon.contains(k) && mon.at(k).is_string()) return mon.at(k).get_string();
			return dv;
		};
		MonsterDef m{};
		m.id = gs("unit_id", "0");
		m.name = gs("name", "mob");
		m.hp = gs("hp", "100");
		m.atk = gs("atk", "50");
		m.def = gs("def", "10");
		m.element = gs("element", "fire");
		m.drop_check = gs("drop_check", "2"); // cristaux BC/HC en combat
		m.exp = "1";
		m.amount = "1";
		m.base_unit = m.id;      // rendu = unite du meme id
		// SPAWN EN PLACE (pas de walk-in hors-ecran). Comparaison cheahjs : Mimic(spawn en place)
		// a move type=2 / move speed=8 ; Mossy(walk-in) a move type=1. atk_move/skill_move =
		// setAttackMoveType/setSkillMoveType = le "move type" (2 = stationnaire).
		m.move_speed = "8";
		m.atk_move = "2";
		m.back_move = "0";
		m.after_image = "0";
		m.max_act = gs("max_act", "1");
		m.min_act = gs("min_act", "1");
		m.act_rate = "100";
		m.skill_move = "2"; // stationnaire (spawn en place)
		m.max_zel = gs("zel_max", "10");
		m.zel_cnt = gs("zel_cnt", "1");
		m.max_karma = gs("karma_max", "5");
		m.karma_cnt = gs("karma_cnt", "1");
		m.wait = "100";
		m.s_eyUo = m.s_6Aou = m.s_2EF0 = m.s_QqfI = m.s_qp37 = m.s_i74v = "";
		m.s_F4bQ = m.s_CEeq = m.s_3BpH = m.s_hjAy = m.s_2Smu = "";
		m.s_CYk8 = m.s_Y6bd = m.s_jm6J = m.s_Lkh6 = "";
		resp.monsters.push_back(m);
	};

	// Fallback (mission non couverte) : 3 vagues x 3 ennemis en formation.
	auto addFormationWave = [&](const char* groupId, const char* order,
							   const char* mL, const char* mM, const char* mR) {
		addWave(groupId, order);
		addGroupMonster(groupId, mL, "90:340");
		addGroupMonster(groupId, mM, "170:140");
		addGroupMonster(groupId, mR, "250:250");
	};

	// === MISSIONS REELLES : missions_data.json (pre-compile cheahjs), indexe par mission_id ===
	static glz::json_t g_missions;
	static bool g_missionsLoaded = false;
	if (!g_missionsLoaded)
	{
		std::string mbuf;
		const auto mec = glz::read_file_json(g_missions, "./system/missions_data.json", mbuf);
		if (mec) LOG_DEBUG << "missions_data.json load FAILED: " << glz::format_error(mec, mbuf);
		else     LOG_DEBUG << "missions_data.json loaded";
		g_missionsLoaded = true;
	}

	// Positions de formation (server "x:y", screen = valeur/2) : ennemis a GAUCHE, scatter 2D.
	// y grand = bas ecran (devant) ; y petit = haut (retrait). Perso joueur a droite (non gere).
	static const char* FORMATION[] = {
		"90:340", "170:150", "250:250", "70:210", "210:330", "150:180",
		"300:260", "120:150", "260:330"
	};
	const int NFORM = (int)(sizeof(FORMATION) / sizeof(FORMATION[0]));

	bool built = false;
	if (g_missions.is_object() && g_missions.contains(mission_id))
	{
		const glz::json_t& md = g_missions.at(mission_id);
		if (md.contains("waves") && md.at("waves").is_array())
		{
			const auto& waves = md.at("waves").get_array();
			std::set<std::string> defsSeen;
			int waveNum = 1;
			for (const auto& wave : waves)
			{
				if (!wave.contains("monsters") || !wave.at("monsters").is_array()) { waveNum++; continue; }
				const std::string groupId = std::to_string(waveNum);
				addWave(groupId.c_str(), groupId.c_str());
				const auto& monsters = wave.at("monsters").get_array();
				int mi = 0;
				for (const auto& mon : monsters)
				{
					if (!mon.contains("unit_id") || !mon.at("unit_id").is_string()) continue;
					const std::string uid = mon.at("unit_id").get_string();
					if (uid.empty()) continue;
					if (defsSeen.insert(uid).second) addMonsterDefJson(mon);
					addGroupMonster(groupId.c_str(), uid.c_str(), FORMATION[mi % NFORM]);
					++mi;
				}
				++waveNum;
			}
			built = !resp.monster_groups.empty();
		}
	}
	if (!built)
	{
		addMonsterDef("30030", "Mossy",      "300",  "290", "50", "3", "1");
		addMonsterDef("40030", "Sparky",     "300",  "320", "0",  "4", "1");
		addMonsterDef("30031", "King Mossy",  "3000", "320", "30", "3", "1");
		addFormationWave("1", "1", "30030", "40030", "30031");
		addFormationWave("2", "2", "40030", "30031", "30030");
		addFormationWave("3", "3", "30031", "30030", "40030");
	}
	LOG_DEBUG << "MissionStart mission_id=" << mission_id << " built=" << built
			  << " waves=" << resp.battle_groups.size() << " mons=" << resp.monster_groups.size();

	std::string buffer{};
	const auto& ec = glz::write_json(resp, buffer);
	if (ec)
	{
		co_return HandleResult::error("Serialization error", glz::format_error(ec, buffer));
	}
	LOG_DEBUG << "=== MissionStart RESPONSE: " << buffer;
	co_return HandleResult::success(buffer);
}
