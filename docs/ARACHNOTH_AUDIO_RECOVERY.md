# Arachnoth audio recovery

The native BTL_A3_2 initializer at `0605400A` selects sound bank **67**
(`06054016: MOV #43,R4`, call through `0605406C: 0602B400`). Upstream's
reconstructed overlay reuses the field battle initializer, selecting bank 4.

That changes sound 118 from the original `(data=3, sound=17, control=6)` to
`(data=2, sound=0, control=0)`. The hardware log shows this incorrect control-0
start followed by a stop for sound 118 during Charge. Control 0 owns music, so
this explains both music replacement and cutoff without blaming capture audio.

The generated adapter now calls the dedicated native initializer: load 67,
check `findSound(100)`, and only when absent set overlay music ownership and
request playback through `playPCM(parent,100)`. No guessed sound mappings,
replacement audio, or copyrighted assets are included. Other battles retain
their existing initializer. Cinematic EPK audio remains separate unfinished work.

`arachnoth_music_test.ps1` checks the original instructions/COMMON mapping and
runs the production initializer with mocked audio services for absent/already
playing music. The Vita battle build compiles. Audible continuity through Charge
and correct boss SFX require hardware confirmation; the supplied OBS recording
has a silent audio track and cannot provide listening acceptance.
