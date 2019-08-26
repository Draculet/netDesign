#include <functional>
#include <memory>
#include <iostream>
using namespace std;
class Test;
typedef shared_ptr<Test> testptr;
typedef function<void (const testptr&)> func1;
typedef function<void (testptr)> func2;
class Test: public enable_shared_from_this<Test>
{
	public:
	Test(){cout << "Test()" << endl;}
	~Test(){cout << "~Test()" << endl;}
	void call()
	{
		ptr2(shared_from_this());
	}
	//类似无名临时对象,右值
	//ptr和ptr2两者无异,use_count都为2
	void ptr(const testptr &p)
	{
		cout << "====calling " << p.use_count() << "====" << endl;
	}
	void ptr2(testptr p)
	{
		cout << "====callin " << p.use_count() << "====" << endl;
	}
};


void testfunc(const testptr &p)
{
	cout << "testfunc()" << endl;
	cout << "====sec " << p.use_count() << "====" << endl;
}
void testfunc2(testptr p)
{
	cout << "testfunc()" << endl;
	cout << "====sec " << p.use_count() << "====" << endl;
}
int main(void)
{
	testptr p(new Test());
	//func2 fun = testfunc2;
	{
	auto fun = bind(testfunc2, p);
	//auto fun = bind(testfunc, p);
	cout << "====fir " << p.use_count() << "====" << endl;
	fun();
	}
	cout << "====thir " << p.use_count() << "====" << endl;
	cout << "====beforeCalling " << p.use_count() << "====" << endl;
	p->call();
}
