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
        os.write_white(indent + 1) << *(d.first) << "= ";
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

void TypeNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Type ";

    if (prog_type) {
        os << "[" << *prog_type << "]\n";
    }
    else {
        os << *symbol << "\n";
        for (const auto& a : args) {
            a->print(os, indent + 1);
        }
    }
};

void LambdaNode::print(OutputStream& os, unsigned indent)const {
    os.write_white(indent) << "Lambda ";
    print_type(os);
    os << "\n";
    for (const auto& a : args) {
        os.write_white(indent + 1) << a.first->name << " = ";
        a.second->print(os, indent + 1);
    }
    for (const auto& b : bindings) {
        os.write_white(indent + 1) << *b << "\n";
    }
    os.write_white(indent + 1) << "ret = ";
    ret_type->print(os, indent + 1);
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
    vtype->print(os, indent + 1);
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
    
    if (parents.size() > 0) {
        os << "extends";
        for (const auto& p : parents) {
            os << ' ' << *p;
        }
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


