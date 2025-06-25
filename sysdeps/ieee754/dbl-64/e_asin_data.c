/* Common data for asin implementations.

Copyright (c) 2022-2025 Alexei Sibidanov.

This file is part of the CORE-MATH project
(https://core-math.gitlabpages.inria.fr/).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "e_asin_data.h"

const uint64_t __asin_b[] = {
  0x5ba2e8ba2e8ad9b7, 0x0004713b13b29079, 0x000000393331e196, 0x0000000002f5c315
};
const u128_u __asin_ch[] = {
  {.bl = 0xaaaaaaaaaaaaaaa5, .bh = 0x0002aaaaaaaaaaaa}, // *+1
  {.bl = 0x3333333333333484, .bh = 0x0000001333333333}, // *+1
  {.bl = 0xb6db6db6db6da950, .bh = 0x0000000000b6db6d}, // *+1
  {.bl = 0x1c71c71c71c76217, .bh = 0x00000000000007c7}, // *+1
};

const u128_u __asin_s[] =
  {{.bl = 0x4e29cf6e5fed0679, .bh = 0x648557de8d99f7e},
   {.bl = 0x76a17954b2b7c517, .bh = 0xc8fb2f886ec09f3},
   {.bl = 0xbeeeae8129a786b9, .bh = 0x12d52092ce19f5cc},
   {.bl = 0xd8e72d912977ee71, .bh = 0x1917a6bc29b42be1},
   {.bl = 0x4e08e535cadaf147, .bh = 0x1f564e56a9730e34},
   {.bl = 0xc002a2684781f080, .bh = 0x259020dd1cc27444},
   {.bl = 0x8ffbbceed62c7c43, .bh = 0x2bc42889167f8ca9},
   {.bl = 0x9732300393f33614, .bh = 0x31f17078d34c156c},
   {.bl = 0x43af186b79b2a0f3, .bh = 0x381704d4fc9ec5f9},
   {.bl = 0x90887712e9dc9663, .bh = 0x3e33f2f642be355e},
   {.bl = 0x4c20ab7aa99a2183, .bh = 0x4447498ac7d9dd82},
   {.bl = 0xd725d3b9ed35fbaa, .bh = 0x4a5018bb567c16a2},
   {.bl = 0x97c4afa25181e605, .bh = 0x504d72505d98050c},
   {.bl = 0x408fca9cc277fc1f, .bh = 0x563e69d6ac7f73f8},
   {.bl = 0x4e61f79b3a36f1dc, .bh = 0x5c2214c3e9167abb},
   {.bl = 0x98916152cf7eee1c, .bh = 0x61f78a9abaa58b46},
   {.bl = 0xd409485edd56b172, .bh = 0x67bde50ea3b628b6},
   {.bl = 0x9b165cba0c171818, .bh = 0x6d744027857300ad},
   {.bl = 0x1439670dfe3d68e6, .bh = 0x7319ba64c711785a},
   {.bl = 0x362474f1a105878f, .bh = 0x78ad74e01bd8ec78},
   {.bl = 0x13e03e4889485c69, .bh = 0x7e2e936fe26ae7ed},
   {.bl = 0xbfd79717f2880abf, .bh = 0x839c3cc917ff6cb4},
   {.bl = 0xb892ca8361d8c84c, .bh = 0x88f59aa0da591421},
   {.bl = 0xbba4cfecbff54867, .bh = 0x8e39d9cd73464364},
   {.bl = 0xb17821911e71c16e, .bh = 0x93682a66e896f544},
   {.bl = 0x19cec845ac87a5c6, .bh = 0x987fbfe70b81a708},
   {.bl = 0xe25e39549638ae68, .bh = 0x9d7fd1490285c9e3},
   {.bl = 0x3b5167ee359a234e, .bh = 0xa267992848eeb0c0},
   {.bl = 0x149f6e75993468a3, .bh = 0xa73655df1f2f489e},
   {.bl = 0x1becda8089c1a94c, .bh = 0xabeb49a46764fd15},
   {.bl = 0xe4cad00d5c94bcd2, .bh = 0xb085baa8e966f6da},
   {.bl = 0x597d89b3754abe9f, .bh = 0xb504f333f9de6484},
   {.bl = 0x9de1e3b22b8bf4db, .bh = 0xb96841bf7ffcb21a},
   {.bl = 0xac85320f528d6d5d, .bh = 0xbdaef913557d76f0},
   {.bl = 0xbdf0715cb8b20bd7, .bh = 0xc1d8705ffcbb6e90},
   {.bl = 0x43da25d99267326b, .bh = 0xc5e40358a8ba05a7},
   {.bl = 0x8335241be1693225, .bh = 0xc9d1124c931fda7a},
   {.bl = 0x23af31db7179a4aa, .bh = 0xcd9f023f9c3a059e},
   {.bl = 0x744fea20e8abef92, .bh = 0xd14d3d02313c0eed},
   {.bl = 0xf630e8b6dac83e69, .bh = 0xd4db3148750d1819},
   {.bl = 0x24b9fe00663574a4, .bh = 0xd84852c0a80ffcdb},
   {.bl = 0x2c19b63253da43fc, .bh = 0xdb941a28cb71ec87},
   {.bl = 0x4b19aa71fec3ae6d, .bh = 0xdebe05637ca94cfb},
   {.bl = 0xf4e8a8372f8c5810, .bh = 0xe1c5978c05ed8691},
   {.bl = 0x122785ae67f5515d, .bh = 0xe4aa5909a08fa7b4},
   {.bl = 0x125129529d48a92f, .bh = 0xe76bd7a1e63b9786},
   {.bl = 0x15ad45b4a1b5e823, .bh = 0xea09a68a6e49cd62},
   {.bl = 0x7e610231ac1d6181, .bh = 0xec835e79946a3145},
   {.bl = 0x86f8c20fb664b01b, .bh = 0xeed89db66611e307},
   {.bl = 0x67127db35b287316, .bh = 0xf1090827b43725fd},
   {.bl = 0xa5486bdc455d56a2, .bh = 0xf314476247088f74},
   {.bl = 0x163c5c7f03b718c5, .bh = 0xf4fa0ab6316ed2ec},
   {.bl = 0x2c791f59cc1ffc23, .bh = 0xf6ba073b424b19e8},
   {.bl = 0xc7adc6b4988891bb, .bh = 0xf853f7dc9186b952},
   {.bl = 0x4504ae08d19b2980, .bh = 0xf9c79d63272c4628},
   {.bl = 0x2172a361fd2a722f, .bh = 0xfb14be7fbae58156},
   {.bl = 0x256778ffcb5c1769, .bh = 0xfc3b27d38a5d49ab},
   {.bl = 0xeae6bd951c1dabbe, .bh = 0xfd3aabf84528b50b},
   {.bl = 0x90cd1d959db674ef, .bh = 0xfe1323870cfe9a3d},
   {.bl = 0x41390efdc726e9ef, .bh = 0xfec46d1e89292cf0},
   {.bl = 0xf668633f1ab858a, .bh = 0xff4e6d680c41d0a9},
   {.bl = 0x421e8edaaf59453e, .bh = 0xffb10f1bcb6bef1d},
   {.bl = 0x5657552366961732, .bh = 0xffec4304266865d9}
  };
  
/* For 0 <= i <= 64, s[i]=floor(sin(pi/2*i/64)*2^63), except for i=64
   where s[i]=2^63-1.
   Thus s[i]/2^63 approximates sin(pi/2*i/64)=cos(pi/2*(64-i)/64).
   We use only 63 bits since we use it as a signed value.
   The maximal difference between s[i] and sin(pi/2*i/64)*2^63 is 1
   (for i=64). */
