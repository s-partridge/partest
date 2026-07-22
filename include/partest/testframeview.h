#ifndef PARTEST_TEST_FRAME_VIEW_H
#define PARTEST_TEST_FRAME_VIEW_H

#include <partest/testframe.h>

namespace partest
{
	class TestFrameView
	{
		const TestFrame &m_testFrame;

	public:
		TestFrameView(const TestFrame &testFrame) : m_testFrame(testFrame) {}
		
		static TestFrameView getNullTestFrameView() { return TestFrameView(TestFrame::getNullTestFrameInstance()); }

		unsigned id() const noexcept { return m_testFrame.id(); }
		unsigned parentId() const noexcept { return m_testFrame.parentId(); }
		const TestInfo &info() const noexcept { return m_testFrame.metadata; }
		const TestFlags &flags() const noexcept { return m_testFrame.flags; }
		const TestState &state() const noexcept { return m_testFrame.state; }

	};
}

#endif