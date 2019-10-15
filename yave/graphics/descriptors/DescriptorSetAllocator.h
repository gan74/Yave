/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETALLOCATOR_H
#define YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETALLOCATOR_H

#include <yave/graphics/vk/vk.h>
#include <yave/device/DeviceLinked.h>

#include <y/concurrent/SpinLock.h>
#include <mutex>
#include <bitset>

namespace std {
template<>
struct hash<vk::DescriptorSetLayoutBinding> {
	auto operator()(const vk::DescriptorSetLayoutBinding& l) const {
		const char* data = reinterpret_cast<const char*>(&l);
		return y::hash_range(data, data + sizeof(l));
	}
};

template<>
struct hash<y::core::Vector<vk::DescriptorSetLayoutBinding>> {
	auto operator()(const y::core::Vector<vk::DescriptorSetLayoutBinding>& k) const {
		return y::hash_range(k);
	}
};
}

namespace yave {

class Descriptor;
class DescriptorSetPool;

class DescriptorSetLayout : NonCopyable, public DeviceLinked {
	public:
		static constexpr usize max_descriptor_type = std::max({
				usize(vk::DescriptorType::eSampler),
				usize(vk::DescriptorType::eCombinedImageSampler),
				usize(vk::DescriptorType::eSampledImage),
				usize(vk::DescriptorType::eStorageImage),
				usize(vk::DescriptorType::eUniformTexelBuffer),
				usize(vk::DescriptorType::eStorageTexelBuffer),
				usize(vk::DescriptorType::eUniformBuffer),
				usize(vk::DescriptorType::eStorageBuffer),
				usize(vk::DescriptorType::eUniformBufferDynamic),
				usize(vk::DescriptorType::eStorageBufferDynamic)
			}) + 1;


		DescriptorSetLayout() = default;
		DescriptorSetLayout(DevicePtr dptr, core::Span<vk::DescriptorSetLayoutBinding> bindings);

		DescriptorSetLayout(DescriptorSetLayout&&) = default;
		DescriptorSetLayout& operator=(DescriptorSetLayout&&) = default;

		~DescriptorSetLayout();

		const math::Vec<max_descriptor_type, u32>& desciptors_count() const;

		vk::DescriptorSetLayout vk_descriptor_set_layout() const;

	private:
		vk::DescriptorSetLayout _layout;
		math::Vec<max_descriptor_type, u32> _sizes;
};

class DescriptorSetData {
	public:
		DescriptorSetData() = default;

		DevicePtr device() const;
		bool is_null() const;

		vk::DescriptorSet vk_descriptor_set() const;

	private:
		friend class LifetimeManager;

		void recycle();

	private:
		friend class DescriptorSetPool;

		DescriptorSetData(DescriptorSetPool* pool, u32 id);

		DescriptorSetPool* _pool = nullptr;
		u32 _index = 0;
};

class DescriptorSetPool : NonMovable, public DeviceLinked {
	public:
		static constexpr usize pool_size = 128;

		DescriptorSetPool(const DescriptorSetLayout& layout);
		~DescriptorSetPool();

		DescriptorSetData alloc();
		void recycle(u32 id);

		bool is_full() const;

		vk::DescriptorSet vk_descriptor_set(u32 id) const;
		vk::DescriptorPool vk_pool() const;

		// Slow: for debug only
		usize free_sets() const;

	private:
		std::bitset<pool_size> _taken;
		u32 _first_free = 0;

		mutable concurrent::SpinLock _lock;

		std::array<vk::DescriptorSet, pool_size> _sets;
		vk::DescriptorPool _pool;
};

class DescriptorSetAllocator : NonCopyable, public DeviceLinked  {

	using Key = core::Vector<vk::DescriptorSetLayoutBinding>;

	struct Pools : NonMovable {
		DescriptorSetLayout layout;
		core::Vector<std::unique_ptr<DescriptorSetPool>> pools;
	};

	public:
		DescriptorSetAllocator(DevicePtr dptr);

		DescriptorSetData create_descritptor_set(const Key& bindings);
		const DescriptorSetLayout& descriptor_set_layout(const Key& bindings);

		// Slow: for debug only
		usize layout_count() const;
		usize pool_count() const;
		usize free_sets() const;

	private:
		Pools& layout(const Key& bindings);

		std::unordered_map<Key, Pools> _layouts;
		mutable std::mutex _lock;
};

}

#endif // YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETALLOCATOR_H
