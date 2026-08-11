#pragma once

#include <systemc.h>
#ifdef DPU_APPROX
#include "dpu_approx.h"
#else
#include "dpu.h"
#endif

#ifdef DPU_APPROX
template <unsigned Width, unsigned Height, double (*F)(double), uint32_t accuracy, int lower, int upper, int step>
#else
template <unsigned Width, unsigned Height>
#endif
struct systolic : public sc_core::sc_module {
	static_assert(Width > 0, "width has to be greater than 0");
	static_assert(Height > 0, "height has to be greater than 0");

	sc_core::sc_in<bool> clk;
	sc_core::sc_in<uint32_t> ins_top[Width];
	sc_core::sc_in<uint32_t> ins_left[Height];
	sc_core::sc_out<uint32_t> outs_bottom[Width];
	sc_core::sc_out<uint32_t> outs_right[Height];

	sc_core::sc_signal<uint32_t> connect_row[Height][Width-1];
	sc_core::sc_signal<uint32_t> connect_col[Height-1][Width];

#ifndef DPU_APPROX
	dpu dpus[Height][Width];
#else
	sc_core::sc_in<bool> approx_start;
	sc_core::sc_signal<bool> approx_done_acc[Height][Width];
	sc_core::sc_out<bool> approx_done;
	dpu_approx<F,accuracy,lower,upper,step> dpus[Height][Width];

	SC_HAS_PROCESS(systolic);
#endif
	systolic(sc_core::sc_module_name name) : sc_module(name) {
#ifdef DPU_APPROX
		SC_METHOD(done_acc)
#endif
		for(unsigned i=0;i<Height;++i) {
			// connect to module inputs/outputs across the columns
			dpus[i][0].in_left(ins_left[i]);
			dpus[i][Width - 1].out_right(outs_right[i]);

			for (unsigned j = 0; j < Width; ++j) {
				dpus[i][j].clk(clk);
#if defined(FAULT_SYSTOLIC) || defined(FAULT_APPROX)
				dpus[i][j].dpu_num = i*Height+j+(HEIGHT==WIDTH?0:(HEIGHT>WIDTH?-i:i));
#endif
#ifdef DPU_APPROX
				dpus[i][j].approx_start(approx_start);
				dpus[i][j].approx_done(approx_done_acc[i][j]);
				sensitive << dpus[i][j].approx_done;
#endif

				// connect rows
				if (Width>1 && j > 0) {
					dpus[i][j].in_left(connect_row[i][j - 1]);
					dpus[i][j - 1].out_right(connect_row[i][j - 1]);
				}

				// connect columns
				if(Height>1 && i > 0) {
					dpus[i-1][j].out_bottom(connect_col[i-1][j]);
					dpus[i][j].in_top(connect_col[i-1][j]);
				}

				// connect to module inputs/outputs across the row
				if(i==0)
					dpus[i][j].in_top(ins_top[j]);
				if(i==Height-1)
					dpus[i][j].out_bottom(outs_bottom[j]);
			}
		}
	}

#ifdef DPU_APPROX
	void done_acc() {
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<Width;++j) {
				if(!approx_done_acc[i][j].read()) {
					approx_done.write(false);
					return;
				}
			}
		}
		approx_done.write(true);
	}
#endif
};

