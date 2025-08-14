/* Data definition for logf.
   Copyright (C) 2017-2025 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include "math_config.h"
#include "e_logf_data.h"

const struct logf_data __logf_data = {
  .tab = {
  { 0x1.661ec79f8f3bep+0, -0x1.57bf7808caadep-2 },
  { 0x1.571ed4aaf883dp+0, -0x1.2bef0a7c06ddbp-2 },
  { 0x1.49539f0f010bp+0, -0x1.01eae7f513a67p-2 },
  { 0x1.3c995b0b80385p+0, -0x1.b31d8a68224e9p-3 },
  { 0x1.30d190c8864a5p+0, -0x1.6574f0ac07758p-3 },
  { 0x1.25e227b0b8eap+0, -0x1.1aa2bc79c81p-3 },
  { 0x1.1bb4a4a1a343fp+0, -0x1.a4e76ce8c0e5ep-4 },
  { 0x1.12358f08ae5bap+0, -0x1.1973c5a611cccp-4 },
  { 0x1.0953f419900a7p+0, -0x1.252f438e10c1ep-5 },
  { 0x1p+0, 0x0p+0 },
  { 0x1.e608cfd9a47acp-1, 0x1.aa5aa5df25984p-5 },
  { 0x1.ca4b31f026aap-1, 0x1.c5e53aa362eb4p-4 },
  { 0x1.b2036576afce6p-1, 0x1.526e57720db08p-3 },
  { 0x1.9c2d163a1aa2dp-1, 0x1.bc2860d22477p-3 },
  { 0x1.886e6037841edp-1, 0x1.1058bc8a07ee1p-2 },
  { 0x1.767dcf5534862p-1, 0x1.4043057b6ee09p-2 },
  },
  .ln2 = 0x1.62e42fefa39efp-1,
  .poly = {
  -0x1.00ea348b88334p-2, 0x1.5575b0be00b6ap-2, -0x1.ffffef20a4123p-2,
  }
};

const double __logf_data_tr[]
  = { 0x1p+0,         0x1.f81f82p-1,  0x1.f07c1fp-1,  0x1.e9131acp-1,
      0x1.e1e1e1ep-1, 0x1.dae6077p-1, 0x1.d41d41dp-1, 0x1.cd85689p-1,
      0x1.c71c71cp-1, 0x1.c0e0704p-1, 0x1.bacf915p-1, 0x1.b4e81b5p-1,
      0x1.af286bdp-1, 0x1.a98ef6p-1,  0x1.a41a41ap-1, 0x1.9ec8e95p-1,
      0x1.999999ap-1, 0x1.948b0fdp-1, 0x1.8f9c19p-1,  0x1.8acb90fp-1,
      0x1.8618618p-1, 0x1.8181818p-1, 0x1.7d05f41p-1, 0x1.78a4c81p-1,
      0x1.745d174p-1, 0x1.702e05cp-1, 0x1.6c16c17p-1, 0x1.6816817p-1,
      0x1.642c859p-1, 0x1.605816p-1,  0x1.5c9882cp-1, 0x1.58ed231p-1,
      0x1.5555555p-1, 0x1.51d07ebp-1, 0x1.4e5e0a7p-1, 0x1.4afd6ap-1,
      0x1.47ae148p-1, 0x1.446f865p-1, 0x1.4141414p-1, 0x1.3e22cbdp-1,
      0x1.3b13b14p-1, 0x1.3813814p-1, 0x1.3521cfbp-1, 0x1.323e34ap-1,
      0x1.2f684bep-1, 0x1.2c9fb4ep-1, 0x1.29e412ap-1, 0x1.27350b9p-1,
      0x1.2492492p-1, 0x1.21fb781p-1, 0x1.1f7047ep-1, 0x1.1cf06aep-1,
      0x1.1a7b961p-1, 0x1.1811812p-1, 0x1.15b1e5fp-1, 0x1.135c811p-1,
      0x1.1111111p-1, 0x1.0ecf56cp-1, 0x1.0c9715p-1,  0x1.0a6810ap-1,
      0x1.0842108p-1, 0x1.0624dd3p-1,  0x1.041041p-1, 0x1.0204081p-1,
      0.5 };

const double __logf_data_tl[]
    = { -0x1.3b40815cd0628p-45, 0x1.fc0a890fbb514p-7, 0x1.f829b1e780b98p-6,
	0x1.77458f532c948p-5,	  0x1.f0a30c2114ef2p-5, 0x1.341d793bbc7f7p-4,
	0x1.6f0d28d256172p-4,	  0x1.a926d3a6acb89p-4, 0x1.e2707722ae90cp-4,
	0x1.0d77e7a90896cp-3,	  0x1.29552f6fff036p-3, 0x1.44d2b6c5b7831p-3,
	0x1.5ff306ee78ee7p-3,	  0x1.7ab890410d41cp-3, 0x1.9525a9e3451c7p-3,
	0x1.af3c94ed0bb06p-3,	  0x1.c8ff7c59a9535p-3, 0x1.e27076d5aedf9p-3,
	0x1.fb9186b5e393ep-3,	  0x1.0a324e38b8e6dp-2, 0x1.1675cacaba398p-2,
	0x1.22941fc0f76efp-2,	  0x1.2e8e2bc311abap-2, 0x1.3a64c56b14373p-2,
	0x1.4618bc31c5c4cp-2,	  0x1.51aad874df5b7p-2, 0x1.5d1bdbea80754p-2,
	0x1.686c81d331238p-2,	  0x1.739d7f6dbcd9p-2,	0x1.7eaf83c82ad4dp-2,
	0x1.89a3385813fe4p-2,	  0x1.947941aa91484p-2, 0x1.9f323edbf95d5p-2,
	0x1.a9cec9a4205d3p-2,	  0x1.b44f77c5c8cecp-2, 0x1.beb4d9ea71905p-2,
	0x1.c8ff7c69a97abp-2,	  0x1.d32fe7f38e95fp-2, 0x1.dd46a0501c22ap-2,
	0x1.e7442617e8511p-2,	  0x1.f128f5eaf0476p-2, 0x1.faf588dd8f0a8p-2,
	0x1.02552a5edcfc4p-1,	  0x1.0723e5c64de05p-1, 0x1.0be72e3852947p-1,
	0x1.109f39d554b5cp-1,	  0x1.154c3d2c4d4aep-1, 0x1.19ee6b38bc834p-1,
	0x1.1e85f5ef03f95p-1,	  0x1.23130d7fabe07p-1, 0x1.2795e1219afep-1,
	0x1.2c0e9ec9c8d5p-1,	  0x1.307d7337f0f83p-1, 0x1.34e289cb4e098p-1,
	0x1.393e0d42e28dep-1,	  0x1.3d9026ad555bfp-1, 0x1.41d8fe8667173p-1,
	0x1.4618bc1ec5d87p-1,	  0x1.4a4f85d303d8p-1,	0x1.4e7d8127f5a75p-1,
	0x1.52a2d26dbc47p-1,	  0x1.56bf9d597f25ep-1, 0x1.5ad404cb59df2p-1,
	0x1.5ee02a928153ap-1,	  0x1.62e42fefa38b4p-1 };

const double __logf_data_b[]
    = { 0x1.00000006342eap+0, -0x1.0001f7fdc3977p-1, 0x1.554a4e5cae9cfp-2 };

const double __logf_data_c[]
   = { -0x1p-1,                0x1.55555555571cap-2, -0x1.0000000002d85p-2,
        0x1.9999987d0c963p-3, -0x1.555554059a8bbp-3,  0x1.24aebcf71a38fp-3,
       -0x1.001c73915d758p-3 };
