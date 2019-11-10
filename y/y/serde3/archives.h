/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include "headers.h"
#include "conversions.h"

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

static constexpr std::string_view version_string = "serde3.v1.0";
static constexpr std::string_view collection_version_string = "serde3.col.v1.0";

}


class WritableArchive final {

	using File = io2::WriterPtr;
	using size_type = detail::size_type;

	template<typename T>
	static constexpr bool need_size_patch = has_serde3_v<T>;

	static constexpr usize buffer_size = 64 * 1024;

	struct SizePatch {
		usize index = 0;
		detail::size_type size = 0;
	};

	public:
		WritableArchive(File file) :
				_file(std::move(file)),
#ifdef Y_SERDE3_BUFFER
				_buffer(buffer_size),
#endif
				_cached_file_size(_file->tell())
		{}

		template<typename F, typename = std::enable_if_t<std::is_base_of_v<io2::Writer, remove_cvref_t<F>>>>
		explicit WritableArchive(F&& file) : WritableArchive(std::make_unique<remove_cvref_t<F>>(y_fwd(file))) {
		}

		~WritableArchive() {
#ifdef Y_SERDE3_BUFFER
			y_debug_assert(!_buffer.tell());
#endif
			y_debug_assert(_patches.is_empty());
		}

		template<typename T>
		Result serialize(const T& t) {
			y_try(serialize_one(NamedObject{t, detail::version_string}));
			return finalize();
		}

	private:
		Result flush() {
			y_try(finalize_in_buffer());
#ifdef Y_SERDE3_BUFFER
			usize buffer_size = _buffer.tell();
			if(buffer_size) {
				y_try_discard(_file->write(_buffer.data(), buffer_size));
				_buffer.clear();
			}
			_cached_file_size = _file->tell();
#endif
			return core::Ok(Success::Full);
		}

		Result finalize_in_buffer() {
			y_debug_assert(_cached_file_size == _file->tell());
#ifdef Y_SERDE3_BUFFER
			while(!_patches.is_empty()) {
				const auto& p = _patches.last();
				if(p.index <= _cached_file_size) {
					break;
				}
				_buffer.seek(p.index - _cached_file_size);
				y_try_discard(_buffer.write_one(p.size));
				_patches.pop();
			}
			_buffer.seek_end();
#endif
			return core::Ok(Success::Full);
		}

		Result finalize() {
			y_try(flush());
			usize pos = _file->tell();
			for(const SizePatch& patch : _patches) {
				_file->seek(patch.index);
				y_try_discard(_file->write_one(patch.size));
			}
			_patches.make_empty();
			_file->seek(pos);

			return core::Ok(Success::Full);
		}

		template<typename T>
		Result serialize_one(NamedObject<T> object) {
			auto header = detail::build_header(object);
			y_try(write_one(header));

			if constexpr(has_serde3_poly_v<T>) {
				return serialize_poly(object);
			} else if constexpr(has_serde3_v<T>) {
				return serialize_object(object);
			} else if constexpr(std::is_trivially_copyable_v<T>) {
				return serialize_pod(object);
			} else {
				return serialize_collection(object);
			}
		}


		// ------------------------------- COLLECTION -------------------------------
		template<typename T>
		Result serialize_collection(NamedObject<T> object) {
			static_assert(is_iterable_v<T>);

			SizePatch patch{tell(), 0};

			{
				y_try(write_one(size_type(-1)));
				y_try(write_one(size_type(object.object.size())));
				for(const auto& item : object.object) {
					y_try(serialize_one(NamedObject{item, detail::collection_version_string}));
				}
			}

			patch.size = (size_type(tell()) - size_type(patch.index)) - sizeof(size_type);
			push_patch(patch);

			return core::Ok(Success::Full);
		}


		// ------------------------------- POLY -------------------------------
		template<typename T>
		Result serialize_poly(NamedObject<T> object) {
			static_assert(!has_serde3_v<T>);

			if(object.object == nullptr) {
				return write_one(size_type(0));
			}

			SizePatch patch{tell(), 0};

			{
				y_try(write_one(size_type(-1)));
				y_try(object.object->_y_serde3_poly_serialize(*this));
			}

			patch.size = (size_type(tell()) - size_type(patch.index)) - sizeof(size_type);
			// make sure size isn't 0 for non null
			if(!patch.size) {
				y_try(write_one(u8(0)));
				++patch.size;
			}
			push_patch(patch);

			return core::Ok(Success::Full);
		}


