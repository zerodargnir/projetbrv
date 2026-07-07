#include "App.hpp"
#include "Handlers.hpp"

// Inventaire persistant des unites obtenues en drop (ecrit par MissionEnd.cpp). On le recharge
// ici pour ajouter ces unites a l'inventaire du joueur.
namespace {
struct InvUnit { std::string u; std::string e; };
struct PlayerInv { std::vector<InvUnit> units; };
}
template <> struct glz::meta<InvUnit> {
	using T = InvUnit; static constexpr auto value = glz::object("u", &T::u, "e", &T::e);
};
template <> struct glz::meta<PlayerInv> {
	using T = PlayerInv; static constexpr auto value = glz::object("units", &T::units);
};

HANDLEF(UserInfo)
{
	UserInfoReq req = {};
	const auto& ec = glz::read_json(req, json);
	if (ec)
	{
		const auto& fmte = glz::format_error(ec, json);
		LOG_DEBUG << "Gme UserInfo Error during JSON read: " << fmte;
		co_return HandleResult::error("Deserialization error", fmte);
	}

	UserInfoResp resp = theServer()->cache().userInfoResp(); // copy !!

	// TODO: A real server should check if user_id == gumi token...

    // TODO: GET THIS FROM A CACHE TOKEN ETC
    resp.login_info.user_id = "0839899613932562"; // I think this is a random UUID according to packet-gen

    // TEMP HACK!! Skip tutorial flag and put a real name
    resp.login_info.account_id = "12345678";
    resp.login_info.handle_name = "OfflineMod!";
    resp.login_info.tutorial_end_flag = true;
    resp.login_info.tutorial_status = 0;
    resp.login_info.feature_gate = 0;

    UserTeamInfo team = {};
    resp.team_info.reinforcement_deck.emplace_back(0);
    resp.team_info.reinforcement_deck.emplace_back(0);
    resp.team_info.reinforcement_deck.emplace_back(0);
    resp.team_info.user_id = resp.login_info.user_id;
    resp.team_info.level = 1;
    resp.team_info.exp = 0;
    resp.team_info.warehouse_count = 100;
    resp.team_info.free_gems = 9999; resp.team_info.paid_gems = 9999;
    resp.team_info.zel = 99999999; resp.team_info.karma = 99999999;
    resp.team_info.brave_coin = 99999; resp.team_info.rainbow_coin = 9999;
    resp.team_info.add_unit_count = 100;
    resp.team_info.max_unit_count = 100;
    resp.team_info.friend_point = 99999; // Honor Points (pour tirer au Honor Summon)
    // Energie (action point = stamina des quetes) et cost (capacite d'equipe) : sans ca,
    // impossible d'entrer en quete (energie 0) ni de placer des unites (cost 0).
    resp.team_info.max_action_point = 999;
    resp.team_info.action_point = 999;
    resp.team_info.action_rest_timer = 0;
    resp.team_info.deck_cost = 100;        // capacite de cost de l'equipe
    resp.team_info.max_fight_point = 99;   // arena BP (au cas ou)
    resp.team_info.fight_point = 99;


    // Inventaire d'unites. Le squad de combat se compose dans l'ecran "equipe / manage unit"
    // du jeu ; il suffit donc d'avoir les unites ICI (dans unit_info) pour pouvoir les placer.
    // unit_id = ID reel (cheahjs info.json) ; user_unit_id = ID d'instance (unique par unite).
    auto addUnit = [&](int uuid, int unit_id, const char* element, int bb_id, int leader_id, int es_id) {
        UserUnitInfo d = {};
        d.user_id = resp.login_info.user_id;
        d.user_unit_id = uuid;
        d.unit_type_id = 1;
        d.element = element;
        d.base_hp = 1000;  d.add_hp = 1001;  d.ext_hp = 1002;
        d.base_def = 1100; d.add_def = 1101; d.ext_def = 1102;
        d.base_heal = 1200; d.add_heal = 1201; d.ext_heal = 1202;
        d.base_atk = 1300; d.add_atk = 1301; d.ext_atk = 1302;
        d.limit_over_atk = 1400; d.limit_over_def = 1401; d.limit_over_heal = 1402; d.limit_over_hp = 1403;
        d.unit_lv = 1;
        d.new_flag = 1;
        d.ext_count = 1500;
        d.fe_bp = 100; d.fe_used_bp = 0; d.fe_max_usable_bp = 200;
        d.unit_img_type = 0;
        d.exp = 1; d.total_exp = 1;
        d.unit_id = unit_id;
        // Brave Burst : sans skill_id/skill_lv, le BB est "niveau 0 / non appris" -> la jauge BB
        // ne se charge pas. skill_id = id du BB (= unit_id pour ces unites), skill_lv = 10 (max).
        d.skill_id = bb_id;
        d.skill_lv = bb_id ? 10 : 0;     // pas de BB si bb_id=0 (mobs droppes)
        d.leader_skill_id = leader_id;   // leader skill
        if (es_id) { d.extra_skill_id = es_id; d.extra_skill_lv = 10; } // extra skill (ES 6*+)
        resp.unit_info.emplace_back(d);
    };
    addUnit(100, 50253, "fire",    50253, 4400, 0);    // Tilith (bb 50253, leader 4400)
    addUnit(101, 10016, "fire",    10016, 106,  100);  // Flame Legend Vargas 7* (bb 10016)
    addUnit(102, 40016, "thunder", 40016, 136,  400);  // Thunder Legend Eze 7* (bb 40016)

    // Unites OBTENUES EN DROP (player_inventory.json, persiste par MissionEnd) -> inventaire.
    {
        PlayerInv inv;
        std::string ib;
        if (!glz::read_file_json(inv, "./system/player_inventory.json", ib))
        {
            int uuid = 200;
            for (const auto& du : inv.units)
            {
                int unit_id = 0;
                try { unit_id = std::stoi(du.u); } catch (...) { continue; }
                if (unit_id <= 0) continue;
                addUnit(uuid++, unit_id, du.e.empty() ? "fire" : du.e.c_str(), 0, 0, 0);
            }
            LOG_DEBUG << "UserInfo: " << inv.units.size() << " unites droppees chargees";
        }
    }

    for (int i = 0; i < 10; i++) {
        UserPartyDeckInfo deck = {};
        deck.deck_num = i;
        deck.deck_type = 1;
        deck.user_unit_id = 100; // Now maps to id from user_units
        resp.party_deck_info.emplace_back(deck);
    }

    // Debloquer TOUS les lieux de quete (permit_place) pour l'ACCES aux zones. On NE marque
    // PAS les missions clearees (clear_mission_info reste VIDE) => progression NORMALE :
    // 1ere cinematique puis une nouvelle au fur et a mesure, au lieu de toutes d'un coup.
    // (fallback si l'injection capture echoue ; l'injection ci-dessous ne remplace PLUS
    // clear_mission pour garder la progression fraiche.)
    for (int i = 1; i <= 2000; i++) {
        PermitPlace pp{};
        pp.place_id = std::to_string(i);
        pp.f1 = "0"; pp.f2 = "0"; pp.f3 = "0"; pp.f4 = "0"; pp.f5 = "";
        resp.permit_place.emplace_back(pp);
    }

    resp.campaign_info.current_day = 1;
    resp.campaign_info.total_days = 96;
    resp.campaign_info.first_for_the_day = true;
    resp.campaign_info.id = 1;

    resp.summoner_journal.user_id = req.login_info.user_id; // we are really trusting the client here (bad)
    // struct SummonerJournalUserInfo non-initialisee (int32 garbage) -> un flag != 0 relance
    // la cinematique summoner en boucle. On force 0 (anti-cinematique). A garder tant que le
    // client est en "mode summoner" ; apres un reset client complet ce n'est plus critique.
    resp.summoner_journal.points = 0;
    resp.summoner_journal.journal_flag = 0;
    resp.summoner_journal.unk = 0;
    resp.signal_key.key = "5EdKHavF";


    std::string buffer{};
    const auto& ec2 = glz::write_json(resp, buffer);
    if (ec2)
    {
        const auto& glze = glz::format_error(ec2, buffer);
        LOG_DEBUG << "Gme UserInfo Error during JSON writing: " << glze;
        co_return HandleResult::error("Serialization error", glze);
    }

    // MINE D'OR : injecter les sections de PROGRESSION depuis une vraie capture serveur
    // (system/userinfo_capture.json = gimufrontier userinfo_resp.json) pour debloquer la
    // navigation quete (permit_place/clear_mission/release/dungeon_key/achievement...).
    // On GARDE notre reponse (user_id, unites, team_info gems/energie) et on remplace
    // seulement ces sections de deblocage par les vraies valeurs. Fallback = generation
    // manuelle ci-dessus si la capture ne charge pas.
    {
        glz::json_t capj, myj;
        std::string cbuf;
        const auto capEc = glz::read_file_json(capj, "./system/userinfo_capture.json", cbuf);
        const auto myEc = glz::read_json(myj, buffer);
        if (capEc) LOG_DEBUG << "UserInfo: capture load FAILED: " << glz::format_error(capEc, cbuf);
        if (!capEc && !myEc && capj.is_object() && myj.is_object())
        {
            LOG_DEBUG << "UserInfo: capture loaded, injecting progression sections";
            static const char* kProgKeys[] = {
                "yXNM8kL3", // permit_place
                "Y73tHKS8", // permit_place_sp
                "Y73mHKS8", // permit_place_ml
                // "UT1SVg59" (clear_mission) VOLONTAIREMENT EXCLU => progression normale
                "Dp0MjKAf", // release_info
                "eFU7Qtb0", // dungeon_key_info
                "Bnc4LpM8", // achievement_info
                "51yQrDBR", // permit_receipes
                "5PR2VmH1", // mission_break
            };
            for (const char* k : kProgKeys)
            {
                if (capj.contains(k))
                    myj[k] = capj[k];
            }
            std::string merged;
            if (!glz::write_json(myj, merged))
                buffer = merged;
        }
    }

    co_return HandleResult::success(buffer);
}
