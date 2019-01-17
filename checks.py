import sys
import re
import os

def warn(file, msg):
	#print(re.split(r"[/\\]", file)[-1], ":", msg)
	print(file, ":", msg)

def extension(file):
	return file.split(".")[-1]
	
def find_all(a_str, sub):
	start = 0
	while True:
		start = a_str.find(sub, start)
		if start == -1: 
			return
		yield start
		start += len(sub)
	
def check_license(content, filename):
	if not content.strip().startswith("""/*******************************"""):
		warn(filename, "is missing license")
		
def check_guards(content, filename):
	if extension(filename) != "h":
		return
		
	if "#pragma once" in content:
		return
		
	ifndefs = find_all(content, "#ifndef ")
	for start in ifndefs:
		end = content.find("\n", start)
		define = content[start+8:end].strip()
		if "#define " + define in content[end:]:
			return
	warn(filename, "is missing include guards")


def process(content, filename):
	check_license(content, filename)
	check_guards(content, filename)

	

projects = {"yave", "y", "editor"}

for proj in projects:
	for root, dirs, files in os.walk(proj):
		for file in files:
			if extension(file) not in {"cpp", "h"}:
				continue
			fullname = root + "/" + file
			with open(fullname, "r") as f:
				content = f.read()
				# content = content.replace("\\r\\n", "\\n")
				process(content, fullname)