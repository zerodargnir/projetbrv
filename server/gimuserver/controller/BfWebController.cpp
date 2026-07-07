#include "App.hpp"
#include "BfWebController.hpp"
#include "WebTerms.hpp"

#include <filesystem>

using namespace drogon;
namespace fs = std::filesystem;

// This code was intended for generating placeholders in assets
// DESACTIVE : le generateur ecrasait les vrais fichiers existants (branche else =
// fichier existe) par des placeholders 21o -> a corrompu les 147 .dat MST et casse
// la synchro ("Refreshing the latest data files" en boucle). Comportement original
// retabli : servir le fichier existant, 404 si absent.
#define ENABLE_FILE_GENERATOR 0

#if ENABLE_FILE_GENERATOR
// Parse dimensions from filename (e.g., "MapVillage_640x0809.png" -> 640, 809)
static bool parseDimensions(const std::string& filename, int& width, int& height)
{
    size_t lastUnderscore = filename.find_last_of('_');
    size_t dotPos = filename.find_last_of('.');
    if (lastUnderscore == std::string::npos || dotPos == std::string::npos || lastUnderscore >= dotPos) {
        return false;
    }

    std::string dimStr = filename.substr(lastUnderscore + 1, dotPos - lastUnderscore - 1);
    size_t xPos = dimStr.find('x');
    if (xPos == std::string::npos) return false;

    try {
        width = std::stoi(dimStr.substr(0, xPos));
        height = std::stoi(dimStr.substr(xPos + 1));
        return true;
    }
    catch (...) {
        return false;
    }
}

// Generate a minimal PNG with specified dimensions
static std::vector<char> generatePlaceholderPng(int width, int height) {
    // NOTE: l'ancien generateur ci-dessous produisait des PNG INVALIDES
    // (IDAT non compresse en zlib + CRC a zero), ce qui faisait crasher le
    // decodeur d'images du client. On renvoie a la place un PNG 1x1 transparent
    // parfaitement valide (le moteur le redimensionne tout seul).
    (void)width; (void)height;
    static const unsigned char kValidPng[] = {
        0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A,
        0x00,0x00,0x00,0x0D,0x49,0x48,0x44,0x52,
        0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,
        0x08,0x06,0x00,0x00,0x00,0x1F,0x15,0xC4,0x89,
        0x00,0x00,0x00,0x0A,0x49,0x44,0x41,0x54,
        0x78,0x9C,0x63,0x00,0x01,0x00,0x00,0x05,0x00,0x01,
        0x0D,0x0A,0x2D,0xB4,
        0x00,0x00,0x00,0x00,0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82
    };
    return std::vector<char>(reinterpret_cast<const char*>(kValidPng),
                             reinterpret_cast<const char*>(kValidPng) + sizeof(kValidPng));

    std::vector<char> pngData;

    // PNG signature
    const char signature[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    pngData.insert(pngData.end(), signature, signature + 8);

    // IHDR chunk
    uint32_t ihdrLength = htonl(13);
    pngData.insert(pngData.end(), (char*)&ihdrLength, (char*)&ihdrLength + 4);
    const char ihdrType[] = "IHDR";
    pngData.insert(pngData.end(), ihdrType, ihdrType + 4);

    uint32_t w = htonl(width);
    uint32_t h = htonl(height);
    pngData.insert(pngData.end(), (char*)&w, (char*)&w + 4);
    pngData.insert(pngData.end(), (char*)&h, (char*)&h + 4);

    const char ihdrData[] = { 8, 2, 0, 0, 0 }; // Bit depth 8, RGB, no compression, filter, interlace
    pngData.insert(pngData.end(), ihdrData, ihdrData + 5);

    uint32_t ihdrCrc = htonl(0); // Simplified CRC
    pngData.insert(pngData.end(), (char*)&ihdrCrc, (char*)&ihdrCrc + 4);

    // IDAT chunk (minimal data for a blank image)
    uint32_t idatLength = htonl(12 + width * 4 * height); // Adjust length based on dimensions
    pngData.insert(pngData.end(), (char*)&idatLength, (char*)&idatLength + 4);
    const char idatType[] = "IDAT";
    pngData.insert(pngData.end(), idatType, idatType + 4);

    // Minimal data: transparent pixels (RGBA)
    std::vector<char> idatData;
    for (int y = 0; y < height; ++y) {
        idatData.push_back(0); // Filter type
        for (int x = 0; x < width; ++x) {
            idatData.push_back(0); idatData.push_back(0); idatData.push_back(0); idatData.push_back(0); // Transparent
        }
    }
    pngData.insert(pngData.end(), idatData.begin(), idatData.end());

    uint32_t idatCrc = htonl(0);
    pngData.insert(pngData.end(), (char*)&idatCrc, (char*)&idatCrc + 4);

    // IEND chunk
    uint32_t iendLength = htonl(0);
    pngData.insert(pngData.end(), (char*)&iendLength, (char*)&iendLength + 4);
    const char iendType[] = "IEND";
    pngData.insert(pngData.end(), iendType, iendType + 4);
    uint32_t iendCrc = htonl(0);
    pngData.insert(pngData.end(), (char*)&iendCrc, (char*)&iendCrc + 4);

    return pngData;
}
#endif

// Pour une image MANQUANTE (asset PC absent de notre backup), trouve une image
// SOEUR existante : meme prefixe alpha (avant le 1er chiffre) + meme extension,
// dans le meme dossier. LECTURE SEULE : ne modifie/ecrit JAMAIS sur le disque
// (contrairement a l'ancien ENABLE_FILE_GENERATOR qui corrompait les fichiers).
// Renvoie le chemin de la soeur trouvee, ou "" si aucune.
static std::string findSiblingImage(const std::string& fullPath, const std::string& filename, const std::string& ext)
{
    std::error_code ec;
    fs::path dir = fs::path(fullPath).parent_path();
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return {};

    std::string prefix = filename;
    for (size_t i = 0; i < filename.size(); ++i) {
        if (filename[i] >= '0' && filename[i] <= '9') { prefix = filename.substr(0, i); break; }
    }
    // Passe 1 : soeur de MEME prefixe alpha (meilleure ressemblance visuelle).
    if (!prefix.empty()) {
        for (const auto& e : fs::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!e.is_regular_file(ec)) continue;
            auto n = e.path().filename().string();
            if (n.size() >= prefix.size() && n.compare(0, prefix.size(), prefix) == 0
                && n.size() >= ext.size() && n.compare(n.size() - ext.size(), ext.size(), ext) == 0
                && e.file_size(ec) > 200) {
                return e.path().string();
            }
        }
    }

    // Passe 2 : N'IMPORTE QUELLE image valide de meme extension dans le meme dossier.
    // Le client telecharge les images de bannieres EN BATCH ; si UNE seule manque (404),
    // tout le batch echoue avec "Error occurred during download" et bloque la porte de summon.
    // Servir une image valide (meme si "fausse") fait reussir le download -> pas d'erreur.
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!e.is_regular_file(ec)) continue;
        auto n = e.path().filename().string();
        if (n.size() >= ext.size() && n.compare(n.size() - ext.size(), ext.size(), ext) == 0
            && e.file_size(ec) > 200) {
            return e.path().string();
        }
    }
    return {};
}

