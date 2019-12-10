import sys
import re
import os
import subprocess

work_dir = "./build/times/"
compiler_logs = open(work_dir + "times_logs.txt", "w")


def try_compile():
    p = subprocess.call(["g++", "-c", "main.cpp"], cwd=work_dir, stdout=compiler_logs, stderr=compiler_logs)
    return p == 0

def extension(file):
    return file.split(".")[-1]
    

def process_file(fullname):
    with open(work_dir + "main.cpp", "w") as f:
        f.write("#include <" + fullname + ">")
        f.write("""
        
        int main() {}
        """)
        if not try_compile():
            print("Unable to compile", fullname)
            quit()

                
                
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
                

projects = {"y", "yave", "editor"}

process_files(projects)