import sys
import re

files = {}

if len(sys.argv) > 1:
	for arg in sys.argv[1:]:
		with open(arg, "rb") as f:
			files[arg] = f.read()
			
			
cpp = ""
for (file, content) in files.items():
	hc = [hex(c) for c in content]
	name = re.sub("[^a-zA-Z0-9_]+", "_", file)
	cpp += "\tconst char " + name + "[] = {" + ", ".join(hc) + "};\n\n"
	

print("namespace resources {\n\n" + cpp + "}")

			