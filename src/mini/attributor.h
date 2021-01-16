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
                        while (symbol_table->cur_scope() != 0) {
                            symbol_table->pop_scope();
                        }
                        binding_cache.clear();
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

        // Process a single statement inside lambda.
        void process_statement(const pAST& node) {
            if (node->is_expr()) {
                process_expr(std::static_pointer_cast<ExprNode>(node));
            }
            else if (node->get_type() == AST::LET) {
                process_let(ast_cast<LetNode>(node));
            }
            else if (node->get_type() == AST::SET) {
                process_set(ast_cast<SetNode>(node));
            }
            else {
                throw std::runtime_error("Incorrect AST type");
            }
        }

        void process_type(const Ptr<TypeNode>& m_node) {

            // If prog_type is already present, omit it.
            if (m_node->skip_attribution) {
                if (!m_node->prog_type) throw std::runtime_error("Skip an unattributed typenode");
                return;
            }

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
                if (!m_node->prog_type) m_node->get_info().throw_exception("Interface not defined: " + m_node->symbol->get_name());
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

        void process_expr(const Ptr<ExprNode>& m_node, const pType& lvalue_prog_type = nullptr) {
            switch (m_node->get_type())
            {
            case AST::CONSTANT: process_constant(ast_cast<ConstantNode>(m_node), lvalue_prog_type); break;
            case AST::VAR: process_var(ast_cast<VarNode>(m_node)); break;
            case AST::TUPLE: process_tuple(ast_cast<TupleNode>(m_node)); break;
            case AST::STRUCT: process_struct(ast_cast<StructNode>(m_node)); break;
            case AST::ARRAY: process_array(ast_cast<ArrayNode>(m_node)); break;
            case AST::FUNCALL: process_funcall(ast_cast<FunCallNode>(m_node)); break;
            case AST::GETFIELD: process_getfield(ast_cast<GetFieldNode>(m_node)); break;
            case AST::NEW: process_new(ast_cast<NewNode>(m_node)); break;
            case AST::CASE: process_case(ast_cast<CaseNode>(m_node)); break;
            case AST::TYPEAPPL: process_typeappl(ast_cast<TypeApplNode>(m_node)); break;
            case AST::LAMBDA: process_lambda(ast_cast<LambdaNode>(m_node)); break;
            default:
                break;
            }
        }

        void process_constant(ConstantNode* m_node, const pType& lvalue_prog_type) {

            // if not a constant or prog_type is unboxed atomic && prog_type, skip; otherwise box it
            if (!m_node->skip_boxing && (
                !lvalue_prog_type || 
                !lvalue_prog_type->is_primitive() || 
                !lvalue_prog_type->as<PrimitiveType>()->is_unboxed() ||
                m_node->value.get_type() == Constant::Type_t::BOOL)) {

                auto t = std::make_shared<ConstantNode>(m_node->value, m_node->get_info()); // copy it; otherwise will deadlock
                t->skip_boxing = true;
                m_node->boxed_expr = make_boxed_constant_node(t);
                process_expr(m_node->boxed_expr);
                m_node->prog_type = m_node->boxed_expr->prog_type;
                return;
            }

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

        Ptr<ExprNode> make_boxed_constant_node(const Ptr<ConstantNode>& node);

        void process_var(VarNode* m_node) {
            if (m_node->skip_attribution) {
                if (!m_node->ref) throw std::runtime_error("Skip an unattributed node");
                return;
            }

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
                    m_node->set_ref(register_binding_variable(ref));
                }
                else {  // local/global/arg
                    m_node->set_ref(ref);
                }
            }
        }

        // Given a variable ref, return a binding variable ref.
        VariableRef register_binding_variable(VariableRef ref) {
            auto r = binding_cache.back().find(ref);
            if (r == binding_cache.back().end()) {
                // create new ref assign to r
                auto new_binding_var = symbol_table->insert_var(
                    ref->symbol, VarMetaData::Source::BINDING, ref->prog_type);
                binding_cache.back().insert({ ref, new_binding_var });
                return new_binding_var;
            }
            else {  // already referenced as binding variable
                return r->second;
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

            pType maximum_type;

            if (m_node->children.empty()) {
                maximum_type = std::make_shared<PrimitiveType>(symbol_table->find_type("bottom"));
            }
            else {  // type inference
                process_expr(m_node->children[0]);
                maximum_type = m_node->children[0]->prog_type;

                for (size_t i = 1; i < m_node->children.size(); i++) {
                    process_expr(m_node->children[i]);
                    make_maximum_type(maximum_type, m_node->children[i]->prog_type, m_node->get_info());
                }
            }
            m_node->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("array"), std::vector<pType>({ maximum_type }));
        }

        void process_typeappl(TypeApplNode* m_node) {
            process_expr(m_node->lhs);

            if (!m_node->lhs->prog_type->is_universal()) {
                m_node->get_info().throw_exception(StringAssembler("Not a universal type: ")(*m_node->lhs->prog_type)());
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
            const auto& func_type_raw = m_node->caller->prog_type;
            const UniversalType* func_type_univ = nullptr;
            const PrimitiveType* func_body = NULL;
            pType func_type_applied;

            // Check types: universal/normal
            if (func_type_raw->is_primitive() && func_type_raw->as<PrimitiveType>()->is_function()) {
                func_body = func_type_raw->as<PrimitiveType>();
                func_type_applied = func_type_raw;
            }
            else if (func_type_raw->is_universal() && func_type_raw->as<UniversalType>()->body->is_primitive() &&
                func_type_raw->as<UniversalType>()->body->as<PrimitiveType>()->is_function()) {
                func_body = func_type_raw->as<UniversalType>()->body->as<PrimitiveType>();
                func_type_univ = func_type_raw->as<UniversalType>();
            }
            else {
                m_node->get_info().throw_exception(StringAssembler("Not a function: ")(*m_node->caller->prog_type)());
            }

            // Match arg number
            if (func_body->args.size() - 1 != m_node->args.size()) {
                m_node->get_info().throw_exception(StringAssembler("Expect ")(func_body->args.size() - 1)(" args, got ")(m_node->args.size())());
            }

            // Process expr & Boxing constants
            for (size_t i = 0; i < m_node->args.size(); i++) {
                process_expr(m_node->args[i], func_body->args[i]);
            }
            
            // Type inferrance
            if (func_type_univ) {
                auto func_type = func_type_univ->body->as<PrimitiveType>();

                std::vector<pType> actual_args;
                for (const auto& a : m_node->args) actual_args.push_back(a->prog_type);
                if (func_type->args.size() - 1 != m_node->args.size()) {
                    m_node->get_info().throw_exception(StringAssembler("Expect ")(func_type->args.size() - 1)(" args, got ")(m_node->args.size())());
                }
                std::vector<pType> inferred_args = std::vector<pType>(func_type_univ->quantifiers.size(), nullptr);
                infer_arguments(func_type->args.begin(), func_type->args.end() - 1, actual_args.begin(), inferred_args, m_node->get_info(), 0);
                for (size_t i = 0; i < inferred_args.size(); i++) {   // check inferred results
                    if (!inferred_args[i]) m_node->get_info().throw_exception(StringAssembler("Type argument #")(i + 1)(" cannot be inferred")());
                }
                func_type_applied = func_type_univ->instanitiate(inferred_args, m_node->get_info());
            }

            // Match argument types
            auto func_type = func_type_applied->as<PrimitiveType>();
            for (size_t i = 0; i < m_node->args.size(); i++) {
                match_type(func_type->args[i], m_node->args[i]->prog_type, m_node->args[i]->get_info());
            }
            m_node->prog_type = func_type->args.back();
        }

        void infer_arguments(std::vector<pType>::const_iterator arg_type_begin, std::vector<pType>::const_iterator arg_type_end, 
            std::vector<pType>::const_iterator actual_type_begin, std::vector<pType>& results, const SymbolInfo& info, unsigned stack_level) {

            for (auto a = arg_type_begin, t = actual_type_begin; a != arg_type_end; a++, t++) {
                // here we only do naked type variables -- ideally we should 'compare and infer' for argument types; but I will see if it is necessary
                if ((*a)->is_universal_variable() && (*a)->as<UniversalTypeVariable>()->stack_id == stack_level) {
                    auto arg_id = (*a)->as<UniversalTypeVariable>()->arg_id;
                    if (results[arg_id]) {
                        make_maximum_type(results[arg_id], *t, info);
                    }
                    else {
                        results[arg_id] = *t;
                    }
                }
            }
        }


        void process_new(NewNode* m_node) {
            const auto& [type_ref, stack_id] = symbol_table->find_type(m_node->symbol.get());
            if (!type_ref) m_node->symbol->get_info().throw_exception("Type not defined: " + m_node->symbol->name);
            if (!type_ref->is_object()) m_node->symbol->get_info().throw_exception("Not an object type: " + m_node->symbol->name);
            auto constructor_ref = symbol_table->find_var(SymbolTable::constructor_name(m_node->symbol->name), m_node->symbol->get_info());
            if (!constructor_ref) m_node->symbol->get_info().throw_exception("Type constructor not defined: " + m_node->symbol->name);
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

        // if self_reference is set, will set its type.
        void process_lambda(LambdaNode* m_node, VariableRef self_reference=NULL) {

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

            if (self_reference) {
                if (!m_node->ret_type) {
                    m_node->get_info().throw_exception("Recursive functions must annotate the returning type");
                }
                else {
                    self_reference->prog_type = std::make_shared<PrimitiveType>(symbol_table->find_type("function"), args_type);
                }
            }

            for (auto& s : m_node->statements) {
                process_statement(s);
            }

            // register external bindings to the lambda
            m_node->bindings.resize(binding_cache.back().size());
            for (const auto& bce : binding_cache.back()) {
                m_node->bindings[bce.second->index] = bce.first;
            }

            binding_cache.pop_back();
            symbol_table->pop_scope();

            // If a child lambda node binds variable not global but outer than this layer, 
            // we should change it to a new binding variable defined in this level;
            for (size_t i = 0; i < m_node->bindings.size(); i++) {
                auto& binding_ref = m_node->bindings[i];
                if (binding_ref->scope != symbol_table->cur_scope() && binding_ref->scope != 0) {
                    m_node->bindings[i] = register_binding_variable(binding_ref);
                }
            }

            // match the type of last statement (or reconstruct). If last statement is not expression, return nil.
            const auto& s_last = m_node->statements.back();
            pType last_prog_type = s_last->is_expr() ?
                s_last->as<ExprNode>()->prog_type :
                std::make_shared<PrimitiveType>(symbol_table->find_type("nil"));

            if (!m_node->ret_type) {
                args_type.push_back(last_prog_type);
                m_node->ret_type = TypeNode::make_attributed(last_prog_type, m_node->get_info());
            }
            else {
                match_type(args_type.back(), last_prog_type, s_last->get_info());
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

        void process_case(CaseNode* m_node) {
            
            /* The process of case expressions has two stages: (1) split it into separated lambdas;
            (2) process these lambdas. Here I use (sort of) CPS style to write this -- each lambda looks like
            \ -> sel(condition, present, future)();
            All the statements are then moved into a lambda so finally it looks like as a single function call
                \n:T->{
                    let Sn = \->sel<function(T)>(Cn, \->En, undefined)(),
                    ...
                    let S2 = \->sel<function(T)>(C2, \->E2, S3)(),
                    let S1 = \->sel<function(T)>(C1, \->E1, S2)(),
                    S1()
                }(expr)
            */

            auto m_lambda_node = std::make_shared<LambdaNode>();
            m_lambda_node->set_info(m_node->get_info());
            process_expr(m_node->lhs);
            pType arg_type = m_node->lhs->prog_type;   // T
            auto arg_name = std::make_shared<Symbol>("case#arg", m_node->get_info());
            m_lambda_node->args.push_back({ arg_name, TypeNode::make_attributed(arg_type, m_node->get_info()) });   // n:T

            // test the availability of sel and Bool
            if (!symbol_table->find_var("sel") || !symbol_table->find_var("and") || !symbol_table->find_global_type("Bool")) {
                m_node->get_info().throw_exception("Cases cannot be parsed before `import std`.");
            }

            if (!m_node->cases.empty()) {
                pSymbol previous_branch_name = std::make_shared<Symbol>("undefined", m_node->get_info());
                int case_id = m_node->cases.size();
                for (auto m_case = m_node->cases.rbegin(); m_case != m_node->cases.rend(); m_case++) { 
                    // let current_case = m_case_branch [which uses previous_case];

                    // TODO type inference so that branch_type is not necesssary
                    auto m_branch = process_case_branch(*m_case, arg_name, previous_branch_name, arg_type, m_case == m_node->cases.rbegin());
                    auto m_branch_name = std::make_shared<Symbol>("case#" + std::to_string(case_id--), m_node->get_info());
                    auto m_assignment = std::make_shared<LetNode>(m_case->condition->get_info(), m_branch_name, nullptr, m_branch);
                    m_lambda_node->statements.push_back(m_assignment);
                    previous_branch_name.swap(m_branch_name);
                }
                m_lambda_node->statements.push_back(std::make_shared<FunCallNode>(previous_branch_name->get_info(),
                    std::make_shared<VarNode>(previous_branch_name), std::vector<Ptr<ExprNode>>{}));   // current_case()

                auto m_funcall = std::make_shared<FunCallNode>(m_node->get_info(), m_lambda_node, std::vector<Ptr<ExprNode>>{m_node->lhs}); // <lambda>(expr)
                process_funcall(m_funcall.get());
                m_node->parsed_expr = m_funcall;
                m_node->prog_type = m_node->parsed_expr->prog_type;
            }
            else{
                m_node->get_info().throw_exception("No cases in case expression");
            }
        }

        // Desugaring of case statements. No type attribution here.
        Ptr<LambdaNode> process_case_branch(const CaseNode::Case& m_case, const pSymbol& arg_name, const pSymbol& previous_branch_name, 
            const pType& arg_type, bool is_last_one);

        void process_let(LetNode* m_node) {

            if (m_node->vtype) process_type(m_node->vtype);

            if (m_node->expr) {
                process_expr(m_node->expr, m_node->vtype ? m_node->vtype->prog_type : nullptr);
                if (m_node->vtype) {
                    match_type_in_decl(m_node->vtype->prog_type, m_node->expr->prog_type, m_node->get_info());
                }
                else {
                    m_node->vtype = TypeNode::make_attributed(m_node->expr->prog_type, m_node->get_info());
                }
            }
            else if (symbol_table->cur_scope() != 0) {
                m_node->get_info().throw_exception("RHS required in local declaration");
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
                    process_expr(m_node->expr, ref->prog_type);

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
                process_type(f->vtype);
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

            // TODO F-omega when base_type is kind *->* base members have type depend on arguments.
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

            // \<X>.self:A(X) -> {\(...)->{} }
            // TODO F-omega constructor_node and self_type_node will have quantifier.

            auto self_type_node = TypeNode::make_attributed(std::make_shared<ObjectType>(ref), m_node->get_info());
            auto constructor_node = std::make_shared<LambdaNode>();                   // contructor as a global function; Defined at the same location as m_node.
            constructor_node->set_info(m_node->get_info());
            constructor_node->args.push_back({
                std::make_shared<Symbol>("self", m_node->get_info()),
                self_type_node
                });
            constructor_node->statements.push_back(std::static_pointer_cast<AST>(m_node->constructor));
            auto ret_type_node = std::make_shared<TypeNode>();
            ret_type_node->set_info(m_node->constructor->get_info());
            ret_type_node->symbol = std::make_shared<Symbol>("function", m_node->constructor->get_info());
            for (const auto& a : m_node->constructor->args) {
                ret_type_node->args.push_back(a.second);
            }
            ret_type_node->args.push_back(self_type_node);
            constructor_node->ret_type = ret_type_node;
            
            m_node->constructor_ref = symbol_table->insert_constructor(m_node->symbol, ref, NULL);
            process_lambda(constructor_node.get(), m_node->constructor_ref);    // Note this is recursive.
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


        // Always lift lhs to maximum
        void make_maximum_type(pType& lhs, const pType& rhs, const SymbolInfo& info) {
            auto result_l_to_r = lhs->partial_compare(rhs.get());
            if (result_l_to_r == Type::Ordering::GREATER || result_l_to_r == Type::Ordering::EQUAL) {
                
            }
            else {  // maybe less?
                auto result_r_to_l = rhs->partial_compare(lhs.get());
                if (result_r_to_l == Type::Ordering::GREATER) {
                    auto p = rhs;
                    lhs.swap(p);
                }
                else {
                    info.throw_exception(StringAssembler("Cannot infer a common maximum type of ")(*lhs)(" and ")(*rhs)());
                    // The ideal maximum type would be the super type of both. However, finding such a type
                    // may be undecideable (in System F) and is not very meaningful, so I just throw here.
                }
            }
        }

    private:

        SymbolTable* symbol_table;

        std::vector<std::unordered_map<VariableRef, VariableRef>> binding_cache;   // cache of bindings that need to be recorded
        // This is the stack of binding cache; For each element, it is a dict {external_ref:binding_ref}
    };

}

#endif