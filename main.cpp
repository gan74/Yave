#define ALL
#ifdef ALL
#include "main.h"

int main(int argc, char **argv) {
	io::File *traceFile = new io::File("./perfdump.json");
	traceFile->open(io::IODevice::Write);
	setTraceOutputStream(traceFile);

	SDL_Window *win = createWindow();

	if(argc > 1 && argv[1] == String("--no-debug")) {
		GLContext::getContext()->setDebugEnabled(false);
	}

	PerspectiveCamera cam;
	cam.setPosition(Vec3(-25, 0, 25));
	cam.setRatio(16/9.0);
	cam.setForward(-cam.getPosition());

	Scene scene;
	scene.insert(&cam);

	{
		auto obj = new Obj("./crytek-sponza/sponza.obj");
		obj->setRotation(Quaternion<>::fromEuler(0, 0, pi * 0.5));
		//auto obj = new Obj("plane.obj");
		obj->setAutoScale(800);
		scene.insert(obj);
	}

	uint max = 10;
	float scale = 5;
	for(uint i = 0; i != max; i++) {
		for(uint j = 0; j != max; j++) {
			auto m = new MaterialTest(i / float(max), j / float(max));
			m->setScale(scale);
			m->setPosition(Vec3(i - max * 0.5, j - max * 0.5, 1) * m->getRadius() * 2.5);
			scene.insert(m);
		}
	}

	{
		BoxLight *l = new BoxLight(600);

		l->setForward(Vec3(0, 1, -1));
		//l->setPosition(Vec3(0, 0, 10));
		l->setIntensity(5);
		//l->setCastShadows<VarianceShadowRenderer>(&scene, 1024, 2);
		scene.insert(l);
	}

	{
		DirectionalLight *l = new DirectionalLight;

		l->setForward(Vec3(0, -1, -1));
		l->setIntensity(1);
		l->setColor(BaseColor::Blue);
		scene.insert(l);
	}


	SceneRenderer *sceRe = new SceneRenderer(&scene);
	GBufferRenderer *gRe = new GBufferRenderer(sceRe);
	DeferredShadingRenderer *ri = new DeferredShadingRenderer(gRe);
	Renderer *renderers[] {new FrameBufferRenderer(ri),
						   sceRe,
						   new FrameBufferRenderer(gRe),
						   new FrameBufferRenderer(gRe, 1),
						   new FrameBufferRenderer(gRe, 2),
						   new DeferredIBLRenderer(gRe),
						   tone = new BasicToneMapRenderer(ri)};

	String renderNames[] = {"Deferred shading",
							"Scene",
							"G-buffer 0",
							"G-buffer 1",
							"G-buffer 2",
							"IBL",
							"Tone mapped"};

	Timer timer;

	while(run(win)) {
		double dt = timer.reset();
		cam.setPosition(cam.getPosition() + (wasd.x() * cam.getForward() + wasd.y() * cam.getTransform().getY()) * dt * 100);
		Vec2 angle = mouse * 0.01;
		float p2 = pi * 0.5 - 0.01;
		angle.y() = std::min(std::max(angle.y(), -p2), p2);
		Vec3 f = Vec3(Vec2(cos(-angle.x()), sin(-angle.x())) * cos(angle.y()), -sin(angle.y()));
		cam.setForward(f);

		uint count = sizeof(renderers) / sizeof(void *);
		uint index = rendererIndex % count;
		(*renderers[index])();
		SDL_SetWindowTitle(win, renderNames[index].toChar());

		GLContext::getContext()->finishTasks();
		GLContext::getContext()->flush();
	}
	return 0;
}


#else

#include <iostream>
#include <cstring>
#include <n/utils.h>
#include <n/core/Timer.h>
#include <n/core/Pair.h>
#include <n/core/String.h>
#include <n/core/Set.h>
#include <n/io/File.h>
#include <csetjmp>


using namespace n;
using namespace n::core;
using namespace n::io;

