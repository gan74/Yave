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
#ifndef N_SCRIPT_GRAMMAR_H
#define N_SCRIPT_GRAMMAR_H

#include "Tokenizer.h"
#include <n/core/Set.h>
#include <n/core/Map.h>

namespace n {
namespace script {

class CompiledGrammar;

class Grammar
{
	enum GrammarType
	{
		Or,
		And
	};

	struct Element
	{
		TokenType token;
		const Grammar *grammar;
	};




	template<typename... Args>
	static void build(core::Array<Element> &e, const Grammar *g, Args... args) {
		e << Element{Error, g};
		build(e, args...);
	}

	template<typename... Args>
	static void build(core::Array<Element> &e, TokenType t, Args... args) {
		e << Element{t, 0};
		build(e, args...);
	}

	static void build(core::Array<Element> &) {
	}

	public:
		template<typename T, typename... Args>
		static Grammar expect(const core::String &name, T t, Args... args) {
			core::Array<Element> e;
			build(e, t, args...);
			return Grammar(name, And, e);
		}

		template<typename T, typename... Args>
		static Grammar any(const core::String &name, T t, Args... args) {
			core::Array<Element> e;
			build(e, t, args...);
			return Grammar(name, Or, e);
		}


		Grammar() : type(Or) {
		}

		CompiledGrammar *compile() const;

	private:
		Grammar(const core::String &n, GrammarType t, const core::Array<Element> &e);

		void computeChildrenFixed(core::Set<TokenType> &fix, core::Set<const Grammar *> &done) const;
		void computeAllChildren(core::Set<const Grammar *> &all) const;
		core::Set<const Grammar *> getChildren() const;

		void compile(CompiledGrammar *comp, const core::Map<const Grammar *, CompiledGrammar *> &compiled) const;

		core::String name;
		GrammarType type;

		core::Set<TokenType> fixed;
		core::Array<const Grammar *> depends;
		core::Array<Element> elements;
};

}
}

#endif // N_SCRIPT_GRAMMAR_H
