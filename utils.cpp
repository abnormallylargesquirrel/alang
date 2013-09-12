#include "utils.h"
#include "eval_t.h"

/*template<class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}*/

bool error_called = false;

std::nullptr_t error(const std::string& str)
{
    error_called = true;
	std::cout << "Error: " << str << std::endl;
	return nullptr;
}

bool is_binop(int c)
{
    char c2 = static_cast<char>(c);
    if(c2 == '+' || c2 == '-' || c2 == '*' || c2 == '/' || c2 == '<' || c2 == '>')
        return true;

    return false;
}

bool is_binop(const std::string& s)
{
    if(s == "==" || s == "<" || s == "<=" || s == ">" || s == ">=" ||
        s == "+" || s == "-" || s == "*" || s == "/")
        return true;

    return false;
}

bool is_cmp(int c)
{
    char c2 = static_cast<char>(c);
    if(c2 == '>' || c2 == '<' || c2 == '=')
        return true;

    return false;
}

/*bool is_toplvl(int t)
{
    if(t == tok::var || t == tok::call || t == tok::proto || t == tok::literal_int)
        return true;

    return false;
}*/
