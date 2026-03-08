# NebiumOS Checklist

Task tracking for all implementation phases. Check each item when complete. Record the date next to each milestone  these dates are your verifiable record of progress.

---

## Phase 0 · Foundations

### 0.1 · C++20 Systems Programming
- [ ] Implement a custom allocator using `operator new` / `operator delete`
- [ ] Write an RAII wrapper for a file descriptor; verify no leaks with valgrind
- [ ] Implement a thread pool: fixed N threads, shared task queue, graceful shutdown
- [ ] Build a Unix domain socket server: accept connections, dispatch messages to handlers
- [ ] Structure a CMake project with: one static lib, one executable, one test target
- [ ] **MILESTONE M0-1:** Multithreaded IPC server  Date completed: ___________

### 0.2 · Linux Systems Programming
- [ ] Trace `fork()` → `exec()` → `waitpid()` with `strace`; annotate each syscall
- [ ] Write an `epoll` loop handling `timerfd`, `signalfd`, and a Unix socket simultaneously
- [ ] Use `mmap` to create an anonymous shared memory region between two processes
- [ ] Read `/dev/dri/card0` capabilities; identify which CRTC/encoder/connector chain your monitor uses
- [ ] Use `strace -f` on a running Wayland compositor; identify the socket and rendering syscalls
- [ ] **MILESTONE M0-2:** epoll event loop  Date completed: ___________

### 0.3 · OpenGL and Rendering Pipeline
- [ ] Set up OpenGL 4.x context with GLFW from scratch  no tutorial source
- [ ] Write a vertex shader + fragment shader; compile and link manually with error checking
- [ ] Render a triangle with correct winding order
- [ ] Load and apply a texture to a quad
- [ ] Derive the projection matrix by hand; implement it in GLSL without GLM
- [ ] Render to an FBO; display result as texture on a fullscreen quad
- [ ] Implement a scene graph node with `parent`, `children[]`, and `local_transform`
- [ ] **MILESTONE M0-3:** Spinning textured cube  Date completed: ___________

### 0.4 · Wayland Protocol
- [ ] Complete all chapters of *The Wayland Book* with notes
- [ ] Write a Wayland client: connect, bind `wl_compositor` and `xdg_wm_base`
- [ ] Draw a solid color to a `wl_surface` using `wl_shm`
- [ ] Handle `wl_keyboard` key events; print keycode to stdout
- [ ] Handle `wl_pointer` motion and button events
- [ ] Read and annotate `tinywl.c` fully  every function explained in comments
- [ ] **MILESTONE M0-4:** Wayland client with input  Date completed: ___________

---

## Phase 1 · Compositor and XR Foundation

### 1.1 · Wayland Compositor
- [ ] Initialize wlroots backend; open DRM device or headless fallback
- [ ] Implement `xdg_toplevel` surface creation and destruction
- [ ] Render one client surface to screen using wlroots OpenGL renderer
- [ ] Implement window focus: click to focus, keyboard routes to focused client
- [ ] Implement window move via pointer drag
- [ ] Implement damage tracking with `wlr_output_damage`
- [ ] **MILESTONE M1-1:** Compositor running terminal + browser  Date completed: ___________

