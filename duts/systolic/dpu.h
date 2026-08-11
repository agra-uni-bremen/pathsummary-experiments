#pragma once

#include <systemc>

struct dpu : public sc_core::sc_module {
	sc_core::sc_in<bool> clk;

	sc_core::sc_in<uint32_t> in_top;
	sc_core::sc_in<uint32_t> in_left;
	sc_core::sc_out<uint32_t> out_bottom;
	sc_core::sc_out<uint32_t> out_right;

	uint32_t sum=0;

#ifdef FAULT_SYSTOLIC
	unsigned dpu_num=0;
	bool last_sum=false;
#endif

	SC_HAS_PROCESS(dpu);
	dpu(sc_core::sc_module_name name) : sc_module(name){
		SC_METHOD(run);
		sensitive << clk.pos();
	}
	dpu() : dpu("dpu") {} // TODO name mit counter

	void run() {
#if FAULT_SYSTOLIC==1
		if(!last_sum)
#elif FAULT_SYSTOLIC==2
		if(dpu_num==(WIDTH*HEIGHT)-1&&last_sum)
			sum += in_top.read()/in_left.read();
		else
#elif FAULT_SYSTOLIC==3
		if(dpu_num==(WIDTH*HEIGHT)-1&&last_sum)
			sum = in_top.read()*in_left.read();
		else
#elif FAULT_SYSTOLIC==4
		if(dpu_num==(WIDTH*HEIGHT)-1&&last_sum)
			sum -= in_top.read()*in_left.read();
		else
#endif
		sum += in_top.read()*in_left.read();
		out_bottom.write(in_top.read());
		out_right.write(in_left.read());
	}
};

