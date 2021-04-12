/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

//#define Y_NO_ARCHIVES
//#define Y_NO_SAFE_DESER

#define y_try_status(result)                                                                    \
    do {                                                                                        \
        if(auto&& _y_try_result = (result); _y_try_result.is_error()) {                         \
            return std::move(_y_try_result.err_object());                                       \
        } else {                                                                                \
            status = status | _y_try_result.unwrap();                                           \
        }                                                                                       \
    } while(false)


namespace y {
namespace serde3 {

namespace detail {

using size_type = u64;

static constexpr std::string_view version_string            = "serde3.v2.0";
static constexpr std::string_view collection_version_string = "serde3.col.v1.0";
static constexpr std::string_view tuple_version_string      = "serde3.tpl.v1.0";
static constexpr std::string_view ptr_version_string        = "serde3.ptr.v1.0";



// Format is compiler dependent for now, so we have to do this to avoid all kind of problems
#if defined(Y_MSVC)
static constexpr u16 compiler_id = 0x01;
#elif defined(Y_CLANG)
static constexpr u16 compiler_id = 0x02;
#elif defined(Y_GCC)
static constexpr u16 compiler_id = 0x03;
#else
static constexpr u16 compiler_id = 0x00;
#endif

static constexpr u16 magic = 0x7966;
static constexpr u16 version_id = 2 | (compiler_id << 12);
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
        inline Result serialize(const T& t) {
#ifdef Y_NO_ARCHIVES
            unused(t);
            y_fatal("Y_NO_ARCHIVES has been defined");
#else
            y_try(write_serde_header());
            y_try(serialize_one(NamedObject{t, detail::version_string}));
            return finalize();
#endif
        }

    private:
#ifndef Y_NO_ARCHIVES
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

