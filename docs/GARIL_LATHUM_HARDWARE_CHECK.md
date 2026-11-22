# Garil Lathum hardware check - 22 November 2026

The source path is connected. No Lathum device acceptance is claimed yet.
Use the existing Camp save and follow authored progression; do not set flags.

## Build

From Lagi on diagnostics/garil-cliff-current:

```powershell
git pull --ff-only origin diagnostics/garil-cliff-current
$env:VITASDK='E:/dev/VitaSDK'
$env:PSP2CGC='E:/PSVITA/sdk/host_tools/bin/psp2cgc.exe'
cmake --build build-vita --parallel 8
```

Install build-vita/Lagi.vpk. Keep matching lagi and lagi.velf for crash analysis.

## One gameplay run

1. Leave Camp, enter Garil Desert and reach the Lathum field mound.
2. Use lock-on to target its mesh. Confirm visibility/selectability and the
   short cutscene before battle entry. Flying past is not the encounter trigger.
3. Check emergence/Gash dialogue, player targeting/attacks and Lathum's attacks
   and reactions. The third attack repairs damaged parts.
4. Finish the fight. Check death camera/animation, results and XP/dyne once,
   then field return and whether the authored main exit becomes selectable.
5. If practical, enter another ordinary battle and return during that run.

Keep normal lagi.log and gameplay capture. If it crashes, preserve fatal log,
crash dump and the exact executable. Matching Saturn/Vita views are not needed.

Host evidence covers native PRG contracts, individual production controllers,
actual main/damage/death integration (other dispatch callbacks spied), shared
one-time rewards and Vita packaging. These do not establish device visuals,
timing, performance or end-to-end progression.

Other enemy variants, remaining visual/effect acceptance and slowdown remain
open. The accepted camera correction and earlier gameplay paths stay accepted.
