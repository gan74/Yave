import sys
import re
import os
import subprocess


def extension(file):
    return file.split(".")[-1]
    
def process_file(fullname):
    content = ""
    with open(fullname, "r") as f:
        content = f.read()
    
    new_content = ""
    for line in content.replace("\n\r", "\n").replace("\r\n", "\n").split("\n"):
        new_line = ""
        for c in line:
            if c != '\t':
                new_line += c
                continue
            tab_len = 4 - (len(new_line) % 4)
            for i in range(0, tab_len):
                new_line += " "
        
        new_content += new_line + "\n"
    new_content = new_content.strip() + "\n\n"
    if new_content != content:
        with open(fullname, "w") as f:
            f.write(new_content)

                
def process_files(folders):
    for proj in folders:
        for root, dirs, files in os.walk(proj):
            for file in files:
                if extension(file) not in {"h", "cpp", "inl", "glsl", "frag", "vert", "geom", "comp"}:
                    continue
                fullname = root + "/" + file
                
                print("Starting", file)
                
                process_file(fullname)
                
                print(file, "Done")
                print()
                
    

projects = {"y", "yave", "editor", "shaders"}
process_files(projects)