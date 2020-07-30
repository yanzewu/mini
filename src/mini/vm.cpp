
#include "vm.h"
#include <iostream>

using namespace mini;


int32_t cmp(char a, char b) { return a > b ? 1 : (a < b ? -1 : 0); }
int32_t cmp(int32_t a, int32_t b) { return a > b ? 1 : (a < b ? -1 : 0); }
int32_t cmp(float a, float b) { return a > b ? 1 : (a < b ? -1 : 0); }
int32_t cmp(Address a, Address b) { return a > b ? 1 : (a < b ? -1 : 0); }

void VM::execute(const ByteCode& code) {
	switch (code.code)
	{
	case ByteCode::OpCode::NOP:break;
	case ByteCode::OpCode::HALT: terminate_flag = true; break;
	case ByteCode::OpCode::THROW: {
		throw RuntimeError(stack.top().aarg);
		break;
	}

	case ByteCode::OpCode::LOADL: 
	case ByteCode::OpCode::LOADLI:
	case ByteCode::OpCode::LOADLF:
	case ByteCode::OpCode::LOADLA:
		load_local(code.arg1.iarg); break;
	case ByteCode::OpCode::LOADI: 
	case ByteCode::OpCode::LOADII:
	case ByteCode::OpCode::LOADIF:
	case ByteCode::OpCode::LOADIA:
	{
		Size_t index = stack.pop().iarg;
		Address addr = stack.pop().aarg;
		load_index(addr, index, code.code - ByteCode::OpCode::LOADI);
		break;
	}
	case ByteCode::OpCode::LOADFIELD: load_field(stack.pop().aarg, code.arg1.iarg); break;
	case ByteCode::OpCode::LOADG: load_field(global_addr, code.arg1.iarg); break;
	case ByteCode::OpCode::LOADC: load_constant(code.arg1.aarg); break;
	case ByteCode::OpCode::STOREL:
	case ByteCode::OpCode::STORELI:
	case ByteCode::OpCode::STORELF:
	case ByteCode::OpCode::STORELA:
		store_local(code.arg1.iarg, stack.pop()); break;
	case ByteCode::OpCode::STOREI:
	case ByteCode::OpCode::STOREII:
	case ByteCode::OpCode::STOREIF:
	case ByteCode::OpCode::STOREIA:
	{
		StackElem value = stack.pop();
		Size_t index = stack.pop().iarg;
		Address addr = stack.pop().aarg;
		store_index(addr, index, value, code.code - ByteCode::OpCode::STOREI);
		break;
	}
	case ByteCode::OpCode::STOREFIELD:
	{
		StackElem value = stack.pop();
		Address addr = stack.pop().aarg;
		store_field(addr, code.arg1.iarg, value);
		break;
	}
	case ByteCode::STOREG: store_field(global_addr, code.arg1.iarg, stack.pop()); break;
	case ByteCode::ALLOC:
	case ByteCode::ALLOCI:
	case ByteCode::ALLOCF:
	case ByteCode::ALLOCA:
		allocate_array(stack.pop().iarg, code.code - ByteCode::OpCode::ALLOC); break;
	case ByteCode::NEW: allocate_class(code.arg1.aarg); break;
	case ByteCode::NEWCLOSURE: allocate_closure(code.arg1.aarg); break;
	case ByteCode::OpCode::CALL: call(code.arg1.aarg); break;
	case ByteCode::OpCode::CALLA: {
		call_closure(stack.sp_offset(-code.arg1.iarg - 1).aarg); break;
	}
	case ByteCode::OpCode::CALLNATIVE: call_native(code.arg1.aarg); break;
	case ByteCode::OpCode::RETN: ret(false); break;
	case ByteCode::OpCode::RET:
	case ByteCode::OpCode::RETI:
	case ByteCode::OpCode::RETF:
	case ByteCode::OpCode::RETA:
		ret(true); break;
	case ByteCode::OpCode::CONST:
	case ByteCode::OpCode::CONSTI:
	case ByteCode::OpCode::CONSTF:
	case ByteCode::OpCode::CONSTA: stack.push(code.arg1); break;
	case ByteCode::OpCode::DUP: stack.push(stack.top()); break;
	case ByteCode::OpCode::POP: stack.pop(); break;
	case ByteCode::OpCode::SWAP: {
		auto t = stack.top(); stack.top() = stack.top2(); stack.top2() = t; break;
	}
	case ByteCode::OpCode::SHIFT: stack.grow(1); break;

	case ByteCode::OpCode::ADDI: stack.top2().iarg = stack.top2().iarg + stack.top().iarg; stack.pop(); break;
	case ByteCode::OpCode::SUBI: stack.top2().iarg = stack.top2().iarg - stack.top().iarg; stack.pop(); break;
	case ByteCode::OpCode::MULI: stack.top2().iarg = stack.top2().iarg * stack.top().iarg; stack.pop(); break;
	case ByteCode::OpCode::DIVI: stack.top2().iarg = stack.top2().iarg / stack.top().iarg; stack.pop(); break;
	case ByteCode::OpCode::REMI: stack.top2().iarg = stack.top2().iarg % stack.top().iarg; stack.pop(); break;
	case ByteCode::OpCode::NEGI: stack.top().iarg = -stack.top().iarg; break;
	case ByteCode::OpCode::AND: stack.top2().iarg = stack.top2().iarg & stack.top().iarg; stack.pop(); break;
	case ByteCode::OpCode::OR: stack.top2().iarg = stack.top2().iarg | stack.top().iarg; stack.pop(); break;
	case ByteCode::OpCode::XOR: stack.top2().iarg = stack.top2().iarg ^ stack.top().iarg; stack.pop(); break;
	case ByteCode::OpCode::NOT: stack.top().iarg = !stack.top().iarg; break;

	case ByteCode::OpCode::ADDF: stack.top2().farg = stack.top2().farg + stack.top().farg; stack.pop(); break;
	case ByteCode::OpCode::SUBF: stack.top2().farg = stack.top2().farg - stack.top().farg; stack.pop(); break;
	case ByteCode::OpCode::MULF: stack.top2().farg = stack.top2().farg * stack.top().farg; stack.pop(); break;
	case ByteCode::OpCode::DIVF: stack.top2().farg = stack.top2().farg / stack.top().farg; stack.pop(); break;
	case ByteCode::OpCode::REMF: stack.top2().farg = fmod(stack.top2().farg, stack.top().farg); stack.pop(); break;
	case ByteCode::OpCode::NEGF: stack.top().farg = -stack.top().farg; break;
	
	case ByteCode::OpCode::CMP: stack.top2().iarg = cmp(stack.top2().carg, stack.top().carg); stack.pop(); break;
	case ByteCode::OpCode::CMPI: stack.top2().iarg = cmp(stack.top2().iarg, stack.top().iarg); stack.pop(); break;
	case ByteCode::OpCode::CMPF: stack.top2().iarg = cmp(stack.top2().farg, stack.top().farg); stack.pop(); break;
	case ByteCode::OpCode::CMPA: stack.top2().iarg = cmp(stack.top2().aarg, stack.top().aarg); stack.pop(); break;
	case ByteCode::OpCode::EQ: stack.top().iarg = stack.top().iarg == 0 ? 1 : 0; break;
	case ByteCode::OpCode::NE: stack.top().iarg = stack.top().iarg != 0 ? 1 : 0; break;
	case ByteCode::OpCode::LT: stack.top().iarg = stack.top().iarg < 0 ? 1 : 0; break;
	case ByteCode::OpCode::LE: stack.top().iarg = stack.top().iarg <= 0 ? 1 : 0; break;
	case ByteCode::OpCode::GT: stack.top().iarg = stack.top().iarg > 0 ? 1 : 0; break;
	case ByteCode::OpCode::GE: stack.top().iarg = stack.top().iarg >= 0 ? 1 : 0; break;

	case ByteCode::OpCode::C2I: stack.top().iarg = stack.top().carg; break;
	case ByteCode::OpCode::C2F: stack.top().farg = stack.top().carg; break;
	case ByteCode::OpCode::I2C: stack.top().carg = static_cast<char>(stack.top().iarg); break;
	case ByteCode::OpCode::I2F: stack.top().farg = static_cast<float>(stack.top().iarg); break;
	case ByteCode::OpCode::F2C: stack.top().carg = static_cast<char>(stack.top().iarg); break;
	case ByteCode::OpCode::F2I: stack.top().iarg = static_cast<int32_t>(stack.top().farg); break;

	default:
		runtime_assert(false, "Invalid opcode");
		break;
	}
}

