CFLAGS = -std=c++20 -ggdb3 -Wall -DDEBUG
# Original line. On Ubuntu 20.10, libXxf86vm and libXi are missing.
# LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXrandr

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest

