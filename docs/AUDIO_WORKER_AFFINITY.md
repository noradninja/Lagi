# Audio worker affinity

The SCSP worker now uses the SDK's `SCE_KERNEL_CPU_MASK_USER_ALL` rather than
`SCE_KERNEL_CPU_MASK_USER_2`. Vita can schedule/migrate the worker across its
user cores. Priority, stack, mixing, DSP, queueing and worker lifetime are
unchanged. Movie audio was already created without a CPU-2 restriction.

Successful startup logs `SCSP worker affinity=all-user-cores mask=...`.
This is still one serial worker: unrestricted affinity does not parallelize
audio processing or lower its total CPU demand. A busy core can still occur.
Any effect on audio underruns or combat frame pacing requires hardware testing;
compare the same encounter and check listening, queue levels and short writes.
