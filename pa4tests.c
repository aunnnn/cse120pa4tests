#include "aux.h"
#include "umix.h"
#include "mycode4.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Swap our functions with prof's version
#ifdef REF
#define MyInitThreads InitThreads
#define MyExitThread ExitThread
#define MyGetThread GetThread
#define MyCreateThread CreateThread
#define MyYieldThread YieldThread
#define MySchedThread SchedThread
#endif

// Use for test of result directly (if needed)
void MyTestAssert(int expression, const char *message, int LINE)
{
	if (!expression)
	{
		DPrintf("❌ ASSERTION FAILURE: %s\n", message);
		DPrintf("\n 🤔 See assertion at line %d (%s)\n---------\n", LINE, __FILE__);
		Exit();
	}
	else
	{
		DPrintf("✅ PASSED: %s\n", message);
	}
}

// (Preferred) Have both actual and expected so we can print and debug. Make sure to pass in correct order.
void MyTestAssertEqualInt(int actual, int expected, const char *message, int LINE)
{
	if (actual != expected)
	{
		DPrintf("❌ ASSERTION FAILURE: %s. (Actual = %d, Expected = %d)\n", message, actual, expected);
		DPrintf("\n 🤔 See assertion at line %d (%s)\n---------\n", LINE, __FILE__);
		Exit();
	}
	else
	{
		DPrintf("✅ PASSED: %s (= %d)\n", message, actual);
	}
}

void MyTestAssertEqualString(const char *actual, const char *expected, const char *message, int LINE)
{
	if (strcmp(actual, expected) != 0)
	{
		DPrintf("❌ ASSERTION FAILURE: %s. (Actual = \"%s\", Expected = \"%s\")\n", message, actual, expected);
		DPrintf("\n 🤔 See assertion at line %d (%s)\n---------\n", LINE, __FILE__);
		Exit();
	}
	else
	{
		DPrintf("✅ PASSED: %s (Both = \"%s\")\n", message, actual);
	}
}

#define ASSERT(expression, message) MyTestAssert(expression, message, __LINE__)
#define ASSERT_EQUAL(actual, expected, message) MyTestAssertEqualInt(actual, expected, message, __LINE__)
#define ASSERT_EQUAL_STR(actual, expected, message) MyTestAssertEqualString(actual, expected, message, __LINE__)

// Global var to store msg string with sprintf
char msg[200];

// Dummy task for threads
void printParam(int param)
{
	DPrintf("Print with param = %d\n", param);
}

// ********************************
// Test creating threads
// ********************************

// Test thread ids
void Test1()
{
	DPrintf("TEST: GetThread from main thread must return 0\n");

	MyInitThreads();

	int me = MyGetThread();
	ASSERT_EQUAL(me, 0, "Current main thread must have id = 0.");

	MyExitThread();
}

void Test2()
{
	DPrintf("TEST: Create thread 1 to 9, all must pass successfully and have correct IDs\n");

	MyInitThreads();

	int i;
	int tid;

	int NUM_THREADS_TO_CREATE = 9;
	for (i = 1; i <= NUM_THREADS_TO_CREATE; i++) // Create thread 1 to 9
	{
		tid = MyCreateThread(printParam, i);

		sprintf(msg, "Create new thread with correct id (expected = %d, actual = %d).", i, tid);

		ASSERT_EQUAL(tid, i, msg);
		ASSERT_EQUAL(MyGetThread(), 0, "Must still stay on thread 0 after create.");
	}
	MyExitThread();
}

void Test3()
{
	DPrintf("TEST: CreateThread must return -1 if exceed 10 threads\n");

	MyInitThreads();

	int i;
	int tid;

	int NUM_THREADS_TO_CREATE = 12;
	for (i = 1; i <= NUM_THREADS_TO_CREATE; i++) // Create new 9 threads
	{
		tid = MyCreateThread(printParam, i + 1);

		// First 9 threads should still be valid
		if (i <= 9)
		{
			continue;
		}
		// Start creating more than 9...
		sprintf(msg, "Create new thread should return -1 (actual = %d).", tid);

		ASSERT_EQUAL(tid, -1, msg);
		ASSERT_EQUAL(MyGetThread(), 0, "Must still stay on thread 0 after create.");
	}
	MyExitThread();
}

