# NebiumOS  Learning Curriculum

This document lists the specific knowledge required to implement each part of NebiumOS, and the best primary sources for acquiring it. It is not a general "learn programming" guide. Every item listed is directly prerequisite to a specific implementation task.

**Principle:** Always prefer the primary source  spec, paper, or source code  over secondary explanations. Secondary sources are listed only where the primary source requires too much context to read cold.

---

## 1 · C++ and Systems Programming

### Required for: All of Phase 0–4

**Primary references:**
- *The C++ Programming Language*  Bjarne Stroustrup (4th ed.)  the authoritative reference
- cppreference.com  use daily; prefer this over Stack Overflow for language questions
- *The Linux Programming Interface*  Michael Kerrisk  the definitive Linux syscall reference

**Supplementary:**
- *Effective Modern C++*  Scott Meyers  for C++11/14/17 idioms
- *C++ Concurrency in Action*  Anthony Williams  for threading and atomics chapters specifically

**Topics required:**
- RAII, move semantics, `unique_ptr` / `shared_ptr`, custom deleters
- `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic`
- Template basics: function templates, class templates, type traits
- `epoll`, `timerfd_create`, `signalfd`, `eventfd`
- `fork`, `exec`, `waitpid`, `kill`, `signal`
- `mmap`, `shm_open`, file descriptor passing over Unix sockets (`SCM_RIGHTS`)
- Unix domain sockets: `SOCK_STREAM` and `SOCK_SEQPACKET`
- CMake: targets, dependencies, `find_package`, `add_subdirectory`

---

## 2 · Linear Algebra and 3D Mathematics

### Required for: Phases 0.3, 1.2, 2.3

**Primary references:**
- *Mathematics for 3D Game Programming and Computer Graphics*  Eric Lengyel (3rd ed.)
  - Read: Chapter 2 (Vectors), Chapter 3 (Matrices), Chapter 4 (Transforms), Chapter 5 (Geometry)
- *3D Math Primer for Graphics and Game Development*  Dunn & Parberry  more accessible entry point

**Topics required:**
- Vector operations: dot product, cross product, normalization
- Matrix multiplication, transposition, inverse
- Homogeneous coordinates: why a 4×4 matrix for 3D transforms
- Model / View / Projection matrix derivation  derive each from first principles, do not use a library until you have done this
- Quaternions: representation, `slerp`, conversion to/from rotation matrix  critical for XR
- Ray–plane intersection, ray–AABB intersection, ray–triangle intersection (Möller–Trumbore)
- Frustum culling: extract frustum planes from a projection matrix

---

## 3 · OpenGL and Vulkan

### Required for: Phases 0.3, 1.1, 1.2

**OpenGL  start here:**
- learnopengl.com  work through all chapters sequentially; do every exercise
- OpenGL 4.6 Core Profile Specification (khronos.org)  read when behavior is unclear
- OpenGL SuperBible (7th ed.)  useful reference

**Vulkan  migrate to this in Phase 2:**
- vulkan-tutorial.com  complete all sections
- *Vulkan Programming Guide*  Graham Sellers  for architecture understanding
- Vulkan 1.3 Specification (khronos.org)  ground truth for all questions
- Brendan Galea's Vulkan series (YouTube)  best video walkthrough of the pipeline

**Topics required (OpenGL):**
- VAO, VBO, EBO; buffer layouts
- Vertex and fragment shaders; GLSL types and built-ins
- Uniform blocks, UBOs, texture samplers
- Framebuffer objects (FBO), renderbuffer objects
- Texture formats, mipmapping, filtering
- Depth test, stencil test, blending

**Topics required (Vulkan):**
- Instance, physical device, logical device, queues
- Swapchain, render pass, framebuffer
- Command buffers, synchronization: semaphores, fences, pipeline barriers
- Descriptor sets and layouts; push constants
- Memory allocation: device-local vs host-visible; `VkDeviceMemory`

---

## 4 · Wayland Protocol

### Required for: Phases 0.4, 1.1

**Primary references:**
- *The Wayland Book* (wayland-book.com)  read completely; it is short
- Wayland protocol XML specifications  in `/usr/share/wayland/` and `wayland-protocols` package
- wlroots source code (gitlab.freedesktop.org/wlroots/wlroots)  especially `tinywl/tinywl.c`
- sway compositor source (github.com/swaywm/sway)  production wlroots usage

**Topics required:**
- Wayland object model: `wl_display`, `wl_registry`, `wl_proxy`
- Wire protocol: message format, argument types, event vs request
- Core interfaces: `wl_compositor`, `wl_surface`, `wl_shm`, `wl_seat`
- Shell protocols: `xdg_wm_base`, `xdg_surface`, `xdg_toplevel`
- Input: `wl_keyboard`, `wl_pointer`, `wl_touch`, `wl_seat`
- Buffer types: `wl_shm` (shared memory) and `wl_drm` / `linux_dmabuf` (GPU buffers)
- Damage protocol: `wl_surface.damage`, `wl_surface.damage_buffer`
- wlroots: backend, renderer, allocator, scene graph APIs

---

## 5 · DRM/KMS and Linux GPU Stack

### Required for: Phase 1.1 (bare-metal path)

**Primary references:**
- "DRM Developer's Guide" (kernel.org/doc/html/latest/gpu/drm-uapi.html)
- Mesa source code (gitlab.freedesktop.org/mesa/mesa)  for understanding how user-space GL talks to kernel
- Daniel Vetter's blog (blog.ffwll.ch)  DRM maintainer; high-signal technical posts

