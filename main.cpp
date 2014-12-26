#include <iostream>

#include<n/types.h>
#include <n/utils.h>
#include <n/core/Array.h>
#include <n/core/String.h>
#include <n/core/Set.h>
#include <n/core/Timer.h>
#include <n/test/Test.h>
#include <n/core/Map.h>
#include <n/core/Functor.h>
#include <n/core/SmartPtr.h>
#include <n/core/Lazy.h>

#include <n/mem/Allocator.h>

#include <n/concurent/Thread.h>
#include <n/concurent/Mutex.h>
#include <n/concurent/Promise.h>
#include <n/concurent/Async.h>
#include <n/concurent/ThreadPool.h>

#include <n/io/ConsoleStream.h>

#include <n/math/Vec.h>
#include <n/math/Matrix.h>
#include <n/math/Quaternion.h>
#include <n/math/Transform.h>

#include <n/assets/AssetBuffer.h>

#include <n/script/Lexer.h>
#include <n/script/Machine.h>

#include <n/graphics/ImageLoader.h>


using namespace n::graphics;
using namespace	n::concurent;
using namespace n::assets;
using namespace n::core;
using namespace n::math;
using namespace n::io;
using namespace n;

struct RGBATest
{
	uint32 red : 8;
	uint32 green : 8;
	uint32 blue : 8;
	uint32 alpha : 8;

};

static_assert(sizeof(RGBATest) == sizeof(uint32), "RGBATest is not 32 bits");

int main(int, char **) {
	//std::cout<<sizeof(long double)<<std::endl;
	Image ima = ImageLoader::load("test.png");
	while(ima.isNull()) {
		if(!ima.isValid()) {
			fatal("Unable to read png");
		}
	}
	//std::cout<<ima.getSize().x()<<" * "<<ima.getSize().y()<<std::endl<<std::endl;
	for(uint i = 0; i != ima.getSize().x(); i++) {
		for(uint j = 0; j != ima.getSize().y(); j++) {
			if(ima.getPixel(Vec2ui(i, j)).sum() > 1.5) {
				std::cout<<" ";
			} else {
				std::cout<<(byte)178;
			}
		}
		std::cout<<std::endl;
	}
	std::cout<<std::endl;

	return 0;
}

