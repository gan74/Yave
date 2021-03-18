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

def remove_comments(content):
    find_comments = re.compile(r"\/\/[^\n\r]+?(?:\*\)|[\n\r])")
    find_ml_comments = re.compile(r"/\*.*?\*/", re.MULTILINE | re.DOTALL)
    return find_comments.sub("", find_ml_comments.sub("", content))
                
def find_classes(content):
    find_class = re.compile(r"(.*)\s+class ([A-Za-z0-9_]+).*{")
    find_struct = re.compile(r"(.*)\s+struct ([A-Za-z0-9_]+).*{")
    types = set()
    for c in find_class.findall(content):
        if "template" in c[0] or "enum" in c[0]:
            continue
        types.add("class " + c[1])
    for s in find_struct.findall(content):
        if "template" in s[0]:
            continue
        types.add("struct " + s[1])
    return types;

def merge_dicts(a, b):
    for (n, c) in b.items():
        if n in a:
            a[n] += c
        else:
            a[n] = c

def find_namespaces(content, current_name):
    namespaces = {current_name: "\n"}
    namespace_index = 0
    while True:
        next_namespace_index = content.find("namespace ", namespace_index)
        if next_namespace_index >= 0:
            namespaces[current_name] += "\n" + content[namespace_index:next_namespace_index].strip()
            namespace_index = next_namespace_index
            if content[:namespace_index].endswith("using "):
                namespace_index += 10
            else:
                begin = content.find("{", namespace_index) + 1
                name = content[namespace_index + 10 : begin - 1].strip()
                end = begin
                braces = 1
                for c in content[begin:]:
                    if c == '{':
                        braces += 1
                    elif c == '}':
                        braces -= 1
                    if braces == 0:
                        break
                    end += 1
                    
                merge_dicts(namespaces, find_namespaces(content[begin:end].strip(), current_name + "::" + name))
                namespace_index = end + 1
        else:
            namespaces[current_name] += "\n" + content[namespace_index:].strip()
            break
    return namespaces
    
    
def find_namespaces_in_folder(folder):
    namespaces = {}
    for root, dirs, files in os.walk(folder):
        for file in files:
            if extension(file) not in {"h"}:
                continue
            fullname = root + "/" + file
            with open(fullname, "r") as f:
                content = f.read()
                content = remove_comments(content)
                merge_dicts(namespaces, find_namespaces(content, ""))
    return namespaces
    
def extension(file):
    return file.split(".")[-1]

                
def process_project(projects):
    for proj in projects:
        forward_file_name = proj + "/utils/forward.h";
        os.remove(forward_file_name)
    
        namespaces = find_namespaces_in_folder(proj);
        cl_per_ns = {}
        for (n, c) in namespaces.items():
            if len(n) == 0:
                continue
            cc = find_classes(c)
            if n in cl_per_ns:
                cl_per_ns[n] += cc
            else:
                cl_per_ns[n] = cc
        
        
        with codecs.open(forward_file_name, "w", "utf-8") as f:
            f.write(license)
            guard = proj.upper() + "_UTILS_FORWARD_H"
            f.write("#ifndef " + guard + "\n")
            f.write("#define " + guard + "\n")
            f.write("\n// Auto generated: forward definitions for most non template classes")
            f.write("\n// TODO: Nested classes will not be declared correctly")
            
            for (namespace, classes) in cl_per_ns.items():
                if len(classes) == 0:
                    continue
                if namespace[0:2] == "::":
                    namespace = namespace[2:]
                if namespace == "std" or len(namespace) == 0:
                    continue
                    
                f.write("\n\nnamespace " + namespace + " {\n");
                for cl in classes:
                    f.write(cl + ";\n")
                f.write("}\n")
            
            f.write("\n\n#endif // " + guard)
 

projects = {"yave", "editor"}
process_project(projects)