// This is thread 2
void Test4_Thread2YieldBackTo0After1And3Exit(int param)
{
	ASSERT_EQUAL(MyGetThread(), 2, "This must be thread 2.");

	printParam(2);
	int yielder;

	// Now after yield, 2 should be put back to end of queue.
	// I.e. [0,1,2,6,7,2]
	DPrintf("Test4: 2 Yielding back to 0...\n");
	yielder = MyYieldThread(0);

	// Then during this time 8,9,1,3 are created by 0: [0,4,5,6,7,2, <<< 8,9,1,3]
	// It continues one by one (FIFO order), and 7 must be the one who yields back to 2 on exit.
	ASSERT_EQUAL(yielder, -1, "Thread 7 must yield back to here to 2, but it yields at thread exit, so it must return -1.");
}

// Multiple things are tested here, not very clean...but it should pass
void Test4()
{
	DPrintf("TEST: Create thread 1 to 7. Then Thread 3 and 1 leave (by yielding to 3 and let both exits naturally), then 2 must get scheduled due to FIFO.\n");
	DPrintf("* Thread 2 then yields back to thread 0 to create new threads.\n");
	DPrintf("* Next two new thread IDs must continue at 8 and 9. Then another two must then reuse ID 1 and 3.\n");
	DPrintf("* Then all are scheduled with current FIFO order... 7 must exit and then 2 get to continue, MyYieldThread must return -1.\n");
	MyInitThreads();

	int i;
	int tid;

	int NUM_THREADS_TO_CREATE = 7;
	for (i = 1; i <= NUM_THREADS_TO_CREATE; i++) // Create new 9 threads
	{
		if (i == 2)
		{
			// If thread 2, must yield back to 0 to continue the test after 3 and 1 exit
			tid = MyCreateThread(Test4_Thread2YieldBackTo0After1And3Exit, i);
		}
		else
		{
			tid = MyCreateThread(printParam, i);
		}
		sprintf(msg, "Create new thread with correct id (expected = %d, actual = %d).", i, tid);
		ASSERT_EQUAL(tid, i, msg);
	}

	// Now FIFO is [0,1,2,3,4,5,6,7]
	// 0 Yield to 3 (3 to front, 0 to back): [3,1,2,4,5,6,7,0],
	// It then prints and exit: [1,2,4,5,6,7,0]
	//
	// Next it will be 1 who get scheduled, again it prints and exit.
	// Then 2 will get which will yield back here: [2,4,5,6,7,0]
	int yieldingThread = MyYieldThread(3);
	ASSERT_EQUAL(yieldingThread, 2, "Thread 2 must yield back.");

	// Here now is [0,4,5,6,7,2]

	// Create two more, IDs must be 8 and 9
	tid = MyCreateThread(printParam, 8);
	sprintf(msg, "New thread must have ID = 8 (actual = %d).", tid);
	ASSERT_EQUAL(tid, 8, msg);

	tid = MyCreateThread(printParam, 9);
	sprintf(msg, "New thread must have ID = 9 (actual = %d).", tid);
	ASSERT_EQUAL(tid, 9, msg);

	// Then another two must be 1 and 3 (reusing IDs)
	tid = MyCreateThread(printParam, 1);
	sprintf(msg, "New thread must have ID = 1 (actual = %d).", tid);
	ASSERT_EQUAL(tid, 1, msg);

	tid = MyCreateThread(printParam, 3);
	sprintf(msg, "New thread must have ID = 3 (actual = %d).", tid);
	ASSERT_EQUAL(tid, 3, msg);

	// Now it's [0,4,5,6,7,2, <<<<  8,9,1,3] (i.e. 0 creates 8,9,1,3);
	// Next it should go in order of FIFO above until all are finished...
	MyExitThread();
}

// ********************************
// Test yielding threads etc.
// ********************************

int Test5_ParameterCheckIsCalled = 0;
void Test5_ParameterCheck(int param)
{
	Test5_ParameterCheckIsCalled = 1;
	ASSERT_EQUAL(param, 999, "param must be 999.");
	ASSERT_EQUAL(MyGetThread(), 1, "Current thread must be 1.");
}

void Test5()
{
	DPrintf("TEST: Correct parameter is passed to thread's function.\n");

	MyInitThreads();

	int tid = MyCreateThread(Test5_ParameterCheck, 999);

	ASSERT_EQUAL(tid, 1, "tid must be 1.");
	ASSERT_EQUAL(Test5_ParameterCheckIsCalled, 0, "Test5_ParameterCheck must not be called yet.");

	int yielder = MyYieldThread(1);

	ASSERT_EQUAL(yielder, -1, "Thread 1 must run to exit and return -1 here.");
	ASSERT_EQUAL(Test5_ParameterCheckIsCalled, 1, "Test5_ParameterCheck must be called.");
}