/*template<typename T, typename Eq = std::equal_to<T>>
class Hash
{
	typedef uint64 hashType;

	hashType hashFunc(const String &e) {
		return hash(&e[0], e.size());
	}

	hashType hashFunc(const int &e) {
		return e;
	}

	public:
		class iterator
		{
			public:
				iterator &operator++() { // ++prefix
					++it;
					if(it == bi->end()) {
						it = (++bi)->begin();
					}
					return *this;
				}

				iterator operator++(int) { // postfix++
					iterator it(*this);
					operator++();
					return it;
				}

				bool operator==(const iterator &i) const {
					return it == i.it;
				}

				bool operator!=(const iterator &i) const {
					return it != i.it;
				}

				const T &operator*() const {
					return *it;
				}

				const T *operator->() const {
					return &(*it);
				}

			private:
				friend class Hash;
				iterator(Array<T> *b) : bi(b), it(bi->begin()) {
				}

				iterator(Array<T> *b, typename Array<T>::iterator i) : bi(b), it(i) {
				}

				Array<T> *bi;
				typename Array<T>::iterator it;

		};

		typedef iterator const_iterator;

		Hash() : bucketCount(7), hSize(0), buckets(new Array<T>[bucketCount + 1]) {
		}

		~Hash() {
			delete[] buckets;
		}

		iterator insert(const T &t) {
			uint b1 = bucketCount + 1;
			if(hSize > b1 >> 2) {
				resize((b1 << 2) - 1);
			}
			hashType h = hashFunc(t);
			Array<T> &a = buckets[h % bucketCount];
			typename Array<T>::iterator it = a.find(t);
			if(it != a.end()) {
				return iterator(&a, it);
			}
			hSize++;
			return iterator(&a, a.insert(t));
		}

		iterator find(const T &t) {
			hashType h = hashFunc(t);
			Array<T> &a = buckets[h % bucketCount];
			typename Array<T>::iterator it = a.find(t);
			if(it != a.end()) {
				return iterator(&a, it);
			}
			return end();
		}

		iterator begin() {
			return iterator(buckets);
		}

		iterator end() {
			return iterator(buckets + bucketCount);
		}

		const_iterator begin() const {
			return const_iterator(buckets);
		}

		const_iterator end() const {
			return const_iterator(buckets + bucketCount);
		}

		uint size() const {
			return hSize;
		}

		uint getCapacity() const {
			return (bucketCount + 1) >> 2;
		}

		void print() const {
			for(uint i = 0; i!= bucketCount; i++) {
				std::cout<<"BUCKET "<<i<<std::endl;
				for(const T &t : buckets[i]) {
					std::cout<<"  "<<t<<std::endl;
				}
			}
		}

	private:
		void resize(uint s) {
			Array<T> *b = buckets;
			uint l = bucketCount;
			bucketCount = s;
			buckets = new Array<T>[bucketCount + 1];
			for(uint i = 0; i != l; i++) {
				for(const T &e : b[i]) {
					insert(e);
				}
			}
			delete[] b;
		}

		uint bucketCount;
		uint hSize;
		Array<T> *buckets;
};*/

//static_assert(Collection<Hash<int>>::isCollection, "hash not col");

#include <n/core/Map.h>
#include <n/utils.h>

class AckMap
{
	static constexpr uint max = 1 << 21;
	struct element
	{
		element(uint64 t = 0) : _2(t) {
		}

		uint64 _2;

		element &operator=(const element &e) {
			_2 = e._2;
			return *this;
		}

		operator uint64() const {
			return _2;
		}
	};

	class Page
	{
		public:
			Page(uint64 m, uint64 n, AckMap *p) : parent(p), pair(m, n), file(0), data(new element[max]) {
				parent->loaded();
			}

			void offload() {
				if(!file) {
					file = new File("./stk/" + String(uniqueId()) + ".map");
					//std::cout<<"saving memoization data as \""<<file->getName()<<"\""<<std::endl;
					if(!file->open(IODevice::Write | IODevice::Binary)) {
						fatal("Unable to open file");
					}
					file->writeBytes(data, max * sizeof(element));
					file->close();
					delete[] data;
					data = 0;
					parent->unloaded();
				}
			}

			void reload() {
				if(file) {
					parent->loaded();
					if(!file->open(IODevice::Read | IODevice::Binary)) {
						fatal("Unable to open file");
					}
					if(file->readBytes(data, max * sizeof(element)) != max * sizeof(element)) {
						fatal("Unable to restore memoization data");
					}
					file->close();
					delete file;
					file = 0;
				}
			}

			element *find(const Pair<uint64> &p) {
				if(pair._1 != p._1) {
					return 0;
				}
				reload();
				if(!data) {
					fatal("Null page");
				}
				uint64 n = p._2 - pair._2;
				if(n >= max) {
					return 0;
				}
				if(!data[n]) {
					//std::cout<<"("<<p._1<<", "<<p._2<<") not found"<<std::endl;
					return 0;
				}
				return data + n;
			}

			element *operator[](const Pair<uint64> &p) {
				if(pair._1 != p._1) {
					fatal("Wrong page");
				}
				reload();
				if(!data) {
					fatal("Null page");
				}
				uint64 n = p._2 - pair._2;
				if(n >= max) {
					return 0;
				}
				return data + n;
			}

		private:
			AckMap *parent;
			Pair<uint64> pair;
			File *file;
			element *data;

	};

	public:
		AckMap() : lives(0) {
		}

		uint64 &operator[](const Pair<uint64> &p) {
			if(!p._1) {
				aux = p._2 + 1;
				return aux._2;
			}
			element *e = findPage(p)[p];
			if(!e) {
				fatal("Mapping error");
			}
			return e->_2;
		}

		element *find(const Pair<uint64> &p) {
			if(!p._1) {
				aux = p._2 + 1;
				return &aux;
			}
			return findPage(p).find(p);
		}