**Topics required:**
- KMS objects: CRTC, encoder, connector, plane, framebuffer
- `drmModeGetResources`, `drmModeSetCrtc`, atomic modesetting
- GBM (Generic Buffer Manager): `gbm_device`, `gbm_surface`, `gbm_bo`
- DMA-BUF: cross-process GPU buffer sharing
- Vsync: `DRM_EVENT_VBLANK`, page flip events

---

## 6 · OpenXR

### Required for: Phases 1.3, 2.3

**Primary references:**
- OpenXR 1.0 Specification (registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html)  the spec is readable; use it
- OpenXR-SDK (github.com/KhronosGroup/OpenXR-SDK)  C headers + loader; read `openxr.h`
- Monado documentation (monado.dev)  for Linux development without hardware
- `hello_xr` sample (in OpenXR-SDK-Source)  canonical starting point

**Topics required:**
- Instance and system: `xrCreateInstance`, `xrGetSystem`, `xrGetSystemProperties`
- Session lifecycle: state machine transitions (IDLE → READY → SYNCHRONIZED → VISIBLE → FOCUSED → STOPPING)
- Swapchain management: `xrCreateSwapchain`, `xrAcquireSwapchainImage`, `xrReleaseSwapchainImage`
- Space and reference frames: `XR_REFERENCE_SPACE_TYPE_STAGE`, `XR_REFERENCE_SPACE_TYPE_VIEW`, `XR_REFERENCE_SPACE_TYPE_LOCAL`
- View configuration: `xrEnumerateViewConfigurations`, `xrLocateViews`, per-eye FOV and pose
- Action system: `xrCreateActionSet`, `xrCreateAction`, `xrSyncActions`, `xrGetActionStatePose`
- Hand tracking extension: `XR_EXT_hand_tracking`, `xrLocateHandJointsEXT`, `XrHandJointLocationEXT`

---

## 7 · Operating Systems Theory

### Required for: Phase 2 (app model, IPC, sandboxing)

**Primary references:**
- *Operating Systems: Three Easy Pieces*  Arpaci-Dusseau (ostep.org  free)  read in full
- xv6 source code (github.com/mit-pdos/xv6-riscv)  implement changes; read with the MIT 6.S081 notes
- *The Design of the UNIX Operating System*  Maurice Bach  for understanding the original model

**Topics required:**
- Process model: address space, file descriptor table, signal disposition
- Scheduling: preemptive, cooperative; priority; real-time constraints (relevant to render loop)
- Virtual memory: page tables, TLB, `mmap`, demand paging, copy-on-write
- IPC mechanisms: pipes, FIFOs, Unix sockets, shared memory, message queues  know the tradeoffs
- Linux namespaces: `CLONE_NEWPID`, `CLONE_NEWNET`, `CLONE_NEWNS`, `CLONE_NEWUSER`
- Capabilities: `cap_set_proc`, capability bounding sets; principle of least privilege
- `seccomp-bpf`: syscall filtering; how to write and apply a BPF filter

---

## 8 · Distributed Systems

### Required for: Phase 3.4 (multi-user sync)

**Primary references:**
- *Designing Data-Intensive Applications*  Martin Kleppmann  read chapters on replication, consistency, time
- Lamport, 1978  "Time, Clocks, and the Ordering of Events in a Distributed System"  read the paper directly
- Shapiro et al., 2011  "Conflict-free Replicated Data Types" (CRDTs)  arXiv:1805.06358

**Topics required:**
- Logical clocks: Lamport timestamps, vector clocks
- Consistency models: eventual consistency, causal consistency, linearizability  know the definitions precisely
- CRDTs: G-Set, OR-Set, LWW-Register  implement at least one from scratch
- Delta-state CRDTs: transmit only deltas, not full state
- Authority and ownership models: how to decide which client's write wins

---

## 9 · AI Integration

### Required for: Phase 4

**Primary references:**
- llama.cpp (github.com/ggml-org/llama.cpp)  study the C++ inference architecture
- Whisper.cpp (github.com/ggml-org/whisper.cpp)  on-device speech recognition
- ONNX Runtime C++ API (onnxruntime.ai/docs)  for deploying arbitrary models
- "World Models"  Ha & Schmidhuber, 2018 (arXiv:1803.10122)
- SpatialVLM  Google, 2024  spatial reasoning in vision-language models

**Topics required:**
- GGUF model format; quantization levels (Q4_K_M, Q8_0) and their quality/speed tradeoffs
- `llama_context`, `llama_batch`, `llama_decode` C API
- Prompt engineering for structured output: JSON schema enforcement, function calling
- Whisper model sizes; real-time transcription loop
- ONNX graph execution: providers (CPU, CUDA), session options, input/output binding

---

## Reading Schedule

This is not a schedule for reading before coding. It is a parallel curriculum: read while building.

| Phase | Read alongside |
|-------|---------------|
| Phase 0.1 | *Effective Modern C++* chapters 1–5; cppreference threading pages |
| Phase 0.2 | *Linux Programming Interface* chapters 3, 5, 20, 23, 44, 52, 63 |
| Phase 0.3 | learnopengl.com Getting Started + Lighting; Lengyel chapters 2–5 |
| Phase 0.4 | *The Wayland Book* in full; read `tinywl.c` |
| Phase 1.1 | wlroots source; DRM/KMS developer guide |
| Phase 1.3 | OpenXR specification chapters 1–7 |
| Phase 2.1–2.2 | *OSTEP* chapters on process API, IPC, sandboxing |
| Phase 2.3 | OpenXR specification chapter 11 (input); `XR_EXT_hand_tracking` extension spec |
| Phase 3.1 | ECS design papers; EnTT source (do not use it, but read it) |
| Phase 3.4 | Kleppmann chapters 5, 9; Lamport 1978; Shapiro 2011 |
| Phase 4 | llama.cpp and whisper.cpp source; Ha & Schmidhuber 2018 |
