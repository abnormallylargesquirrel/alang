#ifndef TOKEN_H
#define TOKEN_H

#include <sstream>

class tok {
public:
	static const int eof = -1;
	static const int nil = -2;
	static const int def = -3;
	static const int id = -4;
	static const int var = -5;
	static const int call = -6;
	static const int proto = -7;
    static const int decl = -8;
	static const int literal_int = -9;
	static const int literal_float = -10;
	static const int t_if = -11;
	static const int t_then = -12;
	static const int t_else = -13;
    static const int binop = -14;

	tok() : _type(nil), _literal_signed(false) {}
	tok(int type, std::string str) : _type(type), _str(str) {}
	
	int type() const {return _type;}
	void set_type(int t) {_type = t;}

    bool literal_signed() const {return _literal_signed;}
    void set_literal_signed(bool b) {_literal_signed = b;}

	//std::string& str() {return _str;}
	std::string str() const {return _str;}
    void set_str(const std::string& str) {_str = str;}

	std::string to_str() const
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
	}

private:
	int _type;
    bool _literal_signed;
	std::string _str;
};

#endif
