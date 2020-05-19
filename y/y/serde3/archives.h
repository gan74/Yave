/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef Y_SERDE3_ARCHIVES_H
#define Y_SERDE3_ARCHIVES_H

#include <y/core/Vector.h>

#include "traits.h"
#include "headers.h"
#include "conversions.h"
#include "property.h"

#include <y/io2/io.h>

#define Y_SERDE3_BUFFER

#ifdef Y_SERDE3_BUFFER
#include <y/io2/Buffer.h>
#endif

#define y_try_status(result)																	\
	do {																						\
		if(auto&& _y_try_result = (result); _y_try_result.is_error()) { 						\
			return std::move(_y_try_result.err_object());										\
		} else {																				\
			status = status | _y_try_result.unwrap();											\
		}																						\
	} while(false)


namespace y {
namespace serde3 {

namespace detail {

using size_type = u64;

static constexpr std::string_view version_string			= "serde3.v2.0";
static constexpr std::string_view collection_version_string = "serde3.col.v1.0";
static constexpr std::string_view tuple_version_string		= "serde3.tpl.v1.0";
static constexpr std::string_view ptr_version_string		= "serde3.ptr.v1.0";

static constexpr u16 magic = 0x7966;
static constexpr u16 version_id = 2;

}


class WritableArchive final {

	//using File = io2::WriterPtr;
	using File = io2::Writer;
	using size_type = detail::size_type;

	template<typename T>
	static constexpr bool need_size_patch = has_serde3_v<T>;

	static constexpr usize buffer_size = 64 * 1024;

	struct SizePatch {
		usize index = 0;
		detail::size_type size = 0;
	};

	public:
		WritableArchive(File& file) :
				_file(file),
#ifdef Y_SERDE3_BUFFER
				_buffer(buffer_size),
#endif
				_cached_file_size(_file.tell())
		{}

		WritableArchive(std::unique_ptr<File> file) : WritableArchive(*file) {
			_storage = std::move(file);
		}

		~WritableArchive() {
#ifdef Y_SERDE3_BUFFER
			y_debug_assert(!_buffer.tell());
#endif
			y_debug_assert(_patches.is_empty());
		}

		template<typename T>
		Result serialize(const T& t) {
			y_try(write_serde_header());
			y_try(serialize_one(NamedObject{t, detail::version_string}));
			return finalize();
		}

	private:
		Result write_serde_header() {
			y_try(write_one(detail::magic));
			return write_one(detail::version_id);
		}

		Result flush() {
			y_try(finalize_in_buffer());
#ifdef Y_SERDE3_BUFFER
			usize buffer_size = _buffer.tell();
			if(buffer_size) {
				if(!_file.write(_buffer.data(), buffer_size)) {
					return core::Err(Error(ErrorType::IOError));
				}
				_buffer.clear();
			}
			_cached_file_size = _file.tell();
#endif
			return core::Ok(Success::Full);
		}

		Result finalize_in_buffer() {
			y_debug_assert(_cached_file_size == _file.tell());
#ifdef Y_SERDE3_BUFFER
			while(!_patches.is_empty()) {
				const auto& p = _patches.last();
				if(p.index <= _cached_file_size) {
					break;
				}
				_buffer.seek(p.index - _cached_file_size);
				if(!_buffer.write_one(p.size)) {
					return core::Err(Error(ErrorType::IOError));
				}
				_patches.pop();
			}
			_buffer.seek_end();
#endif
			return core::Ok(Success::Full);
		}

		Result finalize() {
			y_try(flush());
			usize pos = _file.tell();
			for(const SizePatch& patch : _patches) {
				_file.seek(patch.index);
				if(!_file.write_one(patch.size)) {
					return core::Err(Error(ErrorType::IOError));
				}
			}
			_patches.make_empty();
			_file.seek(pos);

			return core::Ok(Success::Full);
		}

		template<typename T, bool R>
		Result serialize_one(const NamedObject<T, R>& non_const_object) {
			const auto object = non_const_object.make_const_ref();

			if constexpr(is_property_v<T>) {
				return serialize_property(object);
			} else if constexpr(has_serde3_v<T>) {
				return serialize_object(object);
			} else if constexpr(has_serde3_ptr_poly_v<T> || has_serde3_poly_v<T>) {
				return serialize_poly(object);
			} else if constexpr(is_tuple_v<T>) {
				return serialize_tuple(object);
			} else if constexpr(is_range_v<T>) {
				return serialize_range(object);
			} else if constexpr(is_pod_v<T>) {
				return serialize_pod(object);
			} else if constexpr(is_std_ptr_v<T>) {
				return serialize_ptr(object);
			} else {
				static_assert(is_iterable_v<T>, "Unable to serialize type");
				return serialize_collection(object);
			}
		}


