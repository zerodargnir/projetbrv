/**
* @file main.cpp
* @author Arves100
* @date 30/10/2024
* @brief Entrypoint for Brave Frontier Windows offline mod
*/
#include "pch.h"
#include <detours/detours.h>
#include <serverconfig.h>

typedef HINTERNET(WINAPI* InternetConnectW_t)(
    _In_ HINTERNET     hInternet,
    _In_ LPCWSTR       lpszServerName,
    _In_ INTERNET_PORT nServerPort,
    _In_ LPCWSTR       lpszUserName,
    _In_ LPCWSTR       lpszPassword,
    _In_ DWORD         dwService,
    _In_ DWORD         dwFlags,
    _In_ DWORD_PTR     dwContext
);

typedef HINTERNET(WINAPI* InternetConnectA_t)(
    _In_ HINTERNET     hInternet,
    _In_ LPCSTR        lpszServerName,
    _In_ INTERNET_PORT nServerPort,
    _In_ LPCSTR        lpszUserName,
    _In_ LPCSTR        lpszPassword,
    _In_ DWORD         dwService,
    _In_ DWORD         dwFlags,
    _In_ DWORD_PTR     dwContext
);

typedef HINTERNET(WINAPI* HttpOpenRequestA_t)(
    _In_ HINTERNET hConnect,
    _In_ LPCSTR    lpszVerb,
    _In_ LPCSTR    lpszObjectName,
    _In_ LPCSTR    lpszVersion,
    _In_ LPCSTR    lpszReferrer,
    _In_ LPCSTR* lplpszAcceptTypes,
    _In_ DWORD     dwFlags,
    _In_ DWORD_PTR dwContext
);

typedef HINTERNET(WINAPI* HttpOpenRequestW_t)(
    _In_ HINTERNET hConnect,
    _In_ LPCWSTR   lpszVerb,
    _In_ LPCWSTR   lpszObjectName,
    _In_ LPCWSTR   lpszVersion,
    _In_ LPCWSTR   lpszReferrer,
    _In_ LPCWSTR* lplpszAcceptTypes,
    _In_ DWORD     dwFlags,
    _In_ DWORD_PTR dwContext
);

#ifdef OFFLINE_DEPLOY
typedef void (WINAPI* OfflineModStartup_t)(void);
static HMODULE g_ofmLib = nullptr;
static OfflineModStartup_t g_ofmStartup = nullptr;
#endif

static InternetConnectW_t TrueInternetConnectW = InternetConnectW;
static InternetConnectA_t TrueInternetConnectA = InternetConnectA;
static HttpOpenRequestA_t TrueHttpOpenRequestA = HttpOpenRequestA;
static HttpOpenRequestW_t TrueHttpOpenRequestW = HttpOpenRequestW;

static HINTERNET WINAPI MyInternetConnectA(
    _In_ HINTERNET     hInternet,
    _In_ LPCSTR        lpszServerName,
    _In_ INTERNET_PORT nServerPort,
    _In_ LPCSTR        lpszUserName,
    _In_ LPCSTR        lpszPassword,
    _In_ DWORD         dwService,
    _In_ DWORD         dwFlags,
    _In_ DWORD_PTR     dwContext
)
{
#ifndef OFFLINE_DEPLOY
    printf("InternetConnectA: %s:%u (SVC:%x)\n", lpszServerName, nServerPort, dwService);
#endif

    if (strstr(lpszServerName, "microsoft.com") != nullptr || strstr(lpszServerName, "wikia.com") != nullptr || strstr(lpszServerName, "fandom.com") != nullptr)
        return TrueInternetConnectA(hInternet, lpszServerName, INTERNET_DEFAULT_HTTP_PORT, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);

    return TrueInternetConnectA(hInternet, SERVICE_IP, SERVICE_PORT, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);
}

static HINTERNET WINAPI MyInternetConnectW(
    _In_ HINTERNET     hInternet,
    _In_ LPCWSTR        lpszServerName,
    _In_ INTERNET_PORT nServerPort,
    _In_ LPCWSTR        lpszUserName,
    _In_ LPCWSTR        lpszPassword,
    _In_ DWORD         dwService,
    _In_ DWORD         dwFlags,
    _In_ DWORD_PTR     dwContext
)
{
#ifndef OFFLINE_DEPLOY
    wprintf(L"InternetConnectW: %s:%u (SVC:%x)\n", lpszServerName, nServerPort, dwService);
#endif

    if (wcswcs(lpszServerName, L"microsoft.com") != nullptr || wcswcs(lpszServerName, L"wikia.com") != nullptr || wcswcs(lpszServerName, L"fandom.com") != nullptr)
        return TrueInternetConnectW(hInternet, lpszServerName, INTERNET_DEFAULT_HTTP_PORT, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);

    return TrueInternetConnectW(hInternet, SERVICE_IP_W, SERVICE_PORT, lpszUserName, lpszPassword, dwService, dwFlags, dwContext);
}

