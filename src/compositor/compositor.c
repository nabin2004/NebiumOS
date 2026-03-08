#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    NEBIUM_CURSOR_PASSTHROUGH,
    NEBIUM_CURSOR_MOVE,
} nebium_cursor_mode;

struct nebium_server {
    struct wl_display *display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_output_layout;
    struct wlr_output_layout *output_layout;
    struct wlr_xdg_shell *xdg_shell;

    struct wlr_seat *seat;
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;

    nebium_cursor_mode cursor_mode;
    struct nebium_view *grabbed_view;
    double grab_x, grab_y;
    int grab_orig_x, grab_orig_y;

    struct wl_list views;

    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    struct wl_listener new_input;
    struct wl_listener request_set_cursor;
};

struct nebium_output {
    struct wlr_output *wlr_output;
    struct nebium_server *server;
    struct wl_listener frame;
    struct wlr_scene_output *scene_output;
};

struct nebium_view {
    struct wl_list link;
    struct nebium_server *server;
    struct wlr_xdg_surface *xdg_surface;
    struct wlr_scene_tree *scene_tree;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener commit;
    struct wl_listener request_move;
    int x, y;
};

// ── Output ────────────────────────────────────────────────────────────────────

static void output_frame(struct wl_listener *listener, void *data) {
    struct nebium_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(
        output->server->scene, output->wlr_output);
    wlr_scene_output_commit(scene_output, NULL);
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void server_new_output(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) return;
    }

    struct nebium_output *output = calloc(1, sizeof(*output));
    output->wlr_output = wlr_output;
    output->server = server;
    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    struct wlr_output_layout_output *layout_output =
        wlr_output_layout_add_auto(server->output_layout, wlr_output);
    output->scene_output = wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_output_layout,
        layout_output, output->scene_output);

    wlr_log(WLR_INFO, "New output: %s", wlr_output->name);
}

// ── View helpers ──────────────────────────────────────────────────────────────

static struct nebium_view *view_at(struct nebium_server *server,
        double lx, double ly,
        struct wlr_surface **surface, double *sx, double *sy) {
    struct nebium_view *view;
    wl_list_for_each(view, &server->views, link) {
        struct wlr_xdg_surface *xdg = view->xdg_surface;
        if (xdg->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) continue;
        double view_sx = lx - view->x;
        double view_sy = ly - view->y;
        double _sx, _sy;
        struct wlr_surface *_surface =
            wlr_xdg_surface_surface_at(xdg, view_sx, view_sy, &_sx, &_sy);
        if (_surface) {
            *sx = _sx;
            *sy = _sy;
            *surface = _surface;
            return view;
        }
    }
    return NULL;
}

static void focus_view(struct nebium_view *view, struct wlr_surface *surface) {
    if (!view) return;
    struct nebium_server *server = view->server;
    struct wlr_seat *seat = server->seat;
    struct wlr_surface *prev = seat->keyboard_state.focused_surface;

    if (prev == surface) return;

    if (prev) {
        struct wlr_xdg_surface *prev_xdg =
            wlr_xdg_surface_try_from_wlr_surface(prev);
        if (prev_xdg)
            wlr_xdg_toplevel_set_activated(prev_xdg->toplevel, false);
    }

    wl_list_remove(&view->link);
    wl_list_insert(&server->views, &view->link);

    wlr_xdg_toplevel_set_activated(view->xdg_surface->toplevel, true);

    struct wlr_keyboard *kb = wlr_seat_get_keyboard(seat);
    if (kb) {
        wlr_seat_keyboard_notify_enter(seat, surface,
            kb->keycodes, kb->num_keycodes, &kb->modifiers);
    }
}

// ── Interactive move ──────────────────────────────────────────────────────────

static void begin_interactive(struct nebium_view *view,
        nebium_cursor_mode mode) {
    struct nebium_server *server = view->server;
    server->grabbed_view = view;
    server->cursor_mode = mode;
    server->grab_x = server->cursor->x - view->x;
    server->grab_y = server->cursor->y - view->y;
    server->grab_orig_x = view->x;
    server->grab_orig_y = view->y;
}

static void request_move(struct wl_listener *listener, void *data) {
    struct nebium_view *view =
        wl_container_of(listener, view, request_move);
    begin_interactive(view, NEBIUM_CURSOR_MOVE);
}

// ── Cursor ────────────────────────────────────────────────────────────────────