		// ------------------------------- POD -------------------------------
		template<typename T>
		Result serialize_pod(NamedObject<T> object) {
			static_assert(std::is_trivially_copyable_v<T>);
			static_assert(!std::is_pointer_v<T>);

			y_try(write_one(size_type(sizeof(T))));
			return write_one(object.object);
		}


		// ------------------------------- OBJECT -------------------------------
		template<typename T>
		Result serialize_object(NamedObject<T> object) {
			static_assert(!has_serde3_poly_v<T>);

			SizePatch patch{tell(), 0};

			{
				y_try(write_one(size_type(-1)));
				y_try(serialize_members(object.object));
			}

			patch.size = (size_type(tell()) - size_type(patch.index)) - sizeof(size_type);
			push_patch(patch);

			return core::Ok(Success::Full);
		}

		template<usize I, typename... Args>
		Result serialize_members_internal(std::tuple<NamedObject<Args>...> objects) {
			unused(objects);
			if constexpr(I < sizeof...(Args)) {
				y_try(serialize_one(std::get<I>(objects)));
				y_try(serialize_members_internal<I + 1>(objects));
			}
			return core::Ok(Success::Full);
		}

		template<typename T>
		Result serialize_members(const T& object) {
			return serialize_members_internal<0>(object._y_serde3_refl());
		}


		// ------------------------------- WRITE -------------------------------
		template<typename T>
		Result write_one(const T& t) {
#ifdef Y_SERDE3_BUFFER
			if(_buffer.size() + sizeof(T) > buffer_size) {
				y_try(flush());
			}
			y_try_discard(_buffer.write_one(t));
#else
			_cached_file_size += sizeof(T);
			y_try_discard(_file->write_one(t));
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
			_patches << patch;
		}

	private:
		File _file;

#ifdef Y_SERDE3_BUFFER
		io2::Buffer _buffer;
#endif
		usize _cached_file_size = 0;
		core::Vector<SizePatch> _patches;
};



class ReadableArchive final {

	using File = io2::ReaderPtr;
	using size_type = detail::size_type;

	static constexpr bool force_safe = false;

	struct HeaderOffset {
		detail::FullHeader header;
		usize offset = 0;
	};

	struct ObjectData {
		core::Vector<HeaderOffset> members;
		usize end_offset = 0;
	};

	public:
		ReadableArchive(File file) : _file(std::move(file)) {
		}

		template<typename F, typename = std::enable_if_t<std::is_base_of_v<io2::Reader, remove_cvref_t<F>>>>
		explicit ReadableArchive(F&& file) : ReadableArchive(std::make_unique<remove_cvref_t<F>>(y_fwd(file))) {
		}

		template<typename T>
		Result deserialize(T& t) {
			return deserialize_one(NamedObject{t, detail::version_string});
		}

	private:
		template<typename T>
		Result deserialize_one(NamedObject<T> object) {
			detail::FullHeader header;
			size_type size = size_type(-1);

			y_try_discard(read_header(header));
			y_try_discard(_file->read_one(size));

			if constexpr(has_serde3_poly_v<T>) {
				return deserialize_poly(object, header, size);
			} else if constexpr(has_serde3_v<T>) {
				return deserialize_object(object, header, size);
			} else if constexpr(std::is_trivially_copyable_v<T>) {
				return deserialize_pod(object, header, size);
			} else {
				return deserialize_collection(object, header, size);
			}
		}


		// ------------------------------- COLLECTION -------------------------------
		template<typename T>
		Result deserialize_collection(NamedObject<T> object, const detail::FullHeader header, size_type size) {
			static_assert(is_iterable_v<T>);

			object.object.clear();

			usize end = _file->tell() + size;
			const auto check = detail::build_header(object);
			if(header != check) {
				_file->seek(end);
				return core::Ok(Success::Partial);
			}

			size_type collection_size = 0;
			y_try(read_one(collection_size));

			try {
				if constexpr(has_emplace_back_v<T>) {
					if constexpr(has_reserve_v<T>) {
						object.object.reserve(collection_size);
					}
					for(size_type i = 0; i != collection_size; ++i) {
						object.object.emplace_back();
						y_try(deserialize_one(NamedObject{object.object.last(), detail::collection_version_string}));
					}
				} else {
					object.object.resize(collection_size);
					for(size_type i = 0; i != collection_size; ++i) {
						y_try(deserialize_one(NamedObject{object.object[usize(i)], detail::collection_version_string}));
					}
				}
				return core::Ok(Success::Full);
			} catch(std::bad_alloc&) {
				_file->seek(end);
				return core::Ok(Success::Partial);
			}

		}

