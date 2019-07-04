#pragma once

/*
	ExecutorTraits

	MemoryType
	RegisterFileType
	AbstractValueType
	SupervisorType

*/

template<typename ExecutorTraits>
struct micro_executor_t {
	using Memory = typename ExecutorTraits::MemoryType;
	using RegisterFile = typename ExecutorTraits::RegisterFileType;
	using AbstractValue = typename ExecutorTraits::AbstractValueType;

	using ExecutorType = micro_executor_t<ExecutorTraits>;
	using Supervisor =  typename ExecutorTraits::SupervisorType;

	Memory m_memory;
	RegisterFile m_register_file;
	Supervisor m_supervisor;

	mbl_array_t* m_mba;
	mblock_t* m_block;
	minsn_t* m_insn;


	AbstractValue undefined_value() {
		return AbstractValue::undefined();
	}


	void setup(mbl_array_t* mba) {
		m_mba = mba;
		m_block = mba->blocks;
		m_insn = m_block->head;
	}

	AbstractValue read_reg(mreg_t mr, unsigned size) {
		return m_register_file::read_reg(mr, size);
	}

	void write_reg(mreg_t mr, unsigned size, AbstractValue value) {
		m_register_file::write_reg(mr, size, value);
	}


	AbstractValue read_stkvar(stkvar_ref_t* ref) {
		return m_memory::read_stkvar(ref);
	}

	void write_stkvar(stkvar_ref_t* ref, AbstractValue value) {
		m_memory::write_stkvar(ref, value);
	}

	AbstractValue execute_instruction(minsn_t* insn) {

	}




};