		// ------------------------------- PROPERTY -------------------------------
		template<typename T, bool R>
		Result serialize_property(NamedObject<T, R> object) {
			return serialize_one(NamedObject<typename T::value_type, T::return_ref>{object.object.get(), object.name});
		}


		// ------------------------------- PTR -------------------------------
		template<typename T>
		Result serialize_ptr(NamedObject<T> object) {
			static_assert(std::is_const_v<T>);
			static_assert(is_std_ptr_v<T>);

			if(object.object == nullptr) {
				return write_one(u8(0));
			}

			{
				y_try(write_one(u8(1)));
				y_try(serialize_one(NamedObject{*object.object, detail::ptr_version_string}));
			}

			return core::Ok(Success::Full);
		}


		// ------------------------------- COLLECTION -------------------------------
		template<typename T>
		Result serialize_range(NamedObject<T> object) {
			return serialize_collection(object);
		}

		template<typename T, bool R>
		Result serialize_collection(const NamedObject<T, R>& object) {
			static_assert(std::is_const_v<T>);
			static_assert(is_iterable_v<T>);

			if constexpr(detail::use_collection_fast_path<remove_cvref_t<T>>) {
				y_try(write_one(size_type(object.object.size())));
				if(object.object.size()) {
					const auto header = detail::build_header(NamedObject{*object.object.begin(), detail::collection_version_string});
					y_try(write_one(header));
					y_try(write_array(object.object.begin(), object.object.size()));
				}
			} else {
				// Size is patched so we don't have to call .size() on funky objects (like ranges)
				SizePatch size_patch{tell(), 0};
				y_try(write_one(size_type(0)));
				for(const auto& item : object.object) {
					y_try(serialize_one(NamedObject{item, detail::collection_version_string}));
					++size_patch.size;
				}
				push_patch(size_patch);
			}

			return core::Ok(Success::Full);
		}


		// ------------------------------- POLY -------------------------------
		template<typename T>
		Result serialize_poly(NamedObject<T> object) {
			static_assert(std::is_const_v<T>);

			if constexpr(is_std_ptr_v<T>) {
				static_assert(has_serde3_ptr_poly_v<T>);

				if(object.object == nullptr) {
					return write_one(u8(0));
				}
				y_try(write_one(u8(1)));

				y_try(write_one(object.object->_y_serde3_poly_type_id()));
				return object.object->_y_serde3_poly_serialize(*this);
			} else {
				static_assert(has_serde3_poly_v<T>);
				static_assert(!has_serde3_v<T>);

				y_try(write_one(object.object._y_serde3_poly_type_id()));
				return object.object._y_serde3_poly_serialize(*this);
			}
		}


		// ------------------------------- POD -------------------------------
		template<typename T, bool R>
		Result serialize_pod(const NamedObject<T, R>& object) {
			static_assert(std::is_const_v<T>);
			static_assert(is_pod_v<T>);
			static_assert(!std::is_pointer_v<T>);

			{
				y_try(write_header(object));
				y_try(write_one(object.object));
			}

			return core::Ok(Success::Full);
		}


		// ------------------------------- OBJECT -------------------------------
		template<typename T, bool R>
		Result serialize_object(const NamedObject<T, R>& object) {
			static_assert(std::is_const_v<T>);
			static_assert(!has_serde3_ptr_poly_v<T>);

			{
				y_try(write_header(object));
				y_try(serialize_members(object.object));
			}

			return core::Ok(Success::Full);
		}

		template<typename T>
		Result serialize_members(const T& object) {
			return serialize_members_internal<0>(object._y_serde3_refl());
		}

		template<usize I, typename... Args, bool... Refs>
		Result serialize_members_internal(const std::tuple<NamedObject<Args, Refs>...>& objects) {
			unused(objects);
			if constexpr(I < sizeof...(Args)) {
				y_try(serialize_one(std::get<I>(objects)));
				y_try(serialize_members_internal<I + 1>(objects));
			}
			return core::Ok(Success::Full);
		}