		// ------------------------------- POLY -------------------------------
		template<typename T>
		Result deserialize_poly(NamedObject<T> object, const detail::FullHeader header, size_type size) {
			static_assert(!has_serde3_v<T>);

			if(size != 0) {
				using element_type = typename T::element_type;
				object.object = element_type::_y_serde3_poly_base.create_from_id(header.type_id);
				if(!object.object) {
					_file->seek(_file->tell() + size);
					return core::Ok(Success::Partial);
				}
				return object.object->_y_serde3_poly_deserialize(*this);
			} else {
				object.object = nullptr;
				return core::Ok(Success::Full);
			}
		}


		// ------------------------------- POD -------------------------------
		template<typename T>
			Result deserialize_pod(NamedObject<T> object, const detail::FullHeader header, size_type size) {
			static_assert(std::is_trivially_copyable_v<T>);
			static_assert(!std::is_pointer_v<T>);

			const auto check = detail::build_header(object);
			if(header != check) {
				static constexpr usize max_prim_size = 4 * sizeof(float);
				if(header.type.name_hash == check.type.name_hash && size <= max_prim_size) {
					std::array<u8, max_prim_size> buffer;
					y_try_discard(_file->read(buffer.data(), size));
					return try_convert<T>(object.object, header.type, buffer.data());
				} else {
					_file->seek(_file->tell() + size);
					return core::Ok(Success::Partial);
				}
			}
			return read_one(object.object);
		}


		// ------------------------------- OBJECT -------------------------------
		template<typename T>
		Result deserialize_object(NamedObject<T> object, const detail::FullHeader header, size_type) {
			static_assert(has_serde3_v<T>);
			static_assert(!has_serde3_poly_v<T>);

			const auto check = detail::build_header(object);
			if(header != check) {
				if(header.type.type_hash != check.type.type_hash) {
					return core::Err();
				}
				return deserialize_members<true>(object.object, header.members.count);
			} else {
				return deserialize_members<force_safe>(object.object, header.members.count);
			}
		}

		template<bool Safe, usize I, typename... Args>
		Result deserialize_members_internal(std::tuple<NamedObject<Args>...> members,
											const ObjectData& object_data) {
			unused(members, object_data);

			Success status = Success::Full;
			if constexpr(I < sizeof...(Args)) {
				if constexpr(Safe) {
					bool found = false;
					auto header = detail::build_header(std::get<I>(members));
					for(const auto& m : object_data.members) {
						if(m.header.type.name_hash == header.type.name_hash) {
							_file->seek(m.offset);
							y_try_status(deserialize_one(std::get<I>(members)));
							found = true;
							break;
						}
					}
					if(!found) {
						status = Success::Partial;
					}
				} else {
					y_try_status(deserialize_one(std::get<I>(members)));
				}

				y_try_status((deserialize_members_internal<Safe, I + 1>(members, object_data)));
			} else if constexpr(Safe) {
				_file->seek(object_data.end_offset);
			}

			return core::Ok(status);
		}

		template<bool Safe, typename T>
		Result deserialize_members(T& object, usize serialized_member_count) {
			ObjectData data;

			if constexpr(Safe) {
				usize obj_start = _file->tell();
				for(usize i = 0; i != serialized_member_count; ++i) {
					usize offset = _file->tell();

					detail::FullHeader header;
					size_type size = size_type(-1);

					y_try_discard(read_header(header));
					y_try_discard(_file->read_one(size));

					_file->seek(_file->tell() + size);

					data.members << HeaderOffset{header, offset};
				}
				data.end_offset = _file->tell();
				_file->seek(obj_start);
			}

			return deserialize_members_internal<Safe, 0>(object._y_serde3_refl(), data);
		}


		// ------------------------------- READ -------------------------------
		template<typename T>
		Result read_one(T& t) {
			static_assert(!std::is_same_v<std::remove_reference_t<T>, detail::FullHeader>);
			y_try_discard(_file->read_one(t));
			return core::Ok(Success::Full);
		}

		Result read_header(detail::FullHeader& header) {
			y_try_discard(_file->read_one(header.type));
			if(header.type.is_polymorphic()) {
				y_try_discard(_file->read_one(header.type_id));
			} else {
				bool read_members = true;
#ifdef Y_SLIM_POD_HEADER
				read_members = header.type.has_serde();
#endif
				if(read_members) {
					y_try_discard(_file->read_one(header.members));
				}

			}
			return core::Ok(Success::Full);
		}

	private:
		File _file;
};

}
}

#undef y_try_status


#endif // Y_SERDE3_ARCHIVES_H