static void PatchSecurityOptions(HINTERNET hInternet)
{
    DWORD dwFlags2;
    DWORD dwBuffLen = sizeof(dwFlags2);

    if (InternetQueryOption(hInternet, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags2, &dwBuffLen))
    {
#ifndef OFFLINE_DEPLOY
        printf("patched options...\n");
#endif
        dwFlags2 |= SECURITY_SET_MASK;
        InternetSetOption(hInternet, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags2, sizeof(dwFlags2));
    }
}

static HINTERNET WINAPI MyHttpOpenRequestA(
    _In_ HINTERNET hConnect,
    _In_ LPCSTR    lpszVerb,
    _In_ LPCSTR    lpszObjectName,
    _In_ LPCSTR    lpszVersion,
    _In_ LPCSTR    lpszReferrer,
    _In_ LPCSTR* lplpszAcceptTypes,
    _In_ DWORD     dwFlags,
    _In_ DWORD_PTR dwContext
)
{
    dwFlags &= ~INTERNET_FLAG_SECURE;
    dwFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
    auto ret = TrueHttpOpenRequestA(hConnect, lpszVerb, lpszObjectName, lpszVersion, lpszReferrer, lplpszAcceptTypes, dwFlags, dwContext);

    if (!ret)
        return nullptr;

    PatchSecurityOptions(ret);
    return ret;
}

#if SERVICE_DISABLE_HTTPS
HINTERNET WINAPI MyHttpOpenRequestW(
    _In_ HINTERNET hConnect,
    _In_ LPCWSTR   lpszVerb,
    _In_ LPCWSTR   lpszObjectName,
    _In_ LPCWSTR   lpszVersion,
    _In_ LPCWSTR   lpszReferrer,
    _In_ LPCWSTR* lplpszAcceptTypes,
    _In_ DWORD     dwFlags,
    _In_ DWORD_PTR dwContext
)
{
    dwFlags &= ~INTERNET_FLAG_SECURE;
    dwFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
    auto ret = TrueHttpOpenRequestW(hConnect, lpszVerb, lpszObjectName, lpszVersion, lpszReferrer, lplpszAcceptTypes, dwFlags, dwContext);

    if (!ret)
        return nullptr;

    PatchSecurityOptions(ret);
    return ret;
}
#endif

static void DetourDetach()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)TrueInternetConnectA, (PVOID)MyInternetConnectA);
    DetourDetach(&(PVOID&)TrueInternetConnectW, (PVOID)MyInternetConnectW);
#if SERVICE_DISABLE_HTTPS
    DetourDetach(&(PVOID&)TrueHttpOpenRequestA, (PVOID)MyHttpOpenRequestA);
    DetourDetach(&(PVOID&)TrueHttpOpenRequestW, (PVOID)MyHttpOpenRequestW);
#endif
    DetourTransactionCommit();
}