void Test6()
{
	DPrintf("TEST: Yield to illegal IDs must return -1.\n");

	MyInitThreads();

	ASSERT_EQUAL(MyYieldThread(-1), -1, "Yield to thread -1 must return -1.");
	ASSERT_EQUAL(MyYieldThread(10), -1, "Yield to thread 10 must return -1.");
	ASSERT_EQUAL(MyYieldThread(1), -1, "Yield to thread 1 (invalid) must return -1.");
}

void Test7()
{
	DPrintf("TEST: Yield to 0 (self) must return 0.\n");

	MyInitThreads();
	ASSERT_EQUAL(MyYieldThread(0), 0, "Yield to thread 0 (self) must return 0.");
}

void Test8_YieldBackToThread0(int param)
{
	// Immediately yield back
	int yielder = MyYieldThread(0);
	ASSERT_EQUAL(yielder, -1, "Yielder must be -1 as all are exiting in FIFO order...");
}

void Test8()
{
	DPrintf("TEST: Thread 0 creates 9 new threads (id = 1-9) (twice). Each will yield back to 0 must return their correct IDs.\n");

	MyInitThreads();
	int i, tid, expectedId;

	for (i = 1; i <= 9; i++)
	{
		expectedId = i;
		tid = MyCreateThread(Test8_YieldBackToThread0, 0); // thread i

		ASSERT_EQUAL(tid, expectedId, "Thread Id must be correct.");
		sprintf(msg, "Newly created thread %d must yield back it thread 0.", expectedId);
		ASSERT_EQUAL(MyYieldThread(tid), expectedId, msg);

		// if (expectedId == 0) // Thread 0 can't be created since it's running the test here
		// {
		// 	ASSERT_EQUAL(tid, -1, "Should not be able to create thread 0 since it is running the test!");
		// } else {
		// 	ASSERT_EQUAL(tid, expectedId, "Thread Id must be correct.");

		// 	sprintf(msg, "Newly created thread %d must yield back it thread 0.", expectedId);
		// 	ASSERT_EQUAL(MyYieldThread(tid), expectedId, msg);
		// }
	}
	MyExitThread();
}

void Test9_YieldRoundtrip(int tid)
{
	sprintf(msg, "Current thread must be %d", tid);
	ASSERT_EQUAL(MyGetThread(), tid, msg);

	int yielder;

	// If 9, yield back to 8 and that's it.
	if (tid == 9)
	{
		// Reverse: 9->8 (-> ...)
		yielder = MyYieldThread(8);
		// When it comes back here 0 exits and 9 get scheduled next (so it should return -1)
	}
	else
	{
		// Thread 1...8 yield to next one
		// 1->2->...->8->9
		yielder = MyYieldThread(tid + 1);

		// Here it got yield back (i.e. 8 from 9 above), just continue to yield back i.e.:
		// 8->7->...->0

		sprintf(msg, "Thread %d must yield back to current thread (%d).", tid + 1, MyGetThread());
		ASSERT_EQUAL(yielder, tid + 1, msg);

		yielder = MyYieldThread(tid - 1); // Finally this will hit back at 0 and 0 exits. Will return -1 all here.
	}

	sprintf(msg, "Yielder to current thread %d must be -1.", MyGetThread());
	ASSERT_EQUAL(yielder, -1, msg);
}

void Test9()
{
	DPrintf("TEST: Yield 0 -> 1 -> 2 -> ... -> 9 then back 9 -> 8 -> ... -> 0. Finally let all exits in FIFO.\n");

	MyInitThreads();

	int i;
	for (i = 1; i <= 9; i++)
	{
		MyCreateThread(Test9_YieldRoundtrip, i); // thread i
	}

	int yielder = MyYieldThread(1);

	ASSERT_EQUAL(yielder, 1, "Thread 1 must yield back to 0.");

	// Then 0 exits and let all exits...
	MyExitThread();
}

void Test10_ThreadParamShouldBeSquaredOfThreadID(int param)
{
	int tid = MyGetThread();
	ASSERT_EQUAL(param, tid * tid, "Param should be squared of current thread ID.");
}