static void process_cursor_motion(struct nebium_server *server, uint32_t time) {
    if (server->cursor_mode == NEBIUM_CURSOR_MOVE) {
        struct nebium_view *view = server->grabbed_view;
        view->x = server->cursor->x - server->grab_x;
        view->y = server->cursor->y - server->grab_y;
        wlr_scene_node_set_position(&view->scene_tree->node,
            view->x, view->y);
        return;
    }

    double sx, sy;
    struct wlr_surface *surface = NULL;
    struct nebium_view *view = view_at(server,
        server->cursor->x, server->cursor->y, &surface, &sx, &sy);

    if (!view) {
        wlr_cursor_set_xcursor(server->cursor,
            server->cursor_mgr, "default");
    }

    if (surface) {
        wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
    } else {
        wlr_seat_pointer_clear_focus(server->seat);
    }
}

static void cursor_motion(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;
    wlr_cursor_move(server->cursor, &event->pointer->base,
        event->delta_x, event->delta_y);
    process_cursor_motion(server, event->time_msec);
}

static void cursor_motion_absolute(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;
    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base,
        event->x, event->y);
    process_cursor_motion(server, event->time_msec);
}

static void cursor_button(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;

    wlr_seat_pointer_notify_button(server->seat,
        event->time_msec, event->button, event->state);

    if (event->state == WLR_BUTTON_RELEASED) {
        server->cursor_mode = NEBIUM_CURSOR_PASSTHROUGH;
        server->grabbed_view = NULL;
        return;
    }

    double sx, sy;
    struct wlr_surface *surface = NULL;
    struct nebium_view *view = view_at(server,
        server->cursor->x, server->cursor->y, &surface, &sx, &sy);
    focus_view(view, surface);
}

static void cursor_axis(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, cursor_axis);
    struct wlr_pointer_axis_event *event = data;
    wlr_seat_pointer_notify_axis(server->seat,
        event->time_msec, event->orientation, event->delta,
        event->delta_discrete, event->source);
}

static void cursor_frame(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, cursor_frame);
    wlr_seat_pointer_notify_frame(server->seat);
}

// ── Input ─────────────────────────────────────────────────────────────────────

static void server_new_keyboard(struct nebium_server *server,
        struct wlr_input_device *device) {
    struct wlr_keyboard *kb = wlr_keyboard_from_input_device(device);
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(
        ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    wlr_keyboard_set_keymap(kb, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    wlr_keyboard_set_repeat_info(kb, 25, 600);
    wlr_seat_set_keyboard(server->seat, kb);
}

static void server_new_pointer(struct nebium_server *server,
        struct wlr_input_device *device) {
    wlr_cursor_attach_input_device(server->cursor, device);
}

static void server_new_input(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        server_new_keyboard(server, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        server_new_pointer(server, device);
        break;
    default:
        break;
    }

    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (wlr_seat_get_keyboard(server->seat)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, caps);
}

static void request_set_cursor(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, request_set_cursor);
    struct wlr_seat_pointer_request_set_cursor_event *event = data;
    struct wlr_seat_client *focused =
        server->seat->pointer_state.focused_client;
    if (focused == event->seat_client) {
        wlr_cursor_set_surface(server->cursor,
            event->surface, event->hotspot_x, event->hotspot_y);
    }
}

// ── XDG shell ─────────────────────────────────────────────────────────────────

static void xdg_surface_commit(struct wl_listener *listener, void *data) {
    struct nebium_view *view = wl_container_of(listener, view, commit);
    if (view->xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL &&
            view->xdg_surface->surface->mapped) {
        // scene graph handles rendering; nothing extra needed here
        // but having this listener ensures commits are processed
    }
}

static void xdg_surface_map(struct wl_listener *listener, void *data) {
    struct nebium_view *view = wl_container_of(listener, view, map);
    wl_list_insert(&view->server->views, &view->link);
    focus_view(view, view->xdg_surface->surface);
}

static void xdg_surface_unmap(struct wl_listener *listener, void *data) {
    struct nebium_view *view = wl_container_of(listener, view, unmap);
    wl_list_remove(&view->link);
}

static void xdg_surface_destroy(struct wl_listener *listener, void *data) {
    struct nebium_view *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->commit.link);
    wl_list_remove(&view->request_move.link);
    wl_list_remove(&view->link);
    free(view);
}