// Patches binaires appliques au module principal (BraveFrontier.Windows.exe) au
// chargement. INDISPENSABLES a l'affichage des bannieres de summon : sans eux, les
// bannieres n'apparaissent pas (avant on les appliquait a la main via cdb). On les
// rend permanents ici pour jouer SANS debugger.
//  - RVA 0x65650b : remplace une routine de calcul float/division (scaling/mise en
//    page des bannieres) par une version corrigee (47 octets).
//  - RVA 0x15b178 / 0x15b182 : NOP de deux instructions (checks qui echouent offline).
static void ApplyClientPatches()
{
    HMODULE hExe = GetModuleHandleW(nullptr);
    if (!hExe)
        return;
    BYTE* base = reinterpret_cast<BYTE*>(hExe);

    static const BYTE p_nop[] = { 0x90, 0x90 };
    static const BYTE p_calc[] = {
        0x83, 0xf9, 0x01, 0x83, 0xd1, 0x00, 0xf7, 0xf9, 0x66, 0x0f, 0x6e, 0xc2,
        0x0f, 0x5b, 0xc0, 0xf3, 0x0f, 0x5e, 0xc1, 0xf3, 0x0f, 0x2c, 0xc0, 0x85,
        0xc0, 0x79, 0x12, 0x99, 0x8b, 0xc8, 0x8b, 0x46, 0x18, 0x2b, 0x46, 0x14,
        0x33, 0xca, 0x2b, 0xca, 0xc1, 0xf8, 0x02, 0x2b, 0xc1, 0x5e, 0xc3
    };

    struct Patch { DWORD rva; const BYTE* bytes; SIZE_T len; };
    static const Patch patches[] = {
        { 0x15b178, p_nop,  sizeof(p_nop)  },
        { 0x15b182, p_nop,  sizeof(p_nop)  },
        { 0x65650b, p_calc, sizeof(p_calc) },
    };

    for (const auto& pt : patches)
    {
        void* addr = base + pt.rva;
        DWORD oldProt = 0;
        if (VirtualProtect(addr, pt.len, PAGE_EXECUTE_READWRITE, &oldProt))
        {
            memcpy(addr, pt.bytes, pt.len);
            DWORD tmp = 0;
            VirtualProtect(addr, pt.len, oldProt, &tmp);
            FlushInstructionCache(GetCurrentProcess(), addr, pt.len);
        }
    }

    // === PARSE DE POSITION ROBUSTE (fix crash + position PAR ennemi via le serveur) ===
    // Layout confirme par cdb : la position ("150") est une std::string MSVC SSO -> le texte est
    // INLINE a l'offset 0, size a +0x10, capacity a +0x14. On installe dans le code cave un
    // mini-parseur d'entier (SSO-aware) et on retarge les 2 CALL de FUN_0080e270 vers lui.
    // Le cave reste PAGE_EXECUTE_READWRITE (on NE restaure PAS la protection -> executabilite
    // garantie, cause du bug precedent). Ops registre uniquement -> insensible a l'ASLR.
    {
        const DWORD CAVE_RVA = 0x1708d8; // Ghidra 0x5708d8, 73 octets CC libres
        static const BYTE p_parse[] = {
            0x8B,0x41,0x14,       // mov eax,[ecx+0x14]   (capacity)
            0x83,0xF8,0x10,       // cmp eax,0x10
            0x72,0x02,            // jb +2  (SSO: char* = ecx)
            0x8B,0x09,            // mov ecx,[ecx]        (heap: char* = *string)
            0x31,0xC0,            // xor eax,eax
            0x0F,0xB6,0x11,       // movzx edx,byte[ecx]  (<- LOOP @0x0C)
            0x80,0xFA,0x30,       // cmp dl,'0'
            0x72,0x10,            // jb DONE (@0x24)
            0x80,0xFA,0x39,       // cmp dl,'9'
            0x77,0x0B,            // ja DONE
            0x83,0xEA,0x30,       // sub edx,'0'
            0x6B,0xC0,0x0A,       // imul eax,eax,10
            0x01,0xD0,            // add eax,edx
            0x41,                 // inc ecx
            0xEB,0xE8,            // jmp LOOP
            0xF3,0x0F,0x2A,0xC0,  // cvtsi2ss xmm0,eax    (<- DONE @0x24)
            0xC3                  // ret
        };
        BYTE* cave = base + CAVE_RVA;
        DWORD op = 0, tmp = 0;
        // cave laisse en RWX (pas de restauration -> executable garanti)
        if (VirtualProtect(cave, sizeof(p_parse), PAGE_EXECUTE_READWRITE, &op)) {
            memcpy(cave, p_parse, sizeof(p_parse));
            FlushInstructionCache(GetCurrentProcess(), cave, sizeof(p_parse));
        }
        // retarger les 2 CALL (E8 rel32) vers le cave : rel32 = CAVE_RVA - (call_rva + 5)
        const DWORD callRvas[2] = { 0x40e696, 0x40e6b3 };
        for (DWORD cr : callRvas) {
            DWORD rel = CAVE_RVA - (cr + 5);
            BYTE* p = base + cr;
            if (VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &op)) {
                p[0] = 0xE8;
                memcpy(p + 1, &rel, 4);
                FlushInstructionCache(GetCurrentProcess(), p, 5);
            }
        }
    }
}

static void DetourAttach()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueInternetConnectA, (PVOID)MyInternetConnectA);
    DetourAttach(&(PVOID&)TrueInternetConnectW, (PVOID)MyInternetConnectW);
#if SERVICE_DISABLE_HTTPS
    DetourAttach(&(PVOID&)TrueHttpOpenRequestA, (PVOID)MyHttpOpenRequestA);
    DetourAttach(&(PVOID&)TrueHttpOpenRequestW, (PVOID)MyHttpOpenRequestW);
#endif
    DetourTransactionCommit();
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved)  // reserved
{
    (void)hinstDLL;
    (void)lpvReserved;

    if (fdwReason == DLL_PROCESS_DETACH)
    {
#ifdef OFFLINE_DEPLOY
        if (g_ofmLib)
        {
            FreeLibrary(g_ofmLib);
        }
#endif
        DetourDetach();
    }
    else if (fdwReason == DLL_PROCESS_ATTACH)
    {
#ifndef OFFLINE_DEPLOY
        if (!AllocConsole())
        {
            MessageBoxA(nullptr, "ALLOC_CONSOLE_FAILED", "DEBUG", MB_OK | MB_ICONERROR);
        }
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
#else
        g_ofmLib = LoadLibraryW(L"offlinemod.dll");
        if (!g_ofmLib)
        {
            MessageBoxA(nullptr, "Unable to find the offline mod component, the application will now exit!", "Fatal Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        g_ofmStartup = (OfflineModStartup_t)GetProcAddress(g_ofmLib, "OfflineMod_startup");
        if (!g_ofmStartup)
        {
            MessageBoxA(nullptr, "The offline mod component is corrupted, the application will now exit!", "Fatal Error", MB_OK | MB_ICONERROR);
            FreeLibrary(g_ofmLib);
            return FALSE;
        }

        g_ofmStartup();
#endif

        ApplyClientPatches();
        DetourAttach();
    }

    return TRUE;
}