### 1.2 · 3D Scene Placement
- [ ] Replace 2D layout with scene graph; each client is a node with a 3D transform
- [ ] Render client surface as a texture on a `wlr_texture`-backed 3D quad
- [ ] Implement fly camera: WASD + mouse-look
- [ ] Place three windows at different world positions and rotations
- [ ] Implement correct depth sorting (painter's algorithm or depth buffer)
- [ ] **MILESTONE M1-2:** Three apps floating in 3D space  Date completed: ___________

### 1.3 · OpenXR Integration
- [ ] Install Monado; run `hello_xr` successfully
- [ ] Write an OpenXR app from scratch: `xrCreateInstance` through `xrDestroyInstance`
- [ ] Create a stereo swapchain; render a cube per eye with correct view matrices
- [ ] Integrate compositor scene into OpenXR swapchain (render scene → write to XR images)
- [ ] Handle full session state machine: all transitions logged
- [ ] **MILESTONE M1-3:** Compositor in Monado-simulated headset  Date completed: ___________

---

## Phase 2 · Spatial OS Layer

### 2.1 · Spatial App Model
- [ ] Define app manifest schema (TOML); write JSON schema for validation
- [ ] Implement app registry: scan directory, parse manifests, maintain state machine per app
- [ ] Implement launch: `fork` + `exec` + place window at manifest-declared transform
- [ ] Implement terminate: SIGTERM → wait → SIGKILL
- [ ] Build 3D launcher: display app name panels spatially, activate on selection
- [ ] **MILESTONE M2-1:** 3D app launcher operational  Date completed: ___________

### 2.2 · IPC and Sandboxing
- [ ] Define NebiumOS IPC message format; write a C++ serialization library for it
- [ ] Implement message bus: named endpoints, synchronous request/reply and async publish
- [ ] Sandbox one app with `CLONE_NEWPID` + `CLONE_NEWNET` + `CLONE_NEWNS`
- [ ] Implement shared spatial object: two apps reference same entity by ID; OS arbitrates writes
- [ ] Document capability token model in `docs/ARCHITECTURE.md`
- [ ] **MILESTONE M2-2:** Two sandboxed apps communicating  Date completed: ___________

### 2.3 · Spatial Input System
- [ ] Implement ray cast from camera/gaze origin + direction into scene
- [ ] Implement ray–plane intersection; return UV coordinate on hit surface
- [ ] Implement input focus manager: track focused surface; route events accordingly
- [ ] Read `XR_EXT_hand_tracking` joint poses per frame via OpenXR
- [ ] Implement pinch detection: thumb tip to index tip distance threshold
- [ ] Map pinch to pointer press event; route through input focus manager
- [ ] **MILESTONE M2-3:** Gaze targeting + pinch input working  Date completed: ___________

### 2.4 · Spatial UI Primitives
- [ ] Generate MSDF font atlas using `msdf-atlas-gen`; integrate into renderer
- [ ] Implement `SpatialPanel`: a 3D rectangle with a rendered background and border
- [ ] Implement `SpatialButton`: idle / hover / pressed visual states
- [ ] Implement `SpatialSlider`: drag interaction along a 3D axis
- [ ] Implement depth-aware layout: prevent panels from z-fighting
- [ ] Build settings panel using primitives; all controls functional
- [ ] **MILESTONE M2-4:** Interactive spatial settings panel  Date completed: ___________

---

## Phase 3 · World Runtime and Persistence

### 3.1 · Scene Graph and ECS
- [ ] Design entity type: `uint64_t` ID, bitmask of component types present
- [ ] Implement component storage: per-type contiguous arrays with entity → index map
- [ ] Implement transform component with parent/child hierarchy
- [ ] Implement octree spatial index; test insertion, removal, range query, raycast query
- [ ] Implement scene serializer: binary format, versioned header
- [ ] Test: serialize scene, restart process, deserialize  verify all entities and transforms match
- [ ] **MILESTONE M3-1:** Persistent scene survives restart  Date completed: ___________

### 3.2 · Physics
- [ ] Integrate Jolt Physics as CMake subdirectory
- [ ] Add `RigidBodyComponent` to ECS; sync Jolt body state → transform component each frame
- [ ] Implement pick up: switch body to kinematic, attach to hand transform
- [ ] Implement release: switch back to dynamic, apply velocity from hand motion
- [ ] Implement throw: apply linear impulse on release
- [ ] **MILESTONE M3-2:** Physics objects in world  Date completed: ___________

### 3.3 · World Persistence Layer
- [ ] Design world graph schema: node type, edge type, timestamp, payload
- [ ] Implement temporal log per entity: ordered snapshots with monotonic timestamps
- [ ] Implement delta encoder: compute diff between consecutive snapshots; store diffs only
- [ ] Implement point-in-time query: reconstruct entity state at any past timestamp by replaying deltas
- [ ] Expose query interface over Unix socket; test with a Python client
- [ ] **MILESTONE M3-3:** Temporal query operational  Date completed: ___________

### 3.4 · Multi-User Sync
- [ ] Define sync protocol: delta message format, sequence numbers, ACK/NAK
- [ ] Implement local sync server (TCP or Unix socket)
- [ ] Connect two NebiumOS instances; verify world state converges
- [ ] Implement authority model: entity owner ID; non-owners reject write attempts
- [ ] Sync avatar transforms in real time; render remote avatar at correct position
- [ ] Route microphone audio via positional source at avatar location
- [ ] **MILESTONE M3-4:** Two-user shared world  Date completed: ___________

---

## Phase 4 · AI Process Layer

- [ ] Implement spatial context document: serialize visible scene to structured text per frame
- [ ] Integrate llama.cpp or ONNX Runtime; verify on-device inference performance
- [ ] Spawn AI agent as a spatial process; place in scene graph
- [ ] Agent perceives scene via context document; emits actions via IPC
- [ ] Integrate Whisper.cpp for on-device voice recognition
- [ ] Implement command parser with spatial reference resolution
- [ ] Natural language query interface over temporal world graph
- [ ] **MILESTONE M4:** AI agent living in persistent world  Date completed: ___________

---

## Milestone Timeline

| Milestone | Target | Completed |
|-----------|--------|-----------|
| M0-1 Multithreaded IPC |  | |
| M0-2 epoll event loop |  | |
| M0-3 Spinning textured cube |  | |
| M0-4 Wayland client |  | |
| M1-1 Wayland compositor |  | |
| M1-2 Apps in 3D space |  | |
| M1-3 Compositor in OpenXR |  | |
| M2-1 3D app launcher |  | |
| M2-2 Sandboxed IPC |  | |
| M2-3 Spatial input |  | |
| M2-4 Spatial UI |  | |
| M3-1 Persistent scene |  | |
| M3-2 Physics |  | |
| M3-3 Temporal query |  | |
| M3-4 Two-user world |  | |
| M4 AI in world |  | |