		// ------------------------------- TUPLE -------------------------------
		template<typename T>
		Result serialize_tuple(const NamedObject<T>& object) {
			static_assert(std::is_const_v<T>);

			{
				y_try(write_header(object));
				y_try(serialize_tuple_members<0>(object.object));
			}

			return core::Ok(Success::Full);
		}

		template<usize I, typename Tpl>
		Result serialize_tuple_members(const Tpl& object) {
			unused(object);
			if constexpr(I < std::tuple_size_v<Tpl>) {
				y_try(serialize_one(NamedObject{std::get<I>(object), detail::tuple_version_string}));
				return serialize_tuple_members<I + 1>(object);
			}
			return core::Ok(Success::Full);
		}


		// ------------------------------- WRITE -------------------------------
		template<typename T, bool R>
		Result write_header(const NamedObject<T, R>& object) {
			const auto header = detail::build_header(object);
			return write_one(header);
		}

		template<typename T>
		Result write_one(const T& t) {
#ifdef Y_SERDE3_BUFFER
			if(_buffer.size() + sizeof(T) > buffer_size) {
				y_try(flush());
			}
			if(!_buffer.write_one(t)) {
				return core::Err(Error(ErrorType::IOError));
			}
#else
			_cached_file_size += sizeof(T);
			y_try_discard(_file.write_one(t));
#endif
			return core::Ok(Success::Full);
		}

		template<typename T>
		Result write_array(const T* t, usize size) {
#ifdef Y_SERDE3_BUFFER
			if(_buffer.size() + (sizeof(T) * size) > buffer_size) {
				if(!flush()) {
					return core::Err(Error(ErrorType::IOError));
				}
			}

			if(!_buffer.write_array(t, size)) {
				return core::Err(Error(ErrorType::IOError));
			}
#else
			_cached_file_size += sizeof(T) * size;
			y_try_discard(_file.write_array(t, size));
#endif
			return core::Ok(Success::Full);
		}

		usize tell() const {
#ifdef Y_SERDE3_BUFFER
			return _cached_file_size + _buffer.tell();
#else
			return _cached_file_size;
#endif
		}

		void push_patch(SizePatch patch) {
			y_debug_assert(patch.index + patch.size <= tell());
			y_debug_assert(patch.size < size_type(-1));
			_patches << patch;
		}

	private:
		File& _file;

#ifdef Y_SERDE3_BUFFER
		io2::Buffer _buffer;
#endif
		usize _cached_file_size = 0;
		core::Vector<SizePatch> _patches;

		std::unique_ptr<File> _storage;
};








class ReadableArchive final {

	//using File = io2::ReaderPtr;
	using File = io2::Reader;
	using size_type = detail::size_type;

	static constexpr bool force_safe = false;

	struct HeaderOffset {
		detail::ObjectHeader header;
		usize offset = 0;
	};

	struct ObjectData {
		core::Vector<HeaderOffset> members;
		usize end_offset = 0;
	};

	public:
		ReadableArchive(File& file) : _file(file) {
		}

		ReadableArchive(std::unique_ptr<File> file) : ReadableArchive(*file) {
			_storage = std::move(file);
		}

		template<typename T, typename... Args>
		Result deserialize(T& t, Args&&... args) {
			y_try(read_serde_header());
			auto res = deserialize_one(NamedObject{t, detail::version_string});
			if(res) {
				post_deserialize(t, y_fwd(args)...);
			}
			return res;
		}

		Y_TODO(post_deserialize might be called several time in some cases (poly mostly))
		template<typename T, typename... Args>
		static void post_deserialize(T& t, Args&&... args) {
			static_assert(!std::is_const_v<T>);
			post_deserialize_one(t, y_fwd(args)...);
		}


	private:
		Result read_serde_header() {
			u16 magic = 0;
			u16 version = 0;
			y_try(read_one(magic));
			y_try(read_one(version));
			if(magic != detail::magic || version != detail::version_id) {
				return core::Err(Error(ErrorType::VersionError));

			}
			return core::Ok(Success::Full);
		}


