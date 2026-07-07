import frida, sys, time, os

ROOT = r"C:\Users\mewrd\Desktop\brv"
SCRIPT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "frida_patch.js")

def on_message(msg, data):
    if msg.get("type") == "send":
        print("[SEND]", msg.get("payload"), flush=True)
    elif msg.get("type") == "error":
        print("[ERR]", msg.get("stack", msg), flush=True)
    else:
        print("[MSG]", msg, flush=True)

print("[*] attaching to BraveFrontier.Windows.exe ...", flush=True)
session = frida.attach("BraveFrontier.Windows.exe")
with open(SCRIPT, "r", encoding="utf-8") as f:
    code = f.read()
script = session.create_script(code)
script.on("message", on_message)
script.load()
print("[+] script charge:", SCRIPT, flush=True)
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    pass