void VM::handle_error(const RuntimeError& e) {
	// if the stack is corrupted, a segmentation fault will arise

	std::string custom_msg;
	if (e.is_custom) {
		MemoryObject* obj = heap.fetch(e.data);
		if (obj->type == MemoryObject::Type_t::ARRAY && obj->size < 256) {
			custom_msg = std::string(static_cast<char*>(obj->data), obj->size);
		}
	}

	std::cerr << "Traceback (Most recent call first):\n";
	const LineNumberTable* lnt = irprog->line_number_table();

	while (stack.bp > 0) {

		// in jvm, function may have a 'exception table' to claim it is able to handle exception in some region
		// if pc \in exception range then terminate_flag <- false and pc is set. But we will see if that's necessary here.

		try {
			const auto& ln = lnt->query(pc_func, pc);
			const FunctionInfo* fi = irprog->fetch_constant(cur_function->info_index)->as<FunctionInfo>();
			std::string filename;
			if (fi->symbol_info.is_absolute()) {
				filename = "<builtin>";
			}
			else {
				filename = irprog->fetch_string(fi->symbol_info.location.srcno);
			}
			const std::string& functionname = irprog->fetch_string(fi->name_index);
			std::cerr << "  File \"" << filename << "\", line " << ln.line_number + 1 << ", in " << functionname << '\n';
			ret(false);
		}
		catch (const RuntimeError& e2) {
			if (e.is_custom) {
				if (!custom_msg.empty()) {
					std::cerr << custom_msg << '\n';
				}
				else {
					std::cerr << "RuntimeError\n";
				}
			}
			else {
				std::cerr << "RuntimeError: " << e.what() << '\n';
			}
			std::cerr << "\nDuring handling of the above exception, another exception occurred:\n\n";
			handle_error(e2);
			return;
		}
	}
	if (e.is_custom) {
		if (!custom_msg.empty()) {
			std::cerr << custom_msg << '\n';
		}
		else {
			std::cerr << "RuntimeError\n";
		}
	}
	else {
		std::cerr << "RuntimeError: " << e.what() << '\n';
	}
}