        template<typename T>
        inline Result serialize_one(NamedObject<T> non_const_object) {
            const auto object = non_const_object.make_const_ref();

            static_assert(!has_no_serde3_v<T>, "Type has serialization disabled");

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
        template<typename T>
        inline Result serialize_property(NamedObject<T> object) {
            return serialize_one(NamedObject<typename T::value_type>{object.object.get(), object.name});
        }


        // ------------------------------- PTR -------------------------------
        template<typename T>
        inline Result serialize_ptr(NamedObject<T> object) {
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
        inline Result serialize_range(NamedObject<T> object) {
            return serialize_collection(object);
        }

        template<typename T>
        inline Result serialize_collection(NamedObject<T> object) {
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
        inline Result serialize_poly(NamedObject<T> object) {
            static_assert(std::is_const_v<T>);
            static_assert(is_std_ptr_v<T> || std::is_pointer_v<T>);

            if(object.object == nullptr) {
                return write_one(size_type(0));
            }

            SizePatch size_patch{tell(), 0};
            y_try(write_one(size_type(0)));

            if constexpr(is_std_ptr_v<T>) {
                static_assert(has_serde3_ptr_poly_v<T>);

                y_try(write_one(object.object->_y_serde3_poly_type_id()));
                y_try(object.object->_y_serde3_poly_serialize(*this));

            } else {
                static_assert(has_serde3_poly_v<T>);
                static_assert(!has_serde3_v<T>);

                y_try(write_one(object.object._y_serde3_poly_type_id()));
                y_try(object.object._y_serde3_poly_serialize(*this));
            }

            size_patch.size = tell() - size_patch.index;
            push_patch(size_patch);

            return core::Ok(Success::Full);
        }


        // ------------------------------- POD -------------------------------
        template<typename T>
        inline Result serialize_pod(NamedObject<T> object) {
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
        template<typename T>
        inline Result serialize_object(NamedObject<T> object) {
            static_assert(std::is_const_v<T>);
            static_assert(!has_serde3_ptr_poly_v<T>);

            {
                y_try(write_header(object));
                y_try(serialize_members(object.object));
            }

            return core::Ok(Success::Full);
        }

        template<typename T>
        inline Result serialize_members(const T& object) {
            return serialize_members_internal<0>(object._y_reflect());
        }

        template<usize I, typename... Args>
        inline Result serialize_members_internal(const std::tuple<NamedObject<Args>...>& objects) {
            unused(objects);
            if constexpr(I < sizeof...(Args)) {
                {
                    Y_TODO(RAII this?)
                    SizePatch patch{tell(), 0};
                    y_try(write_one(size_type(-1)));

                    y_try(serialize_one(std::get<I>(objects)));

                    patch.size = (size_type(tell()) - size_type(patch.index)) - sizeof(size_type);
                    push_patch(patch);
                }

                //y_try(serialize_one(std::get<I>(objects)));
                y_try(serialize_members_internal<I + 1>(objects));
            }
            return core::Ok(Success::Full);
        }



        // ------------------------------- TUPLE -------------------------------
        template<typename T>
        inline Result serialize_tuple(NamedObject<T> object) {
            static_assert(std::is_const_v<T>);

            {
                y_try(write_header(object));
                y_try(serialize_tuple_members<0>(object.object));
            }

            return core::Ok(Success::Full);
        }

        template<usize I, typename Tpl>
        inline Result serialize_tuple_members(const Tpl& object) {
            unused(object);
            if constexpr(I < std::tuple_size_v<Tpl>) {
                y_try(serialize_one(NamedObject{std::get<I>(object), detail::tuple_version_string}));
                return serialize_tuple_members<I + 1>(object);
            }
            return core::Ok(Success::Full);
        }


        // ------------------------------- WRITE -------------------------------
        template<typename T>
        inline Result write_header(NamedObject<T> object) {
            const auto header = detail::build_header(object);
            return write_one(header);
        }

        template<typename T>
        inline Result write_one(const T& t) {
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
        inline Result write_array(const T* t, usize size) {
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
#endif

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


    struct ObjectData {
        core::Vector<usize> members_offsets;
        usize end_offset = 0;
        Success success_state = Success::Full;
    };

    public:
        ReadableArchive(File& file) : _file(file) {
        }

        ReadableArchive(std::unique_ptr<File> file) : ReadableArchive(*file) {
            _storage = std::move(file);
        }

        template<typename T>
        inline Result deserialize(T& t) {
#ifdef Y_NO_ARCHIVES
            unused(t, args...);
            y_fatal("Y_NO_ARCHIVES has been defined");
#else
            y_try(read_serde_header());
            auto res = deserialize_one(NamedObject{t, detail::version_string});
            return res;
#endif
        }



    private:
#ifndef Y_NO_ARCHIVES
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


        template<typename T>
        inline Result deserialize_one(NamedObject<T> object) {
            static_assert(!has_no_serde3_v<T>, "Type has serialization disabled");

            Result res = core::Ok(Success::Full);

            if constexpr(is_property_v<T>) {
                res = deserialize_property(object);
            } else if constexpr(has_serde3_v<T>) {
                res = deserialize_object(object);
            } else if constexpr(has_serde3_ptr_poly_v<T> || has_serde3_poly_v<T>) {
                res = deserialize_poly(object);
            } else if constexpr(is_tuple_v<T>) {
                res = deserialize_tuple(object);
            } else if constexpr(is_range_v<T>) {
                res = deserialize_range(object);
            } else if constexpr(is_pod_v<T>) {
                res = deserialize_pod(object);
            } else if constexpr(is_std_ptr_v<T>) {
                res = deserialize_ptr(object);
            } else {
                static_assert(is_iterable_v<T>, "Unable to deserialize type");
                res = deserialize_collection(object);
            }

            if(res.is_ok()) {
                if constexpr(has_serde3_post_deser_v<T>) {
                    object.object.post_deserialize();
                }
                if constexpr(has_serde3_post_deser_poly_v<T>) {
                    object.object.post_deserialize_poly();
                }
            }

            return res;
        }

        // ------------------------------- PROPERTY -------------------------------
        template<typename T>
        inline Result deserialize_property(NamedObject<T> object) {
            using inner = typename T::value_type;
            inner i;
            y_try(deserialize_one(NamedObject<inner>{i, object.name}));
            object.object.set(std::move(i));
            return core::Ok(Success::Full);
        }



        // ------------------------------- PTR -------------------------------
        template<typename T>
        inline Result deserialize_ptr(NamedObject<T> object) {
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
        inline Result deserialize_range(NamedObject<T> object) {
            static_assert(!std::is_const_v<std::remove_reference_t<decltype(*object.object.begin())>>);
            return deserialize_collection<T, true>(object);
        }

        template<typename T, bool IsRange = false>
        inline Result deserialize_collection(NamedObject<T> object) {
            static_assert(is_iterable_v<T>);
            static_assert(!std::is_const_v<T>);

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
        inline Result deserialize_poly(NamedObject<T> object) {
            size_type size = 0;
            const usize begin = tell();
            y_try(read_one(size));

            if(size) {
                y_defer(seek(begin + size));

                TypeId type_id;
                y_try(read_one(type_id));

                if constexpr(is_std_ptr_v<T>) {
                    static_assert(!has_serde3_v<T>);

                    using element_type = typename T::element_type;
                    const auto* poly_type = element_type::_y_serde3_poly_base.find_id(type_id);
                    if(!poly_type || !poly_type->create) {
                        return core::Err(Error(ErrorType::UnknownPolyError, object.name.data()));
                    }

                    object.object = poly_type->create();
                    if(object.object) {
                        return object.object->_y_serde3_poly_deserialize(*this);
                    }
                    return core::Ok(Success::Partial);
                } else {
                    static_assert(has_serde3_poly_v<T>);
                    static_assert(!has_serde3_v<T>);

                    if(type_id != object.object._y_serde3_poly_type_id()) {
                        return core::Err(Error(ErrorType::UnknownPolyError, object.name.data()));
                    }

                    return object.object._y_serde3_poly_deserialize(*this);
                }
            } else {
                if constexpr(is_std_ptr_v<T>) {
                    static_assert(!has_serde3_v<T>);
                    object.object = nullptr;
                } else {
                    static_assert(has_serde3_poly_v<T>);
                    static_assert(!has_serde3_v<T>);
                    return core::Err(Error(ErrorType::UnknownPolyError, object.name.data()));
                }

                return core::Ok(Success::Full);
            }
        }


        // ------------------------------- POD -------------------------------
        template<typename T>
        inline Result deserialize_pod(NamedObject<T> object) {
            static_assert(is_pod_v<T>);
            static_assert(!std::is_pointer_v<T>);

            detail::ObjectHeader header;
            Result header_res = check_header(object, header);
            if(!header_res) {
                if(header_res.error().type == ErrorType::MemberTypeError) {
                    auto convert_res = try_convert(object.object, header.type, _file);
                    if(!convert_res) {
                        return core::Err(convert_res.error().with_name(object.name.data()));
                    }
                }
                return header_res;
            }

            return read_one(object.object);
        }


        // ------------------------------- OBJECT -------------------------------
        template<typename T>
        inline Result deserialize_object(NamedObject<T> object) {
            static_assert(has_serde3_v<T>);
            static_assert(!has_serde3_ptr_poly_v<T>);

            detail::ObjectHeader header;
            y_try(read_header(header));
            const auto check = detail::build_header(object);

            if(header != check) {
#ifndef Y_NO_SAFE_DESER
                if(header.type.type_hash == check.type.type_hash) {
                    return deserialize_members<true>(object.object, header);
                }
#endif
                return core::Err(Error(ErrorType::SignatureError, object.name.data()));
            } else {
                return deserialize_members<force_safe>(object.object, header);
            }
        }

        template<bool Safe, typename T>
        inline Result deserialize_members(T& object, const detail::ObjectHeader& header) {
            ObjectData data;
            if constexpr(Safe) {
                usize offset = tell();
                for(usize i = 0; i != header.members.count; ++i) {
                    data.members_offsets << offset;
                    if(i) {
                        seek(offset);
                    }

                    size_type size = size_type(-1);
                    y_try(read_one(size));
                    offset += size + sizeof(size_type);
                }
                data.end_offset = offset;
            }

            y_defer(if constexpr(Safe) { seek(data.end_offset); });
            return deserialize_members_internal<Safe, 0>(object._y_reflect(), data);
        }

        template<bool Safe, usize I, typename... Args>
        inline Result deserialize_members_internal(const std::tuple<NamedObject<Args>...>& members,
                                            ObjectData& object_data) {
            unused(object_data);

#ifdef Y_NO_SAFE_DESER
            static_assert(!Safe);
#endif
            if constexpr(I < sizeof...(Args)) {
                const auto& member = std::get<I>(members);

                if constexpr(Safe) {
                    auto& offsets = object_data.members_offsets;
                    if(offsets.is_empty()) {
                        return core::Ok(Success::Partial);
                    }

                    bool found = false;
                    for(usize i = 0; i != offsets.size(); ++i) {
                        seek(offsets[i]);

                        size_type size = size_type(-1);
                        y_try(read_one(size));
                        const usize end = tell() + size;

                        if(deserialize_one(member)) {
                            if(tell() != end) {
                                return core::Err(Error(ErrorType::SignatureError, member.name.data()));
                            }
                            offsets.erase_unordered(offsets.begin() + i);
                            found = true;
                            break;
                        }
                    }
                    if(!found) {
                        object_data.success_state = Success::Partial;
                    }
                } else {
                    size_type size = size_type(-1);
                    y_try(read_one(size));
                    const usize end = tell() + size;

                    y_try(deserialize_one(member));

                    if(tell() != end) {
                        return core::Err(Error(ErrorType::SignatureError, member.name.data()));
                    }
                }

                return deserialize_members_internal<Safe, I + 1>(members, object_data);
            }

            return core::Ok(object_data.success_state);
        }


        // ------------------------------- TUPLE -------------------------------
        template<typename T>
        inline Result deserialize_tuple(NamedObject<T> object) {
            y_try(check_header(object));
            return deserialize_tuple_members<0>(object.object);
        }

        template<usize I, typename Tpl>
        inline Result deserialize_tuple_members(Tpl& object) {
            unused(object);
            if constexpr(I < std::tuple_size_v<Tpl>) {
                y_try(deserialize_one(NamedObject{std::get<I>(object), detail::tuple_version_string}));
                return deserialize_tuple_members<I + 1>(object);
            }
            return core::Ok(Success::Full);
        }


        // ------------------------------- READ -------------------------------

        template<typename T>
        inline Result check_header(NamedObject<T> object, detail::ObjectHeader& header) {
            y_try(read_header(header));

            const auto check = detail::build_header(object);
            if(header != check) {
                const ErrorType tpe = header.type.name_hash == check.type.name_hash
                    ? ErrorType::MemberTypeError
                    : ErrorType::SignatureError;
                return core::Err(Error(tpe, object.name.data()));
            }
            return core::Ok(Success::Full);
        }

        template<typename T>
        inline Result check_header(NamedObject<T> object) {
            detail::ObjectHeader header;
            return check_header(object, header);
        }


        template<typename T>
        inline Result read_one(T& t) {
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
#endif

    private:
        File& _file;
        std::unique_ptr<File> _storage;
};


namespace detail {

template<typename T>
Result serialize_one(WritableArchive& arc, const T& t) {
    unused(arc, t);
    if constexpr(has_no_serde3_v<T>) {
        return core::Ok(Success::Full);
    } else {
        return arc.serialize(t);
    }
}

template<typename T>
Result deserialize_one(ReadableArchive& arc, T& t) {
    unused(arc, t);
    if constexpr(has_no_serde3_v<T>) {
        return core::Ok(Success::Full);
    } else {
        return arc.deserialize(t);
    }
}

}


}
}

#undef y_try_status


#endif // Y_SERDE3_ARCHIVES_H

