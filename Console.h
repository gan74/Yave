/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef CONSOLE
#define CONSOLE

#include <n/concurent/Thread.h>
#include <n/core/Map.h>
#include <n/core/Functor.h>
#include <n/core/String.h>

#include <iostream>


using namespace n;
using namespace n::graphics;
using namespace n::math;
using namespace n::core;

class Console : public n::concurent::Thread
{
	public:
		Console() {
			funcs["exit"] = [&](String s) -> String {
				one(s);
				exit(0);
				return "";
			};

			funcs["load"] = [&](String s) -> String {
				load(s);
				return "";
			};

			funcs["save"] = [&](String s) -> String {
				save(s);
				return "";
			};

			load();
		}

		~Console() {
			save();
		}


		virtual void run() override {
			while(true) {
				std::cout<<std::endl<<">>>";
				std::string line;
				std::getline(std::cin, line);
				String cmd = String(line);
				while(!cmd.isEmpty()) {
					cmd = one(cmd);
					if(isNum(cmd)) {
						std::cout<<cmd<<std::endl;
						break;
					}
				}
			}
		}

		String operator()(const String &i) const {
			auto it = vars.find(i);
			if(it == vars.end()) {
				return "";
			}
			return (*it)._2;
		}

	private:
		String one(const String &line) {
			String cmd = removeSpaces(line);
			if(cmd.isEmpty()) {
				return "";
			}
			if(isNum(cmd)) {
				return cmd;
			}
			if(cmd.beginWith("$")) {
				uint i = cmd.find("=");
				if(i == -1u) {
					if(AsCollection(cmd).findOne([] (char c) { return isspace(c); }) != cmd.end()) {
						std::cerr<<"Invalid variable name \""<<cmd<<"\""<<std::endl;
						return "";
					} else {
						return vars[cmd];
					}
				} else {
					String name = removeSpaces(cmd.subString(0, i));
					return vars[name] = one(cmd.subString(i + 1));
				}
			} else {
				uint i = cmd.find(" ");
				String s = removeSpaces(cmd.subString(0, i));
				auto it = funcs.find(s);
				if(it == funcs.end()) {
					std::cerr<<"Unknown command \""<<s<<"\""<<std::endl;
					return "";
				} else {
					return (*it)._2(removeSpaces(cmd.subString(s.size())));
				}

			}
			std::cerr<<"Unknown error \""<<line<<"\""<<std::endl;
			return "";
		}

		static bool isNum(const String &s) {
			bool num = true;
			s.to<float>([&] { num = false; });
			return num;
		}

		static String removeSpaces(String s) {
			while(!s.isEmpty() && isspace(s[0])) {
				s = s.subString(1);
			}
			while(!s.isEmpty() && isspace(s[s.size() - 1])) {
				s = s.subString(0, s.size() - 1);
			}
			return s;
		}

		static constexpr auto cfg = "vars.cfg";


		bool load(const String &f = "") {
			io::File file(f.isEmpty() ? cfg : f);
			if(file.open(io::IODevice::Read || io::IODevice::Binary)) {
				char *data = new char[file.size() + 1];
				data[file.readBytes(data)] = 0;
				String all(data);
				for(const String &line : all.split("\n")) {
					one(line);
				}
				delete[] data;
				file.close();
				return true;
			}
			std::cerr<<"Unable to load config from \""<<(f.isEmpty() ? cfg : f)<<"\""<<std::endl;
			return false;
		}

		void save(const String &f = "") {
			io::File file(f.isEmpty() ? cfg : f);
			if(file.open(io::IODevice::Write)) {
				for(const Pair<const String, String> &p : vars) {
					file.write(p._1 + "=" + p._2 + "\n");
				}
				file.close();
			}
		}


		Map<String, Functor<String(String)>> funcs;
		Map<String, String> vars;
};

#endif // CONSOLE