void Test10()
{
	DPrintf("TEST: Multiple params are set correctly for each thread (param = tid^2).\n");
	MyInitThreads();

	int i;
	int tid;

	int NUM_THREADS_TO_CREATE = 9;
	for (i = 1; i <= NUM_THREADS_TO_CREATE; i++) // Create thread 1 to 9
	{
		MyCreateThread(Test10_ThreadParamShouldBeSquaredOfThreadID, i * i);
		MyYieldThread(i);
	}
}

void Test11_Create9Threads(int isMasterThread)
{
	// The master thread will drive the testing
	if (isMasterThread)
	{
		DPrintf("Test11: Thread %d is master.\n", MyGetThread());

		int i, tid, expectedID;

		// Example: If current master is 3, it will create 4, 5, 6, ..., 9, 0, 1, and 2.

		// Loop through 10 threads
		for (i = 0; i < 10; i++)
		{
			tid = MyCreateThread(Test11_Create9Threads, 0); // New slave threads shouldn't create threads yet, pass 0.

			expectedID = (MyGetThread() + i + 1) % 10;

			if (expectedID == MyGetThread())
			{
				continue; // Skip if expectedID is current thread (master)
			}

			ASSERT_EQUAL(tid, expectedID, "Must have correct thread ID.");
		}

		// Current master is done. Time to clean up and transfer to next master.

		if (MyGetThread() < 9)
		{
			// Yield to next one, which then will clean up dummy threads and get back to it
			ASSERT_EQUAL(MyYieldThread(MyGetThread() + 1), -1, "Yield must return -1.");

			// Now assign the same work to the next thread (cur + 1)
			// Pass 1 to let it be the master and does the same work.
			tid = MyCreateThread(Test11_Create9Threads, 1);

			ASSERT_EQUAL(tid, MyGetThread() + 1, "Next master thread must have correct thread ID.");
		}
		else
		{
			// Here current thread is 9, no more master.
		}
	}
	else
	{
		// Act as a slave, do nothing
	}

	// Current master exits here and let the next master does the work...
}

// Comprehensive create threads testing
void Test11()
{
	DPrintf("TEST: Each thread takes turn to create the rest of 9 threads with correct IDs.\nI.e. thread 0 creates 1-9. Thread 1 creates 2-9,0 etc.\n");
	MyInitThreads();

	// Start off with thread 0.
	Test11_Create9Threads(1);
	MyExitThread();
}

// ********************************
// Test MySchedThread
// ********************************

void Test12()
{
	DPrintf("TEST: MySchedThread with only thread 0 should not break, it should go back to self\n");
	MyInitThreads();
	MySchedThread(); // Should not break, return back here
	ASSERT(1, "Test is passed.");
}

// ********************************
// Test13:
// * MyYieldThread should return -1 if get control back via MySchedThread.
// Note that new threads haven't call MyYieldThread yet and it probably get its first control in my create thread.
//
// Checking if a yielding thread isn't valid wasn't enough.
// You could pass Test 1 to 12 and fail 13 without knowing.
// * Also run all tests to make sure your fix for this doesn't break others!
//
// It's a requirement that (kinda) doesn't break other main functionalities, but tricky.
// ********************************

void Test13_YieldBackTo0()
{
	// 2. Yield back to 0...
	int yielder = MyYieldThread(0);

	// 5. 0 called MySchedThread (second time), it should get back control with -1.
	ASSERT_EQUAL(yielder, -1, "Yielder must be -1 as it gets CPU from MySchedThread (invoked by thread 0).");
}

void Test13()
{
	DPrintf("TEST: MySchedThread from 0 to 1 must return -1 on MyYieldThread.\n");
	MyInitThreads();

	MyCreateThread(Test13_YieldBackTo0, 0); // thread 1

	// 1. Thread 1 should get it
	MySchedThread();

	// 3. Thread 1 yields back and continues here...

	// 4. Go to thread 1 again
	MySchedThread();

	// 6. Done.
	MyExitThread();
}

void Test14_MySchedThreadToSelf(int param)
{
	// 2. Thread 1 (here) should be able to do MySchedThread without problem.
	ASSERT_EQUAL(MyGetThread(), 1, "Current thread is 1.");
	MySchedThread();
	ASSERT_EQUAL(MyGetThread(), 1, "Current thread is still 1 after MySchedThread().");
}

