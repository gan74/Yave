import sys
import re
import os
import subprocess


def try_compile():
	p = subprocess.call(["mingw32-make", "-j6"], cwd = "build/debug")
	return p == 0

def extension(file):
	return file.split(".")[-1]
	
	
def try_cpp(filename, line):
	if not filename.endswith(".h"):
		return False
	fullname = filename[:-2] + ".cpp"
	if not os.path.isfile(fullname):
		return False
	with open(fullname, "r") as f:
		content = f.read()
	with open(fullname, "w") as f:
		f.write(line + "\n" + content)
	if try_compile():
		return True
	with open(fullname, "w") as f:
		f.write(content)
	return False
	
def process_file(fullname):
	content = ""
	with open(fullname, "r") as f:
		content = f.read()
	
	run = True
	while run:
		run = False
		for line in content.split("\n"):
			if line.startswith("#include "):
				sep = ["\"", "\""] 
				if "<" in line:
					sep = ["<", ">"]
				
				start = line.find(sep[0]) + 1
				end = line.find(sep[1], start)
				name = line[start:end]
				if name.endswith(".h"):
					name = line[start:end-2]
				slash = name.rfind("/")
				if slash >= 0:
					name = name[slash+1:]
					
				if name not in classes:
					print(name, "is unknown")
					continue
				
				new_line = "namespace " + classes[name] + " { class " + name + "; }"
				print(line, "=>", new_line)
				new_content = content.replace(line, new_line)
				
				with open(fullname, "w") as f:
					f.write(new_content)
				
				
				if try_compile() or try_cpp(fullname, line):
					print("Success")
					content = new_content
					run = True
					break
				else:
					print("Failed")
					
	
	with open(fullname, "w") as f:
		f.write(content)
	

	
	
	


#find classes
def find_classes(folders):
	find_class = re.compile(r"class ([A-Za-z]+)")
	find_struct = re.compile(r"struct ([A-Za-z]+)")
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
				for c in cl:
					classes[c] = proj
				for s in st:
					classes[s] = proj
				
				
#remove includes
def process_includes(folders):
	for proj in folders:
		for root, dirs, files in os.walk(proj):
			for file in files:
				if extension(file) not in {"h"}:
					continue
				fullname = root + "/" + file
				
				print("starting", file)
				process_file(fullname)
				print(file, "done")
				
	
	
projects = {"yave", "editor"}
classes = {}
find_classes(projects)
process_includes(projects)