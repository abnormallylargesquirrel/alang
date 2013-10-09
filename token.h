#ifndef TOKEN_H
#define TOKEN_H

#include <sstream>

class tok {
public:
    //only used type in lexer/parser
	static const int eof = -1;
	static const int nil = -2;
	static const int def = -3;
	static const int id = -4;
	static const int var = -5;
	static const int call = -6;
	static const int anon_proto = -7;
    static const int decl = -8;
	static const int literal_int = -9;
	static const int literal_float = -10;
	static const int t_if = -11;
	static const int t_then = -12;
	static const int t_else = -13;
    static const int add = -14;
    static const int sub = -15;
    static const int mul = -16;
    static const int div = -17;
    static const int lt = -18;
    static const int gt = -19;
    static const int lte = -20;
    static const int gte = -21;
    static const int eq = -22;
    //static const int binop = -14;

	tok() : _type(nil) {}
	tok(int type, std::string str) : _type(type), _str(str) {}
    tok(const tok& t) : _type(t._type), _str(t._str) {}
    tok(const tok&& t) : _type(std::move(t._type)), _str(std::move(t._str)) {}

    tok& operator=(tok&& other)
    {
        //if(this != &other) {
        _type = std::move(other._type);
        _str = std::move(other._str);
        //}
        return *this;
    }
	
	int type() const {return _type;}
	void set_type(int t) {_type = t;}

	std::string str() const {return _str;}
    void set_str(const std::string& str) {_str = str;}

	/*std::string to_str() const
	{
		std::stringstream ss;
		ss << "<'" << _str << "',";
		switch(_type) {
		case eof:
			ss << "eof";
			break;
		case nil:
			ss << "nil";
			break;
		case def:
			ss << "def";
			break;
		case id:
			ss << "id";
			break;
		case var:
			ss << "var";
			break;
		case call:
			ss << "call";
			break;
		case proto:
			ss << "proto";
			break;
		case literal_int:
			ss << "literal_int";
			break;
		case literal_float:
			ss << "literal_float";
			break;
		case t_if:
			ss << "if";
			break;
		case t_then:
			ss << "then";
			break;
		case t_else:
			ss << "else";
			break;
        case binop:
            ss << _str;
            break;
		default:
			ss << static_cast<char>(_type);
			break;
		}
		ss << ">";
		return ss.str();
	}*/
    int prec() const
	{
		switch(_type) {
		case add: return 20;
		case sub: return 20;
		case mul: return 40;
		case div: return 40;
		case lt: return 10;
		case gt: return 10;
		case lte: return 10;
		case gte: return 10;
		case eq: return 5;
		default: return -1;
		}
	}

private:
	int _type;
	std::string _str;
};

#endif
