#ifndef ATTRIBUTOR_H
#define ATTRIBUTOR_H

#include "ast.h"
#include "symtable.h"
#include "type.h"
#include "errors.h"
#include "builtin.h"

namespace mini {

    // Semantic analysis for AST
    class Attributor {

    public:

        Attributor() {}

        void process(const std::vector<pAST>& nodes, SymbolTable& sym_table, ErrorManager* error_manager=NULL) {
            this->symbol_table = &sym_table;
            
            for (const auto& node : nodes) {
                try {
                    process_node(node);
                }
                catch (const ParsingError& e) {
                    if (error_manager && !error_manager->has_enough_errors()) {
                        error_manager->count_and_print_error(e);
                    }
                    else {
                        throw;
                    }
                }
            }
            this->symbol_table = nullptr;
        }

        void process_node(const pAST& node) {
            if (node->is_expr()) {
                process_expr(std::static_pointer_cast<ExprNode>(node));
            }
            else if (node->get_type() == AST::LET) {
                process_let(ast_cast<LetNode>(node));
            }
            else if (node->get_type() == AST::SET) {
                process_set(ast_cast<SetNode>(node));
            }
            else if (node->get_type() == AST::INTERFACE) {
                process_interface(ast_cast<InterfaceNode>(node));
            }
            else if (node->get_type() == AST::CLASS) {
                process_class(ast_cast<ClassNode>(node));
            }
            else {
                throw std::runtime_error("Incorrect AST type");
            }
        }

        void process_type(const Ptr<TypeNode>& m_node) {

            // for universal type
            if (m_node->quantifiers.size() > 0) {
                process_universal_type(m_node);
            }
            else {
                process_concrete_type(m_node);
            }
        }
            
        void process_concrete_type(const Ptr<TypeNode>& m_node) {

            auto [ref, stack_id] = symbol_table->find_type(m_node->symbol.get());  // ref,stack

            if (!ref) {
                m_node->get_info().throw_exception("Undefined type: " + m_node->symbol->name);
            }
            else if (ref->is_primitive()) {
                auto rt = ref->as<PrimitiveTypeMetaData>();
                if (rt->min_arg() > m_node->args.size() && rt->max_arg() < m_node->args.size()) {
                    m_node->get_info().throw_exception("Incorrect type parameter number");
                }
                std::vector<pType> type_args;
                for (const auto& a : m_node->args) {
                    process_type(a);
                    type_args.push_back(a->prog_type);
                }
                m_node->prog_type = std::make_shared<PrimitiveType>(rt, type_args);
            }
            else if (ref->is_interface()) {
                m_node->prog_type = ref->as<InterfaceTypeMetaData>()->actual_type;
            }
            else if (ref->is_variable()) {
                // We do not allow type variable take argument. It would be a F-omega extension.
                if (m_node->args.size() > 0) m_node->get_info().throw_exception("Type variable cannot take arguments");
                auto rt = ref->as<TypeVariableMetaData>();
                m_node->prog_type = std::make_shared<UniversalTypeVariable>(stack_id, rt->arg_id, rt->quantifier.get());
            }
            else if (ref->is_object()) {
                m_node->prog_type = std::make_shared<ObjectType>(ref->as<ObjectTypeMetaData>());
            }
            else {
                // TODO advanced types
                throw std::runtime_error("ref not defined");
            }
        }

        void process_universal_type(const Ptr<TypeNode>& m_node) {
            
            auto t = std::make_shared<UniversalType>();
            process_quantifiers(m_node->quantifiers, t);
            process_concrete_type(m_node);    // TODO in F-omega t->body may not be concrete.
            t->body = m_node->prog_type;
            pType t1 = std::static_pointer_cast<Type>(t);
            m_node->prog_type.swap(t1);

            symbol_table->pop_type_scope();
        }

