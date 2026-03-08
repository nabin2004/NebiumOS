# NebiumOS Architecture

This document records the technical design of NebiumOS and the rationale for each decision. It is updated as the design evolves. Decisions made and later revised are kept in the log with their revision history.

---

## System Overview

NebiumOS is a spatial computing operating system implemented as a userspace runtime on top of the Linux kernel. It does not modify the kernel. It provides a spatial application environment: a 3D display compositor, a spatial app lifecycle manager, a world state runtime, and AI process services.

The system is organized into layers with explicit interfaces between them. No layer calls downward more than one layer.

---

## Architecture Decision Records (ADR)

### ADR-001: Do not write a custom kernel

**Decision:** NebiumOS runs on the Linux kernel. Kernel-level code is out of scope.

**Reasoning:** The novel problems in a spatial computing OS are at the compositor, app model, and world runtime layers  not at the hardware abstraction layer. Writing a kernel from scratch would require years of work to reach the level of stability and driver coverage that Linux already provides, and would produce no additional understanding of spatial computing specifically. Every production spatial OS (visionOS, Horizon OS, SteamOS) is built on a Unix kernel.

**Implications:** DRM/KMS, GPU drivers, USB HID, and network drivers are treated as stable interfaces. NebiumOS code begins at the DRM/KMS user-space API and Wayland socket layer.

---

### ADR-002: Use Wayland as the display protocol

**Decision:** NebiumOS implements a Wayland compositor using the wlroots library.

**Reasoning:** Wayland is the current Linux display protocol standard. It has a clean, minimal protocol design with explicit ownership semantics for buffers. X11 is legacy and has fundamental architectural problems (global input, synchronous rendering, no buffer ownership). The wlroots library provides well-abstracted primitives for backends (DRM, headless, X11), renderers, and allocators, while exposing enough of the underlying mechanism to remain educational.

**Implications:** NebiumOS apps are Wayland clients. The compositor implements `xdg_shell`. Legacy X11 apps require XWayland; this is deferred and optional.

---

### ADR-003: Use OpenXR as the XR abstraction layer

**Decision:** NebiumOS targets the OpenXR 1.x API for all XR device access.

**Reasoning:** OpenXR is the Khronos open standard for XR runtimes, supported by Meta (Quest), Valve (SteamVR), Microsoft (Windows Mixed Reality), and open-source implementations (Monado). Writing to OpenXR means the compositor is portable across any conformant headset. Monado is an open-source OpenXR runtime for Linux that enables full development without hardware.

**Implications:** All headset input (hand tracking, eye tracking, controller poses) is accessed through OpenXR action sets. Vendor-specific extensions may be used optionally but are never required for core functionality.

---

### ADR-004: OpenGL for Phase 0–1, Vulkan for Phase 2+

**Decision:** The rendering backend begins with OpenGL 4.x. It is replaced with Vulkan no later than the end of Phase 2.

**Reasoning:** Vulkan requires approximately 800 lines of setup code to render a triangle. During Phase 0–1, the primary learning objectives are compositing architecture and spatial placement, not GPU synchronization. OpenGL allows faster iteration. However, OpenGL driver overhead is unacceptable for real-time XR: the target is ≤20ms total frame latency at 72–90Hz, which requires explicit GPU synchronization and low-overhead command submission. Vulkan is required to achieve this.

**Migration plan:** The renderer is abstracted behind a `INebiumRenderer` interface from the start. The OpenGL and Vulkan implementations are interchangeable behind this interface.

---

### ADR-005: C++20 as primary implementation language

**Decision:** Core NebiumOS components are written in C++20. Tooling and scripting are Python 3. Safe subsystems (IPC serialization, world state graph) may be written in Rust with a C ABI boundary.

**Reasoning:** The graphics, XR, compositor, and physics ecosystems are all C/C++. Direct use of wlroots, OpenXR, Vulkan, and Jolt Physics requires C++ or C. Rust bindings for these libraries lag behind their C++ counterparts and introduce dependency risk. C++20 provides sufficient expressiveness (concepts, ranges, coroutines) for safe systems code when used with discipline (RAII, smart pointers, no raw `new`/`delete` outside allocator code).