		element *end() const {
			return 0;
		}

	private:
		Page &findPage(const Pair<uint64> &p) {
			while(pages.size() < p._1) {
				pages.append(Array<Page *>());
			}
			Array<Page *> &row = pages[p._1 - 1];
			uint64 index = p._2 / max;
			while(row.size() <= index) {
				//std::cout<<lives<<" live pages ("<<lives * max * sizeof(element) / (1024 * 1024)<<"MB)"<<std::endl;

				Page *pa = new Page(p._1, row.size() * max, this);
				all.append(pa);
				row.append(pa);
			}
			return *(row[index]);
		}

		void loaded() {
			lives++;
			while(lives * max * sizeof(element) / (1024 * 1024) > 512) {
				uint r = random(all.size(), 0);
				all[r]->offload();
				//std::cout<<"offloading "<<r<<std::endl;
			}
		}

		void unloaded() {
			lives--;
		}


		Array<Page *> all;
		Array<Array<Page *>> pages;
		uint lives;
		element aux;
};

class AckStack
{
	static constexpr uint max = 1 << 20;

	public:
		AckStack() : data(new Pair<uint64>[max]), end(data){
		}

		~AckStack() {
			delete[] data;
		}

		void push(const Pair<uint64> &p) {
			if(end == data + max) {
				offload();
			}
			new(end++) Pair<uint64>(p);
		}

		Pair<uint64> pop() {
			if(end == data) {
				reload();
			}
			return *(--end);
		}

		bool isEmpty() const {
			return end == data && files.isEmpty();
		}

		void operator<<(const Pair<uint64> &p) {
			push(p);
		}

	private:
		void offload() {
			File *save = new File("./stk/" + String(uniqueId()) + ".stk");
			//std::cout<<"saving stack as \""<<save->getName()<<"\""<<std::endl;
			if(!save->open(IODevice::Write | IODevice::Binary)) {
				fatal("Unable to open file");
			}
			files << save;
			save->writeBytes(data, max * sizeof(Pair<uint64>));
			save->close();
			end = data;
		}

		void reload() {
			File *f = files.last();
			files.pop();
			if(!f->open(IODevice::Read | IODevice::Binary)) {
				fatal("Unable to open file");
			}
			if(f->readBytes((char *)data, max * sizeof(Pair<uint64>)) != max * sizeof(Pair<uint64>)) {
				fatal("Unable to restore stack");
			}
			f->close();
			delete f;
			end = data + max;
		}

		Pair<uint64> *data;
		Array<File *> files;
		Pair<uint64> *end;
};

uint64 ack(uint64 m, uint64 n) {
	static AckMap map;
	AckStack stack;
	Pair<uint64> p(m, n);
	auto w = map.find(Pair<uint64>(m, n));
	if(w != map.end()) {
		return w->_2;
	}
	while(true) {
		if(!p._1) {
			uint x = map[p] = p._2 + 1;
			if(stack.isEmpty()) {
				return x;
			}
			p = stack.pop();
		} else if(!p._2) {
			Pair<uint64> p2(p._1 - 1, 1);
			auto it = map.find(p2);
			if(it == map.end()) {
				stack << p;
				p = p2;
			} else {
				uint x = map[p] = it->_2;
				if(stack.isEmpty()) {
					return x;
				}
				p = stack.pop();
			}
		} else {
			Pair<uint64> p2(p._1, p._2 - 1);
			auto it = map.find(p2);
			if(it == map.end()) {
				stack << p;
				p = p2;
			} else {
				uint64 val = it->_2;
				Pair<uint64> p3(p._1 - 1, val);
				it = map.find(p3);
				if(it == map.end()) {
					stack << p;
					p = p3;
				} else {
					uint x = map[p] = it->_2;
					if(stack.isEmpty()) {
						return x;
					}
					p = stack.pop();
				}
			}
		}
		/*return map[p] =
			m ? n ? ack(m - 1, ack(m, n - 1)) : ack(m - 1, 1) : n + 1;*/
	}
	return 0;
}

uint64 rack(uint64 m, uint64 n) {
	static Map<Pair<uint64>, uint64> map;
	Pair<uint64> p(m, n);
	auto it = map.find(p);
	if(it == map.end()) {
		return map[p] =
				m ? n ? rack(m - 1, rack(m, n - 1)) : rack(m - 1, 1) : n + 1;
	}
	return it->_2;
}

#include <n/io/TextInputStream.h>
#include <n/io/BufferedInputStream.h>


class A
{
	virtual void lel() {
	}
};

class C
{
};

struct B  : public C, public virtual A
{
	virtual void lel() override {
	}
};

int main(int, char **) {

	A *b = new B();
	std::cout<<dynamic_cast<B *>(b)<<", "<<reinterpret_cast<B *>(b)<<std::endl;
	return 0;
}

#endif

