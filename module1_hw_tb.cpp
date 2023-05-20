#include <hls_stream.h>
#include <ap_int.h>
#include <stdio.h>
#include <cmath>
#define GPR_SIZE 46848
#define W_SIZE 256
#define H_SIZE 183
using namespace hls;

struct axis_data{
	float data;
	ap_uint<1> last;
};

void module1_hw(stream<axis_data> &TARGET, stream<axis_data> &X, stream<axis_data> &W_IN, stream<axis_data> &H, stream<axis_data> &W_OUT);
void software(float TARGET[GPR_SIZE], float X[GPR_SIZE], float W_IN[W_SIZE], float H[H_SIZE], float W_OUT[GPR_SIZE]);

int main(){
	int fail_flag = 0;
	int err = 0;

	float TARGET[GPR_SIZE];
	float X[GPR_SIZE];
	float W_IN[W_SIZE];
	float H[H_SIZE];

	for(int i = 0 ; i < GPR_SIZE ; i++){
		TARGET[i] = i * 0.2345342;
		X[i] = i * 0.23462345;
	}
	for(int i = 0 ; i < W_SIZE ; i++){
		W_IN[i] = i * 0.457452;
	}
	for(int i = 0 ; i < H_SIZE ; i++){
		H[i] = i * 0.567854;
	}
/*                  TARGET                  */
	//from right to left not from up to bottom
	float TARGET_temp[GPR_SIZE];
	int target_tmp;
	int k = 0;
	for(int i = 0 ; i < W_SIZE ; i++){
		for(int j = 0 ; j < H_SIZE ; j++){
			target_tmp = i + (j << 8);
			TARGET_temp[k] = TARGET[target_tmp];
			k++;
		}
	}
	axis_data local_read_target;
	stream<axis_data> target_hw;
	for(int i = 0 ; i < GPR_SIZE ; i++){
		local_read_target.data = TARGET_temp[i];
		if(i == (GPR_SIZE-1))
			local_read_target.last = (ap_uint<1>)1;
		else
			local_read_target.last = (ap_uint<1>)0;
		target_hw.write(local_read_target);
	}
/*                  X                  */
	//from right to left not from up to bottom(same as TARGET)
	float X_temp[GPR_SIZE];
	k = 0;
	for(int i = 0 ; i < W_SIZE ; i++){
		for(int j = 0 ; j < H_SIZE ; j++){
			target_tmp = i + (j << 8);
			X_temp[k] = X[target_tmp];
			k++;
		}
	}
	axis_data local_read_x;
	stream<axis_data> x_hw;
	for(int i = 0 ; i < GPR_SIZE ; i++){
		local_read_x.data = X_temp[i];
		if(i == (GPR_SIZE-1))
			local_read_x.last = (ap_uint<1>)1;
		else
			local_read_x.last = (ap_uint<1>)0;
		x_hw.write(local_read_x);
	}
/*                  H                  */
	axis_data local_read_h;
	stream<axis_data> h_hw;
	for(int i = 0 ; i < H_SIZE ; i++){
		local_read_h.data = H[i];
		if(i == (H_SIZE-1))
			local_read_h.last = (ap_uint<1>)1;
		else
			local_read_h.last = (ap_uint<1>)0;
		h_hw.write(local_read_h);
	}
/*                  W                  */
	axis_data local_read_w;
	stream<axis_data> w_in_hw;
	for(int i = 0 ; i < W_SIZE ; i++){
		local_read_w.data = W_IN[i];
		if(i == (W_SIZE-1))
			local_read_w.last = (ap_uint<1>)1;
		else
			local_read_w.last = (ap_uint<1>)0;
		w_in_hw.write(local_read_w);
	}
/*                  end                  */

	stream<axis_data> w_out_hw;
	module1_hw(target_hw, x_hw, w_in_hw, h_hw, w_out_hw);
	float W_OUT_sw[W_SIZE];
	software(TARGET, X, W_IN, H, W_OUT_sw);

	axis_data local_read_out;
	for(int i = 0 ; i < W_SIZE ; i++){
		local_read_out = w_out_hw.read();
		if(local_read_out.data != W_OUT_sw[i]){
			printf("Failed at [%d] | Hw:[%f] Sw:[%f]\n", i, local_read_out.data, W_OUT_sw[i]);
			err++;
			fail_flag = 1;
		}
	}
	printf("Total error: %d\n", err);
	if(fail_flag != 0)
		printf("FAILED\n");
	else
		printf("PASSED\n");
	return fail_flag;
}
void software(float TARGET[GPR_SIZE], float X[GPR_SIZE], float W_IN[W_SIZE], float H[H_SIZE], float W_OUT[GPR_SIZE]){
	float MSXHt[W_SIZE];
	float norms, b;
	int i0, i1, target_tmp;
	for(i0 = 0 ; i0 < W_SIZE ; i0++){
		norms = 0.0;
		b = 0.0;
		for(i1 = 0 ; i1 <H_SIZE ; i1++){
			target_tmp = i0 + (i1 << 8);
			norms = norms + TARGET[target_tmp] * H[i1];
			b = b + X[target_tmp] * H[i1];
		}
		norms = norms - b;
		MSXHt[i0] = -norms;
		if(norms > 0.0)
			MSXHt[i0] = 0.0;
	}
	norms = 0.0;
	for(i1 = 0 ; i1 < H_SIZE ; i1++){
		norms = norms + H[i1] * H[i1];
	}
	for(i0 = 0 ; i0 < W_SIZE ; i0++){
		W_OUT[i0] = MSXHt[i0] * W_IN[i0]/fmaxf(W_IN[i0] * norms, 1.0E-20);
	}
}


