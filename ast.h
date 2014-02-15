#ifndef AST_H
#define AST_H

//#include <cstdint>
#include <memory>
//#include <vector>
#include <sstream>
//#include <functional>
//#include <algorithm>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>
//#include "token.h"
//#include "jit_engine.h"
//#include "func_manager.h"
//#include "eval_t.h"
#include "ast_detail.h"
#include "utils.h"

struct inferencer;

class type_variable;
class type_operator;

typedef boost::variant<
    type_variable,
    boost::recursive_wrapper<type_operator>
> type;

class ast {
    public:
        ast(std::size_t t) : _tag(t) {}
        ast(const std::string& name, std::size_t t) : _node_str(name), _tag(t) {}

        //ast(const ast& other)
        //: _node_str(other._node_str), _node0(other._node0), _node1(other._node1), _tag(other._tag) {}

        virtual ~ast() {}

        virtual std::string node_str() const {return _node_str;}
        //virtual void set_str(std::string s) {_node_str = s;}

        /*int num_nodes() const
          {
          if(_node0 == nullptr && _node1 == nullptr)
          return 0;
          else if (_node0 == nullptr || _node1 == nullptr)
          return 1;
          else
          return 2;
          }*/

        void set_car(const shared_ast& c) {_node0 = c;}
        void set_cdr(const shared_ast& c) {_node1 = c;}
        void set_car(shared_ast&& c) {_node0 = std::move(c);}
        void set_cdr(shared_ast&& c) {_node1 = std::move(c);}

        //eval_t eval_type() const {return _eval_type;}
        //void set_eval_type(eval_t type) {_eval_type = type;}

        const shared_ast& car(void) const {return _node0;}
        const shared_ast& cdr(void) const {return _node1;}

        std::size_t tag(void) const {return _tag;}

        //virtual void clone(shared_ast& node) {node = std::make_shared<ast>(*this);}
        virtual type infer_type(inferencer& inf);

    private:
        std::string _node_str;
        shared_ast _node0;
        shared_ast _node1;
        std::size_t _tag;
        //std::vector<shared_ast> _nodes;
        //eval_t _eval_type;
};

inline std::ostream& operator<<(std::ostream& os, const shared_ast& n)
{
    return os << n->node_str();
}

class ast_cons : public ast {
    public:
        // what are these names?
        ast_cons(const shared_ast& e0, const shared_ast& e1)
            : ast(COMMENT_STR(cons), tags::tcons)
        {
            set_car(e0);
            set_cdr(e1);
        }

        ast_cons(shared_ast&& e0, shared_ast&& e1)
            : ast(COMMENT_STR(cons), tags::tcons)
        {
            set_car(std::move(e0));
            set_cdr(std::move(e1));
        }

        ast_cons(const ast& other)
            : ast(COMMENT_STR(cons), tags::tcons)
        {
            set_car(other.car());
            set_cdr(other.cdr());
        }

        virtual std::string node_str() const
        {
            if(car() == nullptr)
                return "()";

            std::ostringstream oss;
            oss << "(" << car()->node_str() << ", ";
            if(cdr() == nullptr) {
                oss << "nil)";
                return oss.str();
            } else {
                oss << cdr()->node_str() << ")";
                return oss.str();
            }
        }

        virtual type infer_type(inferencer& inf);
};

class ast_str : public ast {
    public:
        ast_str(const std::string& name)
            : ast(name, tags::tstr) {}

        virtual std::string node_str() const
        {
            return "\"" + ast::node_str() + "\"";
        }

        virtual type infer_type(inferencer& inf);
};

class ast_int : public ast { //tok::literal_int
    public:
        ast_int(const std::string& name)
            : ast(name, tags::tint) {}

        //virtual void clone(std::shared_ptr<ast_int>& node) {node = std::make_shared<ast_int>(*this);}
        virtual type infer_type(inferencer& inf);
};

class ast_float : public ast { //tok::literal_float
    public:
        ast_float(const std::string& name)
            : ast(name, tags::tfloat) {}

        //virtual void clone(std::shared_ptr<ast_float>& node) {node = std::make_shared<ast_float>(*this);}
        virtual type infer_type(inferencer& inf);
};

class ast_sym : public ast {
    public:
        ast_sym(const std::string& name)
            : ast(name, tags::tsym) {}

        //virtual void clone(std::shared_ptr<ast_sym>& node) {node = std::make_shared<ast_sym>(*this);}
        virtual type infer_type(inferencer& inf);
};

#endif
