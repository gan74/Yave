import sys
import re
import os
import subprocess


def extension(file):
    return file.split(".")[-1]
    

def clean_name(name):
    return name.replace("\\\\", "/").replace("\\", "/")


def process_file(fullname):
    content = ""
    with open(fullname, "r") as f:
        content = f.read()
        
    parent = ""
    s = fullname.rfind("/")
    if s >= 0:
        parent = fullname[:s] + "/"
    
    includes = []
    for line in content.split("\n"):
        if line.startswith("#include "):
            sep = ["\"", "\""] 
            absolute = False
            if "<" in line:
                sep = ["<", ">"]
                absolute = True
            
            start = line.find(sep[0]) + 1
            end = line.find(sep[1], start)
            name = line[start:end]
            if not name.endswith(".h"):
                continue
            if not absolute:
                name = parent + name
            name = clean_name(name)
            if name not in includes:
                includes.append(name)
    return includes
    
    


def process_includes(folders):
    includes = {}
    for proj in folders:
        for root, dirs, files in os.walk(proj):
            for file in files:
                if extension(file) not in {"h"}:
                    continue
                fullname = clean_name(root + "/" + file)
                
                includes[fullname] = process_file(fullname)
    return includes
    
    
def print_includes(file, includes, tabs, printed):
    total = 0
    if file in printed:
        # print("    " * tabs, "*", file)
        return total
        
    print("\t" * tabs + file)
    printed.add(file)
    
    try:
        for i in includes[file]:
            total += print_includes(i, includes, tabs + 1, printed)
    except:
        print("\t" * (tabs + 1) + "????")
        pass
        
    return total + 1



projects = {"yave"}
classes = {}
includes = process_includes(projects)

include_count = {}
for file in includes.keys():
    printed = set()
    include_count[file] = print_includes(file, includes, 0, printed)
    print()
    print()

    
include_count = [(i, c) for i, c in include_count.items()]
include_count.sort(key = lambda p : p[1], reverse = True)

for p in include_count:
    print(p[0], "=>", p[1]) 