void Test14()
{
	DPrintf("TEST: MySchedThread with only thread 1 should not break, it should go back to self\n");
	MyInitThreads();

	MyCreateThread(Test14_MySchedThreadToSelf, 1);
	// 1. 0 exits, and let thread 1 do MySchedThread on itself.
	MyExitThread();
}

static size_t Test15_Counter = 0;
static int Test15_ThreadOrderRecord[18] = {0};

void Test15_JustCallSchedThreadAndRecord(int param)
{
	Test15_ThreadOrderRecord[Test15_Counter] = MyGetThread();
	Test15_Counter += 1;

	MySchedThread();

	Test15_ThreadOrderRecord[Test15_Counter] = MyGetThread();
	Test15_Counter += 1;

	if (Test15_Counter > 18)
	{
		ASSERT(0, "Counter should never go beyond 18.");
	}
}

void Test15()
{
	DPrintf("TEST: MySchedThread with simple FIFO, will schedule next one in chain.\n");

	Test15_Counter = 0;

	MyInitThreads();

	int i;
	for (i = 1; i <= 9; i++) // Create thread 1 to 9
	{
		MyCreateThread(Test15_JustCallSchedThreadAndRecord, i);
	}

	// Yield to 1 and start chain of sched thread, i.e.:
	// [1, 2, 3, 4, 5, 6, 7, 8, 9, 0]
	// [2, 3, 4, 5, 6, 7, 8, 9, 0, 1]
	// [3, 4, 5, 6, 7, 8, 9, 0, 1, 2]
	// ...
	// [9, 0, 1, 2, 3, 4, 5, 6, 7, 8]
	// [0, 1, 2, 3, 4, 5, 6, 7, 8, 9] <- Back to 0
	int yielder = MyYieldThread(1);
	ASSERT_EQUAL(yielder, -1, "Must get back to here with -1.");

	MySchedThread(); // Go to 1 and let all runs, until back here...
	// I.e.
	// [1, 2, 3, 4, 5, 6, 7, 8, 9, 0]
	// [2, 3, 4, 5, 6, 7, 8, 9, 0]
	// [3, 4, 5, 6, 7, 8, 9, 0]
	// ...
	// [9, 0]
	// [0] ...Continue below

	// Start checking the order below.
	int Test15_ExpectedOrder[18] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	ASSERT_EQUAL(Test15_Counter, 18, "Counter must be 18 here.");

	for (i = 0; i < Test15_Counter; i++)
	{
		ASSERT_EQUAL(Test15_ThreadOrderRecord[i], Test15_ExpectedOrder[i], "Correct order.");
	}
	MyExitThread();
}

static int Test16_SetupPhase = 1;
static size_t Test16_Counter = 0;
static int Test16_ThreadOrderRecord[9];

void Test16_DummyThread(int param)
{
	if (Test16_SetupPhase)
	{
		// Always back to 0
		int yielder = MyYieldThread(0);
		ASSERT_EQUAL(yielder, -1, "Must be -1 as no one explicit yields to it.");
	}

	Test16_ThreadOrderRecord[Test16_Counter] = MyGetThread();
	Test16_Counter++;
	if (Test16_Counter > 9)
	{
		ASSERT(0, "Counter should never go beyond 9.");
	}
}

// Test some specific order of exits
void Test16()
{
	DPrintf("TEST: Threads exit in FIFO order after setup (2, 6, 8, 5, 7, 3, 9, 1, 4).\n");

	MyInitThreads();

	Test16_SetupPhase = 1;

	int i;
	for (i = 1; i <= 9; i++) // Create thread 1 to 9
	{
		MyCreateThread(Test16_DummyThread, i);
	}

	// Order the thread queue
	// ----------------------

	MyYieldThread(5);
	// [0, 1, 2, 3, 4, 6, 7, 8, 9, 5]

	MyYieldThread(7);
	// [0, 1, 2, 3, 4, 6, 8, 9, 5, 7]

	MyYieldThread(3);
	// [0, 1, 2, 4, 6, 8, 9, 5, 7, 3]

	MyYieldThread(9);
	// [0, 1, 2, 4, 6, 8, 5, 7, 3, 9]

	MyYieldThread(1);
	// [0, 2, 4, 6, 8, 5, 7, 3, 9, 1]

	MyYieldThread(4);
	// [0, 2, 6, 8, 5, 7, 3, 9, 1, 4]*

	// Turn-off setup (So it doesn't yield back to here)
	Test16_SetupPhase = 0;

	// Finish all threads, back to 0
	MySchedThread();

	int Test16_ExpectedOrder[9] = {2, 6, 8, 5, 7, 3, 9, 1, 4};

	ASSERT_EQUAL(Test16_Counter, 9, "Counter must be 9 here.");

	for (i = 0; i < Test16_Counter; i++)
	{
		ASSERT_EQUAL(Test16_ThreadOrderRecord[i], Test16_ExpectedOrder[i], "Correct order.");
	}

	MyExitThread();
}

