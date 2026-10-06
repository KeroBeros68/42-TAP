# 42-TAP

[Color palette]https://coolors.co/palette/606c38-283618-fefae0-dda15e-bc6c25
#606c38
#283618
#fefae0
#dda15e
#bc6c25

## GUI: valgrind and the Qt "leaks"

valgrind reports a few hundred KB of leaks in `clients/gui/42TAP`. None of it is ours:
~360 KB is Mesa's OpenGL driver (`libGLX_mesa.so`), which Qt's xcb plugin loads even
though this GUI is pure `QWidget`; the rest is `still reachable` memory, a global pointer
still owns at exit — not a leak, and `ERROR SUMMARY` rightly reports 0 errors.

- **Never stop the GUI with Ctrl-C.** SIGINT skips every destructor, so the whole live
  widget tree is reported as leaked (~7 MB of noise). Close the window instead.
- **`still reachable` is not a leak.** Only `definitely`, `indirectly` and `possibly
  lost` count. Even an empty `QApplication` has ~96 KB of it.

For a zero-leak run:

```
QT_XCB_GL_INTEGRATION=none valgrind --leak-check=full --error-exitcode=1 ./clients/gui/42TAP
```

Disabling the GLX integration leaves rendering pixel-identical, so it is safe for this
GUI. Drop the variable if OpenGL widgets are ever added.

# DEV INFO
Add implementation to as much memory as possible in GUI, like when the window is properly closed.
Right now, the ctrl+c is not calling the clean process of QT.