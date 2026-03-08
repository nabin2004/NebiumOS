project('nebium', 'c')

wayland = dependency('wayland-server')
wlroots = dependency('wlroots')
xkbcommon = dependency('xkbcommon')

executable('nebium',
    ['compositor.c', 'xdg-shell-protocol.c'],
    dependencies: [wayland, wlroots, xkbcommon],
    c_args: ['-DWLR_USE_UNSTABLE'],
    link_args: ['-lm'])