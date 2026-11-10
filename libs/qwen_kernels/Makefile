CUDA ?= /opt/cuda
NVCC := $(CUDA)/bin/nvcc

CUBIN := build/embedding.cubin

.PHONY: all clean

all: $(CUBIN)

$(CUBIN): embedding.cu
	@mkdir -p build
	$(NVCC) -cubin -arch=sm_120 -O2 $< -o $@

clean:
	rm -rf build
