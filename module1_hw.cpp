#include <hls_stream.h>
#include <ap_int.h>
#include <cmath>
#define GPR_SIZE 46848
#define W_SIZE 256
#define H_SIZE 183
using namespace hls;

struct axis_data{
	float data;
	ap_uint<1> last;
};

void module1_hw(stream<axis_data> &TARGET, stream<axis_data> &X, stream<axis_data> &W_IN, stream<axis_data> &H, stream<axis_data> &W_OUT){
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis register both port=W_OUT
#pragma HLS INTERFACE axis register both port=H
#pragma HLS INTERFACE axis register both port=W_IN
#pragma HLS INTERFACE axis register both port=TARGET
#pragma HLS INTERFACE axis register both port=X
	axis_data local_read_target;
	axis_data local_read_x;
	axis_data local_read_w, local_write_w;
	axis_data local_read_h;

	float h_array[H_SIZE];
	float norms2 = 0.0;
	for(int i = 0 ; i < H_SIZE ; i++){
#pragma HLS PIPELINE II = 1
		local_read_h = H.read();
		h_array[i] = local_read_h.data;
		norms2 = norms2 + h_array[i] * h_array[i];
	}
	float norms;
	float b;
	float MSXHt[W_SIZE];
	for(int i0 = 0 ; i0 < W_SIZE ; i0++){
#pragma HLS PIPELINE II = 1
		norms = 0.0;
		b = 0.0;
		for(int i1 = 0 ; i1 < H_SIZE ; i1++){
			local_read_target = TARGET.read();
			local_read_x = X.read();
			norms = norms + local_read_target.data * h_array[i1];
			b = b + local_read_x.data * h_array[i1];
		}
		if(norms > b)
			MSXHt[i0] = 0.0;
		else
			MSXHt[i0] = b - norms;
	}
	for(int i = 0 ; i < W_SIZE ; i++){
#pragma HLS PIPELINE II = 1
		local_read_w = W_IN.read();
		local_write_w.data = MSXHt[i] * local_read_w.data/fmaxf(local_read_w.data * norms2, 1.0E-20);
		if(i == (W_SIZE-1))
			local_write_w.last = (ap_uint<1>)1;
		else
			local_write_w.last = (ap_uint<1>)0;
		W_OUT.write(local_write_w);
	}
}

