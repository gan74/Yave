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
		static Grammar expect(T t, Args... args) {
			core::Array<Element> e;
			build(e, t, args...);
			return Grammar(And, e);
		}

		template<typename T, typename... Args>
		static Grammar any(T t, Args... args) {
			core::Array<Element> e;
			build(e, t, args...);
			return Grammar(Or, e);
		}

		CompiledGrammar *compile() const;

	private:
		Grammar(GrammarType t, const core::Array<Element> &e) : type(t), expecteds(e) {
		}

		CompiledGrammar *compile(core::Map<const Grammar *, CompiledGrammar *> &compiled) const;
		core::Set<const Grammar *> computeNexts(TokenType type) const;
		core::Set<const Grammar *> computeNexts(core::Map<const Grammar *, core::Set<const Grammar *> > &done, TokenType tk) const;

		GrammarType type;
		core::Array<Element> expecteds;
};

}
}

#endif // N_SCRIPT_GRAMMAR_H
