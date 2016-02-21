#include "main.h"

int main(int, char **) {

	const char raw[] = "long and short string test...";

	for(uint i = 0; i != sizeof(raw); i++) {
		String2 str(raw, i);
		std::cout<<std::boolalpha<<"long = "<<str.isLong()<<" (size = "<<str.size()<<")"<<std::endl;
		std::cout<<"data = \""<<str.data()<<"\""<<std::endl<<std::endl;
	}

	return 0;
}
