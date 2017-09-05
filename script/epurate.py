
import os

files = dict()

for f in os.listdir("log"):
    try:
        p, a, d = f.replace(".log", "").split("_")
    except Exception:
        continue
    files.setdefault(p, dict())
    files[p].setdefault(a, list())
    files[p][a].append((f, d))

for p, f in files.items():
    for a in f.values():
        try:
            for fi in sorted(a, key=lambda b: int(b[1]))[:-2]:
                os.remove("log/" + fi[0])
        except Exception:
            continue
