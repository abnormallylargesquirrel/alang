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

/*void function(E *pE)
{
    std::cout << "functionE" << std::endl;
}*/

struct B {
    B(int n) {}
    virtual void func() {function(this);}
    std::vector<int> v;
};

struct D : public B {
    D(int n) : B(n) {v.push_back(0); v.push_back(1);}
    virtual void func() {function(this);}
};

struct E : public D {
    E(int n) : D(n) {}
    virtual void func() {function(this);}
};

int main()
{
    B *pE = new E(2);
    pE->func();

    return 0;
}
