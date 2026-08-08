// Entry point for testing the partest framework
#include <iostream>

#include <partest/bootstrap.h>
#include "baseTests.h"
#include "assertionTests.h"
#include "semaphoreTests.h"
#include "dispatcherTests.h"
#include "xmlnodeTests.h"
#include "typeTests.h"

// For memory leak validation on MSVC
#if defined(_MSVC_LANG)
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

int main(int argc, const char **argv)
{
// For memory leak validation on MSVC
#if defined(_MSVC_LANG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	partest::initializeSuite(argc, argv);
	partest::addTestClass(partest::make_unique<TestBaseTests>());
	partest::addTestClass(partest::make_unique<AssertionTests>());
	partest::addTestClass(partest::make_unique<SemaphoreTests>());
	partest::addTestClass(partest::make_unique<DispatcherTests>());
	partest::addTestClass(partest::make_unique<XMLNodeTests>());
	partest::addTestClass(partest::make_unique<FlagTests>());

	partest::runAllTests();
	partest::displayAllTests();
	size_t assertions = partest::getAssertionFailureCount();
	size_t results = partest::getTopLevelFailures();

	std::cout << "Total assertion failures: " << assertions << std::endl;
	std::cout << "Total top-level failures: " << results << std::endl;

	return (int)results;
}