static void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, new_xdg_surface);
    struct wlr_xdg_surface *xdg_surface = data;

    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) return;

    struct nebium_view *view = calloc(1, sizeof(*view));
    view->server = server;
    view->xdg_surface = xdg_surface;
    view->scene_tree = wlr_scene_xdg_surface_create(
        &server->scene->tree, xdg_surface);
    view->scene_tree->node.data = view;
    xdg_surface->data = view;

    // Set initial position
    view->x = 100;
    view->y = 100;
    wlr_scene_node_set_position(&view->scene_tree->node, view->x, view->y);

    view->map.notify = xdg_surface_map;
    wl_signal_add(&xdg_surface->surface->events.map, &view->map);
    view->unmap.notify = xdg_surface_unmap;
    wl_signal_add(&xdg_surface->surface->events.unmap, &view->unmap);
    view->destroy.notify = xdg_surface_destroy;
    wl_signal_add(&xdg_surface->events.destroy, &view->destroy);
    view->commit.notify = xdg_surface_commit;
    wl_signal_add(&xdg_surface->surface->events.commit, &view->commit);
    view->request_move.notify = request_move;
    wl_signal_add(&xdg_surface->toplevel->events.request_move,
        &view->request_move);

    // Send initial configure — lets client know compositor is ready
    wlr_xdg_toplevel_set_size(xdg_surface->toplevel, 0, 0);
    wlr_xdg_surface_schedule_configure(xdg_surface);

    wlr_log(WLR_INFO, "New window created");
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    wlr_log_init(WLR_DEBUG, NULL);

    struct nebium_server server = {0};
    wl_list_init(&server.views);

    server.display = wl_display_create();
    if (!server.display) { fprintf(stderr, "Failed to create display\n"); return 1; }

    server.backend = wlr_backend_autocreate(server.display, NULL);
    if (!server.backend) { fprintf(stderr, "Failed to create backend\n"); return 1; }

    server.renderer = wlr_renderer_autocreate(server.backend);
    if (!server.renderer) { fprintf(stderr, "Failed to create renderer\n"); return 1; }
    wlr_renderer_init_wl_display(server.renderer, server.display);

    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    if (!server.allocator) { fprintf(stderr, "Failed to create allocator\n"); return 1; }

    wlr_compositor_create(server.display, 5, server.renderer);
    wlr_subcompositor_create(server.display);
    wlr_data_device_manager_create(server.display);

    server.scene = wlr_scene_create();
    server.output_layout = wlr_output_layout_create();
    server.scene_output_layout = wlr_scene_attach_output_layout(
        server.scene, server.output_layout);

    server.xdg_shell = wlr_xdg_shell_create(server.display, 3);
    server.new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server.xdg_shell->events.new_surface,
        &server.new_xdg_surface);

    server.cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server.cursor, server.output_layout);
    server.cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    wlr_xcursor_manager_load(server.cursor_mgr, 1);

    server.cursor_motion.notify = cursor_motion;
    wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
    server.cursor_motion_absolute.notify = cursor_motion_absolute;
    wl_signal_add(&server.cursor->events.motion_absolute,
        &server.cursor_motion_absolute);
    server.cursor_button.notify = cursor_button;
    wl_signal_add(&server.cursor->events.button, &server.cursor_button);
    server.cursor_axis.notify = cursor_axis;
    wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);
    server.cursor_frame.notify = cursor_frame;
    wl_signal_add(&server.cursor->events.frame, &server.cursor_frame);

    server.seat = wlr_seat_create(server.display, "seat0");
    server.new_input.notify = server_new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input);
    server.request_set_cursor.notify = request_set_cursor;
    wl_signal_add(&server.seat->events.request_set_cursor,
        &server.request_set_cursor);

    server.new_output.notify = server_new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    const char *socket = wl_display_add_socket_auto(server.display);
    if (!socket) { fprintf(stderr, "Failed to add socket\n"); return 1; }

    printf("Nebium compositor started on %s\n", socket);

    if (!wlr_backend_start(server.backend)) {
        fprintf(stderr, "Failed to start backend\n"); return 1;
    }

    wl_display_run(server.display);

    wlr_xcursor_manager_destroy(server.cursor_mgr);
    wlr_cursor_destroy(server.cursor);
    wlr_allocator_destroy(server.allocator);
    wlr_renderer_destroy(server.renderer);
    wlr_backend_destroy(server.backend);
    wl_display_destroy(server.display);

    return 0;
}