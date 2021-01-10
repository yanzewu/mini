#ifndef TYPEAUX_H
#define TYPEAUX_H

#include "value.h"
#include "symtable.h"
#include "type.h"

namespace mini {

    class TypeBuilder {
    public:

        TypeBuilder() {}
        explicit TypeBuilder(const std::string& s) : s(s) {}
        
        // Primitive types only
        virtual pType operator()()const { return NULL; }
        // Concrete types only
        virtual pType operator()(const SymbolTable&)const { return NULL; }
        // For non-concrete types
        virtual pType build(SymbolTable&)const { return NULL;}
        
        std::string s;
        virtual ~TypeBuilder() {}

    };


    // build type quickly from string
    class PrimitiveTypeBuilder : public TypeBuilder {
    public:
        PrimitiveTypeBuilder(const std::string& s) : TypeBuilder(s) {}

        PrimitiveTypeBuilder& operator()(const std::string& s);
        PrimitiveTypeBuilder& operator()(const TypeBuilder& arg) {
            args.push_back(&arg); return *this;
        }

        pType operator()()const override;
        pType operator()(const SymbolTable&)const override;
        pType build(SymbolTable&)const override;

        std::vector<const TypeBuilder*> args;
        
    };

    using ptb = PrimitiveTypeBuilder;

    class StructTypeBuilder : public TypeBuilder {
    public:

        StructTypeBuilder& operator()(const std::string& field, const TypeBuilder& val) {
            fields.push_back({ field, &val }); return *this;
        }

        pType operator()()const override;
        pType operator()(const SymbolTable&)const override;
        pType build(SymbolTable&) const override;

        std::vector<std::pair<std::string, const TypeBuilder*>> fields;
    };

    class ObjectTypeBuilder : public TypeBuilder {
    public:
        ObjectTypeBuilder(const std::string& s) : TypeBuilder(s) {}

        pType operator()(const SymbolTable&)const override;
        pType build(SymbolTable&) const override;
    };

    class TypeVariableBuilder : public TypeBuilder {
    public:

        explicit TypeVariableBuilder(const std::string& s) : TypeBuilder(s) {}

        pType build(SymbolTable&)const override;
    };

    class UniversalTypeBuilder : public TypeBuilder {
    public:
        UniversalTypeBuilder& operator()(const std::string& name) {
            quantifiers.push_back({ name, NULL }); return *this;
        }
        UniversalTypeBuilder& operator()(const std::string& name, const TypeBuilder& val) {
            quantifiers.push_back({ name, &val }); return *this;
        }
        UniversalTypeBuilder& operator()(const TypeBuilder& body) {
            this->body = &body; return *this;
        }

        pType build(SymbolTable&)const override;

        std::vector<std::pair<std::string, const TypeBuilder*>> quantifiers;
        const TypeBuilder* body;
    };
}

#endif