void VM::load_local(Size_t index) {
	Offset_t arg_offset = cur_function->sz_arg + cur_function->sz_bind;
	if (index < arg_offset) {
		stack.push(stack.bp_offset(-arg_offset - 1 + index));     // bp
	}
	else {
		runtime_assert(index - arg_offset < cur_function->sz_local, "Local variable does not exist");
		stack.push(stack.bp_offset(index - arg_offset + 2));    // pcfunc, pc
	}
}

void VM::store_local(Size_t index, StackElem value) {
	Offset_t arg_offset = cur_function->sz_arg + cur_function->sz_bind;
	if (index < arg_offset) {
		stack.bp_offset(-arg_offset - 1 + index) = value;   // bp
	}
	else {
		runtime_assert(index - arg_offset < cur_function->sz_local, "Local variable does not exist");
		stack.bp_offset(index - arg_offset + 2) = value;    // pcfunc, pc
	}
}

void VM::load_index(Address addr, Size_t index, Size_t typebit) {
	MemoryObject* obj = heap.fetch(addr);

	if (typebit == 0) {
		runtime_assert(obj->size > index, "Array index out of range");
		stack.push(obj->fetch(index)); // fetch can be used to any object
	}
	else {
		runtime_assert(obj->size > index * 4, "Array index out of range");
		stack.push(obj->fetch4(index * 4)); // fetch can be used to any object
	}
}

void VM::store_index(Address addr, Size_t index, StackElem value, Size_t typebit) {
	MemoryObject* obj = heap.fetch(addr);

	if (typebit == 0) {
		runtime_assert(obj->size > index, "Array index out of range");
		obj->store(index, value.carg);
	}
	else {
		runtime_assert(obj->size > index * 4, "Array index out of range");
		obj->store4(index * 4, value);
	}
}

