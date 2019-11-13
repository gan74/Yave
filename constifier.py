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
    
    

def process_file(fullname):
    content = ""
    with open(fullname, "r") as f:
        content = f.read()
        
    run = True
    while run:
        run = False
        for line in content.split("\n"):
            trimmed = line.lstrip()
            split = trimmed.split(" ", 2)
            if len(split) != 2:
                continue
                
            if split[1][0] == '_':
                continue
                
            type_name = split[0]
            if type_name[-1] == '*' or type_name[-1] == '&':
                type_name = type_name[:-1]
            if type_name in types:
                new_line = line[:len(line) - len(trimmed)] + "const " + trimmed
                
                new_content = content.replace(line, new_line)
                with open(fullname, "w") as f:
                    f.write(new_content)
                    
                if try_compile():
                    content = new_content
                    run = True
                    break
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
    find_class = re.compile(r"class ([A-Za-z]+)")
    find_struct = re.compile(r"struct ([A-Za-z]+)")
    find_using = re.compile(r"using ([A-Za-z]+)")
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




projects = {"y"} #{"yave", "editor"}

types = {"float", "usize", "isize", "T", "U", "C", "R", "auto", "u32", "math::Vec2", "math::Vec3", "math::Vec3", "math::Vec4", "math::Matrix2<>", "math::Matrix3<>", "math::Matrix4<>", "math::Quaternion<>"}
find_classes(projects)

print("Initial compilation")
if not try_compile():
    print("Unable to compile base project")
    quit()
print("Ok")
print()


process_files(projects)