        // Will push type scope.
        void process_quantifiers(const std::vector<std::pair<pSymbol, Ptr<TypeNode>>>& quantifiers, const Ptr<UniversalType>& t) {
            for (const auto& q : quantifiers) {
                if (!q.second) continue;

                process_type(q.second);
                if (!q.second->prog_type->qualifies_interface()) {
                    q.second->get_info().throw_exception(StringAssembler("Not an interface: ")(*q.second->prog_type)());
                }
            }
            symbol_table->push_type_scope();

            for (size_t i = 0; i < quantifiers.size(); i++) {
                auto q = quantifiers[i];
                if (q.second) {
                    symbol_table->insert_type_var(q.first, i, q.second->prog_type);
                    t->quantifiers.push_back(q.second->prog_type);
                }
                else {
                    auto mytop = std::make_shared<PrimitiveType>(symbol_table->find_type("@Addressable"));
                    symbol_table->insert_type_var(q.first, i, mytop);
                    t->quantifiers.push_back(mytop);
                }
            }
        }

        void process_expr(const Ptr<ExprNode>& m_node) {
            switch (m_node->get_type())
            {
            case AST::CONSTANT: process_constant(ast_cast<ConstantNode>(m_node)); break;
            case AST::VAR: process_var(ast_cast<VarNode>(m_node)); break;
            case AST::TUPLE: process_tuple(ast_cast<TupleNode>(m_node)); break;
            case AST::STRUCT: process_struct(ast_cast<StructNode>(m_node)); break;
            case AST::ARRAY: process_array(ast_cast<ArrayNode>(m_node)); break;
            case AST::FUNCALL: process_funcall(ast_cast<FunCallNode>(m_node)); break;
            case AST::GETFIELD: process_getfield(ast_cast<GetFieldNode>(m_node)); break;
            case AST::NEW: process_new(ast_cast<NewNode>(m_node)); break;
            case AST::TYPEAPPL: process_typeappl(ast_cast<TypeApplNode>(m_node)); break;
            case AST::LAMBDA: process_lambda(ast_cast<LambdaNode>(m_node)); break;
            default:
                break;
            }
        }