void VM::load_field(Address addr, Size_t field_index) {

	ClassObject* cobj;
	Size_t sz_field, field_offset;
	_get_class_and_field(addr, field_index, cobj, field_offset, sz_field);
	if (sz_field == 1) {
		stack.push(cobj->fetch(field_offset));
	}
	else {  // may differentiate 4/8 when adding double support
		stack.push(cobj->fetch4(field_offset));
	}
}

void VM::store_field(Address addr, Size_t field_index, StackElem value) {

	ClassObject* cobj;
	Size_t sz_field, field_offset;
	_get_class_and_field(addr, field_index, cobj, field_offset, sz_field);
	if (sz_field == 1) {
		cobj->store(field_offset, value.carg);
	}
	else {  // may differentiate 4/8 when adding double support
		cobj->store4(field_offset, value);
	}
}

void VM::_get_class_and_field(Address addr, Size_t field_index, ClassObject*& cobj, Size_t& field_offset, Size_t& sz_field) {
	MemoryObject* obj = heap.fetch(addr);
	runtime_assert(obj->type == MemoryObject::Type_t::CLASS, "Load field from non-class type");
	cobj = obj->as<ClassObject>();
	const ClassLayout* cl = irprog->fetch_constant(cobj->layout_addr())->as<ClassLayout>();  // get the layout first
	try {
		sz_field = cl->offset[field_index + 1] - cl->offset[field_index];
	}
	catch (const std::out_of_range&) {
		throw RuntimeError("Field out of range");
	}
	field_offset = cl->offset[field_index];
}

void VM::load_constant(Size_t index) {
	// This only works with string now
	const StringConstant* s = irprog->fetch_constant(index)->as<StringConstant>();
	// cpo->get_type() == ConstantPoolObject::STRING
	Address addr = heap.allocate(MemoryObject::Type_t::ARRAY, s->value.size());
	memcpy(heap.fetch(addr)->data, s->value.c_str(), s->value.size());
	stack.push(addr);
}

void VM::allocate_array(Size_t size, Size_t typebit) {
	if (typebit == 0) {
		stack.push(heap.allocate(MemoryObject::Type_t::ARRAY, size));
	}
	else {
		stack.push(heap.allocate(MemoryObject::Type_t::ARRAY, size * 4));
	}
}

void VM::allocate_class(Size_t index) {
	const ClassLayout* cl = irprog->fetch_constant(index)->as<ClassLayout>();
	Address addr = heap.allocate(MemoryObject::Type_t::CLASS, cl->offset.back());
	heap.fetch(addr)->as<ClassObject>()->set_layout_addr(index);
	stack.push(addr);
}

void VM::allocate_closure(Size_t index) {
	const Function* f = irprog->fetch_constant(index)->as<Function>();
	Address addr = heap.allocate(MemoryObject::Type_t::CLOSURE, f->sz_bind * 4 + 4);
	ClosureObject* cobj = heap.fetch(addr)->as<ClosureObject>();
	cobj->set_function_addr(index);
	Offset_t nargs = cobj->data_size() / 4;
	cobj->move_from(&stack.sp_offset(-nargs));
	stack.sp -= nargs - 1;
	stack.top() = addr;
}

void VM::call_closure(Address addr) {
	MemoryObject* obj = heap.fetch(addr);
	runtime_assert(obj->type == MemoryObject::Type_t::CLOSURE, "Call a non-closure");
	ClosureObject* cobj = obj->as<ClosureObject>();
	stack.grow(cobj->data_size() / 4);
	cobj->move_to(&stack.top());
	call(cobj->function_addr());
}

void VM::call(Size_t index) {
	stack.push_bp();
	stack.push(pc_func);
	stack.push(pc);
	cur_function = irprog->fetch_constant(index)->as<Function>();
	stack.grow(cur_function->sz_local);
	pc_func = index;
	pc = 0;
}

void VM::ret(bool has_value) {
	StackElem value;
	if (has_value) value = stack.pop();
	Size_t sz_arg = cur_function->sz_arg + cur_function->sz_bind;

	pc_func = stack.bp_offset(0).aarg;
	pc = stack.bp_offset(1).aarg;
	cur_function = irprog->fetch_constant(pc_func)->as<Function>();
	stack.pop_bp();
	stack.shrink(sz_arg);   // remove argument space

	if (has_value) stack.push(value);
	else stack.grow(1);
}

