# NebiumOS  Roadmap

Each phase has a defined scope, a set of implementation tasks, and a milestone artifact  something that compiles, runs, and demonstrates the phase objective. Phases are sequential. Do not begin a phase until the previous milestone is achieved.

---

## Phase 0  Foundations
**Objective:** Acquire working proficiency in the tools and subsystems required to build NebiumOS. No NebiumOS code is written in this phase  only prerequisite skills.

**Duration estimate:** 8–12 weeks

### 0.1 · C++20 Systems Programming
The core of NebiumOS is C++. This phase requires comfort with systems-level C++, not application-level.

Tasks:
- Implement manual memory management: raw pointers, placement new, custom allocators
- Implement RAII wrappers for OS resources (file descriptors, sockets)
- Write a thread pool using `std::thread`, `std::mutex`, `std::condition_variable`
- Build a Unix domain socket IPC system: client sends messages, server dispatches them
- Set up a multi-target CMake project with static libraries

Milestone artifact: **A multithreaded IPC server in C++ using Unix domain sockets.**

---

### 0.2 · Linux Systems Programming
NebiumOS sits directly on top of the Linux kernel interface. This layer must be understood, not abstracted away.

Tasks:
- Trace a full `fork()` → `exec()` → `wait()` cycle; understand what the kernel does at each step
- Write an `epoll`-based event loop that handles: a timer fd, a signal fd, and a Unix socket simultaneously
- Understand virtual memory: `mmap`, anonymous mappings, shared memory (`shm_open`)
- Read the DRM/KMS subsystem documentation; understand what `/dev/dri/card0` exposes and why
- Use `strace` to observe system calls of an existing Wayland compositor

Milestone artifact: **An epoll event loop in C handling timer, signal, and socket events concurrently.**

---

### 0.3 · OpenGL and the Rendering Pipeline
Before writing a spatial compositor, understand how pixels get from GPU memory to a display.

Tasks:
- Set up an OpenGL 4.x context using GLFW  no tutorial copy-paste, type from spec
- Write vertex and fragment shaders in GLSL from scratch
- Render a triangle; then a textured quad; then a cube with correct winding order
- Implement model/view/projection transforms manually  no GLM until you have derived each matrix
- Render to a framebuffer object (FBO); read pixels back; display FBO texture on a quad
- Implement a simple scene graph: nodes with transform hierarchies

Milestone artifact: **A spinning, textured 3D cube rendered from a hand-written OpenGL program.**

---

### 0.4 · Wayland Protocol
NebiumOS uses Wayland as its display protocol. Understand it from the wire level before using wlroots.

Tasks:
- Read *The Wayland Book* in full (wayland-book.com)  all chapters
- Write a Wayland client in C that opens a window using `wl_compositor` and `xdg_wm_base`  no toolkit
- Draw to a `wl_surface` using `wl_shm` shared memory
- Handle `wl_keyboard` and `wl_pointer` input events
- Read `tinywl.c` (the reference minimal wlroots compositor) line by line with comments

Milestone artifact: **A Wayland client in C that opens a window, draws a colored buffer, and handles keyboard input.**

---

## Phase 1  Compositor and XR Foundation
**Objective:** Build a working Wayland compositor that renders its output into a 3D scene viewable through an OpenXR-compatible device or simulator.

**Duration estimate:** 10–14 weeks

### 1.1 · Wayland Compositor with wlroots
Tasks:
- Initialize a wlroots backend (auto-detect: DRM/KMS on bare metal, headless for CI)
- Implement the `xdg_shell` protocol to accept client windows
- Render client surfaces as textured quads using OpenGL
- Handle focus, raise/lower, and basic keyboard routing
- Implement damage tracking: only re-render changed regions

Milestone artifact: **A Wayland compositor that runs a terminal emulator and a browser side-by-side.**

---

### 1.2 · 3D Scene Placement
Tasks:
- Replace the 2D layout engine with a 3D scene graph
- Each client window is a textured quad placed at a 3D transform (position, rotation, scale)
- Implement a fly camera: WASD translation, mouse-look rotation
- Support placing windows at arbitrary 3D positions and orientations
- Implement basic depth sorting for transparent surfaces

Milestone artifact: **Three application windows floating at different positions and angles in 3D space, navigable with a keyboard/mouse camera.**

---

### 1.3 · OpenXR Integration
Tasks:
- Install Monado on Linux; verify with `hello_xr` sample
- Write an OpenXR application from scratch: create instance, session, swapchain
- Render a stereo scene (one cube per eye) with correct per-eye view/projection matrices
- Integrate compositor output: render the 3D window scene into the OpenXR swapchain
- Handle session lifecycle: IDLE → READY → SYNCHRONIZED → VISIBLE → FOCUSED

Milestone artifact: **The 3D compositor output rendered into a Monado-simulated headset with correct stereo separation.**

---

## Phase 2  Spatial OS Layer
**Objective:** Design and implement the core OS abstractions: app lifecycle in 3D space, inter-process communication, spatial input routing, and basic UI primitives.

**Duration estimate:** 12–16 weeks

### 2.1 · Spatial App Model
Tasks:
- Define the NebiumOS app manifest format (TOML): name, entry point, initial 3D placement, declared permissions
- Implement an app registry: scan a directory for manifests, maintain app state machine
- Implement app launch: spawn process, place window at declared 3D position
- Implement app termination: clean shutdown + forced kill after timeout
- Build a 3D app launcher: display app icons as 3D panels, launch on selection

Milestone artifact: **A 3D app launcher that reads manifests, displays icons spatially, and launches/terminates apps.**

---

