/* Internal data for powf implementation.

Copyright (c) 2022-2025 Alexei Sibidanov and Paul Zimmermann

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

#include "e_powf_data.h"

const uint32_t __powf_data_xmax[]
    = { 0, 0xffffff, 4095, 255, 63, 27, 15, 9, 7, 5, 5, 3, 3, 3, 3, 3 };

const double __powf_data_ix[]
    = { 0x1p+0,		  0x1.f07c1f07cp-1, 0x1.e1e1e1e1ep-1,
	0x1.d41d41d42p-1, 0x1.c71c71c72p-1, 0x1.bacf914c2p-1,
	0x1.af286bca2p-1, 0x1.a41a41a42p-1, 0x1.99999999ap-1,
	0x1.8f9c18f9cp-1, 0x1.861861862p-1, 0x1.7d05f417dp-1,
	0x1.745d1745dp-1, 0x1.6c16c16c1p-1, 0x1.642c8590bp-1,
	0x1.5c9882b93p-1, 0x1.555555555p-1, 0x1.4e5e0a72fp-1,
	0x1.47ae147aep-1, 0x1.414141414p-1, 0x1.3b13b13b1p-1,
	0x1.3521cfb2bp-1, 0x1.2f684bda1p-1, 0x1.29e4129e4p-1,
	0x1.249249249p-1, 0x1.1f7047dc1p-1, 0x1.1a7b9611ap-1,
	0x1.15b1e5f75p-1, 0x1.111111111p-1, 0x1.0c9714fbdp-1,
	0x1.084210842p-1, 0x1.041041041p-1, 0x1p-1 };

const double __powf_data_lix[][2] = { { 0x0p+0, 0x0p+0 },
				      { -0x1.6cp-5, 0x1.4b229b87f3f89p-15 },
				      { -0x1.66p-4, -0x1.fb7d654235799p-15 },
				      { -0x1.08p-3, -0x1.8b119b2c9c87bp-12 },
				      { -0x1.5cp-3, -0x1.a39fa6533294dp-19 },
				      { -0x1.acp-3, -0x1.ebc5b663dd4b8p-12 },
				      { -0x1.fcp-3, 0x1.f4a37fe0fa46fp-14 },
				      { -0x1.24p-2, -0x1.01eac33103e6bp-12 },
				      { -0x1.4ap-2, 0x1.61ed0d15725ep-12 },
				      { -0x1.6ep-2, -0x1.10e6ceb499ba9p-13 },
				      { -0x1.92p-2, 0x1.115db8ada837dp-12 },
				      { -0x1.b4p-2, -0x1.fafdce266d7aep-12 },
				      { -0x1.d6p-2, -0x1.d4f80cd19906fp-12 },
				      { -0x1.f8p-2, 0x1.5ea5ccd0a7396p-12 },
				      { 0x1.e8p-2, -0x1.0500d67fe62ebp-13 },
				      { 0x1.c8p-2, 0x1.9dc2d41aa4626p-14 },
				      { 0x1.a8p-2, 0x1.ff2e2ff321344p-11 },
				      { 0x1.8ap-2, 0x1.130157f4c3a3ep-11 },
				      { 0x1.6cp-2, 0x1.61ed0cad929ccp-11 },
				      { 0x1.5p-2, -0x1.2089a632d7949p-11 },
				      { 0x1.32p-2, 0x1.7fdc6dfb2d21ap-11 },
				      { 0x1.16p-2, 0x1.380a6c36088f3p-11 },
				      { 0x1.f6p-3, -0x1.3ab7dc7ba81acp-18 },
				      { 0x1.cp-3, -0x1.cc2c0061ef1a2p-14 },
				      { 0x1.8ap-3, 0x1.130157c97bbep-12 },
				      { 0x1.56p-3, 0x1.ee14ff34c4128p-14 },
				      { 0x1.22p-3, 0x1.b5b854c4fde69p-12 },
				      { 0x1.ep-4, 0x1.635d1df7cb0b5p-13 },
				      { 0x1.7ep-4, -0x1.3f6d2636c101ep-13 },
				      { 0x1.1cp-4, -0x1.33567f1b193a4p-14 },
				      { 0x1.78p-5, -0x1.8d66c5313a71dp-14 },
				      { 0x1.74p-6, 0x1.f7430ee200ep-17 },
				      { 0x0p+0, 0x0p+0 } };

const double __powf_data_c[]
    = { 0x1.71547652b82fep+0,  -0x1.71547652b82fep-1, 0x1.ec709dc3a2d0bp-2,
	-0x1.71547652bc4a9p-2, 0x1.2776c441b72ep-2,   -0x1.ec709bdf453ecp-3,
	0x1.a6406efd4b877p-3,  -0x1.717d824a520f7p-3 };

const double __powf_data_ce[]
    = { 0x1.62e42fefa398bp-5,  0x1.ebfbdff84555ap-11, 0x1.c6b08d4ad86d3p-17,
	0x1.3b2ad1b1716a2p-23, 0x1.5d7472718ce9dp-30, 0x1.4a1d7f457ac56p-37 };
const double __powf_data_tb[] = { 0x1p+0,
				  0x1.0b5586cf9890fp+0,
				  0x1.172b83c7d517bp+0,
				  0x1.2387a6e756238p+0,
				  0x1.306fe0a31b715p+0,
				  0x1.3dea64c123422p+0,
				  0x1.4bfdad5362a27p+0,
				  0x1.5ab07dd485429p+0,
				  0x1.6a09e667f3bcdp+0,
				  0x1.7a11473eb0187p+0,
				  0x1.8ace5422aa0dbp+0,
				  0x1.9c49182a3f09p+0,
				  0x1.ae89f995ad3adp+0,
				  0x1.c199bdd85529cp+0,
				  0x1.d5818dcfba487p+0,
				  0x1.ea4afa2a490dap+0 };

const double __powf_data_ch[][2]
    = { { 0x1.71547652b82fep+1, 0x1.777d0ffda2b89p-55 },
	{ 0x1.ec709dc3a03fdp-1, 0x1.d27f04ff73b3ap-55 },
	{ 0x1.2776c50ef9bfep-1, 0x1.e4b514251d0ecp-55 },
	{ 0x1.a61762a7aded9p-2, 0x1.de632dc7f6998p-57 },
	{ 0x1.484b13d7c02aep-2, 0x1.a320ec342ddb3p-56 },
	{ 0x1.0c9a84993fd48p-2, -0x1.e6425ce9a74a4p-57 },
	{ 0x1.c68f568d8beafp-3, -0x1.03a175487feabp-57 },
	{ 0x1.89f3b14657dfbp-3, 0x1.f04a3acf0bcf7p-57 },
	{ 0x1.5b9ad2f2d12ap-3, -0x1.68fdff6815a6fp-58 },
	{ 0x1.3702165b88acbp-3, 0x1.45b052ace6c8ep-60 },
	{ 0x1.1998f60f2f005p-3, -0x1.79a94f62fb524p-57 },
	{ 0x1.f9bc428e30809p-4, -0x1.51f063387e47p-59 },
	{ 0x1.1ac0ab871296ap-3, 0x1.2ba6a2e1a625bp-57 } };

const double __powf_data_ce2[][2]
    = { { 0x1p+0, 0x1.f7d70599926c4p-98 },
	{ 0x1.62e42fefa39efp-1, 0x1.abc9e3b39856bp-56 },
	{ 0x1.ebfbdff82c58fp-3, -0x1.5e43a540c283dp-57 },
	{ 0x1.c6b08d704a0cp-5, -0x1.d3316277451e6p-59 },
	{ 0x1.3b2ab6fba4e77p-7, 0x1.4e66003ba7f85p-62 },
	{ 0x1.5d87fe78a6731p-10, 0x1.07183d46a9697p-66 },
	{ 0x1.430912f86c787p-13, 0x1.bc81afca4c93p-67 },
	{ 0x1.ffcbfc588b0c7p-17, -0x1.e63f6f0116f4cp-71 },
	{ 0x1.62c0223a5c826p-20, -0x1.30542d98ea4a5p-74 },
	{ 0x1.b5253d395e7c6p-24, -0x1.9285a132ce05ep-80 },
	{ 0x1.e4cf5158b7b01p-28, -0x1.9ac1facae1b88p-83 },
	{ 0x1.e8cac7351a7a8p-32, -0x1.4fb82adebd76bp-91 },
	{ 0x1.c3bd65182746dp-36, 0x1.84ad0689d30ep-91 },
	{ 0x1.8161931d765c3p-40, -0x1.254c6535279cep-95 },
	{ 0x1.314943a26c9e2p-44, -0x1.f4f2fdc14fb82p-98 },
	{ 0x1.c36e53b459602p-49, 0x1.f0d06a5a63c41p-103 },
	{ 0x1.397637b3876a4p-53, -0x1.5632c551ae458p-107 },
	{ 0x1.98fbfefdddb51p-58, -0x1.fd134923d52b4p-115 } };
