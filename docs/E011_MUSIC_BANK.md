# E011 music bank correction

Device logs show cinematic configuration 68 loading E14SE/E14DSP while the
boss uses configuration 67 with BOSS01_2.SEQ and A5BOSBGM.BIN. The original
Disc 1 TWN_E011.PRG (LBA 218450, load base 06054000) has D231 at 0605401C,
JSR @R2 at 06054020, and E443 (MOV #67,R4) in its delay slot at 06054022.
The target literal at 060540E4 is 0602B400, loadSoundBanks. R5 is zero.
Upstream reconstructed E011 instead passes 68.

The generated E011 adapter corrects that argument to 67 without editing
extern/Azel, remapping songs in the audio service, or changing the native
script's music-start timing. The disc-backed test verifies instructions,
target literal and generated argument. Audible acceptance requires a fresh
device run: the cinematic should load boss banks and play the same theme as
the following battle. Cloud performance remains a separate open issue.
