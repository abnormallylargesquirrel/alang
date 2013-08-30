#include <iostream>
#include <vector>
#include <sstream>

template<class T>
struct str_to_num {
	T operator()(const std::string& str) {
		T num;
		std::stringstream ss(str);
		ss >> num;
		return num;
	}
};

class B;
class D;
class E;

void function(B *pB)
{
    std::cout << "functionB" << std::endl;
}

void function(D *pD)
{
    std::cout << "functionD" << std::endl;
}

void function(E *pE)
{
    std::cout << "functionE" << std::endl;
}

struct B {
    virtual void func() {function(this);}
};

struct D : public B {
    virtual void func() {function(this);}
};

struct E : public B {
    virtual void func() {function(this);}
};

int fib(int n)
{
    if(n < 2)
        return n;
    return fib(n - 1) + fib(n - 2);
}

int fib2(int n)
{
    int a = 0;
    int b = 1;
    int ret = 0;
    if(n < 2)
        return n;

    for(int i = 1; i < n; i++) {
        ret = a + b;
        a = b;
        b = ret;
    }

    return ret;
}

void function2(int n)
{
    std::cout << "int" << std::endl;
}

void function2(char n)
{
    std::cout << "char" << std::endl;
}

void function2(long n)
{
    std::cout << "long" << std::endl;
}

int main()
{
    //D *pD = new D;
    //reinterpret_cast<E*>(pD)->func();
    long a = 100000;
    char b = 100;

    function2(a + b);
    function2(b + a);

    return 0;
}
