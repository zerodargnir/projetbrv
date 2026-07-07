import os, glob, sys
import numpy as np

LOCAL = r"C:\Users\mewrd\AppData\Local\Packages\gumi.BraveFrontier_9n4b6myyx21tw\LocalState"
os.chdir(LOCAL)

# cle de de-corruption : byte[i] += (i*i) % 256  ->  inverse = byte[i] -= (i*i)%256
# (i*i)%256 == ((i%256)**2)%256, donc cle periodique de 256 octets
key = np.array([(i * i) % 256 for i in range(256)], dtype=np.uint8)

PNG = b"\x89PNG"
JPG = b"\xff\xd8"

def is_valid(head, ext):
    if ext == "png":
        return head[:4] == PNG
    return head[:2] == JPG

def inverse(arr):
    idx = np.arange(len(arr)) % 256
    return (arr - key[idx]).astype(np.uint8)

fixed = skipped_valid = unfixable = 0
unfix_list = []

files = glob.glob("*.png") + glob.glob("*.jpg")
total = len(files)
for n, f in enumerate(files):
    ext = f.lower().rsplit(".", 1)[-1]
    try:
        arr = np.fromfile(f, dtype=np.uint8)
    except Exception:
        continue
    if len(arr) < 4:
        continue
    if is_valid(bytes(arr[:4]), ext):
        skipped_valid += 1
        continue
    fx = inverse(arr)
    if is_valid(bytes(fx[:4]), ext):
        fx.tofile(f)
        fixed += 1
    else:
        unfixable += 1
        if len(unfix_list) < 25:
            unfix_list.append(f)

print(f"TOTAL images: {total}")
print(f"deja valides : {skipped_valid}")
print(f"REPARES      : {fixed}")
print(f"non reparable: {unfixable}")
if unfix_list:
    print("exemples non reparables:", unfix_list)