// ********************************
// 	Test17 is pa4c
// ********************************
// Test by compare strings directly

#define NUMYIELDS 5

static int Test17_Counter = 0;
static const char *Test17_ExpectedOutput[15] = {
	"T2: 0 cubed = 0",
	"T1: 0 squared = 0",
	"T0: square = 0, cube = 0",
	"T2: 1 cubed = 1",
	"T1: 1 squared = 1",
	"T0: square = 1, cube = 1",
	"T2: 2 cubed = 8",
	"T1: 2 squared = 4",
	"T0: square = 4, cube = 8",
	"T2: 3 cubed = 27",
	"T1: 3 squared = 9",
	"T0: square = 9, cube = 27",
	"T2: 4 cubed = 64",
	"T1: 4 squared = 16",
	"T0: square = 16, cube = 64"};

static int square, cube; // global variables, shared by threads

void printSquares(int t)
// t: thread to yield to
{
	int i;

	for (i = 0; i < NUMYIELDS; i++)
	{
		square = i * i;
		// Printf("T%d: %d squared = %d\n", MyGetThread(), i, square);
		sprintf(msg, "T%d: %d squared = %d", MyGetThread(), i, square);

		const char *actual = &msg[0];
		const char *expected = Test17_ExpectedOutput[Test17_Counter];

		ASSERT_EQUAL_STR(actual, expected, "Output string must be the same.");
		Test17_Counter++;

		MyYieldThread(t);
	}
}

void printCubes(int t)
// t: thread to yield to
{
	int i;

	for (i = 0; i < NUMYIELDS; i++)
	{
		cube = i * i * i;
		// Printf("T%d: %d cubed = %d\n", MyGetThread(), i, cube);
		sprintf(msg, "T%d: %d cubed = %d", MyGetThread(), i, cube);

		const char *actual = &msg[0];
		const char *expected = Test17_ExpectedOutput[Test17_Counter];

		ASSERT_EQUAL_STR(actual, expected, "Output string must be the same.");
		Test17_Counter++;

		MyYieldThread(t);
	}
}

void Test17()
{
	int i, t, me;
	void printSquares(), printCubes();

	MyInitThreads();

	me = MyGetThread();
	t = MyCreateThread(printSquares, me);
	t = MyCreateThread(printCubes, t);

	for (i = 0; i < NUMYIELDS; i++)
	{
		MyYieldThread(t);
		// Printf("T%d: square = %d, cube = %d\n", me, square, cube);
		sprintf(msg, "T%d: square = %d, cube = %d", me, square, cube);

		const char *actual = &msg[0];
		const char *expected = Test17_ExpectedOutput[Test17_Counter];

		ASSERT_EQUAL_STR(actual, expected, "Output string must be the same.");
		Test17_Counter++;
	}

	ASSERT_EQUAL(Test17_Counter, 15, "Counter must be 15.");
	MyExitThread();
}

void Main()
{
#ifdef REF
	DPrintf("***** Using REF Version *****\n");
#else
	DPrintf("***** Using My Version *****\n");
#endif

	void (*func_ptr[20])() = {
		Test1, Test2, Test3, Test4,
		Test5, Test6, Test7, Test8,
		Test9, Test10, Test11, Test12,
		Test13, Test14, Test15, Test16,
		Test17};

	char *N = getenv("N");

	if (N == NULL)
	{
		DPrintf("You must provide N.\n");
		Exit();
	}
	int Nint = atoi(N);
	if (Nint <= 0 || Nint > sizeof(func_ptr) / sizeof(func_ptr[0])) // Probably a wrong check but whatever
	{
		DPrintf("Invalid N = %d.\n", Nint);
		Exit();
	}

	DPrintf("N is %d.\n", Nint);

	// Run the selected test
	(*func_ptr[Nint - 1])();

	Exit();
}
