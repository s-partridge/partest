#ifndef TYPE_TESTS_H
#define TYPE_TESTS_H

#include <string>

#include <partest/testbase.h>
#include <partest/types.h>

class FlagTests : public partest::TestBase
{
public:
	FlagTests() : TestBase("FlagTests", "Validation for TestFlags state and aggregation")
	{
		addTest("TestFlagConstruction", "Validate construction of TestFlags instances",
			partest::TEST_FLAGS_INHERIT,
			[this]() { this->testFlagConstruction(); });
		addTest("TestChaining", "Validate that flag state can be propagated via chain syntax",
			partest::TEST_FLAGS_INHERIT,
			[this]() { this->testChaining(); });
		addTest("TestInheritance", "Ensure flag state can be resolved by combining parent and child states",
			partest::TEST_FLAGS_INHERIT,
			[this]() { this->testChaining(); });

	}

	void testFlagConstruction()
	{
		partest::FlagState flags[5];

		partest::TestFlags ref;
		ref.skip = partest::FlagState::Disabled;
		ref.stopOnFail = partest::FlagState::Disabled;
		ref.stopSubtestOnFail = partest::FlagState::Disabled;
		ref.expectFailure = partest::FlagState::Disabled;
		ref.verbose = partest::FlagState::Disabled;

		// Validate disabled
		subtest("TestDefaultDisabled", [&](){
			for(unsigned idx = 0; idx < 5; ++idx)
				flags[idx] = partest::FlagState::Disabled;

			partest::TestFlags defaultFlags();
			ASSERT_EQUAL(defaultFlags, ref);

			partest::TestFlags disabledFlags(flags[0], flags[1], flags[2], flags[3], flags[4]);
			ASSERT_EQUAL(disabledFlags, ref);

			ASSERT_EQUAL(partest::TEST_FLAGS_DISABLED, ref);
		});

		// Enabled
		subtest("TestSetEnabled", [&](){
			ref.skip = partest::FlagState::Enabled;
			ref.stopOnFail = partest::FlagState::Enabled;
			ref.stopSubtestOnFail = partest::FlagState::Enabled;
			ref.expectFailure = partest::FlagState::Enabled;
			ref.verbose = partest::FlagState::Enabled;

			for(unsigned idx = 0; idx < 5; ++idx)
				flags[idx] = partest::FlagState::Enabled;

			partest::TestFlags defaultFlags();
			ASSERT_NOT_EQUAL(defaultFlags, ref);

			partest::TestFlags enabledFlags(flags[0], flags[1], flags[2], flags[3], flags[4]);
			ASSERT_EQUAL(enabledFlags, ref);
		});

		// Inherit
		subtest("TestSetInherit", [&](){
			ref.skip = partest::FlagState::Inherit;
			ref.stopOnFail = partest::FlagState::Inherit;
			ref.stopSubtestOnFail = partest::FlagState::Inherit;
			ref.expectFailure = partest::FlagState::Disabled;
			ref.verbose = partest::FlagState::Inherit;

			for(unsigned idx = 0; idx < 5; ++idx)
				flags[idx] = partest::FlagState::Inherit;

			partest::TestFlags inheritFlags(flags[0], flags[1], flags[2], partest::FlagState::Disabled, flags[4]);
			ASSERT_EQUAL(inheritFlags, ref);

			ASSERT_EQUAL(partest::TEST_FLAGS_INHERIT, ref);
		});
		// Masked
		subtest("TestSetMasked", [&](){
			ref.skip = partest::FlagState::Masked;
			ref.stopOnFail = partest::FlagState::Masked;
			ref.stopSubtestOnFail = partest::FlagState::Masked;
			ref.expectFailure = partest::FlagState::Masked;
			ref.verbose = partest::FlagState::Masked;

			for(unsigned idx = 0; idx < 5; ++idx)
				flags[idx] = partest::FlagState::Masked;

			partest::TestFlags maskedFlags(flags[0], flags[1], flags[2], flags[3], flags[4]);
			ASSERT_EQUAL(maskedFlags, ref);

			ASSERT_EQUAL(partest::TEST_FLAGS_MASKED, ref);
		});

		subtest("TestCopyAssignment", [&](){
			partest::TestFlags masked;

			masked = partest::TEST_FLAGS_MASKED;
			ASSERT_EQUAL(masked, partest::TEST_FLAGS_MASKED);
		});

		subtest("TestSkipped", [&]() {
			partest::TestFlags base = partest::TEST_FLAGS_DISABLED;
			base.skip = partest::FlagState::Enabled;

			ASSERT_EQUAL(base, partest::TEST_FLAGS_SKIP);
		});
	}

	void testChaining()
	{
		subtest("TestStopOnFail", [&](){
			partest::TestFlags ref = partest::TEST_FLAGS_INHERIT;
			ref.stopOnFail = partest::FlagState::Enabled;

			partest::TestFlags withStopOnFail = partest::TEST_FLAGS_INHERIT.withStopOnFail(partest::FlagState::Enabled);

			ASSERT_EQUAL(withStopOnFail, ref);
		});

		subtest("TestExpectFailure", [&](){
			partest::TestFlags ref = partest::TEST_FLAGS_INHERIT;
			ref.expectFailure = partest::FlagState::Enabled;

			partest::TestFlags withExpectFailure = partest::TEST_FLAGS_INHERIT.withExpectFailure(partest::FlagState::Enabled);

			ASSERT_EQUAL(withExpectFailure, ref);
		});

		
		subtest("TestMixing", [&](){
			partest::TestFlags ref = partest::TEST_FLAGS_INHERIT;
			ref.stopOnFail = partest::FlagState::Enabled;
			ref.expectFailure = partest::FlagState::Enabled;

			partest::TestFlags mixedA = partest::TEST_FLAGS_INHERIT.withExpectFailure(partest::FlagState::Enabled).withStopOnFail(partest::FlagState::Enabled);
			ASSERT_EQUAL(mixedA, ref);
			partest::TestFlags mixedB = partest::TEST_FLAGS_INHERIT.withStopOnFail(partest::FlagState::Enabled).withExpectFailure(partest::FlagState::Enabled);
			ASSERT_EQUAL(mixedB, ref);
		});
	}

	void testFlagResolution()
	{
		partest::TestFlags child = partest::TEST_FLAGS_INHERIT;
		partest::TestFlags parent = partest::TEST_FLAGS_DISABLED.withExpectFailure(partest::FlagState::Enabled);

		partest::TestFlags merged = child.mergeWithParentFlags(parent);

		// Ensure child is unmodified
		ASSERT_EQUAL(child, partest::TEST_FLAGS_INHERIT);
		// Ensure child matches parent
		ASSERT_EQUAL(merged, parent);

		partest::TestFlags intermediate = partest::TEST_FLAGS_INHERIT.withStopOnFail(partest::FlagState::Enabled);
		intermediate.verbose = partest::FlagState::Enabled;

		partest::TestFlags complexMerged = child.mergeWithParentFlags(intermediate.mergeWithParentFlags(parent));

		partest::TestFlags ref = partest::TEST_FLAGS_INHERIT.withStopOnFail(partest::FlagState::Enabled).withExpectFailure(partest::FlagState::Enabled);
		ref.verbose = partest::FlagState::Enabled;

		ASSERT_EQUAL(complexMerged, ref);
	}
};

#endif // TYPE_TESTS_H