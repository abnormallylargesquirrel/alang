#ifndef SYMBOL_H
#define SYMBOL_H

#include <memory>
#include <map>
#include <string>

class abs_type {
public:
	virtual ~abs_type() {}
	virtual std::string name() const = 0;
};

class sym {
public:
	sym(const std::string& name) : _name(name) {}
	sym(const std::string& name, const std::shared_ptr<abs_type>& type) : _name(name), _type(type) {}
	virtual ~sym() {}

	std::string name() const {return _name;}
private:
	std::string _name;
	std::shared_ptr<abs_type> _type;
};

class sym_var : public sym {
public:
	sym_var(const std::string& name, const std::shared_ptr<abs_type>& type) : sym(name, type) {}
};

class sym_type : public sym, public abs_type {
public:
	sym_type(const std::string& name) : sym(name) {}
	std::string name() const {return sym::name();}
};

class scope {
public:
	scope(const std::shared_ptr<scope>& enc, const std::string& name) : _name(name), _enclosing(enc)
	{
		init_types();
	}

	scope(const std::shared_ptr<scope>& enc) : _name(std::string()), _enclosing(enc)
	{
		init_types();
	}

	std::string name() const {return _name;}
	std::shared_ptr<scope> enclosing() const {return _enclosing;}
	void define(const std::shared_ptr<sym>& s) {_table[s->name()] = s;}
	std::shared_ptr<sym> resolve(const std::string& n)
	{
		if(_table.find(n) != _table.end()) {
			return _table[n];
		} else if(_enclosing != nullptr) {
			return _enclosing->resolve(n);
		} else {
			return nullptr;
		}
	}
private:
	void init_types()
	{
		define(std::make_shared<sym>(sym_type("int")));
	}

	std::map<std::string, std::shared_ptr<sym>> _table;
	std::shared_ptr<scope> _enclosing;
	std::string _name;
};

#endif