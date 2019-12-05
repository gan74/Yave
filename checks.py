import sys
import re
import os


def warn(file, msg):
	global warn_count
	#print(re.split(r"[/\\]", file)[-1], ":", msg)
	print(file, ":", msg)
	warn_count += 1

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
	if not content.strip().startswith("""/*******************************\nCopyright"""):
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
	
	
def check_rval_getter(content, filename):
	global try_fix
	is_getter = re.compile(r"(.*)& [\w+::]*\w+\(.*\) const\s*[;{]")
	for line in content.split("\n"):
		line = line.strip()
		getter = is_getter.match(line)
		if getter:
			warn(filename, "reference returning getter (" + line + ") should be const&")
			if try_fix:
				index = line.rfind("const")
				fixed = line[:index] + "const&" + line[index+5:]
				content = content.replace(line, fixed)
	return content
	
def check_default_moves(content, filename):
	is_move = re.compile(r"(\w+)\(\1&&.+\)\w*[;{]")
	# is_default = re.compile(r"(\w+)\(\1&&\) = default;")
	for line in content.split("\n"):
		line = line.strip()
		move = is_move.match(line)
		# if not move:
		# 	move = is_default.match(line)
			
		if move:
			dtor = "~" + move.group(1) + "()" 
			non_move = move.group(1) + " : NonMovable" 
			if (dtor not in content) and (non_move not in content):
				warn(filename, move.group(1) + " has an explicitly declared move ctor but no dtor")
	
	

#def check_value_types(content, filename):
#	is_func = re.compile(r"\w+ \w+\((.*)\)\s*[;{]")
#	for line in content.split("\n"):
#		line = line.strip()
#		func = is_func.match(line)
#		if func:
#			print(func.group(1))
#			args = [a.strip() for a in func.group(1).split(",")]
		

def process(content, filename):
	funcs = {check_license, check_guards, check_default_moves}
	for f in funcs:
		r = f(content, filename)
		if r is not None:
			content = r
	return content

	
try_fix = True
warn_count = 0
projects = {"yave", "y", "editor", "ecs"}

for proj in projects:
	for root, dirs, files in os.walk(proj):
		for file in files:
			if extension(file) not in {"cpp", "h"}:
				continue
			fullname = root + "/" + file
			
			with open(fullname, "r") as f:
				content = f.read()
				new_content = process(content, fullname)
				fix = try_fix and new_content != content
				
			if fix:
				print("fixing", fullname)
				with open(fullname, "w") as f:
					f.write(new_content)
print()
print(warn_count, "warnings")