#include "lagi/save_growth_migration.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(){
 std::array<std::uint8_t,304> data{};
 for(unsigned i=8;i<256;++i)data[i]=i;
 const std::uint32_t oldVersion=0x10000;std::memcpy(data.data()+4,&oldVersion,4);
 auto sum=lagi::save::checksum(data.data(),256);std::memcpy(data.data(),&sum,4);
 const auto original=data;
 assert(lagi::save::expand_growth(data.data(),256,304,180));
 for(unsigned i=8;i<180;++i)assert(data[i]==original[i]);
 for(unsigned i=180;i<228;++i)assert(!data[i]);
 for(unsigned i=228;i<304;++i)assert(data[i]==original[i-48]);
 assert(lagi::save::word(data.data()+4)==0x10001);
 assert(lagi::save::word(data.data())==lagi::save::checksum(data.data(),304));
 assert(lagi::save::expand_growth(data.data(),304,304,180));
 auto invalid=original;invalid[30]^=1;const auto before=invalid;
 assert(!lagi::save::expand_growth(invalid.data(),256,304,180)&&invalid==before);
 invalid=original;assert(!lagi::save::expand_growth(invalid.data(),255,304,180));
 invalid=original;assert(!lagi::save::expand_growth(invalid.data(),256,304,257));
 puts("PASS save growth migration: legacy prefix/tail preserved, zero extension, checksum/version, current roundtrip and corrupt/truncated rejection");
}
