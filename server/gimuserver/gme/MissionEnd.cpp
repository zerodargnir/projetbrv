#include "App.hpp"
#include "Handlers.hpp"

#include <glaze/glaze.hpp>
#include <string>
#include <chrono>
#include <map>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>

// Inventaire persistant des unites obtenues en drop (server/deploy/system/player_inventory.json).
// UserInfo.cpp le recharge pour ajouter ces unites a l'inventaire du joueur.
namespace {
struct InvUnit { std::string u; std::string e; }; // unit_id, element
struct PlayerInv { std::vector<InvUnit> units; };
}
template <> struct glz::meta<InvUnit> {
	using T = InvUnit; static constexpr auto value = glz::object("u", &T::u, "e", &T::e);
};
template <> struct glz::meta<PlayerInv> {
	using T = PlayerInv; static constexpr auto value = glz::object("units", &T::units);
};

// MissionEnd (9TvyNR5H) = fin de combat / clear de mission. Cle AES = "oINq0rfUFPx5MgmT".
//
// La requete contient : Kz7qfSs5[{k9cxD7Ba=serial "70000", j3g5P4cq}], rXvA1E5y[{30 stats de
// combat}], 0xgIw1Ns[{edy7fq3L,...}]. La reponse doit fournir RECOMPENSES + CLEAR.
//
// RE (2026-07-02, via .so + factory Windows) :
//  - Section **"F5Vs19mb"** = `MissionRewardResponse` (readParam .so 0xf778fc, Win FUN_008f6050
//    vtable 0x18f3d80) -> remplit `MissionRewardInfo`. Champs (key -> setter) :
//      92ms7J5M=id, wPb5uMR9=setLvupFlg, 81CsGAR9=setBeforeLv, d96tuT2E=setIncExp (XP joueur),
//      0egM9p7I=setReinReqState, Najhr8m6=setZel, HTVh8a65=setKarma, b8D18b7e=setExpGuild,
//      7adBa819=setKarmaGuild, gzs3Bw10=setZelGuild, 03fFk30f=setEToken.
//  - Section **"UT1SVg59"** = `UserClearMissionInfoResponse` (readParam .so 0xf82074, Win
//    FUN_00923960 vtable 0x18f4fc8) -> ajoute la mission a UserClearMissionInfoList (progression).
//    Champs : h7eY3sAK=user_id, j28VNcUW=mission_id, JATWnN57=clear_date, HIQoGb10/Wi4EFe6f/
//    RWX38boG/bGx5ku0F=annexes.
extern std::string g_lastMissionId; // pose par MissionStart.cpp

namespace {
struct MissionReward
{
	std::string id;          // 92ms7J5M
	std::string lvup_flg;    // wPb5uMR9
	std::string before_lv;   // 81CsGAR9
	std::string inc_exp;     // d96tuT2E (XP joueur gagnee)
	std::string rein_state;  // 0egM9p7I
	std::string zel;         // Najhr8m6 (argent)
	std::string karma;       // HTVh8a65 (karma)
	std::string exp_guild;   // b8D18b7e
	std::string karma_guild; // 7adBa819
	std::string zel_guild;   // gzs3Bw10
	std::string etoken;      // 03fFk30f
};

struct ClearInfo
{
	std::string user_id;    // h7eY3sAK
	std::string mission_id; // j28VNcUW
	std::string clear_date; // JATWnN57
	std::string f1;         // HIQoGb10
	std::string f2;         // Wi4EFe6f
	std::string f3;         // RWX38boG
	std::string f4;         // bGx5ku0F
};

struct MissionEndResp
{
	MissionReward reward;
	ClearInfo clear;
};
} // namespace

template <>
struct glz::meta<MissionReward>
{
	using T = MissionReward;
	static constexpr auto value = glz::object(
		"92ms7J5M", &T::id,
		"wPb5uMR9", &T::lvup_flg,
		"81CsGAR9", &T::before_lv,
		"d96tuT2E", &T::inc_exp,
		"0egM9p7I", &T::rein_state,
		"Najhr8m6", &T::zel,
		"HTVh8a65", &T::karma,
		"b8D18b7e", &T::exp_guild,
		"7adBa819", &T::karma_guild,
		"gzs3Bw10", &T::zel_guild,
		"03fFk30f", &T::etoken
	);
};

template <>
struct glz::meta<ClearInfo>
{
	using T = ClearInfo;
	static constexpr auto value = glz::object(
		"h7eY3sAK", &T::user_id,
		"j28VNcUW", &T::mission_id,
		"JATWnN57", &T::clear_date,
		"HIQoGb10", &T::f1,
		"Wi4EFe6f", &T::f2,
		"RWX38boG", &T::f3,
		"bGx5ku0F", &T::f4
	);
};

template <>
struct glz::meta<MissionEndResp>
{
	using T = MissionEndResp;
	static constexpr auto value = glz::object(
		"F5Vs19mb", pkg::glaze::single_array<&T::reward>(),
		"UT1SVg59", pkg::glaze::single_array<&T::clear>()
	);
};

