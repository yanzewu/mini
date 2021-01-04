#include "ast.h"

using namespace mini;

void ExprNode::print_type(OutputStream& os)const {
    if (prog_type) {
        os << "[" << *prog_type << "]";
    }
}


void ConstantNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Constant ";

    print_type(os);
    os << " " << value.to_str() << " " << info << "\n";
};

void VarNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Variable ";
    if (!ref) {
        os << *symbol;
    }
    else {
        os << *ref;
    }
    os.write_white(1);
    print_type(os);
    if (!ref) {
        os << " " << info;
    }
    os << "\n";
};

void TupleNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Tuple ";
    print_type(os);
    os << "\n";
    for (const auto& c : children) {
        c->print(os, indent + 1);
    }
}

void ArrayNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Array ";
    print_type(os);
    os << "\n";
    for (const auto& c : children) {
        c->print(os, indent + 1);
    }
}

void StructNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Struct ";
    print_type(os);
    os << "\n";
    for (const auto& d : children) {
        os.write_white(indent + 1) << *(d.first) << "= \n";
        d.second->print(os, indent + 1);
    }
};

void FunCallNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Call ";
    print_type(os);
    os << "\n";
    caller->print(os, indent + 1);
    for (const auto& a : args) {
        a->print(os, indent + 1);
    }
};

void GetFieldNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "GetField " << *field;
    print_type(os);
    os << '\n';
    lhs->print(os, indent + 1);
}

void NewNode::print(OutputStream& os, unsigned indent) const {
    os.write_white(indent) << "New ";
    if (!type_ref) os << *symbol;
    else os << *type_ref;

    os.write_white(1);
    print_type(os);

}

void TypeNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Type ";

    if (prog_type) {
        os << "[" << *prog_type << "]\n";
    }
    else {
        os << *symbol << "\n";
        if (quantifiers.size() > 0) {
            os.write_white(indent + 1) << "<";
            for (const auto& q : quantifiers) {
                os.write_white(indent + 1) << *q.first;
                if (q.second) {
                    os << ": \n";
                    q.second->print(os, indent + 1);
                }
                else {
                    os << "\n";
                }
            }
            os.write_white(indent + 1) << ">";
        }
        for (const auto& a : args) {
            a->print(os, indent + 1);
        }
    }
};

void TypeApplNode::print(OutputStream& os, unsigned indent) const {
    os.write_white(indent) << "TypeAppl ";

    print_type(os);
    os << "\n";

    lhs->print(os, indent + 1);
    for (const auto& a : args) {
        a->print(os, indent + 1);
    }
}

void LambdaNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Lambda ";
    print_type(os);
    os << "\n";
    if (quantifiers.size() > 0) {
        os.write_white(indent + 1) << "<";
        for (const auto& q : quantifiers) {
            os.write_white(indent + 1) << *q.first;
            if (q.second) {
                os << ": \n";
                q.second->print(os, indent + 1);
            }
            else {
                os << "\n";
            }
        }
        os.write_white(indent + 1) << ">";
    }
    for (const auto& a : args) {
        os.write_white(indent + 1) << a.first->name << " = \n";
        a.second->print(os, indent + 1);
    }
    for (const auto& b : bindings) {
        os.write_white(indent + 1) << *b << "\n";
    }
    os.write_white(indent + 1) << "ret = ";
    if (ret_type) ret_type->print(os, indent + 1);
    else os.write_white(indent + 1) << "auto";
    for (const auto& st : statements) {
        st->print(os, indent + 1);
    }
};

void LetNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Let ";
    if (ref) {
        os << *ref;
    }
    else {
        os << *symbol;
    }
    os << " \n";
    if (vtype) vtype->print(os, indent + 1);
    else os.write_white(indent + 1) << "auto";
    if (expr) {
        expr->print(os, indent + 1);
    }
};

void SetNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Set ";
    lhs->print(os, indent + 1);
    os << '\n';
    if (expr) {
        expr->print(os, indent + 1);
    }
};

void ClassNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Class " << *symbol;
    
    if (base) {
        os << "extends " << *base;
    }
    if (interfaces.size() > 0) {
        os << "implements";
        for (const auto& p : interfaces) {
            os << ' ' << *p;
        }
    }
    os << " \n";
    for (const auto& m : members) {
        m.first->print(os, indent + 1);
    }
};

void InterfaceNode::print(OutputStream& os, unsigned indent) const
{
    os.write_white(indent) << "Interface " << *symbol << '\n';
    for (const auto& m : members) {
        m->print(os, indent + 1);
    }
}

void ImportNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Import ";
    os << get_filename();
};

