<h1 align='center'>NebiumOS</h1>
<p align='center'>An operating system for spatial computing devices.</p>

NebiumOS is a ground-up implementation of a spatial computing runtime. The software layer that sits between hardware and applications on a head-worn spatial device. It handles display compositing, 3D app lifecycle, spatial input routing, world-state persistence, and AI-native process services.

---

## What "Spatial Computing OS" Means

A conventional OS manages a 2D desktop metaphor: windows, a cursor, a flat display. A spatial computing OS manages a fundamentally different environment:

- The display is stereoscopic and head-tracked. The compositor must account for eye position, lens distortion, and reprojection
- Input comes from gaze, hand pose, and voice not a mouse and keyboard
- Applications exist at arbitrary positions and orientations in 3D space
- The "desktop" is the physical room around the user
- Multiple users can share the same spatial environment

---

## Architecture

NebiumOS is structured as a layered stack. Each layer has a clearly defined interface to the one above and below it.

```
┌──────────────────────────────────────────────────────┐
│  Layer 5 · Social & Presence Services                │
│  Multi-user session management, avatar state,        │
│  shared object authority, spatial audio routing      │
├──────────────────────────────────────────────────────┤
│  Layer 4 · World Runtime                             │
│  Scene graph (ECS), physics, persistence, delta sync │
├──────────────────────────────────────────────────────┤
│  Layer 3 · Spatial OS  ← primary focus               │
│  3D window manager, app model, IPC, spatial I/O      │
├──────────────────────────────────────────────────────┤
│  Layer 2 · Rendering & Compositor                    │
│  Wayland compositor, Vulkan render pipeline,         │
│  OpenXR session management, reprojection             │
├──────────────────────────────────────────────────────┤
│  Layer 1 · Host Platform                             │
│  Linux kernel, DRM/KMS, GPU drivers (not modified)   │
└──────────────────────────────────────────────────────┘
```

**Layer 1 is not reimplemented.** NebiumOS runs on Linux. The kernel, drivers, and hardware abstraction are treated as a stable substrate.

**Layers 2–4 are the implementation target.** Layer 5 is planned but deferred.

---

## Repository Structure

```
NebiumOS/
├── README.md
├── docs/
│   ├── ROADMAP.md           phased build plan with milestones
│   ├── CHECKLIST.md         granular task tracking
│   ├── LEARNING.md          study curriculum and references
│   └── ARCHITECTURE.md      design decisions and rationale (ADR log)
├── src/
│   ├── compositor/          Wayland compositor (wlroots-based)
│   ├── xr-runtime/          OpenXR session, swapchain, view management
│   ├── spatial-wm/          3D window manager, scene placement
│   ├── app-model/           app lifecycle, IPC, sandboxing
│   └── world-runtime/       ECS scene graph, persistence, sync
└── resources/
    ├── papers/              reference papers
    ├── diagrams/            architecture diagrams
    └── notes/               research notes
```

---

## Build Status

| Component | Phase | Status |
|-----------|-------|--------|
| Wayland compositor | Phase 1 | Not started |
| OpenXR integration | Phase 1 | Not started |
| 3D window manager | Phase 2 | Not started |
| Spatial app model | Phase 2 | Not started |
| IPC + sandboxing | Phase 2 | Not started |
| Spatial input | Phase 2 | Not started |
| Scene graph (ECS) | Phase 3 | Not started |
| World persistence | Phase 3 | Not started |
| Multi-user sync | Phase 3 | Not started |
| AI process layer | Phase 4 | Not started |

---

## Technology Stack

| Concern | Technology | Reason |
|---------|-----------|--------|
| Host OS | Linux (kernel 6.x) | Stable, open, DRM/KMS access |
| Display protocol | Wayland + wlroots | Modern compositor protocol, no X11 legacy |
| XR standard | OpenXR 1.x | Vendor-neutral, Khronos-backed |
| Development runtime | Monado | Open-source OpenXR runtime for Linux |
| Rendering (early) | OpenGL 4.x + GLFW | Fast iteration in Phase 0–1 |
| Rendering (target) | Vulkan 1.3 | Required latency targets for XR (<20ms) |
| Primary language | C++20 | Native access to all system libraries |
| Safe subsystems | Rust | IPC, serialization, world state |
| Scripting / tooling | Python 3 | Build tools, data processing |
| Physics | Jolt Physics | Performance, open license |
| World state | Custom graph store | Temporal, AI-queryable (see below) |

---

## Design Constraints

- Must run on commodity Linux hardware (no proprietary headset required for development)
- No vendored black-box runtime dependencies for core layers
- Every interface between layers must be documented before it is implemented
- Latency budget for render loop: ≤20ms end-to-end (50fps minimum)