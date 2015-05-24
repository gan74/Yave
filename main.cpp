#define ALL
#ifdef ALL
#include "main.h"


int main(int argc, char **argv) {
	SDL_Window *win = createWindow();

	if(argc > 1 && argv[1] == String("--no-debug")) {
		GLContext::getContext()->setDebugEnabled(false);
	}

	Camera<> cam;
	cam.setPosition(Vec3(-10, 0, 10));
	cam.setRatio(4/3.0);
	cam.setForward(-cam.getPosition());

	Light<> *light = 0;

	Scene<> scene;
	scene.insert(&cam);

	for(uint i = 0; i != 100; i++) {
		auto obj = new Obj("scube.obj");
		obj->setAutoScale(6);
		obj->setPosition(Vec3(random(), random(), random()) * 100 - 50);
		scene.insert(obj);
	}

	{
		PointLight<> *l = new PointLight<>();
		l->setPosition(Vec3(-5, 5, 10));
		l->setRadius(25);
		scene.insert(light = l);
	}
	{
		DirectionalLight<> *l = new DirectionalLight<>();
		l->setPosition(Vec3(-5, -5, 5));
		l->setColor(Color<>(Blue) * 2);
		scene.insert(l);
	}

	BufferedRenderer *ri = 0;
	ri = new DeferredShadingRenderer(new GBufferRenderer(new SceneRenderer(&scene)));
	FrameBufferRenderer renderer(ri);

	Timer timer;
	Timer total;

	while(run(win)) {
		double dt = timer.reset();
		cam.setPosition(cam.getPosition() + (wasd.x() * cam.getForward() + wasd.y() * cam.getTransform().getY()) * dt * 50);
		Vec2 angle = mouse * 0.01;
		float p2 = pi<>() * 0.5 - 0.01;
		angle.y() = std::min(std::max(angle.y(), -p2), p2);
		cam.setForward(Vec3(Vec2(cos(angle.x()), sin(angle.x())) * cos(angle.y()), -sin(angle.y())));

		if(light) {
			light->setColor(Vec4(exp(
					(sin(total.elapsed()) + 1) * 2
				)));
		}

		(renderer)();

		GLContext::getContext()->finishTasks();
		GLContext::getContext()->flush();

		if(GLContext::getContext()->checkGLError()) {
			fatal("OpenGL error");
		}

		/*if(total.elapsed() > 20) {
			break;
		}*/
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


using namespace n;
using namespace n::core;
using namespace n::io;

template<typename T, typename Eq = std::equal_to<T>>
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
};

//static_assert(Collection<Hash<int>>::isCollection, "hash not col");


uint max = 467576;
Array<String> array;
const char *cstr = "azertyuiopqsdfghjklmwxcvbn123456789";

void init() {
	while(array.size() < max) {
		array.append((math::random(0, max * 2)) + String(cstr));
	}
}

template<typename T>
float test(uint r, uint &x) {
	init();
	T h;
	Timer ti;
	for(uint i = 0; i != max; i++) {
		h.insert(String(i % (max / r)) + String(cstr));
	}
	for(const String &w : array) {
		if(h.find(w) != h.end()) {
			x++;
		}
	}
	return ti.elapsed();
}


int main(int, char **) {
	double t10 = 0;
	double t20 = 0;
	for(uint r = 2; r != 100; r++) {
		uint a = 0;
		uint b = 0;
		float t1 = test<Hash<String>>(r, a);
		float t2 = test<Set<String>>(r, b);
		t10 += t1;
		t20 += t2;
		std::cout<<"R = "<<r<<" ("<<a<<"/"<<b<<")"<<std::endl;
		std::cout<<"    Hash = "<<round(t1 * 1000)<<"ms"<<std::endl;
		std::cout<<"    Set  = "<<round(t2 * 1000)<<"ms"<<std::endl;
		std::cout<<std::endl;
	}
	std::cout<<"Total : "<<std::endl;
	std::cout<<"    Hash = "<<round(t10 * 1000)<<"ms"<<std::endl;
	std::cout<<"    Set  = "<<round(t20 * 1000)<<"ms"<<std::endl;
	return 0;
}

#endif

