#pragma once
namespace lagi {
constexpr bool shared_color_offset(unsigned enable,unsigned select,
    int ar,int ag,int ab,int br,int bg,int bb)
{
    select&=0x7fu;
    return (enable&0x7fu)==0x7fu &&
        (select==0 || select==0x7fu || (ar==br && ag==bg && ab==bb));
}
}
