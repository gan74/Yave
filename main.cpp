#include "main.h"

struct String
{
	String() {
		fatal("ERROR : String used");
	}
};

void printInfos(const String2 &str) {
	std::cout<<"\t"<<std::boolalpha<<"long = "<<str.isLong()<<" (size = "<<str.size()<<")"<<std::endl;
	std::cout<<"\t"<<"data = \""<<str.data()<<"\""<<std::endl<<std::endl;
}

int main(int, char **) {

	const char raw[] = "long/short test";

	std::cout<<"Long and short test: "<<std::endl;
	for(uint i = 0; i != sizeof(raw); i++) {
		printInfos(String2(raw, i));
	}

	std::cout<<"Default test: "<<std::endl;
	printInfos(String2());

	std::cout<<"Copy test: "<<std::endl;
	String2 str("long copy test");
	String2 cpy = str;
	printInfos(cpy);
	str = "s-test";
	printInfos(str);
	cpy = str;
	printInfos(cpy);


	std::cout<<"Short + test: "<<std::endl;
	str = "+";
	printInfos(str + " test");

	std::cout<<"Long + test: "<<std::endl;
	str = "long";
	printInfos(str + " +" + " test");

	return 0;
}