void format(const char* fmt, size_t fmt_size, const StackElem* args, size_t arg_size, const MemorySection& heap, std::string& output) {

	size_t start = -1;
	char fmt_buffer[256];	// TODO buffer overflow
	char str_buffer[256];
	size_t cur_arg = 0;

	for (size_t j = 0; j < fmt_size; j++) {
		if (start != -1) {
			switch (fmt[j]) {
			case 'c':
			case 's':
			case 'd':
			case 'i':
			case 'o':
			case 'x':
			case 'X':
			case 'u':
			case 'f':
			case 'F':
			case 'e':
			case 'E':
			case 'a':
			case 'A':
			case 'g':
			case 'G':
			case 'p':
			{
				if (cur_arg >= arg_size) {		// No enough for formatting
					for (size_t k = start; k <= j; k++) output.push_back(fmt[k]);
					start = -1;
					break;
				}
				else {
					memcpy(fmt_buffer, fmt + start, j - start + 1);
					fmt_buffer[j - start + 1] = 0;
					if (fmt[j] == 'c') {	// char
						snprintf(str_buffer, 256, fmt_buffer, args[cur_arg++].carg);
					}
					else if (fmt[j] == 's') {	// array(char)
						const MemoryObject* obj = heap.fetch(args[cur_arg++].aarg);
						auto s = std::string(static_cast<char*>(obj->data), obj->size);
						snprintf(str_buffer, 256, fmt_buffer, s.c_str());	// TODO if > 256
					}
					else if (fmt[j] == 'p') {	// p is treated as x
						fmt_buffer[j - start] = 'x';
						snprintf(str_buffer, 256, fmt_buffer, args[cur_arg++].aarg);
					}
					else if (fmt[j] == 'e' || fmt[j] == 'E' || fmt[j] == 'f' || fmt[j] == 'F' || fmt[j] == 'g' || fmt[j] == 'G') {
						snprintf(str_buffer, 256, fmt_buffer, static_cast<double>(args[cur_arg++].farg));
					}
					else {
						snprintf(str_buffer, 256, fmt_buffer, args[cur_arg++].aarg);
					}
					start = -1;
					output += std::string(str_buffer);
				}
				break;
			}
			case ' ':
			case '+':
			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
				break;
			default:	// failed
				for (size_t k = start; k <= j; k++) output.push_back(fmt[k]);
				start = -1;
				break;
			}
		}

		else if (fmt[j] == '%' && j+1 < fmt_size) {
			start = j;
		}
		else {
			output.push_back(fmt[j]);
		}
	}
}


void VM::call_native(Size_t index) {
	switch (index)
	{
		// @print
	case 0: {
		MemoryObject* obj = heap.fetch(stack.pop().aarg);
		runtime_assert(obj->type == MemoryObject::Type_t::ARRAY, "Array required");
		std::cout << std::string(static_cast<char*>(obj->data), obj->size);
		break;
	}
		  // @input
	case 1: {
		std::string buffer;
		std::getline(std::cin, buffer);
		Address addr = heap.allocate(MemoryObject::Type_t::ARRAY, buffer.size());
		memcpy(heap.fetch(addr)->data, &buffer[0], buffer.size());
		stack.push(addr);
		break;
	}
		  // @format
	case 2: {
		std::string buffer;
		MemoryObject* obj_data = heap.fetch(stack.pop().aarg);
		MemoryObject* obj_fmt = heap.fetch(stack.pop().aarg);
		runtime_assert(obj_fmt->type == MemoryObject::Type_t::ARRAY, "Array required");
		format(static_cast<char*>(obj_fmt->data), obj_fmt->size, static_cast<StackElem*>(obj_data->data), obj_data->size / 4, heap, buffer);
		Address addr = heap.allocate(MemoryObject::Type_t::ARRAY, buffer.size());
		memcpy(heap.fetch(addr)->data, &buffer[0], buffer.size());
		stack.push(addr);
		break;
	}
		  // @len
	case 3: {
		stack.top().iarg = heap.fetch(stack.top().aarg)->size;
		break;
	}
	default:
		break;
	}
}