const uint64_t __asin_s1[] = {
  0, 0x3242abef46ccfbf, 0x647d97c437604f9, 0x96a9049670cfae6, 
  0xc8bd35e14da15f0, 0xfab272b54b9871a, 0x12c8106e8e613a22, 0x15e214448b3fc654, 
  0x18f8b83c69a60ab6, 0x1c0b826a7e4f62fc, 0x1f19f97b215f1aaf, 0x2223a4c563eceec1, 
  0x25280c5dab3e0b51, 0x2826b9282ecc0286, 0x2b1f34eb563fb9fc, 0x2e110a61f48b3d5d, 
  0x30fbc54d5d52c5a3, 0x33def28751db145b, 0x36ba2013c2b98056, 0x398cdd326388bc2d, 
  0x3c56ba700dec763c, 0x3f1749b7f13573f6, 0x41ce1e648bffb65a, 0x447acd506d2c8a10, 
  0x471cece6b9a321b2, 0x49b41533744b7aa2, 0x4c3fdff385c0d384, 0x4ebfe8a48142e4f1, 
  0x5133cc9424775860, 0x539b2aef8f97a44f, 0x55f5a4d233b27e8a, 0x5842dd5474b37b6d, 
  0x5a827999fcef3242, 0x5cb420dfbffe590d, 0x5ed77c89aabebb78, 0x60ec382ffe5db748, 
  0x62f201ac545d02d3, 0x64e88926498fed3d, 0x66cf811fce1d02cf, 0x68a69e81189e0776, 
  0x6a6d98a43a868c0c, 0x6c2429605407fe6d, 0x6dca0d1465b8f643, 0x6f5f02b1be54a67d, 
  0x70e2cbc602f6c348, 0x72552c84d047d3da, 0x73b5ebd0f31dcbc3, 0x7504d3453724e6b1, 
  0x7641af3cca3518a2, 0x776c4edb3308f183, 0x78848413da1b92fe, 0x798a23b1238447ba, 
  0x7a7d055b18b76976, 0x7b5d039da1258cf4, 0x7c29fbee48c35ca9, 0x7ce3ceb193962314, 
  0x7d8a5f3fdd72c0ab, 0x7e1d93e9c52ea4d5, 0x7e9d55fc22945a85, 0x7f0991c3867f4d1e, 
  0x7f62368f44949678, 0x7fa736b40620e854, 0x7fd8878de5b5f78e, 0x7ff62182133432ec, ~0ull>>1 };