### 2.2 · IPC and Sandboxing
Tasks:
- Design the NebiumOS IPC message format (fixed-size header + variable payload)
- Implement a message bus: apps register named endpoints, send/receive typed messages
- Sandbox an app using Linux namespaces (`CLONE_NEWPID`, `CLONE_NEWNET`, `CLONE_NEWNS`)
- Implement shared spatial objects: two apps hold references to the same 3D entity
- Define capability tokens: apps declare which spatial permissions they hold

Milestone artifact: **Two sandboxed apps exchanging messages and sharing a reference to the same 3D object.**

---

### 2.3 · Spatial Input System
Tasks:
- Implement ray casting from camera/eye forward vector into the scene
- Implement ray–plane intersection for input targeting on 3D panels
- Implement input focus: track which spatial surface currently owns input
- Integrate OpenXR hand tracking (`XR_EXT_hand_tracking`): read joint poses per frame
- Map pinch gesture (thumb–index proximity) to a click event
- Route all input events through the spatial input arbiter  no direct app access to raw input

Milestone artifact: **Interact with floating app panels using gaze targeting and pinch gesture.**

---

### 2.4 · Spatial UI Primitives
Tasks:
- Implement SDF text rendering in 3D using `msdf-atlas-gen`
- Implement: `SpatialPanel`, `SpatialButton` (idle/hover/pressed states), `SpatialSlider`
- Implement depth-aware layout: panels auto-space to avoid overlap
- Implement a notification surface: system messages appear briefly in a fixed world position
- Document the UI primitive API in `docs/ARCHITECTURE.md`

Milestone artifact: **A spatial system settings panel with working buttons and sliders, controllable via gaze + pinch.**

---

## Phase 3  World Runtime and Persistence
**Objective:** Build a persistent, shared world: a scene that survives process restarts and can be shared between multiple connected users.

**Duration estimate:** 14–20 weeks

### 3.1 · Scene Graph and ECS
Tasks:
- Design and implement an entity-component system (ECS): entities are integer IDs; components are typed data
- Implement transform hierarchy: parent/child spatial relationships
- Implement a spatial index (octree or BVH) for efficient proximity and raycast queries
- Serialize the full scene to a binary format on shutdown; deserialize on startup
- Implement change events: components emit notifications when mutated

Milestone artifact: **A scene that serializes to disk and restores with full fidelity on restart.**

---

### 3.2 · Physics
Tasks:
- Integrate Jolt Physics as a statically linked library
- Add rigid body components to spatial entities
- Implement: pick up an object (kinematic mode), release (dynamic mode), throw (impulse)
- Sync physics state back to the scene graph each frame

Milestone artifact: **Objects in the scene with accurate rigid body physics; user can pick up and throw them.**

---

### 3.3 · World Persistence Layer
Tasks:
- Design world state as a directed, typed graph: nodes are entities, edges are named relations
- Each node carries a temporal log: ordered list of state snapshots with timestamps
- Implement delta encoding: store diffs between snapshots, not full copies
- Expose a query interface: retrieve entity state at arbitrary past timestamp
- Connect to Dagestan memory architecture: world graph is the grounding layer for LLM memory

Milestone artifact: **Query the state of any object at any past time; LLM can answer "what was on the desk at 14:00?"**

---

### 3.4 · Multi-User Sync
Tasks:
- Define the NebiumOS world sync protocol: delta messages over a reliable transport
- Implement a local sync server; two NebiumOS clients connect and share world state
- Implement authority model: one client owns each object; only the owner sends state updates
- Implement basic presence: avatar positions and orientations sync in real time
- Implement positional audio routing: voice arrives from the direction of the speaker's avatar

Milestone artifact: **Two users in the same persistent world, seeing each other's avatars and sharing object state.**

---

## Phase 4  AI Process Layer
**Objective:** Make AI a first-class OS service: agents that live in the world, world state queryable by language models, voice control grounded in spatial context.

**Duration estimate:** Ongoing

### 4.1 · Spatial Context Service
- World state graph exposed as a structured context document for LLMs
- Context is scoped: only objects within user's spatial attention window are included
- Updated incrementally per frame; stale context is evicted

### 4.2 · Embodied Agent Processes
- AI agents are spawned as spatial processes with a position in the scene graph
- Agents perceive: nearby objects, user gaze direction, recent interaction events
- Agents act: move objects, emit speech, update their own state
- Agent lifecycle managed by the spatial app model (same as human-facing apps)

### 4.3 · Voice Control with Spatial Grounding
- Whisper.cpp for on-device speech recognition
- Command parsed against spatial context (current focus, gaze target, nearby objects)
- Commands like "move that next to the clock" resolved via spatial reference resolution

### 4.4 · Temporal World Queries
- Natural language interface over the temporal world graph
- "What did I do in this room this morning?" resolved against event log
- Powered by Dagestan temporal knowledge graph + local LLM inference

---

## Milestone Summary

| ID | Milestone | Demonstrates |
|----|-----------|-------------|
| M0-1 | Multithreaded IPC server | C++ systems competency |
| M0-2 | epoll event loop | Linux systems competency |
| M0-3 | Spinning textured cube | Rendering pipeline competency |
| M0-4 | Wayland client with input | Display protocol competency |
| M1-1 | Wayland compositor | Compositor architecture |
| M1-2 | Apps in 3D space | Spatial rendering |
| M1-3 | Compositor in OpenXR | XR device integration |
| M2-1 | 3D app launcher | Spatial app model |
| M2-2 | Sandboxed IPC | OS process isolation |
| M2-3 | Gaze + pinch input | Spatial input pipeline |
| M2-4 | Spatial UI panel | Spatial UI system |
| M3-1 | Persistent scene | World serialization |
| M3-2 | Physics in world | Physical simulation |
| M3-3 | Temporal world graph | AI-queryable world state |
| M3-4 | Two-user shared world | Multi-user presence |
| M4-x | AI agents in world | AI-native OS |
