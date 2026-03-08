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
#include <wlr/util/log.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

struct nebium_server {
    struct wl_display *display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_output_layout;
    struct wlr_output_layout *output_layout;
    struct wlr_xdg_shell *xdg_shell;

    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
};

struct nebium_output {
    struct wlr_output *wlr_output;
    struct nebium_server *server;
    struct wl_listener frame;
    struct wlr_scene_output *scene_output;
};

struct nebium_view {
    struct wlr_xdg_surface *xdg_surface;
    struct wlr_scene_node *scene_node;
};

static void output_frame(struct wl_listener *listener, void *data) {
    struct nebium_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene *scene = output->server->scene;

    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(
        scene, output->wlr_output);

    wlr_scene_output_commit(scene_output, NULL);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void server_new_output(struct wl_listener *listener, void *data) {
    struct nebium_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if (!wlr_output_commit(wlr_output)) {
            return;
        }
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

static void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    struct nebium_server *server =
        wl_container_of(listener, server, new_xdg_surface);
    struct wlr_xdg_surface *xdg_surface = data;

    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    struct nebium_view *view = calloc(1, sizeof(*view));
    view->xdg_surface = xdg_surface;

    view->scene_node = &wlr_scene_xdg_surface_create(
        &server->scene->tree, xdg_surface)->node;

    wlr_log(WLR_INFO, "New window created");
}

int main() {
    wlr_log_init(WLR_DEBUG, NULL);

    struct nebium_server server = {0};

    server.display = wl_display_create();
    if (!server.display) {
        fprintf(stderr, "Failed to create display\n");
        return 1;
    }

    server.backend = wlr_backend_autocreate(server.display, NULL);
    if (!server.backend) {
        fprintf(stderr, "Failed to create backend\n");
        return 1;
    }

    server.renderer = wlr_renderer_autocreate(server.backend);
    if (!server.renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        return 1;
    }
    wlr_renderer_init_wl_display(server.renderer, server.display);

    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    if (!server.allocator) {
        fprintf(stderr, "Failed to create allocator\n");
        return 1;
    }

    wlr_compositor_create(server.display, 5, server.renderer);
    wlr_subcompositor_create(server.display);

    server.scene = wlr_scene_create();
    server.output_layout = wlr_output_layout_create();
    server.scene_output_layout = wlr_scene_attach_output_layout(
        server.scene, server.output_layout);

    server.xdg_shell = wlr_xdg_shell_create(server.display, 3);
    server.new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server.xdg_shell->events.new_surface,
        &server.new_xdg_surface);

    server.new_output.notify = server_new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    const char *socket = wl_display_add_socket_auto(server.display);
    if (!socket) {
        fprintf(stderr, "Failed to add socket\n");
        return 1;
    }

    printf("Nebium compositor started on %s\n", socket);

    if (!wlr_backend_start(server.backend)) {
        fprintf(stderr, "Failed to start backend\n");
        return 1;
    }

    wl_display_run(server.display);

    wlr_allocator_destroy(server.allocator);
    wlr_renderer_destroy(server.renderer);
    wlr_backend_destroy(server.backend);
    wl_display_destroy(server.display);

    return 0;
}