		template<typename T, bool R>
		Result deserialize_one(const NamedObject<T, R>& non_ref) {
			NamedObject<T> object = non_ref.make_ref();

			if constexpr(is_property_v<T>) {
				return deserialize_property(object);
			} else if constexpr(has_serde3_v<T>) {
				return deserialize_object(object);
			} else if constexpr(has_serde3_ptr_poly_v<T> || has_serde3_poly_v<T>) {
				return deserialize_poly(object);
			} else if constexpr(is_tuple_v<T>) {
				return deserialize_tuple(object);
			} else if constexpr(is_range_v<T>) {
				return deserialize_range(object);
			} else if constexpr(is_pod_v<T>) {
				return deserialize_pod(object);
			} else if constexpr(is_std_ptr_v<T>) {
				return deserialize_ptr(object);
			} else {
				static_assert(is_iterable_v<T>, "Unable to deserialize type");
				return deserialize_collection(object);
			}
		}



		// ------------------------------- PROPERTY -------------------------------
		template<typename T, bool R>
		Result deserialize_property(NamedObject<T, R> object) {
			using inner = typename T::value_type;
			inner i;
			y_try(deserialize_one(NamedObject<inner, true>{i, object.name}));
			object.object.set(std::move(i));
			return core::Ok(Success::Full);
		}



		// ------------------------------- PTR -------------------------------
		template<typename T>
		Result deserialize_ptr(NamedObject<T> object) {
			static_assert(is_std_ptr_v<T>);

			u8 non_null = 0;
			y_try(read_one(non_null));

			if(non_null) {
				object.object = make_std_ptr<T>();
				return deserialize_one(NamedObject{*object.object, detail::ptr_version_string});
			}

			object.object = nullptr;
			return core::Ok(Success::Full);
		}


		// ------------------------------- COLLECTION -------------------------------
		template<typename T>
		Result deserialize_range(NamedObject<T> object) {
			static_assert(!std::is_const_v<std::remove_reference_t<decltype(*object.object.begin())>>);
			return deserialize_collection<T, true>(object);
		}

		template<typename T, bool IsRange = false>
		Result deserialize_collection(NamedObject<T> object) {
			static_assert(is_iterable_v<T>);

			if constexpr(!IsRange) {
				if constexpr(has_make_empty_v<T>) {
					object.object.make_empty();
				} else if constexpr(has_clear_v<T>) {
					object.object.clear();
				} else {
					object.object = T();
				}
			}

			size_type collection_size = 0;
			y_try(read_one(collection_size));

			if constexpr(IsRange) {
				if(collection_size != object.object.size()) {
					return core::Err(Error(ErrorType::SizeError, object.name.data()));
				}
			}

			try {
				if constexpr(detail::use_collection_fast_path<T>) {
					if(collection_size) {
						y_try(check_header(NamedObject{*object.object.begin(), detail::collection_version_string}));
						if constexpr(!IsRange) {
							if constexpr(has_resize_v<T>) {
								object.object.resize(collection_size);
							} else {
								if constexpr(has_reserve_v<T>) {
									object.object.reserve(collection_size);
								}
								while(object.object.size() < collection_size) {
									object.object.emplace_back();
								}
							}
						}
						if(!_file.read_array(object.object.begin(), collection_size)) {
							return core::Err(Error(ErrorType::IOError, object.name.data()));
						}
					}

				} else {
					if constexpr(has_reserve_v<T>) {
						object.object.reserve(collection_size);
					}

					if constexpr(has_emplace_back_v<T>) {
						for(size_type i = 0; i != collection_size; ++i) {
							object.object.emplace_back();
							y_try(deserialize_one(NamedObject{object.object.last(), detail::collection_version_string}));
						}
					} else if constexpr(has_resize_v<T>) {
						object.object.resize(collection_size);
						for(size_type i = 0; i != collection_size; ++i) {
							y_try(deserialize_one(NamedObject{object.object[usize(i)], detail::collection_version_string}));
						}
					} else {
						if constexpr(is_array_v<T> || IsRange) {
							for(size_type i = 0; i != collection_size; ++i) {
								y_try(deserialize_one(NamedObject{object.object[i], detail::collection_version_string}));
							}
						} else {
							using value_type = deconst_t<typename T::value_type>;
							for(size_type i = 0; i != collection_size; ++i) {
								value_type value;
								y_try(deserialize_one(NamedObject{value, detail::collection_version_string}));
								object.object.insert(std::move(value));
							}
						}
					}
				}

				if constexpr(has_size_v<T>) {
					if(object.object.size() != collection_size) {
						return core::Err(Error(ErrorType::SizeError, object.name.data()));
					}
				}

				return core::Ok(Success::Full);

			} catch(std::bad_alloc&) {
				return core::Err(Error(ErrorType::SizeError, object.name.data()));
			}
		}

