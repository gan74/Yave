#include <n/core/String.h>
#include <iostream>

using namespace n;
using namespace n::core;

void printInfos(const String2 &str) {
	std::cout<<"\t"<<std::boolalpha<<"long = "<<str.isLong()<<" (size = "<<str.size()<<")"<<std::endl;
	std::cout<<"\t"<<"data = \""<<str.data()<<"\""<<std::endl<<std::endl;
}


class A
{
	public:
		A(int i) {
			std::cout<<i<<std::endl;
		}

		A(const std::string &str) {
			std::cout<<str<<std::endl;
		}

		A(const  char *str) {
			std::cout<<str<<std::endl;
		}

		A &operator=(A &&) {
			std::cout<<"&&"<<std::endl;
			return *this;
		}

};

class complexe {};

 std::ostream &operator <<(std::ostream &output, complexe const z) {
	output << "prout";
	return output;
}


int main(int, char **) {
	complexe c;
	std::cout << c;

	std::string str("string");
	A a(27);
	a = str;

	return 0;






	/*core::Array<int> arr;
	arr = {1, 2, 3};

	const char raw[] = "long/short test";

	std::cout<<"Long and short test:"<<std::endl;
	for(uint i = 0; i != sizeof(raw); i++) {
		printInfos(String2(raw, i));
	}


	std::cout<<"Default test:"<<std::endl;
	printInfos(String2());


	std::cout<<"Copy test:"<<std::endl;
	String2 str("long copy test");
	String2 cpy = str;
	printInfos(cpy);
	str = "s-test";
	printInfos(str);
	cpy = str;
	printInfos(cpy);


	std::cout<<"Short + test:"<<std::endl;
	str = "+";
	printInfos(str + " test");


	std::cout<<"Long + test: "<<std::endl;
	str = "long";
	printInfos(str + " +" + " test");

	std::cout<<"Operator < test:"<<std::endl;
	std::cout<<"\t"<<std::boolalpha<<(String2("abcd") < "abce")<<std::endl;
	std::cout<<"\t"<<std::boolalpha<<(String2("abcd") < "abc")<<std::endl<<std::endl;


	std::cout<<"Operator > test:"<<std::endl;
	std::cout<<"\t"<<std::boolalpha<<(String2("abcd") > "abc")<<std::endl;
	std::cout<<"\t"<<std::boolalpha<<(String2("abcd") > "abcc")<<std::endl<<std::endl;


	std::cout<<"Swap test:"<<std::endl;
	str = ", rhs";
	cpy = "left hand size";
	cpy.swap(str);
	std::cout<<"\t"<<str.data()<<cpy.data()<<std::endl<<std::endl;


	return 0;*/
}
