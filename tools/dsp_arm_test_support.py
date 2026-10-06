"""Run a freestanding ARM test ELF; accelerate libc, not DSP execution."""
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_PC, UC_ARM_REG_C1_C0_2, UC_ARM_REG_FPEXC
from elftools.elf.elffile import ELFFile

def run_arm_elf(path, limit=1000000000):
    uc=Uc(UC_ARCH_ARM,UC_MODE_ARM)
    uc.mem_map(0x10000,0x1000000)
    with path.open('rb') as f:
        elf=ELFFile(f)
        for seg in elf.iter_segments():
            if seg['p_type']=='PT_LOAD': uc.mem_write(seg['p_vaddr'],seg.data())
        symbols={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
    uc.reg_write(UC_ARM_REG_SP,0x1000000); uc.reg_write(UC_ARM_REG_LR,0xffff00)
    uc.reg_write(UC_ARM_REG_C1_C0_2,0xf<<20); uc.reg_write(UC_ARM_REG_FPEXC,1<<30)
    def failure(uc,address,size,name):
        raise RuntimeError(f'{name}: status={uc.reg_read(UC_ARM_REG_R0)}')
    for name in ['exit','__assert_func']:
        if name in symbols: uc.hook_add(UC_HOOK_CODE,failure,name,begin=symbols[name]&~1,end=symbols[name]&~1)
    def libc(uc,address,size,name):
        a=uc.reg_read(UC_ARM_REG_R0); b=uc.reg_read(UC_ARM_REG_R1); n=uc.reg_read(UC_ARM_REG_R2)
        if name=='memcmp': result=0 if uc.mem_read(a,n)==uc.mem_read(b,n) else 1
        elif name=='memcpy': uc.mem_write(a,bytes(uc.mem_read(b,n))); result=a
        else: uc.mem_write(a,bytes([b&255])*n); result=a
        if name!="memcmp" and n: uc.ctl_remove_cache(a,a+n)
        uc.reg_write(UC_ARM_REG_R0,result); uc.reg_write(UC_ARM_REG_PC,uc.reg_read(UC_ARM_REG_LR))
    for name in ['memcmp','memcpy','memset']:
        if name in symbols: uc.hook_add(UC_HOOK_CODE,libc,name,begin=symbols[name]&~1,end=symbols[name]&~1)
    uc.emu_start(symbols['main'],0xffff00,count=limit)
    if uc.reg_read(UC_ARM_REG_PC)!=0xffff00: raise RuntimeError('ARM instruction limit reached')
    if uc.reg_read(UC_ARM_REG_R0)!=0: raise RuntimeError(f'ARM test returned {uc.reg_read(UC_ARM_REG_R0)}; ' + str({n:bytes(uc.mem_read(v,24 if 'calls' in n else 4)).hex() for n,v in symbols.items() if any(k in n for k in ['calls','executeResult','vmReady','failOperation','wrongReturn'])}))