void BfWebController::HandleWebPage(const HttpRequestPtr& rq, std::function<void(const HttpResponsePtr&)>&& callback)
{
    DumpLog log;
    theServer()->tryOpenHttpDumpLog("webpage", log);
    log << rq;

	if (rq->getPath() == "/bf/web/terms.htm")
	{
        // TODO: convert this to a drogon ctl controller
        auto resp = HttpResponse::newFileResponse((const unsigned char*)WEB_TERMS_DATA, strlen(WEB_TERMS_DATA), "", CT_TEXT_HTML);
        resp->setCloseConnection(true); // kill session
		callback(resp);
		return;
	}

    // Construct the full filesystem path
	auto path = getDocumentRoot() + rq->getPath();
    LOG_DEBUG << "Requesting path: " << path;


    // Serve existing file if it exists
	if (!fs::exists(path) || fs::is_directory(path))
	{
		logDlc() << rq->getPath();
		// Image manquante : servir une image soeur existante (lecture seule) pour
		// eviter "Error occurred during download" sur les assets PC absents du backup.
		std::string fname = rq->getPath().substr(rq->getPath().find_last_of('/') + 1);
		std::string ext;
		{ size_t dp = fname.find_last_of('.'); if (dp != std::string::npos) ext = fname.substr(dp); }
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
		{
			std::string sib = findSiblingImage(path, fname, ext);
			if (!sib.empty()) { callback(HttpResponse::newFileResponse(sib)); return; }

			// Fallback GLOBAL : si meme le dossier n'a aucune image valide, servir une
			// image gacha connue-valide (cachee une fois) pour que tout download d'image
			// reussisse -> jamais d'"Error occurred during download" sur la porte de summon.
			static std::string g_fallbackImg = []() -> std::string {
				std::error_code fec;
				fs::path gdir = fs::path(getDocumentRoot()) / "content" / "gacha";
				if (fs::exists(gdir, fec) && fs::is_directory(gdir, fec)) {
					for (const auto& e : fs::directory_iterator(gdir, fec)) {
						if (fec) break;
						if (!e.is_regular_file(fec)) continue;
						auto n = e.path().filename().string();
						if (n.size() > 4 && n.compare(n.size() - 4, 4, ".png") == 0
							&& e.file_size(fec) > 200)
							return e.path().string();
					}
				}
				return {};
			}();
			if (!g_fallbackImg.empty()) { callback(HttpResponse::newFileResponse(g_fallbackImg)); return; }
		}
		callback(HttpResponse::newNotFoundResponse());
	}
	else
	{
#if !ENABLE_FILE_GENERATOR
		callback(HttpResponse::newFileResponse(path));
#else
        LOG_INFO << "Generating placeholder for: " << path;

        try {
            // Ensure directory exists
            fs::path dirPath = path.substr(0, path.find_last_of('/'));
            if (!dirPath.empty() && !fs::exists(dirPath)) {
                fs::create_directories(dirPath);
            }

            std::ofstream outFile(path, std::ios::binary);
            if (!outFile.is_open()) {
                LOG_ERROR << "Failed to create placeholder: " << path;
                callback(HttpResponse::newNotFoundResponse());
                return;
            }

            // Generate placeholder based on file extension
            std::string filename = rq->getPath().substr(rq->getPath().find_last_of('/') + 1);
            std::string ext;
            { size_t dp = filename.find_last_of('.'); if (dp != std::string::npos) ext = filename.substr(dp); }
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                // Copie une VRAIE image existante du meme type (meme prefixe alpha +
                // meme extension) dans le meme dossier. Un placeholder invalide/1x1
                // fait crasher le rendu OU declenche "Error occurred during download".
                std::string prefix = filename;
                for (size_t i = 0; i < filename.size(); ++i) {
                    char c = filename[i];
                    if (c >= '0' && c <= '9') { prefix = filename.substr(0, i); break; }
                }
                std::string templatePath;
                std::error_code fec;
                if (!prefix.empty() && fs::exists(dirPath, fec)) {
                    for (const auto& e : fs::directory_iterator(dirPath, fec)) {
                        if (!e.is_regular_file()) continue;
                        auto n = e.path().filename().string();
                        if (n.size() >= prefix.size() && n.compare(0, prefix.size(), prefix) == 0
                            && n.size() > ext.size() && n.substr(n.size() - ext.size()) == ext
                            && e.file_size() > 200) {
                            templatePath = e.path().string();
                            break;
                        }
                    }
                }
                if (templatePath.empty() && (ext == ".jpg" || ext == ".jpeg")) {
                    // fallback global : un vrai JPG (wallpaper) toujours valide
                    std::string fb = getDocumentRoot() + "/content/wallpaper/title_logo20151028.jpg";
                    if (fs::exists(fb, fec)) templatePath = fb;
                }
                if (!templatePath.empty()) {
                    std::ifstream src(templatePath, std::ios::binary);
                    outFile << src.rdbuf();
                } else {
                    auto placeholderPng = generatePlaceholderPng(1, 1);
                    outFile.write(placeholderPng.data(), placeholderPng.size());
                }
            }
            else if (filename.find(".csv") != std::string::npos) {
                std::string placeholderCsv = "id,value\n0,placeholder";
                outFile.write(placeholderCsv.data(), placeholderCsv.size());
            }
            else if (filename.find(".dat") != std::string::npos) {
                std::string placeholderDat = "placeholder_game_data";
                outFile.write(placeholderDat.data(), placeholderDat.size());
            }
            else {
                std::string placeholderGeneric = "placeholder";
                outFile.write(placeholderGeneric.data(), placeholderGeneric.size());
            }
            outFile.close();
            LOG_INFO << "Generated placeholder for: " << path;

            // Serve the newly created placeholder file
            auto resp = HttpResponse::newFileResponse(path);
            // Drogon will automatically detect MIME type from extension
            callback(resp);
        }
        catch (const std::exception& e) {
            LOG_ERROR << "Error generating placeholder: " << e.what();
            callback(HttpResponse::newNotFoundResponse());
        }
#endif
	}
}

void BfWebController::HandleDefault(const HttpRequestPtr& rq, std::function<void(const HttpResponsePtr&)>&& callback)
{
	auto resp = HttpResponse::newHttpResponse();
	resp->setStatusCode(k200OK);
	resp->setBody("gimuserver server works!");
	callback(resp);
}
