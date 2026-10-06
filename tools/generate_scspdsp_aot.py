"""Translate captured SCSP programs into straight-line, bit-exact C."""
from pathlib import Path
import argparse, re, struct

def program_hash(words):
    h = ((2166136261 ^ (len(words)//4))*16777619) & 0xffffffff
    for word in words:
        for byte in (word & 255, word >> 8): h=((h ^ byte)*16777619)&0xffffffff
    return h

def decode(words, schema):
    fields = re.findall(r'LAGI_DSP_FIELD\((\w+), (\d), (\d+), (0x[0-9A-F]+)\)', schema.read_text())
    if len(fields)!=25: raise ValueError('invalid instruction schema')
    ops=[{name:(words[i+int(w)] >> int(shift)) & int(mask,16) for name,w,shift,mask in fields} for i in range(0,len(words),4)]
    if any(o['IRA']>=0x32 for o in ops): raise ValueError('invalid IRA')
    frc=any(o['FRCL'] for o in ops); y=any(o['YRL'] for o in ops); adrs=any(o['ADRL'] for o in ops)
    for i,o in enumerate(ops):
        o['odd']=i&1
        o['multiply']=o['YSEL']==1 or (frc if o['YSEL']==0 else y)
        o['needsInput']=(o['XSEL'] and o['multiply']) or o['YRL'] or (o['ADRL'] and o['SHIFT']!=3)
        o['needsTemp']=(not o['ZERO'] and not o['BSEL']) or (not o['XSEL'] and o['multiply'])
        o['needsShift']=o['TWT'] or o['FRCL'] or (o['ADRL'] and o['SHIFT']==3) or o['EWT'] or (o['odd'] and o['MWT'])
        o['adrsLive']=adrs
    return ops

def emit_function(name, ops):
    lines=[f'extern const char __start_{name}_code[], __stop_{name}_code[];', f'static __attribute__((noinline,section("{name}_code"))) void {name}(struct _SCSPDSP *DSP) {{',
           '    INT32 ACC=0, SHIFTED=0, INPUTS=0, MEMVAL=0, FRC_REG=0, Y_REG=0, B=0, X=0, Y=0, tempValue=0;',
           '    UINT32 ADRS_REG=0, addr=0;',
           '    const UINT32 dec=DSP->DEC, rbp=DSP->RBP << 12, ringMask=DSP->RBL-1;',
           '    memset(DSP->EFREG,0,sizeof(DSP->EFREG));']
    def add(s): lines.append('    '+s)
    for i,o in enumerate(ops):
        add(f'/* step {i}: constants folded; SHIFTED always uses previous ACC. */')
        if o['needsInput']:
            v=f'DSP->MEMS[{o["IRA"]}]' if o['IRA']<32 else (f'(UINT32)DSP->MIXS[{o["IRA"]-32}] << 4' if o['IRA']<48 else '0')
            add(f'INPUTS=lagi_sign_extend24_u32((UINT32)({v}));')
        if o['IWT']:
            add(f'DSP->MEMS[{o["IWA"]}]=MEMVAL;')
            if o['needsInput'] and o['IRA']==o['IWA']: add('INPUTS=MEMVAL;')
        if o['needsTemp']: add(f'tempValue=lagi_sign_extend24_u32((UINT32)DSP->TEMP[({o["TRA"]}+dec)&127]);')
        add('B='+('0' if o['ZERO'] else ('ACC' if o['BSEL'] else 'tempValue'))+';')
        if o['NEGB'] and not o['ZERO']: add('B=(INT32)(0u-(UINT32)B);')
        if o['multiply']:
            add('X='+('INPUTS' if o['XSEL'] else 'tempValue')+';')
            ys=['FRC_REG',f'*(const volatile INT16 *)&DSP->COEF[{o["COEF"]}] >> 3','(Y_REG >> 11)&8191','(Y_REG >> 4)&4095'][o['YSEL']]
            add(f'Y=lagi_sign_extend13_u32((UINT32)({ys}));')
        if o['YRL']: add('Y_REG=INPUTS;')
        if o['needsShift']:
            expr=['lagi_dsp_native_sat(ACC,0)','lagi_dsp_native_sat(ACC,1)','lagi_sign_extend24_u32((UINT32)ACC << 1)','lagi_sign_extend24_u32((UINT32)ACC)'][o['SHIFT']]
            add('SHIFTED='+expr+';')
        add('ACC='+('(INT32)((UINT32)(INT32)(((INT64)X*Y)>>12)+(UINT32)B)' if o['multiply'] else 'B')+';')
        if o['TWT']: add(f'DSP->TEMP[({o["TWA"]}+dec)&127]=SHIFTED;')
        if o['FRCL']: add('FRC_REG='+('SHIFTED&4095' if o['SHIFT']==3 else '(SHIFTED >> 11)&8191')+';')
        if o['odd'] and (o['MRD'] or o['MWT']):
            add(f'addr=*(const volatile UINT16 *)&DSP->MADRS[{o["MASA"]}];')
            if not o['TABLE']: add('addr+=dec;')
            if o['ADREB'] and o['adrsLive']: add('addr+=ADRS_REG&4095;')
            if o['NXADR']: add('++addr;')
            add('addr=(addr&'+('65535u' if o['TABLE'] else 'ringMask')+')+rbp;')
            if o['MRD']: add('MEMVAL='+('(INT32)((UINT32)DSP->SCSPRAM[addr] << 8)' if o['NOFL'] else 'UNPACK(DSP->SCSPRAM[addr])')+';')
            if o['MWT']: add('DSP->SCSPRAM[addr]='+('(UINT16)(SHIFTED >> 8)' if o['NOFL'] else 'PACK(SHIFTED)')+';')
        if o['ADRL']: add('ADRS_REG='+('(SHIFTED >> 12)&4095' if o['SHIFT']==3 else '(UINT32)(INPUTS >> 16)')+';')
        if o['EWT']: add(f'DSP->EFREG[{o["EWA"]}]=(INT16)((INT32)DSP->EFREG[{o["EWA"]}]+(SHIFTED >> 8));')
    add('--DSP->DEC;'); add('memset(DSP->MIXS,0,sizeof(DSP->MIXS));'); lines.append('}')
    return '\n'.join(lines)

def load_corpus(folder):
    unique=set()
    for p in sorted(folder.rglob('*.ldsp')):
        blob=p.read_bytes()
        if len(blob)<16: raise ValueError(f'{p}: truncated header')
        magic,version,steps,h=struct.unpack_from('<4I',blob)
        if magic!=0x5053444c or version!=1 or steps>128 or len(blob)!=16+steps*8: raise ValueError(f'{p}: invalid capture')
        words=struct.unpack_from('<'+str(steps*4)+'H',blob,16)
        if h!=program_hash(words): raise ValueError(f'{p}: hash mismatch')
        unique.add(words)
    return sorted(unique)

def generate(programs, schema):
    lines=['/* Generated from LDSP captures. Do not edit. No game data is bundled. */']
    entries=[]
    for index,words in enumerate(programs):
        name=f'lagi_dsp_aot_{index}'
        lines.append(emit_function(name,decode(words,schema)))
        entries.append('{ %du, 0x%08Xu, { %s }, %s, __start_%s_code, __stop_%s_code }' % (len(words)//4,program_hash(words),','.join(str(w) for w in words) or '0',name,name,name))
    lines.append('static const LagiDspAotEntry g_lagiDspAotEntries[] = {\n'+(',\n'.join(entries) if entries else '{ 0, 0, {0}, 0 }')+'\n};')
    lines.append(f'static const unsigned g_lagiDspAotCount={len(entries)};')
    return '\n\n'.join(lines)+'\n'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--corpus',type=Path,required=True); parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--schema',type=Path,default=Path(__file__).resolve().parents[1]/'src/integration/lagi_dsp_fields.def')
    args=parser.parse_args()
    programs=load_corpus(args.corpus)
    output=generate(programs,args.schema)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    if not args.output.exists() or args.output.read_text()!=output: args.output.write_text(output)
    print(f'DSP AOT: {len(programs)} unique programs; {len(output)} source bytes')
if __name__=='__main__': main()
