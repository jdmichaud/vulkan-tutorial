### Setup

* `vulkan-tool`: A collection of tool to help debug vulkan installation on your system.
* `libvulkan-dev`: The library interfacing with the Vulkan driver.
* `vulkan-validationlayers-dev` and `spirv-tools`: Check and debug validation layers.
* `libglfw3-dev`: A cross-platform windowing library.
* `libglm-dev`: A linear algebra library.

```bash
sudo apt install vulkan-tools libvulkan-dev vulkan-validationlayers-dev spirv-tools libglfw3-dev libglm-dev
```

To remove:
```bash
sudo apt remove vulkan-tools libvulkan-dev vulkan-validationlayers-dev spirv-tools libglfw3-dev libglm-dev
```

### Compilation

calling `make` might fail with:
```
g++ -std=c++17 -O2 -o VulkanTest main.cpp -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
/usr/bin/ld: cannot find -lXxf86vm
/usr/bin/ld: cannot find -lXi
collect2: error: ld returned 1 exit status
make: *** [Makefile:5: VulkanTest] Error 1
```


