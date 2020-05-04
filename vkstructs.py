
type_enum = []


def is_ext(name):
    prefixes = [
        "_KHR",
        "_EXT",
        "_NV",
        "_NVX",
        "_AMD",
        "_GOOGLE",
        "_INTEL",
        "_FUCHSIA",
        "_ANDROID",
        "_MVK",
        "_GGP",
        "_NN"
    ]
    for p in prefixes:
        if name.endswith(p):
            return True
    return False

while True:
    line = input()
    if len(line) == 0:
        break
    type_enum.append(line.strip())

for t in type_enum:
    eq = t.find("=")
    if eq > 0:
        try:
            r = t[eq + 1:]
            if r.endswith(","):
                r = r[:-1]
            int(r)
        except:
            continue
        t = t[:eq].strip()
        if is_ext(t):
            continue
    
    vk_stype = t
    if not vk_stype.startswith("VK_STRUCTURE_TYPE_"):
        print("\"" + vk_stype + "\" is not a value from VkStructureType")
        break
        
    words = vk_stype[18:].split("_")
    vk_type = "Vk"
    for w in words:
        vk_type += w[0] + w[1:].lower()
    
    print("VK_STRUCT_INIT(" + vk_type + ",\t\t\t" + vk_stype + ")")