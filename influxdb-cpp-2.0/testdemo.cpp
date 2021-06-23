// testdemo.cpp : Defines the entry point for the application.
//

#include "testdemo.h"

int main(int argc, char ** argv)
{
	std::cout << "Hello CMake." << std::endl;	
	DbHelper dbHelper = {};
	dbHelper.do_test_v2();
	return 0;
}
