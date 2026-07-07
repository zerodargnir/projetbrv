import struct, sys, shutil

EXE = r"c:\Users\mewrd\Desktop\brv\client-appx\BraveFrontierAppxClient\BraveFrontier.Windows.exe"

# RVA -> octet attendu (1er octet du saut conditionnel) ; on remplace 2 octets par 90 90
TARGETS = [
    (0x15b178, 0x7e),  # JLE date #1
    (0x15b182, 0x7e),  # JLE date #2
    (0x15b18d, 0x74),  # JZ items
]

data = bytearray(open(EXE, "rb").read())

# Parse PE
e_lfanew = struct.unpack_from("<I", data, 0x3c)[0]
assert data[e_lfanew:e_lfanew+4] == b"PE\0\0", "pas un PE"
coff = e_lfanew + 4
num_sections = struct.unpack_from("<H", data, coff+2)[0]
opt_size = struct.unpack_from("<H", data, coff+16)[0]
opt_off = coff + 20
sec_off = opt_off + opt_size

sections = []
for i in range(num_sections):
    base = sec_off + i*40
    name = data[base:base+8].rstrip(b"\0").decode(errors="replace")
    vsize = struct.unpack_from("<I", data, base+8)[0]
    vaddr = struct.unpack_from("<I", data, base+12)[0]
    rawsize = struct.unpack_from("<I", data, base+16)[0]
    rawptr = struct.unpack_from("<I", data, base+20)[0]
    sections.append((name, vaddr, vsize, rawptr, rawsize))

def rva_to_off(rva):
    for name, vaddr, vsize, rawptr, rawsize in sections:
        if vaddr <= rva < vaddr + max(vsize, rawsize):
            return rawptr + (rva - vaddr), name
    return None, None

print("Sections:", [(s[0], hex(s[1])) for s in sections])
ok = True
for rva, expect in TARGETS:
    off, sec = rva_to_off(rva)
    if off is None:
        print(f"RVA {rva:#x}: introuvable"); ok=False; continue
    cur = data[off]
    print(f"RVA {rva:#x} -> file {off:#x} (sect {sec}) octet={cur:#04x} attendu={expect:#04x} suivant={data[off+1]:#04x}")
    if cur != expect:
        print("  !! octet inattendu, abandon"); ok=False

if not ok:
    print("ECHEC verification"); sys.exit(1)

if "--apply" in sys.argv:
    shutil.copy2(EXE, EXE + ".bak")
    for rva, expect in TARGETS:
        off, sec = rva_to_off(rva)
        data[off] = 0x90
        data[off+1] = 0x90
    open(EXE, "wb").write(data)
    print("PATCH APPLIQUE (sauvegarde .bak creee)")
else:
    print("Verification OK. Relancer avec --apply pour patcher.")