/* For 0 <= i <= 64, sh[i] = round(sin(i*pi/2/64)*2^69) mod 2^64,
   with maximal error < 0.496 (for i=17). */
const uint64_t __asin_sh[] = {
  0, 0xc90aafbd1b33efca, 0x91f65f10dd813e6f, 0x5aa41259c33eb998, 
  0x22f4d78536857c3b, 0xeac9cad52e61c68a, 0xb2041ba3984e8898, 0x78851122cff19532, 
  0x3e2e0f1a6982ad93, 0x2e09a9f93d8bf28, 0xc67e5ec857c6abd2, 0x88e93158fb3bb04a, 
  0x4a03176acf82d45b, 0x9ae4a0bb300a193, 0xc7cd3ad58fee7f08, 0x8442987d22cf576a, 
  0x3ef1535754b168d3, 0xf7bca1d476c516db, 0xae8804f0ae6015b3, 0x63374c98e22f0b43, 
  0x15ae9c037b1d8f07, 0xc5d26dfc4d5cfda2, 0x73879922ffed9698, 0x1eb3541b4b228437, 
  0xc73b39ae68c86c97, 0x6d054cdd12dea896, 0xff7fce17034e103, 0xaffa292050b93c7c, 
  0x4cf325091dd61807, 0xe6cabbe3e5e913c3, 0x7d69348cec9fa2a3, 0x10b7551d2cdedb5d, 
  0xa09e667f3bcc908b, 0x2d0837efff964354, 0xb5df226aafaede16, 0x3b0e0bff976dd218, 
  0xbc806b151740b4e8, 0x3a22499263fb4f50, 0xb3e047f38740b3c4, 0x29a7a0462781ddaf, 
  0x9b66290ea1a3033f, 0x90a581501ff9b65, 0x728345196e3d90e6, 0xd7c0ac6f95299f69, 
  0x38b2f180bdb0d23f, 0x954b213411f4f682, 0xed7af43cc772f0c2, 0x4134d14dc939ac43, 
  0x906bcf328d4628b0, 0xdb13b6ccc23c60f1, 0x212104f686e4bfad, 0x6288ec48e111ee95, 
  0x9f4156c62dda5d83, 0xd740e76849633d06, 0xa7efb9230d72a59, 0x38f3ac64e588c509, 
  0x6297cff75cb02ac4, 0x8764fa714ba93565, 0xa7557f08a516a17d, 0xc26470e19fd347b2, 
  0xd88da3d125259e08, 0xe9cdad01883a1522, 0xf621e3796d7de3a8, 0xfd886084cd0cbb2b, 0};