        void process_constant(ConstantNode* m_node) {
            switch (m_node->value.get_type())
            {
            case Constant::Type_t::NIL: m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("nil")); break;
            case Constant::Type_t::BOOL: m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("bool")); break;
            case Constant::Type_t::CHAR: m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("char")); break;
            case Constant::Type_t::INT: m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("int")); break;
            case Constant::Type_t::FLOAT: m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("float")); break;
            case Constant::Type_t::STRING: m_node->prog_type = PrimitiveTypeBuilder("array")("char")(*symbol_table); break;
            default:
                break;
            }
        }

        void process_var(VarNode* m_node) {
            VariableRef ref = symbol_table->find_var(m_node->symbol.get());
            if (!ref) {
                m_node->get_info().throw_exception("Undefined variable: " + m_node->symbol->name);
            }
            else {
                m_node->prog_type = ref->prog_type;
                
                if (!ref->has_assigned) {
                //    node->cur_info().throw_exception("Variable " + node->symbol->symbol + "has been used before assignment");
                    // not sure if an exception is necessary
                }

                // In a closure, we record the appearance of a local variable outside current scope since
                // it may have side-effects. Calling f[bindings](args) is equivalent of calling
                // f_naked(bindings)(args) where f_naked(bindings) returns f with all binding variables filled.
                // For a static language, what we store is f_naked. Another advantage is that we can allocate these
                // bound variables separately (usually useful for language without GC)
                if (ref->scope != 0 && ref->scope != symbol_table->cur_scope() && binding_cache.size() > 0) {
                    auto r = binding_cache.back().find(ref);
                    if (r == binding_cache.back().end()) {
                        // create new ref assign to r
                        auto new_binding_var = symbol_table->insert_var(
                            ref->symbol, VarMetaData::Source::BINDING, ref->prog_type);
                        binding_cache.back().insert({ ref, new_binding_var });
                        m_node->set_ref(new_binding_var);
                    }
                    else {  // already referenced as binding variable
                        m_node->set_ref(r->second);
                    }
                }
                else {  // local/global/arg
                    m_node->set_ref(ref);
                }
            }
        }

        void process_tuple(TupleNode* m_node) {

            if (m_node->children.size() == 0) {
                m_node->get_info().throw_exception("Tuple cannot be empty");
            }

            std::vector<Ptr<Type>> type_args;
            for (auto& c : m_node->children) {
                process_expr(c);
                type_args.push_back(c->prog_type);
            }
            m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("tuple"), type_args);
        }

        void process_struct(StructNode* m_node) {

            std::unordered_map<std::string, pType> fields;

            for (auto& c : m_node->children) {
                process_expr(c.second);
                auto r = fields.insert({ c.first->name, c.second->get_prog_type() });
                if (!r.second) {
                    c.first->get_info().throw_exception("Field redefinition: " + c.first->get_name());
                }

            }
            if (m_node->children.size() == 0) {
                m_node->prog_type = std::make_shared<StructType>();
            }
            else {
                m_node->prog_type = std::make_shared<StructType>(fields);
            }
        }

        void process_array(ArrayNode* m_node) {

            pType maximum_type = std::make_shared<PrimitiveType>(symbol_table->find_type("bottom"));

            if (m_node->children.empty()) {
            }
            else {  // type inference
                process_expr(m_node->children[0]);
                maximum_type = m_node->children[0]->prog_type;

                for (size_t i = 1; i < m_node->children.size(); i++) {
                    process_expr(m_node->children[i]);
                    auto result_l_to_r = maximum_type->partial_compare(m_node->children[i]->prog_type.get());
                    if (result_l_to_r == Type::Ordering::GREATER || result_l_to_r == Type::Ordering::EQUAL) {
                    }
                    else {  // maybe less?
                        auto result_r_to_l = m_node->children[i]->prog_type->partial_compare(maximum_type.get());
                        if (result_r_to_l == Type::Ordering::GREATER) {
                            maximum_type = m_node->children[i]->prog_type;
                        }
                        else {
                            m_node->get_info().throw_exception("Type not compatible");
                            // The ideal maximum type would be the super type of both. However, finding such a type
                            // may be undecideable (in System F) and is not very meaningful, so I just throw here.
                        }
                    }
                }
            }
            m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("array"), std::vector<pType>({ maximum_type }));
        }

        void process_typeappl(TypeApplNode* m_node) {
            process_expr(m_node->lhs);

            if (!m_node->lhs->prog_type->is_universal()) {
                m_node->get_info().throw_exception("Universal type required");
            }
            auto utype = m_node->lhs->prog_type->as<UniversalType>();

            std::vector<pType> quantifiers;

            for (auto& arg : m_node->args) {
                process_type(arg);
                quantifiers.push_back(arg->prog_type);
            }
            
            m_node->prog_type = utype->instanitiate(quantifiers, m_node->get_info());
        }

        void process_funcall(FunCallNode* m_node) {

            process_expr(m_node->caller);
            const auto& func_type_1 = m_node->caller->prog_type;
            if (!func_type_1->is_primitive() || !func_type_1->as<PrimitiveType>()->is_function()) {
                m_node->get_info().throw_exception("Function type required");
            }
            auto func_type = func_type_1->as<PrimitiveType>();

            if (func_type->args.size() - 1 != m_node->args.size()) {
                m_node->get_info().throw_exception(StringAssembler("Expect ")(func_type->args.size() - 1)(" args, got ")(m_node->args.size())());
            }
            for (size_t i = 0; i < m_node->args.size(); i++) {
                process_expr(m_node->args[i]);
                match_type(func_type->args[i], m_node->args[i]->prog_type, m_node->args[i]->get_info());
            }
            m_node->prog_type = func_type->args.back();

        }

        void process_new(NewNode* m_node) {
            auto constructor_ref = symbol_table->find_var(SymbolTable::constructor_name(m_node->symbol->name), m_node->symbol->get_info());
            if (!constructor_ref) m_node->symbol->get_info().throw_exception("Type constructor not defined: " + m_node->symbol->name);
            const auto& [type_ref, stack_id] = symbol_table->find_type(m_node->symbol.get());
            if (!type_ref) m_node->symbol->get_info().throw_exception("Type not defined: " + m_node->symbol->name);
            if (!type_ref->is_object()) m_node->symbol->get_info().throw_exception("Not an object type: " + m_node->symbol->name);
            m_node->type_ref = type_ref->as<ObjectTypeMetaData>();
            m_node->constructor_ref = constructor_ref;

            std::vector<pType> type_args;
            for (const auto& q : m_node->type_args) { process_type(q); type_args.push_back(q->prog_type); }

            /* Strictly speaking, in the presence of type constructors, the constructor should have type
            new_A : <X>.A(X)->(<Y>.Y->A(X)) where Y is argument types. So when it applies to new A the type parameter should also be applied.
            new A<X,Y> : new_A<X>(self)<Y>(others).
            */

            // TODO F-omega: if constructor_ref is universal, it is initiated by amount of type_args it needed.
            auto constructor_type = constructor_ref->prog_type;

            // This is a call to base class constructor
            if (m_node->self_arg) { // new_B<...>(self)
                process_var(m_node->self_arg.get());
                match_type(constructor_type->as<PrimitiveType>()->args[0], m_node->self_arg->prog_type, m_node->get_info());
            }// otherwise we will new an object
            
            auto& arg1 = constructor_type->as<PrimitiveType>()->args[1];
            if (arg1->is_universal()) {
                // F-omega: this is the remaining args.
                m_node->prog_type = arg1->as<UniversalType>()->instanitiate(type_args, m_node->get_info());
            }
            else {
                m_node->prog_type = arg1;
            }
        }

        void process_getfield(GetFieldNode* m_node) {
            process_expr(m_node->lhs);
            auto lhs_type = const_cast<ConstTypeRef>(m_node->lhs->get_prog_type().get());
            const std::unordered_map<std::string, pType>* fields = NULL;

            while (lhs_type->is_universal_variable() && lhs_type->as<UniversalTypeVariable>()->quantifier) {
                lhs_type = lhs_type->as<UniversalTypeVariable>()->quantifier;
            }

            if (lhs_type->is_struct()) {
                fields = &(lhs_type->as<StructType>()->fields);
            }
            else if (lhs_type->is_object()) {
                fields = &(lhs_type->as<ObjectType>()->ref->fields);
            }
            else {
                m_node->lhs->get_info().throw_exception("Struct/class required");
            }

            auto f = fields->find(m_node->field->get_name());
            if (f == fields->end()) {
                m_node->field->get_info().throw_exception(StringAssembler("Type '")(*lhs_type)("' does not have field '")(m_node->field->get_name())("'")());
            }
            m_node->set_prog_type(f->second);
        }

        void process_lambda(LambdaNode* m_node) {

            auto t = std::make_shared<UniversalType>();
            if (!m_node->quantifiers.empty()) process_quantifiers(m_node->quantifiers, t);

            std::vector<pType> args_type;

            // build function types and local variables
            symbol_table->push_scope(symbol_table->create_scope());
            for (const auto& v : m_node->args) {
                process_type(v.second);
                args_type.push_back(v.second->prog_type);
                symbol_table->insert_var(v.first, VarMetaData::Source::ARG, v.second->prog_type);
            }

            if (m_node->ret_type) {
                process_type(m_node->ret_type);
                args_type.push_back(m_node->ret_type->prog_type);
            }
            binding_cache.push_back({});


            for (auto& s : m_node->statements) {
                if (s->is_expr()) {
                    Ptr<ExprNode> st = std::dynamic_pointer_cast<ExprNode>(s);
                    process_expr(st);
                }
                else if (s->get_type() == AST::LET) {
                    process_let(ast_cast<LetNode>(s));
                }
                else if (s->get_type() == AST::SET) {
                    process_set(ast_cast<SetNode>(s));
                }
                else {
                    throw std::runtime_error("Incorrect AST type");
                }
            }
            // register external bindings to the lambda
            m_node->bindings.resize(binding_cache.back().size());
            for (const auto& bce : binding_cache.back()) {
                m_node->bindings[bce.second->index] = bce.first;
            }

            binding_cache.pop_back();
            symbol_table->pop_scope();

            // match the type of last statement (or reconstruct)
            // the full monad feature will be improved in the future -- where the last statement should be 'return x'
            const auto& s_last = m_node->statements.back();
            if (!m_node->ret_type) {
                args_type.push_back(s_last->as<ExprNode>()->prog_type);
            }
            else if (s_last->is_expr()) {
                match_type(args_type.back(), s_last->as<ExprNode>()->prog_type, s_last->get_info());
            }
            else if (m_node->ret_type->prog_type->is_primitive() && 
                m_node->ret_type->prog_type->as<PrimitiveType>()->is_nil()
                ) {       // nil is allowed to be implicit
            }
            else {
                s_last->get_info().throw_exception("Statements inside function return non-nil must end with expression");
            }

            m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("function"), args_type);

            // if there are quantifiers, then it is a universal type
            if (!m_node->quantifiers.empty()) {
                t->body = m_node->prog_type;
                auto t1 = std::static_pointer_cast<Type>(t);
                m_node->prog_type.swap(t1);
                symbol_table->pop_type_scope();
            }
        }

        void process_let(LetNode* m_node) {

            if (m_node->vtype) process_type(m_node->vtype);

            if (m_node->expr) {
                process_expr(m_node->expr);
                if (m_node->vtype) {
                    match_type_in_decl(m_node->vtype->prog_type, m_node->expr->prog_type, m_node->get_info());
                }
                else {
                    m_node->vtype = std::make_shared<TypeNode>(
                        std::make_shared<Symbol>("auto", m_node->get_info()));
                    m_node->vtype->prog_type = m_node->expr->prog_type;
                }
            }

            if (symbol_table->cur_scope() != 0) {
                m_node->ref = symbol_table->insert_var(m_node->symbol, VarMetaData::LOCAL, m_node->vtype->prog_type);
            }
            else {
                if (BuiltinSymbolGenerator::is_builtin_symbol(m_node->symbol.get())) {
                    m_node->symbol->get_info().throw_exception("Redefinition of system symbol: " + m_node->symbol->get_name());
                }
                m_node->ref = symbol_table->insert_var(m_node->symbol, VarMetaData::GLOBAL, m_node->vtype->prog_type);
            }

            // usually we need an initialization; unless it's a system lib function
            m_node->ref->has_assigned = (m_node->expr != NULL);
        }

        void process_set(SetNode* m_node) {

            if (m_node->lhs->get_type() == AST::VAR) {
                auto lhs = ast_cast<VarNode>(m_node->lhs);
                VariableRef ref = symbol_table->find_var(lhs->symbol.get());
                if (!ref) {
                    m_node->get_info().throw_exception("Undefined variable: " + lhs->symbol->get_name());
                }
                else {
                    if (ref->scope == 0 && BuiltinSymbolGenerator::is_builtin_symbol(lhs->symbol.get())) {
                        m_node->get_info().throw_exception("Redefinition of system symbol: " + lhs->symbol->get_name());
                        // 'let' does not need this because there is redefinition check.
                    }

                    ref->has_assigned = true;
                    lhs->ref = ref;
                    process_expr(m_node->expr);

                    match_type(lhs->ref->prog_type, m_node->expr->prog_type, m_node->get_info());

                    // ! Assignment to external (i.e bind) variable is not allowed. (It can be allowed by making a new
                    // variable with same symbol, like in Python, but just too confusing)
                    if (ref->scope != 0 && ref->scope != symbol_table->cur_scope()) {
                        m_node->get_info().throw_exception("Cannot assign external variable inside closure");
                    }
                }
            }
            else if (m_node->lhs->get_type() == AST::GETFIELD) {
                auto lhs = ast_cast<GetFieldNode>(m_node->lhs);
                process_getfield(lhs);
                process_expr(m_node->expr);
                match_type(lhs->prog_type, m_node->expr->prog_type, m_node->get_info());
            }
            else {
                throw std::runtime_error("Incorrect AST Type");
            }

            
        }

        void process_interface(InterfaceNode* m_node) {
            
            const auto& [r, stack_id] = symbol_table->find_type(m_node->symbol.get());
            if (!r) {
                throw std::runtime_error("Interface not registered in the first pass");
            }
            else if (!r->is_interface()) {
                m_node->symbol->get_info().throw_exception("Interface redefinition");   // normally it should not happen
            }

            symbol_table->push_scope(symbol_table->create_scope());
            auto t = std::make_shared<StructType>();
            for (auto& p : m_node->parents) {
                const auto& [parent_t, stack_id] = symbol_table->find_type(p.get());
                if (parent_t && parent_t->is_interface()) {
                    for (const auto& f : parent_t->as<InterfaceTypeMetaData>()->actual_type->as<StructType>()->fields) {
                        const auto& [iter, succ] = t->fields.insert(f);
                        if (!succ && !f.second->equals(iter->second.get())) {
                            p->get_info().throw_exception("Field conflict: " + f.first);
                        }
                    }
                }
                else {
                    p->get_info().throw_exception("Not an interface: " + p->get_name());
                }
            }

            for (auto& f : m_node->members) {
                process_let(f.get());
                if (t->fields.count(f->symbol->get_name()) > 0) {
                    f->symbol->get_info().throw_exception("Fields redefinition: " + f->symbol->get_name());
                }
                t->fields.insert({ f->symbol->get_name(), f->vtype->prog_type });
            }
            r->as<InterfaceTypeMetaData>()->actual_type = std::static_pointer_cast<Type>(t);
            symbol_table->pop_scope();
        }

        void process_class(ClassNode* m_node) {

            const auto& [r, stack_id] = symbol_table->find_type(m_node->symbol.get());
            if (!r) {
                throw std::runtime_error("Class not registered in the first pass");
            }
            else if (!r->is_object()) {
                m_node->symbol->get_info().throw_exception("Object redefinition");   // normally it should not happen
            }
            auto ref = r->as<ObjectTypeMetaData>();
            m_node->ref = ref;

            // Inheritance
            const ObjectTypeMetaData* base_ref;
            if (m_node->base) {
                process_type(m_node->base);
                auto base_type = m_node->base->prog_type;
                if (!base_type->is_object()) {
                    m_node->base->get_info().throw_exception(StringAssembler("Inherit a non-object type")(*base_type)());
                }
                base_ref = base_type->as<ObjectType>()->ref;
                ref->fields = base_ref->fields;
                ref->field_names = base_ref->field_names;
                ref->virtual_fields = base_ref->virtual_fields;
                ref->base = base_type;
            }
            else {
                base_ref = NULL;
                ref->base = std::make_shared<PrimitiveType>(symbol_table->find_type("object"));
            }

            for (const auto& [member, meta] : m_node->members) {
                
                const auto& member_name = member->symbol->get_name();
                process_type(member->vtype);

                if (ref->fields.find(member_name) != ref->fields.end()) {

                    // virtual field.
                    if (base_ref && base_ref->virtual_fields.find(member_name) != base_ref->virtual_fields.end()) {
                        match_type(base_ref->fields.find(member_name)->second, member->vtype->prog_type, member->get_info());
                    }
                    else {
                        member->get_info().throw_exception("Field redefinition: " + member_name);
                    }
                }
                else {
                    ref->fields.insert({ member_name, member->vtype->prog_type });
                    ref->field_names.push_back(member_name);
                }

                if (meta.is_virtual) {
                    ref->virtual_fields.insert(member_name);
                }
            }

            if (!m_node->constructor) { // fake an empty constructor
                m_node->constructor = std::make_shared<LambdaNode>();
                m_node->constructor->set_info(m_node->get_info());
            }
            
            // last statement must be self
            m_node->constructor->statements.push_back(
                std::make_shared<VarNode>(
                    std::make_shared<Symbol>("self", m_node->constructor->get_info())));

            // \<X>.self:A(X) -> {\<Y>.(...)->{} }
            auto constructor_node = std::make_shared<LambdaNode>();                   // contructor as a global function; Defined at the same location as m_node.
            auto self_type_node = std::make_shared<TypeNode>(m_node->symbol);         // self must have same type as A
            constructor_node->set_info(m_node->get_info());

            // TODO F-omega constructor_node and self_type_node will have quantifier.
            constructor_node->args.push_back({
                std::make_shared<Symbol>("self", m_node->get_info()),
                self_type_node
                });
            constructor_node->statements.push_back(std::static_pointer_cast<AST>(m_node->constructor));

            /* Recursive Functions: Fake a type */
            // F-omega: may be universal
            auto constructor_type = std::make_shared<PrimitiveType>(symbol_table->find_type("function"));
            auto self_type = std::make_shared<ObjectType>(ref);

            constructor_type->args.push_back(self_type);
            bool has_quantifiers = !m_node->constructor->quantifiers.empty();
            auto ut = std::make_shared<UniversalType>();
            auto ft = std::make_shared<PrimitiveType>(symbol_table->find_type("function"));
            if (has_quantifiers) process_quantifiers(m_node->constructor->quantifiers, ut);
            for (const auto& a : m_node->constructor->args) { process_type(a.second); ft->args.push_back(a.second->prog_type); }
            ft->args.push_back(self_type);
            if (has_quantifiers) { symbol_table->pop_type_scope(); ut->body = ft; constructor_type->args.push_back(ut); }
            else constructor_type->args.push_back(ft);

            m_node->constructor_ref = symbol_table->insert_constructor(m_node->symbol, ref, constructor_type);

            process_lambda(constructor_node.get());
            match_type(constructor_node->prog_type, constructor_type, m_node->get_info());  // Checking
            m_node->constructor.swap(constructor_node);  // NOTE now the m_node->constructor is changed to the global one with signature A->((...)->A)

            // Interface checking
            for (const auto& p : m_node->interfaces) {
                process_type(p); 
                if (!p->prog_type->qualifies_interface()) {
                    p->get_info().throw_exception("Implements a non-interface");
                }
                if (!p->prog_type->is_interface_of(ObjectType(ref).member_unfold().get())) {
                    p->get_info().throw_exception(StringAssembler("Interface not match: ")(*p->prog_type)());
                };
            }
        }

        // If lhs >= rhs return 1, if lhs < rhs return 0. Raises exception if not comparable.
        unsigned match_type(const pType& lhs, const pType& rhs, const SymbolInfo& info) {

            auto result = lhs->partial_compare(rhs.get());
            switch (result)
            {
            case Type::Ordering::UNCOMPARABLE: {
                info.throw_exception(StringAssembler("Uncomparable type: ")(*lhs)(" and ")(*rhs)());
                return 0;
            }
            case Type::Ordering::GREATER: return 1;
            case Type::Ordering::EQUAL: return 0;
            default:
                return 0;
            }
        }

        // similar as match_type, except allowing contravariant assign with struct => class
        unsigned match_type_in_decl(const pType& lhs, const pType& rhs, const SymbolInfo& info) {    
            return match_type(lhs, rhs, info);
        }

    private:

        SymbolTable* symbol_table;

        std::vector<std::unordered_map<VariableRef, VariableRef>> binding_cache;   // cache of bindings that need to be recorded
        // This is the stack of binding cache; For each element, it is a dict {external_ref:binding_ref}
    };

}

#endif