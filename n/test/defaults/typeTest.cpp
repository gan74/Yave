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

#include <n/types.h>
#include <n/core/Map.h>
#include <n/core/String.h>
#include <n/core/Collection.h>
#include <n/core/Array.h>
#include <n/core/Set.h>
#include <n/core/List.h>
#include <n/test/TestTemplate.h>

namespace n {
namespace test {

N_GEN_TYPE_HAS_METHOD(HasBeg, begin)
N_GEN_TYPE_HAS_METHOD(HasEnd, end)
N_GEN_TYPE_HAS_METHOD(HasCBeg, cbegin)
N_GEN_TYPE_HAS_METHOD(HascEnd, cend)

template<typename T>
struct TestPrimitive
{
	using TI = TypeInfo<T>;
	static_assert(TI::isPrimitive, "TypeInfo test failure");
	static_assert(TypeInfo<T *>::isPrimitive && TypeInfo<T *>::isPointer, "TypeInfo test failure");
	static_assert(TypeInfo<T &>::isPrimitive && TypeInfo<T &>::isRef && TypeInfo<T &>::isPointer == TI::isPointer, "TypeInfo test failure");
	static_assert(TI::isPod && TypeInfo<T &>::isPod && TypeInfo<T &>::isPod, "TypeInfo test failure");
	static_assert(TypeInfo<const T *>::isConst, "TypeInfo test failure");
	static_assert(TypeInfo<const T &>::isConst, "TypeInfo test failure");
	static_assert(TypeInfo<const T>::isConst, "TypeInfo test failure");

	static constexpr bool isSimple = !(TI::isPointer || TI::isRef);
	static_assert(!isSimple || std::is_same<typename TypeInfo<T *>::decayed, T>::value, "TypeInfo test failure");
	static_assert(!isSimple || std::is_same<typename TypeInfo<const T *>::decayed, T>::value, "TypeInfo test failure");
	static_assert(!isSimple || std::is_same<typename TypeInfo<const T &>::decayed, T>::value, "TypeInfo test failure");

};

template<typename T, typename E>
struct TestCollection
{
	using TI = TypeInfo<T>;
	using C = core::Collection<T>;
	using Cof = typename C::template isCollectionOf<E>;

	static_assert(C::isCollection, "TypeInfo test failure");
	static_assert(Cof::value, "TypeInfo test failure");
	static_assert(!TI::isPrimitive, "TypeInfo test failure");
	static_assert(TI::isIterable, "TypeInfo test failure");
	static_assert(std::is_same<E, typename TypeInfo<typename C::Element>::nonRef>::value, "TypeInfo test failure");

	static_assert(HasBeg<T, typename C::iterator>::value, "TypeInfo test failure");
	static_assert(HasEnd<T, typename C::iterator>::value, "TypeInfo test failure");
	static_assert(HasBeg<const T, typename C::const_iterator>::value, "TypeInfo test failure");
	static_assert(HasEnd<const T, typename C::const_iterator>::value, "TypeInfo test failure");

	static_assert(HasCBeg<T, typename C::const_iterator>::value, "TypeInfo test failure");
	static_assert(HascEnd<T, typename C::const_iterator>::value, "TypeInfo test failure");


	static_assert(core::ShouldInsertAsCollection<T, E>::isCollectionOfCompatibles, "TypeInfo test failure");
	static_assert(core::ShouldInsertAsCollection<T, E>::value, "TypeInfo test failure");
};


class TypeTest : public TestTemplate<TypeTest>
{
	public:
		virtual bool run() override {
			TestPrimitive<int> i;
			TestPrimitive<int*> ip;

			TestCollection<core::Map<int, Nothing>, core::Pair<const int, Nothing>> m;
			TestCollection<core::Array<Nothing>, Nothing> a;
			TestCollection<core::String, char> s;
			TestCollection<core::List<Nothing>, Nothing> l;
			TestCollection<core::Set<Nothing>, const Nothing> st;

			unused(i, ip, m, a, s, l, st);
			return true;
		}
};

}
}
