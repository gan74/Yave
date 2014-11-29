#ifndef N_MEM_ALLOCATOR_H
#define N_MEM_ALLOCATOR_H


#include <n/utils.h>
#include <n/core/List.h>

namespace n {
namespace mem {

class Allocator : public core::NonCopyable
{
	class Region : public core::NonCopyable
	{
		public:
			Region(uint blckSize, uint nBlck = 0) : blockSize(blckSize), size(nBlck ? nBlck : std::min(16, 1 << (14 - blckSize))), left(size), mem(malloc(blockSize * size)), ptr(mem) {
				byte *it = (byte *)mem;
				void **last = (void **)ptr;
				for(uint i = 0; i != size; i++, it += blockSize) {
					*last = it;
					last = (void **)it;
				}
				*last = 0;
			}

			~Region() {
				free(mem);
			}

			void *allocate() {
				if(!ptr) {
					return 0;
				}
				void **addr = (void **)ptr;
				ptr = *addr;
				left--;
				return addr;
			}

			bool desallocate(void *p) {
				if(!contains(p)) {
					return false;
				}
				*((void **)p) = ptr;
				ptr = p;
				left++;
				return true;
			}

			uint getFreeCapacity() const {
				return left;
			}

			bool isEmpty() const {
				return !left;
			}

			bool contains(void *p) const {
				return p >= mem && p <= ((byte *)mem + blockSize * size);
			}

		private:
			uint blockSize;
			uint size;
			uint left;
			void *mem;
			void *ptr;

	};


	public:
		Allocator() : regTable(new core::List<Region *>[regs]) {
		}

		~Allocator() {
			exit(-2);
		}


		void *allocate(uint size) {
			size = std::min(size, sizeof(void *));
			uint l2 = core::log2ui(size);
			l2 += (1u << l2) != size;
			l2 -= core::log2ui(sizeof(void *));
			if(l2 < regs) {
				for(core::List<Region *>::iterator it = regTable[l2].begin(); it != regTable[l2].end(); ++it) {
					Region *reg = *it;
					void *ptr = reg->allocate();
					if(ptr) {
						regTable[l2].remove(it);
						regTable[l2].prepend(reg);
						return ptr;
					}
				}
				regTable[l2].prepend(new Region(sizeof(void *) << l2));
				return regTable[l2].first()->allocate();
			}
			return malloc(size);
		}

		void desallocate(void *ptr) {
			for(uint i = 0; i != regs; i++) {
				for(core::List<Region *>::iterator it = regTable[i].begin(); it != regTable[i].end(); ++it) {
					Region *reg = *it;
					if(reg->desallocate(ptr)) {
						regTable[i].remove(it);
						if(reg->isEmpty()) {
							delete reg;
						} else {
							regTable[i].prepend(reg);
						}
						return;
					}
				}
			}
			free(ptr);
		}


	private:
		static const uint regs = 12;
		core::List<Region *> *regTable;


};



}
}

#endif // N_MEM_ALLOCATOR_H
