#include <map>
#include <iostream>
using namespace std;

class Test
{
    public:
    Test()
    {
        cout << "build" << endl;
    }
    ~Test()
    {
        cout << "free" << endl;
    }
    Test(const Test&)
    {
        cout << "copy build" << endl;
    }
};
int main(void)
{
    Test tes;
    map<string, Test> m;
    m["a"] = tes;
    cout << "===begin===" << endl;
    m.erase("a");
    cout << "===End===" << endl;
    return 0;
}