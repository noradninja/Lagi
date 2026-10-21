# Devour countdown recovery

Original BTL_A3_2 06057E48..74 exits through 060585A6 during phase 0,
both while waiting and when selecting the eating model. The upstream `break`
instead reached the common timer increment, cancelling every decrement.
Restore the native return in the generated adapter, without editing Azel.

`tests/arachnoth_devour_dispatch_test.ps1` compiles the complete generated
production dispatcher with mocked external services and executes its full
215-frame attack budget. It checks the 30-frame countdown, one animation
start, both fades, one 0x4E damage call at update 144, and release phase.
The separate spring/camera fixture also passes. Full-resolution Vita VPK
compilation passes; actual animation/damage remain to be confirmed on Vita.
