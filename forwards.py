import sys
import re
import os
import codecs;

license = """/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
"""

def extension(file):
    return file.split(".")[-1]
                
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
                
    
def find_classes(folder):
    types = set()
    find_class = re.compile(r"(.*)[\n\r]+class ([A-Za-z0-9_]+).*{")
    find_struct = re.compile(r"(.*)[\n\r]+struct ([A-Za-z0-9_]+).*{")
    for root, dirs, files in os.walk(folder):
        for file in files:
            if extension(file) not in {"h"}:
                continue
            fullname = root + "/" + file
            with open(fullname, "r") as f:
                content = f.read()
            cl = find_class.findall(content) + find_struct.findall(content)
            for c in cl:
                if "template" in c[0]:
                    continue
                # if c[1] not in file:
                #     continue
                types.add(c[1])
    return list(types)
                
def process_project(projects):
    for proj in projects:
        types = find_classes(proj);
        types.sort()
        
        with codecs.open(proj + "/utils/forward.h", "w", "utf-8") as f:
            f.write(license)
            guard = proj.upper() + "_UTILS_FORWARD_H"
            f.write("#ifndef " + guard + "\n")
            f.write("#define " + guard + "\n")
            f.write("\n// Auto generated: forward definitions for most non template classes")
            f.write("\n// TODO: Classes nested in namespaces or other classes will not be declared correctly")
            f.write("\nnamespace " + proj + " {\n")
            
            for t in types:
                f.write("class " + t + ";\n")
            f.write("}\n\n#endif // " + guard)
 

projects = {"yave", "editor"}
process_project(projects)