		// ------------------------------- POLY -------------------------------
		template<typename T>
		Result deserialize_poly(NamedObject<T> object) {
			if constexpr(is_std_ptr_v<T>) {
				static_assert(!has_serde3_v<T>);

				u8 non_null = 0;
				y_try(read_one(non_null));
				if(non_null) {
					TypeId type_id;
					y_try(read_one(type_id));

					using element_type = typename T::element_type;
					object.object = element_type::_y_serde3_poly_base.create_from_id(type_id);
					if(!object.object) {
						return core::Err(Error(ErrorType::UnknownPolyError, object.name.data()));
					}

					return object.object->_y_serde3_poly_deserialize(*this);
				} else {
					object.object = nullptr;
					return core::Ok(Success::Full);
				}
			} else {
				static_assert(has_serde3_poly_v<T>);
				static_assert(!has_serde3_v<T>);

				TypeId type_id;
				y_try(read_one(type_id));

				if(type_id != object.object._y_serde3_poly_type_id()) {
					return core::Err(Error(ErrorType::UnknownPolyError, object.name.data()));
				}

				return object.object._y_serde3_poly_deserialize(*this);
			}

		}


		// ------------------------------- POD -------------------------------
		template<typename T>
		Result deserialize_pod(NamedObject<T> object) {
			static_assert(is_pod_v<T>);
			static_assert(!std::is_pointer_v<T>);

			y_try(check_header(object));

			/*if(header != check) {
				static constexpr usize max_prim_size = 4 * sizeof(float);
				if(header.type.name_hash == check.type.name_hash && size <= max_prim_size) {
					std::array<u8, max_prim_size> buffer;
					y_try_discard(_file.read(buffer.data(), size));
					return try_convert<T>(object.object, header.type, buffer.data());
				} else {
					return core::Err();
				}
			} else*/ {
				return read_one(object.object);
			}
		}


		// ------------------------------- OBJECT -------------------------------
		template<typename T>
		Result deserialize_object(NamedObject<T> object) {
			static_assert(has_serde3_v<T>);
			static_assert(!has_serde3_ptr_poly_v<T>);

			detail::ObjectHeader header;
			y_try(read_header(header));
			const auto check = detail::build_header(object);

			if(header != check) {
				if(header.type.type_hash != check.type.type_hash) {
					return core::Err(Error(ErrorType::SignatureError, object.name.data()));
				}
				return deserialize_members<true>(object.object);
			} else {
				return deserialize_members<force_safe>(object.object);
			}
		}

		template<bool Safe, typename T>
		Result deserialize_members(T& object) {
			ObjectData data;
			return deserialize_members_internal<Safe, 0>(object._y_serde3_refl(), data);
		}

		template<bool Safe, usize I, typename... Args, bool... Refs>
		Result deserialize_members_internal(const std::tuple<NamedObject<Args, Refs>...>& members,
											const ObjectData& object_data) {
			unused(members, object_data);

			if constexpr(I < sizeof...(Args)) {
				if constexpr(Safe) {
					const auto& mem = std::get<I>(members);
					return core::Err(Error(ErrorType::SignatureError, mem.name.data()));
				}
				/*if constexpr(Safe) {
					bool found = false;
					const auto header = detail::build_header(std::get<I>(members));
					for(const auto& m : object_data.members) {
						if(m.header.type.name_hash == header.type.name_hash) {
							seek(m.offset);
							y_try_status(deserialize_one(std::get<I>(members)));
							found = true;
							break;
						}
					}
					if(!found) {
						status = Success::Partial;
					}
				}*/

				y_try(deserialize_one(std::get<I>(members)));
				return deserialize_members_internal<Safe, I + 1>(members, object_data);
			}

			return core::Ok(Success::Full);
		}


		// ------------------------------- TUPLE -------------------------------
		template<typename T>
		Result deserialize_tuple(NamedObject<T> object) {
			y_try(check_header(object));
			return deserialize_tuple_members<0>(object.object);
		}