HANDLEF(MissionEnd)
{
	LOG_DEBUG << "=== MissionEnd (9TvyNR5H) REQUEST: " << json;

	const std::string mid = g_lastMissionId.empty() ? "1" : g_lastMissionId;
	const auto now = std::chrono::duration_cast<std::chrono::seconds>(
						 std::chrono::system_clock::now().time_since_epoch())
						 .count();

	// Vraies recompenses de la mission (missions_data.json : xp/zel/karma reels par mission_id)
	// + table de drops (unit_id -> {drop_pct, element}) pour rouler les unites obtenues.
	std::string r_xp = "100", r_zel = "1000", r_karma = "100";
	std::map<std::string, std::pair<int, std::string>> dropMap;
	{
		static glz::json_t g_md;
		static bool g_mdLoaded = false;
		if (!g_mdLoaded)
		{
			std::string b;
			glz::read_file_json(g_md, "./system/missions_data.json", b);
			g_mdLoaded = true;
		}
		if (g_md.is_object() && g_md.contains(mid))
		{
			const glz::json_t& m = g_md.at(mid);
			auto gs = [&](const char* k, const std::string& dv) -> std::string {
				if (m.contains(k) && m.at(k).is_string()) return m.at(k).get_string();
				return dv;
			};
			r_xp = gs("xp", r_xp);
			r_zel = gs("zel", r_zel);
			r_karma = gs("karma", r_karma);
			if (m.contains("waves") && m.at("waves").is_array())
			{
				for (const auto& wave : m.at("waves").get_array())
				{
					if (!wave.contains("monsters") || !wave.at("monsters").is_array()) continue;
					for (const auto& mon : wave.at("monsters").get_array())
					{
						auto mg = [&](const char* k) -> std::string {
							return (mon.contains(k) && mon.at(k).is_string()) ? mon.at(k).get_string() : std::string();
						};
						std::string uid = mg("unit_id");
						if (uid.empty()) continue;
						int pct = 0; try { pct = std::stoi(mg("drop_pct")); } catch (...) {}
						dropMap[uid] = { pct, mg("element") };
					}
				}
			}
		}
	}

	// Zel/karma COLLECTES pendant le combat : le client les tallie et les envoie dans la requete,
	// section rXvA1E5y[0] -> Najhr8m6=zel collecte, HTVh8a65=karma collecte. On les ajoute au
	// bonus de mission (sinon le joueur ne recoit pas ce qu'il a ramasse).
	std::string col_zel = "0", col_karma = "0", defeated;
	{
		glz::json_t req{};
		if (!glz::read_json(req, json) && req.is_object() && req.contains("rXvA1E5y") &&
			req.at("rXvA1E5y").is_array() && !req.at("rXvA1E5y").get_array().empty())
		{
			const glz::json_t& st = req.at("rXvA1E5y").get_array()[0];
			auto g = [&](const char* k) -> std::string {
				return (st.contains(k) && st.at(k).is_string()) ? st.at(k).get_string() : std::string();
			};
			std::string z = g("Najhr8m6"), k = g("HTVh8a65");
			if (!z.empty()) col_zel = z;
			if (!k.empty()) col_karma = k;
			defeated = g("L16VDrk5"); // "unitId:count,unitId:count" = monstres vaincus
		}
	}

	// ROLL des drops d'unites (le mob obtenu) + PERSISTANCE (player_inventory.json).
	if (!defeated.empty() && !dropMap.empty())
	{
		static bool seeded = false;
		if (!seeded) { std::srand((unsigned)std::time(nullptr)); seeded = true; }
		PlayerInv inv;
		std::string ib;
		glz::read_file_json(inv, "./system/player_inventory.json", ib); // ok si le fichier n'existe pas
		int gained = 0;
		std::stringstream ss(defeated);
		std::string tok;
		while (std::getline(ss, tok, ','))
		{
			auto c = tok.find(':');
			if (c == std::string::npos) continue;
			std::string uid = tok.substr(0, c);
			int count = 0; try { count = std::stoi(tok.substr(c + 1)); } catch (...) {}
			auto it = dropMap.find(uid);
			if (it == dropMap.end()) continue;
			const int pct = it->second.first;
			for (int n = 0; n < count; ++n)
				if (pct > 0 && (std::rand() % 100) < pct)
				{
					inv.units.push_back({ uid, it->second.second });
					++gained;
				}
		}
		if (gained > 0)
		{
			// cap anti-croissance infinie (garde les 3000 plus recentes)
			if (inv.units.size() > 3000)
				inv.units.erase(inv.units.begin(), inv.units.begin() + (inv.units.size() - 3000));
			std::string ob;
			glz::write_file_json(inv, "./system/player_inventory.json", ob);
			LOG_DEBUG << "MissionEnd: " << gained << " unites droppees (inventaire total="
					  << inv.units.size() << ")";
		}
	}
	auto toI = [](const std::string& s) -> long long { try { return std::stoll(s); } catch (...) { return 0; } };
	const std::string tot_zel   = std::to_string(toI(col_zel)   + toI(r_zel));
	const std::string tot_karma = std::to_string(toI(col_karma) + toI(r_karma));

	MissionEndResp resp{};
	// recompenses (section F5Vs19mb)
	resp.reward.id = mid;
	resp.reward.lvup_flg = "0";
	resp.reward.before_lv = "1";
	resp.reward.inc_exp = r_xp;      // XP joueur reelle (bonus mission)
	resp.reward.rein_state = "0";
	resp.reward.zel = tot_zel;       // zel collecte + bonus mission
	resp.reward.karma = tot_karma;   // karma collecte + bonus mission
	resp.reward.exp_guild = "0";
	resp.reward.karma_guild = "0";
	resp.reward.zel_guild = "0";
	resp.reward.etoken = "0";
	// clear de la mission (section UT1SVg59) -> progression
	resp.clear.user_id = "0839899613932562";
	resp.clear.mission_id = mid;
	resp.clear.clear_date = std::to_string(now);
	resp.clear.f1 = "0";
	resp.clear.f2 = "0";
	resp.clear.f3 = "0";
	resp.clear.f4 = "0";

	std::string buffer{};
	const auto& ec = glz::write_json(resp, buffer);
	if (ec)
	{
		co_return HandleResult::error("Serialization error", glz::format_error(ec, buffer));
	}
	LOG_DEBUG << "=== MissionEnd RESPONSE: " << buffer;
	co_return HandleResult::success(buffer);
}
