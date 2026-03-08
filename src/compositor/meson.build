project('nebium', 'c')

wayland = dependency('wayland-server')
wlroots = dependency('wlroots')

executable('nebium',
    ['compositor.c', 'xdg-shell-protocol.c'],
    dependencies: [wayland, wlroots],
    c_args: ['-DWLR_USE_UNSTABLE'],
    link_args: ['-lm'])