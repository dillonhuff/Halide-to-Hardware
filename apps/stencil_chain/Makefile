include ../support/Makefile.inc

BIN ?= bin

all: $(BIN)/process

$(BIN)/stencil_chain.generator: stencil_chain_generator.cpp $(GENERATOR_DEPS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -g -fno-rtti $(filter-out %.h,$^) -o $@ $(LDFLAGS) $(HALIDE_SYSTEM_LIBS)

$(BIN)/stencil_chain.a: $(BIN)/stencil_chain.generator
	@mkdir -p $(@D)
	$^ -g stencil_chain -o $(BIN) -f stencil_chain target=$(HL_TARGET) auto_schedule=false

$(BIN)/stencil_chain_auto_schedule.a: $(BIN)/stencil_chain.generator
	@mkdir -p $(@D)
	$^ -g stencil_chain -o $(BIN) -f stencil_chain_auto_schedule target=$(HL_TARGET)-no_runtime auto_schedule=true

$(BIN)/process: process.cpp $(BIN)/stencil_chain.a $(BIN)/stencil_chain_auto_schedule.a
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I$(BIN) -Wall -O3 $^ -o $@ $(LDFLAGS) $(IMAGE_IO_FLAGS) $(CUDA_LDFLAGS) $(OPENCL_LDFLAGS) $(OPENGL_LDFLAGS)

$(BIN)/out.png: $(BIN)/process
	@mkdir -p $(@D)
	$(BIN)/process $(IMAGES)/rgb.png 10 $(BIN)/out.png

clean:
	rm -rf $(BIN)

test: $(BIN)/out.png
