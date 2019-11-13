import sys
import re
import os
import subprocess

compiler_logs = open("constifier_logs.txt", "w")
def try_compile():
    p = subprocess.call(["mingw32-make"], cwd = "./build/Release", stdout=compiler_logs, stderr=compiler_logs)
    return p == 0

def extension(file):
    return file.split(".")[-1]
    
    
def de_templatize(type):
    return type.split("<", 1)[0]
    
def in_types(type):
    if type[-1] == '*' or type[-1] == '&':
        type = type[:-1]
    return de_templatize(type) in types
    
    
def in_namespaces(type):
    type = de_templatize(type)
    if '(' in type:
        return False
    for ns in namespaces:
        if type.startswith(ns):
            return True
    return False

def is_prototype(line):
    line = line.strip()
    if "operator=" in line:
        return True
    if line.endswith(") {") or line.endswith(") const {") or line.endswith("();") or line.endswith(") const;"):
        return True
    if "virtual " in line or "override " in line:
        return True
    return False

def process_file(fullname):
    content = ""
    with open(fullname, "r") as f:
        content = f.read()
    
    comment = False
    for line in content.split("\n"):
        
        trimmed = line.lstrip()
        
        
        if trimmed.startswith("/*"):
            comment = True
        if "*/" in trimmed:
            comment = False
            
        if comment:
            continue
        
        if trimmed.startswith("if("):
            trimmed = trimmed[3:]
        else:
            if is_prototype(line):
                continue
        
        split = trimmed.split(" ")
        if len(split) < 2:
            continue
            
        if len(split[1]) == 0 or split[1][0] == '_':
            continue
            
        type_name = split[0]
        
        if in_types(type_name) or in_namespaces(type_name):
            print("   ", de_templatize(type_name))
            new_line = line[:len(line) - len(trimmed)] + "const " + trimmed
            
            new_content = content.replace(line, new_line)
            with open(fullname, "w") as f:
                f.write(new_content)
                
            if try_compile():
                content = new_content
                
    with open(fullname, "w") as f:
        f.write(content)

                
                
def process_files(folders):
    for proj in folders:
        for root, dirs, files in os.walk(proj):
            for file in files:
                if extension(file) not in {"h", "cpp"}:
                    continue
                fullname = root + "/" + file
                
                print("Starting", file)
                
                process_file(fullname)
                
                print(file, "Done")
                print()
                
    
def find_classes(folders):
    find_class = re.compile(r"class ([A-Za-z0-9_]+)")
    find_struct = re.compile(r"struct ([A-Za-z0-9_]+)")
    find_using = re.compile(r"using ([A-Za-z0-9_]+)")
    for proj in folders:
        for root, dirs, files in os.walk(proj):
            for file in files:
                if extension(file) not in {"h"}:
                    continue
                fullname = root + "/" + file
                with open(fullname, "r") as f:
                    content = f.read()
                cl = find_class.findall(content)
                st = find_struct.findall(content)
                us = find_using.findall(content)
                for c in cl:
                    types.add(c)
                for s in st:
                    types.add(s)
                for u in us:
                    types.add(u)


projects = {"y", "yave", "editor"}

namespaces = {"core::", "math::", "std::", "vk::", "detail::"}
types = {"float", "double", "int", "auto", "T", "U", "C", "R", "F", "It"}

find_classes(projects)


print("Initial compilation")
if not try_compile():
    print("Unable to compile base project")
    quit()
print("Ok")
print()


process_files(projects)