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
#include "Grammar.h"
#include "CompiledGrammar.h"


namespace n {
namespace script {

core::Set<const Grammar *> Grammar::computeNexts(TokenType type) const {
	core::Map<const Grammar *, core::Set<const Grammar *>> done;
	return computeNexts(done, type);
}

core::Set<const Grammar *> Grammar::computeNexts(core::Map<const Grammar *, core::Set<const Grammar *>> &done, TokenType tk) const {
	auto it = done.find(this);
	if(it != done.end()) {
		return it->_2;
	}

	core::Set<const Grammar *> c;
	done[this] = {};
	switch(type) {
		case Or:
			for(Element e : expecteds) {
				if(!e.grammar && e.token == tk) {
					c += this;
				}
			}
			done[this] = c;
			for(Element e : expecteds) {
				if(e.grammar) {
					auto gc = e.grammar->computeNexts(done, tk);
					if(!gc.isEmpty()) {
						c += gc;
						c += e.grammar;
					}
				}
			}
		break;

		case And:
			if(!expecteds.isEmpty()) {
				Element e = expecteds.first();
				if(e.grammar) {
					auto gc = e.grammar->computeNexts(done, tk);
					if(!gc.isEmpty()) {
						c += gc;
						c += e.grammar;
					}
				} else if(e.token == tk) {
					c += this;
				}
			}
		break;
	}

	done[this] = c;
	return c;
}

CompiledGrammar *Grammar::compile() const {
	core::Map<const Grammar *, CompiledGrammar *> c;
	return compile(c);
}


CompiledGrammar *Grammar::compile(core::Map<const Grammar *, CompiledGrammar *> &compiled) const {
	auto it = compiled.find(this);
	if(it != compiled.end()) {
		return it->_2;
	}

	CompiledGrammar *c = new CompiledGrammar();
	compiled[this] = c;

	for(uint i = 0; i != uint(TokenType::End) + 1; i++) {
		c->nexts[i] = computeNexts(TokenType(i)).mapped([&](const Grammar *g) {
			return g->compile(compiled);
		});
		/*for(const Grammar *g : nexts[i]) {
			c->nexts[i] += g->compile(compiled)->nexts[i];
		}*/
	}

	if(type == And && !expecteds.isEmpty()) {
		core::Array<Element> nextElem(expecteds.begin() + 1, expecteds.end());
		Grammar next(And, nextElem);
		for(uint i = 0; i != uint(TokenType::End) + 1; i++) {
			c->nexts[i] += next.computeNexts(TokenType(i)).mapped([&](const Grammar *g) {
				return g->compile(compiled);
			});
		}
	}


	return c;
}

}
}