**Code standards:**
- No raw `new` / `delete` outside of allocator implementations
- All resources wrapped in RAII types
- No `nullptr` dereferences: use `std::optional` or explicit null checks
- Threading: no data races; shared state protected by `std::mutex` or atomic operations

---

### ADR-006: Entity-Component System for the scene graph

**Decision:** The NebiumOS world runtime uses an ECS architecture. Entities are `uint64_t` IDs. Components are plain data structures stored in contiguous arrays per type. Systems iterate over component arrays.

**Reasoning:** The spatial world contains potentially thousands of entities. A deep inheritance hierarchy (SceneNode → SpatialObject → PhysicsObject → ...) causes poor cache performance and rigid coupling. ECS decouples data from behavior, enables cache-friendly iteration, and makes serialization straightforward (serialize component arrays, not object graphs). This is the architecture used by Unity (DOTS), Unreal (Mass Entity), and custom engines at all major XR companies.

**Implications:** No virtual dispatch in hot paths. Components are POD or near-POD structs. Systems are free functions or stateless functors that take component arrays as parameters.

---

### ADR-007: World state as a temporal knowledge graph

**Decision:** World state is stored as a directed, typed graph with temporal indexing. Each entity is a node. Named, typed relations are edges. Every mutation is appended to a per-entity log with a monotonic timestamp; state is reconstructed by replaying the log.

**Reasoning:** A flat ECS component store with no history cannot answer temporal queries ("what was here at time T?"). An append-only temporal graph can. Temporal world state is directly usable as the grounding layer for LLM memory  this connects to the Dagestan temporal knowledge graph architecture. The graph structure also models inter-object relationships (this object is on top of that surface; this app window is owned by that process) that a pure component store cannot represent without special-casing.

**Implications:** Write amplification: every mutation writes a delta to the log. The delta encoder must be efficient. Queries reconstruct state by replaying deltas; a snapshot cache is maintained for the current time to avoid full replay on every read.

---

### ADR-008: AI as a first-class OS process, not an application

**Decision:** AI inference services in NebiumOS are OS-level processes, not application-layer libraries.

**Reasoning:** An LLM embedded in each app wastes memory and prevents cross-app coordination. A single AI process managed by the OS can: serve multiple apps, maintain a consistent world model, receive OS-level events (app launch, spatial focus change, object interaction), and respond with actions that affect the OS directly. This mirrors how the kernel provides system services  apps request them; the OS fulfills them.

**Implications:** An AI process receives a structured context document from the OS each frame (or on event), performs inference, and emits typed action messages back to the OS via IPC. The inference model runs locally (llama.cpp or ONNX Runtime); no cloud dependency is required.

---

## Interface Specifications

*(Interface definitions are added here as each layer is implemented.)*

### Compositor → Spatial Window Manager
- Not yet defined. To be designed in Phase 1.2.

### Spatial WM → App Model
- Not yet defined. To be designed in Phase 2.1.

### App Model → IPC Bus
- Not yet defined. To be designed in Phase 2.2.

### World Runtime → Persistence Layer
- Not yet defined. To be designed in Phase 3.1.

---

## Performance Targets

| Metric | Target | Rationale |
|--------|--------|-----------|
| Frame-to-photon latency | ≤20ms | Below vestibulo-ocular reflex threshold (~25ms) |
| Compositor frame time | ≤11ms at 90Hz | Leaves headroom for reprojection |
| Input-to-response latency | ≤50ms | Perceptible interaction threshold |
| World sync delta | ≤100ms | Acceptable presence latency |
| AI inference (voice command) | ≤300ms | Natural conversation threshold |

---

## Open Design Questions

These questions are unresolved. They will be resolved  with documented rationale  before the relevant phase begins.

1. **Rendering architecture:** Single GPU process handling all rendering, or per-app GPU isolation with a compositor merging outputs? (tradeoff: security vs. complexity)
2. **App isolation depth:** Linux namespaces sufficient, or add `seccomp-bpf` syscall filtering? What is the attack surface of a spatial app?
3. **World sync transport:** Custom binary protocol over TCP, or adapt an existing protocol (e.g., WebRTC data channels for NAT traversal)?
4. **Snapshot cache policy:** How many historical snapshots to cache in memory before forcing a log replay from disk?
5. **AI model selection:** Which model size and quantization hits the ≤300ms inference target on a mid-range CPU (no discrete GPU)?