		template<usize I, typename Tpl>
		Result deserialize_tuple_members(Tpl& object) {
			unused(object);
			if constexpr(I < std::tuple_size_v<Tpl>) {
				y_try(deserialize_one(NamedObject{std::get<I>(object), detail::tuple_version_string}));
				return deserialize_tuple_members<I + 1>(object);
			}
			return core::Ok(Success::Full);
		}


		// ------------------------------- POST -------------------------------
		template<typename T, typename... Args>
		static void post_deserialize_one(T& t, Args&&... args) {
			unused(t);

			if constexpr(has_serde3_post_deser_v<T>) {
				t.post_deserialize();
			}
			if constexpr(has_serde3_post_deser_v<T, Args...>) {
				t.post_deserialize(args...);
			}

			if constexpr(!has_serde3_v<T> && has_serde3_poly_v<T>) {
				if constexpr(has_serde3_post_deser_poly_v<T>) {
					t.post_deserialize_poly();
				}
				if constexpr(has_serde3_post_deser_poly_v<T, Args...>) {
					t.post_deserialize_poly(args...);
				}
			}


			if constexpr(is_property_v<T>) {
				post_deserialize_property(t, args...);
			} else if constexpr(has_serde3_v<T>) {
				auto elems = t._y_serde3_refl();
				post_deserialize_named_tuple<0>(elems, args...);
			} else if constexpr(is_tuple_v<T>) {
				post_deserialize_tuple<0>(t, args...);
			} else if constexpr(is_std_ptr_v<T> || std::is_pointer_v<T>) {
				if(t != nullptr) {
					post_deserialize_one(*t, args...);
				}
			} else if constexpr(is_iterable_v<T>) {
				post_deserialize_collection(t, args...);
			}
		}

		template<typename P, typename... Args>
		static void post_deserialize_property(P& prop, Args&&... args) {
			if constexpr(P::return_ref) {
				post_deserialize_one(prop.get(), args...);
			} else {
				auto t = prop.get();
				post_deserialize_one(t, args...);
			}
		}

		template<typename C, typename... Args>
		static void post_deserialize_collection(C& col, Args&&... args) {
			for(auto&& item : col) {
				post_deserialize_one(item, args...);
			}
		}

		template<usize I, typename Tpl, typename... Args>
		static void post_deserialize_tuple(Tpl& objects, Args&&... args) {
			if constexpr(I < std::tuple_size_v<Tpl>) {
				post_deserialize_one(std::get<I>(objects), args...);
				post_deserialize_tuple<I + 1>(objects, args...);
			}
		}

		template<usize I, typename Tpl, typename... Args>
		static void post_deserialize_named_tuple(Tpl& objects, Args&&... args) {
			if constexpr(I < std::tuple_size_v<Tpl>) {
				post_deserialize_one(std::get<I>(objects).object, args...);
				post_deserialize_named_tuple<I + 1>(objects, args...);
			}
		}


		// ------------------------------- READ -------------------------------

		template<typename T, bool R>
		Result check_header(const NamedObject<T, R>& object) {
			detail::ObjectHeader header;
			y_try(read_header(header));

			const auto check = detail::build_header(object);
			if(header != check) {
				return core::Err(Error(ErrorType::SignatureError, object.name.data()));
			}
			return core::Ok(Success::Full);
		}


		template<typename T>
		Result read_one(T& t) {
			static_assert(!std::is_same_v<std::remove_reference_t<T>, detail::ObjectHeader>);
			if(!_file.read_one(t)) {
				return core::Err(Error(ErrorType::IOError));
			}
			return core::Ok(Success::Full);
		}

		Result read_header(detail::ObjectHeader& header) {
			Y_TODO(set member name on error)
			if(!_file.read_one(header.type)){
				return core::Err(Error(ErrorType::IOError));
			}
			bool read_members = true;
#ifdef Y_SLIM_POD_HEADER
			read_members = header.type.has_serde();
#endif
			if(read_members) {
				if(!_file.read_one(header.members)) {
					return core::Err(Error(ErrorType::IOError));
				}
			}
			return core::Ok(Success::Full);
		}

		usize tell() const {
			return _file.tell();
		}

		void seek(usize offset) {
			_file.seek(offset);
		}

	private:
		File& _file;
		std::unique_ptr<File> _storage;
};

}
}

#undef y_try_status


#endif // Y_SERDE3_ARCHIVES_H
