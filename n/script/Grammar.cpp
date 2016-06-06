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


Grammar::Grammar(const core::String &n, GrammarType t, const core::Array<Element> &e) : name(n), type(t), elements(e) {
	if(type == Or) {
		for(Element e : elements) {
			if(e.grammar) {
				depends += e.grammar;
			} else {
				fixed += e.token;
			}
		}
	} else if(type == And) {
		if(elements.first().grammar) {
			depends += elements.first().grammar;
		} else {
			fixed += elements.first().token;
		}
	}

}

void Grammar::computeAllChildren(core::Set<const Grammar *> &all) const {
	if(!all.exists(this)) {
		all += this;
		for(Element e : elements) {
			if(e.grammar) {
				e.grammar->computeAllChildren(all);
			}
		}
	}
}

core::Set<const Grammar *> Grammar::getChildren() const {
	core::Set<const Grammar *> children;
	for(Element e : elements) {
		if(e.grammar) {
			children += e.grammar;
		}
	}
	return children;
}

void Grammar::computeChildrenFixed(core::Set<TokenType> &fix, core::Set<const Grammar *> &done) const {
	if(!done.exists(this)) {
		done += this;
		fix += fixed;
		for(const Grammar *g : depends) {
			g->computeChildrenFixed(fix, done);
		}
	}
}

CompiledGrammar *Grammar::compile() const {
	core::Set<const Grammar *> all;
	computeAllChildren(all);

	core::Map<const Grammar *, CompiledGrammar *> compiled;
	for(const Grammar *g : all) {
		compiled[g] = new CompiledGrammar(g->name);
	}

	for(auto p : compiled) {
		p._1->compile(p._2, compiled);
	}

	for(auto p : compiled) {
		for(uint i = 0; i != uint(TokenType::End) + 1; i++) {
			p._2->terminal &= p._2->nexts[i].isEmpty();
		}
	}
	return compiled[this];
}

void Grammar::compile(CompiledGrammar *comp, const core::Map<const Grammar *, CompiledGrammar *> &compiled) const {
	core::Set<const Grammar *> children = getChildren();
	for(const Grammar *c : children) {
		core::Set<TokenType> fix;
		core::Set<const Grammar *> done;
		c->computeChildrenFixed(fix, done);
		for(TokenType tk : fix) {
			comp->nexts[tk] += compiled.get(c, 0);
		}
	}
}


}
}
