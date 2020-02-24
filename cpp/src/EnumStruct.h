//-----------------------------------------------------------------------------
//
//	enum.h
//
//	Pre-Processor File for Creating Stringable Enums
//
//	Copyright (c) 2020 Justin Hammond <justin@dynam.ac>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------


#ifndef ENUMSTRUCT_H
#define ENUMSTRUCT_H
/* this is good for upto 768 entries per ENUM. I shall predict that 768 entries shall be enough for any CommandClass :) */

#define MAP(macro, ...) \
    IDENTITY( \
        APPLY(CHOOSE_MAP_START, COUNT(__VA_ARGS__)) \
            (macro, __VA_ARGS__))

#define CHOOSE_MAP_START(count) MAP ## count

#define APPLY(macro, ...) IDENTITY(macro(__VA_ARGS__))

// Needed to expand __VA_ARGS__ "eagerly" on the MSVC preprocessor.
#define IDENTITY(x) x

#define MAP1(m, x)      m(x)
#define MAP2(m, x, ...) m(x) IDENTITY(MAP1(m, __VA_ARGS__))
#define MAP3(m, x, ...) m(x) IDENTITY(MAP2(m, __VA_ARGS__))
#define MAP4(m, x, ...) m(x) IDENTITY(MAP3(m, __VA_ARGS__))
#define MAP5(m, x, ...) m(x) IDENTITY(MAP4(m, __VA_ARGS__))
#define MAP6(m, x, ...) m(x) IDENTITY(MAP5(m, __VA_ARGS__))
#define MAP7(m, x, ...) m(x) IDENTITY(MAP6(m, __VA_ARGS__))
#define MAP8(m, x, ...) m(x) IDENTITY(MAP7(m, __VA_ARGS__))
#define MAP9(m, x, ...) m(x) IDENTITY(MAP8(m, __VA_ARGS__))
#define MAP10(m, x, ...) m(x) IDENTITY(MAP9(m, __VA_ARGS__))
#define MAP11(m, x, ...) m(x) IDENTITY(MAP10(m, __VA_ARGS__))
#define MAP12(m, x, ...) m(x) IDENTITY(MAP11(m, __VA_ARGS__))
#define MAP13(m, x, ...) m(x) IDENTITY(MAP12(m, __VA_ARGS__))
#define MAP14(m, x, ...) m(x) IDENTITY(MAP13(m, __VA_ARGS__))
#define MAP15(m, x, ...) m(x) IDENTITY(MAP14(m, __VA_ARGS__))
#define MAP16(m, x, ...) m(x) IDENTITY(MAP15(m, __VA_ARGS__))
#define MAP17(m, x, ...) m(x) IDENTITY(MAP16(m, __VA_ARGS__))
#define MAP18(m, x, ...) m(x) IDENTITY(MAP17(m, __VA_ARGS__))
#define MAP19(m, x, ...) m(x) IDENTITY(MAP18(m, __VA_ARGS__))
#define MAP20(m, x, ...) m(x) IDENTITY(MAP19(m, __VA_ARGS__))
#define MAP21(m, x, ...) m(x) IDENTITY(MAP20(m, __VA_ARGS__))
#define MAP22(m, x, ...) m(x) IDENTITY(MAP21(m, __VA_ARGS__))
#define MAP23(m, x, ...) m(x) IDENTITY(MAP22(m, __VA_ARGS__))
#define MAP24(m, x, ...) m(x) IDENTITY(MAP23(m, __VA_ARGS__))
#define MAP25(m, x, ...) m(x) IDENTITY(MAP24(m, __VA_ARGS__))
#define MAP26(m, x, ...) m(x) IDENTITY(MAP25(m, __VA_ARGS__))
#define MAP27(m, x, ...) m(x) IDENTITY(MAP26(m, __VA_ARGS__))
#define MAP28(m, x, ...) m(x) IDENTITY(MAP27(m, __VA_ARGS__))
#define MAP29(m, x, ...) m(x) IDENTITY(MAP28(m, __VA_ARGS__))
#define MAP30(m, x, ...) m(x) IDENTITY(MAP29(m, __VA_ARGS__))
#define MAP31(m, x, ...) m(x) IDENTITY(MAP30(m, __VA_ARGS__))
#define MAP32(m, x, ...) m(x) IDENTITY(MAP31(m, __VA_ARGS__))
#define MAP33(m, x, ...) m(x) IDENTITY(MAP32(m, __VA_ARGS__))
#define MAP34(m, x, ...) m(x) IDENTITY(MAP33(m, __VA_ARGS__))
#define MAP35(m, x, ...) m(x) IDENTITY(MAP34(m, __VA_ARGS__))
#define MAP36(m, x, ...) m(x) IDENTITY(MAP35(m, __VA_ARGS__))
#define MAP37(m, x, ...) m(x) IDENTITY(MAP36(m, __VA_ARGS__))
#define MAP38(m, x, ...) m(x) IDENTITY(MAP37(m, __VA_ARGS__))
#define MAP39(m, x, ...) m(x) IDENTITY(MAP38(m, __VA_ARGS__))
#define MAP40(m, x, ...) m(x) IDENTITY(MAP39(m, __VA_ARGS__))
#define MAP41(m, x, ...) m(x) IDENTITY(MAP40(m, __VA_ARGS__))
#define MAP42(m, x, ...) m(x) IDENTITY(MAP41(m, __VA_ARGS__))
#define MAP43(m, x, ...) m(x) IDENTITY(MAP42(m, __VA_ARGS__))
#define MAP44(m, x, ...) m(x) IDENTITY(MAP43(m, __VA_ARGS__))
#define MAP45(m, x, ...) m(x) IDENTITY(MAP44(m, __VA_ARGS__))
#define MAP46(m, x, ...) m(x) IDENTITY(MAP45(m, __VA_ARGS__))
#define MAP47(m, x, ...) m(x) IDENTITY(MAP46(m, __VA_ARGS__))
#define MAP48(m, x, ...) m(x) IDENTITY(MAP47(m, __VA_ARGS__))
#define MAP49(m, x, ...) m(x) IDENTITY(MAP48(m, __VA_ARGS__))
#define MAP50(m, x, ...) m(x) IDENTITY(MAP49(m, __VA_ARGS__))
#define MAP51(m, x, ...) m(x) IDENTITY(MAP50(m, __VA_ARGS__))
#define MAP52(m, x, ...) m(x) IDENTITY(MAP51(m, __VA_ARGS__))
#define MAP53(m, x, ...) m(x) IDENTITY(MAP52(m, __VA_ARGS__))
#define MAP54(m, x, ...) m(x) IDENTITY(MAP53(m, __VA_ARGS__))
#define MAP55(m, x, ...) m(x) IDENTITY(MAP54(m, __VA_ARGS__))
#define MAP56(m, x, ...) m(x) IDENTITY(MAP55(m, __VA_ARGS__))
#define MAP57(m, x, ...) m(x) IDENTITY(MAP56(m, __VA_ARGS__))
#define MAP58(m, x, ...) m(x) IDENTITY(MAP57(m, __VA_ARGS__))
#define MAP59(m, x, ...) m(x) IDENTITY(MAP58(m, __VA_ARGS__))
#define MAP60(m, x, ...) m(x) IDENTITY(MAP59(m, __VA_ARGS__))
#define MAP61(m, x, ...) m(x) IDENTITY(MAP60(m, __VA_ARGS__))
#define MAP62(m, x, ...) m(x) IDENTITY(MAP61(m, __VA_ARGS__))
#define MAP63(m, x, ...) m(x) IDENTITY(MAP62(m, __VA_ARGS__))
#define MAP64(m, x, ...) m(x) IDENTITY(MAP63(m, __VA_ARGS__))
#define MAP65(m, x, ...) m(x) IDENTITY(MAP64(m, __VA_ARGS__))
#define MAP66(m, x, ...) m(x) IDENTITY(MAP65(m, __VA_ARGS__))
#define MAP67(m, x, ...) m(x) IDENTITY(MAP66(m, __VA_ARGS__))
#define MAP68(m, x, ...) m(x) IDENTITY(MAP67(m, __VA_ARGS__))
#define MAP69(m, x, ...) m(x) IDENTITY(MAP68(m, __VA_ARGS__))
#define MAP70(m, x, ...) m(x) IDENTITY(MAP69(m, __VA_ARGS__))
#define MAP71(m, x, ...) m(x) IDENTITY(MAP70(m, __VA_ARGS__))
#define MAP72(m, x, ...) m(x) IDENTITY(MAP71(m, __VA_ARGS__))
#define MAP73(m, x, ...) m(x) IDENTITY(MAP72(m, __VA_ARGS__))
#define MAP74(m, x, ...) m(x) IDENTITY(MAP73(m, __VA_ARGS__))
#define MAP75(m, x, ...) m(x) IDENTITY(MAP74(m, __VA_ARGS__))
#define MAP76(m, x, ...) m(x) IDENTITY(MAP75(m, __VA_ARGS__))
#define MAP77(m, x, ...) m(x) IDENTITY(MAP76(m, __VA_ARGS__))
#define MAP78(m, x, ...) m(x) IDENTITY(MAP77(m, __VA_ARGS__))
#define MAP79(m, x, ...) m(x) IDENTITY(MAP78(m, __VA_ARGS__))
#define MAP80(m, x, ...) m(x) IDENTITY(MAP79(m, __VA_ARGS__))
#define MAP81(m, x, ...) m(x) IDENTITY(MAP80(m, __VA_ARGS__))
#define MAP82(m, x, ...) m(x) IDENTITY(MAP81(m, __VA_ARGS__))
#define MAP83(m, x, ...) m(x) IDENTITY(MAP82(m, __VA_ARGS__))
#define MAP84(m, x, ...) m(x) IDENTITY(MAP83(m, __VA_ARGS__))
#define MAP85(m, x, ...) m(x) IDENTITY(MAP84(m, __VA_ARGS__))
#define MAP86(m, x, ...) m(x) IDENTITY(MAP85(m, __VA_ARGS__))
#define MAP87(m, x, ...) m(x) IDENTITY(MAP86(m, __VA_ARGS__))
#define MAP88(m, x, ...) m(x) IDENTITY(MAP87(m, __VA_ARGS__))
#define MAP89(m, x, ...) m(x) IDENTITY(MAP88(m, __VA_ARGS__))
#define MAP90(m, x, ...) m(x) IDENTITY(MAP89(m, __VA_ARGS__))
#define MAP91(m, x, ...) m(x) IDENTITY(MAP90(m, __VA_ARGS__))
#define MAP92(m, x, ...) m(x) IDENTITY(MAP91(m, __VA_ARGS__))
#define MAP93(m, x, ...) m(x) IDENTITY(MAP92(m, __VA_ARGS__))
#define MAP94(m, x, ...) m(x) IDENTITY(MAP93(m, __VA_ARGS__))
#define MAP95(m, x, ...) m(x) IDENTITY(MAP94(m, __VA_ARGS__))
#define MAP96(m, x, ...) m(x) IDENTITY(MAP95(m, __VA_ARGS__))
#define MAP97(m, x, ...) m(x) IDENTITY(MAP96(m, __VA_ARGS__))
#define MAP98(m, x, ...) m(x) IDENTITY(MAP97(m, __VA_ARGS__))
#define MAP99(m, x, ...) m(x) IDENTITY(MAP98(m, __VA_ARGS__))
#define MAP100(m, x, ...) m(x) IDENTITY(MAP99(m, __VA_ARGS__))
#define MAP101(m, x, ...) m(x) IDENTITY(MAP100(m, __VA_ARGS__))
#define MAP102(m, x, ...) m(x) IDENTITY(MAP101(m, __VA_ARGS__))
#define MAP103(m, x, ...) m(x) IDENTITY(MAP102(m, __VA_ARGS__))
#define MAP104(m, x, ...) m(x) IDENTITY(MAP103(m, __VA_ARGS__))
#define MAP105(m, x, ...) m(x) IDENTITY(MAP104(m, __VA_ARGS__))
#define MAP106(m, x, ...) m(x) IDENTITY(MAP105(m, __VA_ARGS__))
#define MAP107(m, x, ...) m(x) IDENTITY(MAP106(m, __VA_ARGS__))
#define MAP108(m, x, ...) m(x) IDENTITY(MAP107(m, __VA_ARGS__))
#define MAP109(m, x, ...) m(x) IDENTITY(MAP108(m, __VA_ARGS__))
#define MAP110(m, x, ...) m(x) IDENTITY(MAP109(m, __VA_ARGS__))
#define MAP111(m, x, ...) m(x) IDENTITY(MAP110(m, __VA_ARGS__))
#define MAP112(m, x, ...) m(x) IDENTITY(MAP111(m, __VA_ARGS__))
#define MAP113(m, x, ...) m(x) IDENTITY(MAP112(m, __VA_ARGS__))
#define MAP114(m, x, ...) m(x) IDENTITY(MAP113(m, __VA_ARGS__))
#define MAP115(m, x, ...) m(x) IDENTITY(MAP114(m, __VA_ARGS__))
#define MAP116(m, x, ...) m(x) IDENTITY(MAP115(m, __VA_ARGS__))
#define MAP117(m, x, ...) m(x) IDENTITY(MAP116(m, __VA_ARGS__))
#define MAP118(m, x, ...) m(x) IDENTITY(MAP117(m, __VA_ARGS__))
#define MAP119(m, x, ...) m(x) IDENTITY(MAP118(m, __VA_ARGS__))
#define MAP120(m, x, ...) m(x) IDENTITY(MAP119(m, __VA_ARGS__))
#define MAP121(m, x, ...) m(x) IDENTITY(MAP120(m, __VA_ARGS__))
#define MAP122(m, x, ...) m(x) IDENTITY(MAP121(m, __VA_ARGS__))
#define MAP123(m, x, ...) m(x) IDENTITY(MAP122(m, __VA_ARGS__))
#define MAP124(m, x, ...) m(x) IDENTITY(MAP123(m, __VA_ARGS__))
#define MAP125(m, x, ...) m(x) IDENTITY(MAP124(m, __VA_ARGS__))
#define MAP126(m, x, ...) m(x) IDENTITY(MAP125(m, __VA_ARGS__))
#define MAP127(m, x, ...) m(x) IDENTITY(MAP126(m, __VA_ARGS__))
#define MAP128(m, x, ...) m(x) IDENTITY(MAP127(m, __VA_ARGS__))
#define MAP129(m, x, ...) m(x) IDENTITY(MAP128(m, __VA_ARGS__))
#define MAP130(m, x, ...) m(x) IDENTITY(MAP129(m, __VA_ARGS__))
#define MAP131(m, x, ...) m(x) IDENTITY(MAP130(m, __VA_ARGS__))
#define MAP132(m, x, ...) m(x) IDENTITY(MAP131(m, __VA_ARGS__))
#define MAP133(m, x, ...) m(x) IDENTITY(MAP132(m, __VA_ARGS__))
#define MAP134(m, x, ...) m(x) IDENTITY(MAP133(m, __VA_ARGS__))
#define MAP135(m, x, ...) m(x) IDENTITY(MAP134(m, __VA_ARGS__))
#define MAP136(m, x, ...) m(x) IDENTITY(MAP135(m, __VA_ARGS__))
#define MAP137(m, x, ...) m(x) IDENTITY(MAP136(m, __VA_ARGS__))
#define MAP138(m, x, ...) m(x) IDENTITY(MAP137(m, __VA_ARGS__))
#define MAP139(m, x, ...) m(x) IDENTITY(MAP138(m, __VA_ARGS__))
#define MAP140(m, x, ...) m(x) IDENTITY(MAP139(m, __VA_ARGS__))
#define MAP141(m, x, ...) m(x) IDENTITY(MAP140(m, __VA_ARGS__))
#define MAP142(m, x, ...) m(x) IDENTITY(MAP141(m, __VA_ARGS__))
#define MAP143(m, x, ...) m(x) IDENTITY(MAP142(m, __VA_ARGS__))
#define MAP144(m, x, ...) m(x) IDENTITY(MAP143(m, __VA_ARGS__))
#define MAP145(m, x, ...) m(x) IDENTITY(MAP144(m, __VA_ARGS__))
#define MAP146(m, x, ...) m(x) IDENTITY(MAP145(m, __VA_ARGS__))
#define MAP147(m, x, ...) m(x) IDENTITY(MAP146(m, __VA_ARGS__))
#define MAP148(m, x, ...) m(x) IDENTITY(MAP147(m, __VA_ARGS__))
#define MAP149(m, x, ...) m(x) IDENTITY(MAP148(m, __VA_ARGS__))
#define MAP150(m, x, ...) m(x) IDENTITY(MAP149(m, __VA_ARGS__))
#define MAP151(m, x, ...) m(x) IDENTITY(MAP150(m, __VA_ARGS__))
#define MAP152(m, x, ...) m(x) IDENTITY(MAP151(m, __VA_ARGS__))
#define MAP153(m, x, ...) m(x) IDENTITY(MAP152(m, __VA_ARGS__))
#define MAP154(m, x, ...) m(x) IDENTITY(MAP153(m, __VA_ARGS__))
#define MAP155(m, x, ...) m(x) IDENTITY(MAP154(m, __VA_ARGS__))
#define MAP156(m, x, ...) m(x) IDENTITY(MAP155(m, __VA_ARGS__))
#define MAP157(m, x, ...) m(x) IDENTITY(MAP156(m, __VA_ARGS__))
#define MAP158(m, x, ...) m(x) IDENTITY(MAP157(m, __VA_ARGS__))
#define MAP159(m, x, ...) m(x) IDENTITY(MAP158(m, __VA_ARGS__))
#define MAP160(m, x, ...) m(x) IDENTITY(MAP159(m, __VA_ARGS__))
#define MAP161(m, x, ...) m(x) IDENTITY(MAP160(m, __VA_ARGS__))
#define MAP162(m, x, ...) m(x) IDENTITY(MAP161(m, __VA_ARGS__))
#define MAP163(m, x, ...) m(x) IDENTITY(MAP162(m, __VA_ARGS__))
#define MAP164(m, x, ...) m(x) IDENTITY(MAP163(m, __VA_ARGS__))
#define MAP165(m, x, ...) m(x) IDENTITY(MAP164(m, __VA_ARGS__))
#define MAP166(m, x, ...) m(x) IDENTITY(MAP165(m, __VA_ARGS__))
#define MAP167(m, x, ...) m(x) IDENTITY(MAP166(m, __VA_ARGS__))
#define MAP168(m, x, ...) m(x) IDENTITY(MAP167(m, __VA_ARGS__))
#define MAP169(m, x, ...) m(x) IDENTITY(MAP168(m, __VA_ARGS__))
#define MAP170(m, x, ...) m(x) IDENTITY(MAP169(m, __VA_ARGS__))
#define MAP171(m, x, ...) m(x) IDENTITY(MAP170(m, __VA_ARGS__))
#define MAP172(m, x, ...) m(x) IDENTITY(MAP171(m, __VA_ARGS__))
#define MAP173(m, x, ...) m(x) IDENTITY(MAP172(m, __VA_ARGS__))
#define MAP174(m, x, ...) m(x) IDENTITY(MAP173(m, __VA_ARGS__))
#define MAP175(m, x, ...) m(x) IDENTITY(MAP174(m, __VA_ARGS__))
#define MAP176(m, x, ...) m(x) IDENTITY(MAP175(m, __VA_ARGS__))
#define MAP177(m, x, ...) m(x) IDENTITY(MAP176(m, __VA_ARGS__))
#define MAP178(m, x, ...) m(x) IDENTITY(MAP177(m, __VA_ARGS__))
#define MAP179(m, x, ...) m(x) IDENTITY(MAP178(m, __VA_ARGS__))
#define MAP180(m, x, ...) m(x) IDENTITY(MAP179(m, __VA_ARGS__))
#define MAP181(m, x, ...) m(x) IDENTITY(MAP180(m, __VA_ARGS__))
#define MAP182(m, x, ...) m(x) IDENTITY(MAP181(m, __VA_ARGS__))
#define MAP183(m, x, ...) m(x) IDENTITY(MAP182(m, __VA_ARGS__))
#define MAP184(m, x, ...) m(x) IDENTITY(MAP183(m, __VA_ARGS__))
#define MAP185(m, x, ...) m(x) IDENTITY(MAP184(m, __VA_ARGS__))
#define MAP186(m, x, ...) m(x) IDENTITY(MAP185(m, __VA_ARGS__))
#define MAP187(m, x, ...) m(x) IDENTITY(MAP186(m, __VA_ARGS__))
#define MAP188(m, x, ...) m(x) IDENTITY(MAP187(m, __VA_ARGS__))
#define MAP189(m, x, ...) m(x) IDENTITY(MAP188(m, __VA_ARGS__))
#define MAP190(m, x, ...) m(x) IDENTITY(MAP189(m, __VA_ARGS__))
#define MAP191(m, x, ...) m(x) IDENTITY(MAP190(m, __VA_ARGS__))
#define MAP192(m, x, ...) m(x) IDENTITY(MAP191(m, __VA_ARGS__))
#define MAP193(m, x, ...) m(x) IDENTITY(MAP192(m, __VA_ARGS__))
#define MAP194(m, x, ...) m(x) IDENTITY(MAP193(m, __VA_ARGS__))
#define MAP195(m, x, ...) m(x) IDENTITY(MAP194(m, __VA_ARGS__))
#define MAP196(m, x, ...) m(x) IDENTITY(MAP195(m, __VA_ARGS__))
#define MAP197(m, x, ...) m(x) IDENTITY(MAP196(m, __VA_ARGS__))
#define MAP198(m, x, ...) m(x) IDENTITY(MAP197(m, __VA_ARGS__))
#define MAP199(m, x, ...) m(x) IDENTITY(MAP198(m, __VA_ARGS__))
#define MAP200(m, x, ...) m(x) IDENTITY(MAP199(m, __VA_ARGS__))
#define MAP201(m, x, ...) m(x) IDENTITY(MAP200(m, __VA_ARGS__))
#define MAP202(m, x, ...) m(x) IDENTITY(MAP201(m, __VA_ARGS__))
#define MAP203(m, x, ...) m(x) IDENTITY(MAP202(m, __VA_ARGS__))
#define MAP204(m, x, ...) m(x) IDENTITY(MAP203(m, __VA_ARGS__))
#define MAP205(m, x, ...) m(x) IDENTITY(MAP204(m, __VA_ARGS__))
#define MAP206(m, x, ...) m(x) IDENTITY(MAP205(m, __VA_ARGS__))
#define MAP207(m, x, ...) m(x) IDENTITY(MAP206(m, __VA_ARGS__))
#define MAP208(m, x, ...) m(x) IDENTITY(MAP207(m, __VA_ARGS__))
#define MAP209(m, x, ...) m(x) IDENTITY(MAP208(m, __VA_ARGS__))
#define MAP210(m, x, ...) m(x) IDENTITY(MAP209(m, __VA_ARGS__))
#define MAP211(m, x, ...) m(x) IDENTITY(MAP210(m, __VA_ARGS__))
#define MAP212(m, x, ...) m(x) IDENTITY(MAP211(m, __VA_ARGS__))
#define MAP213(m, x, ...) m(x) IDENTITY(MAP212(m, __VA_ARGS__))
#define MAP214(m, x, ...) m(x) IDENTITY(MAP213(m, __VA_ARGS__))
#define MAP215(m, x, ...) m(x) IDENTITY(MAP214(m, __VA_ARGS__))
#define MAP216(m, x, ...) m(x) IDENTITY(MAP215(m, __VA_ARGS__))
#define MAP217(m, x, ...) m(x) IDENTITY(MAP216(m, __VA_ARGS__))
#define MAP218(m, x, ...) m(x) IDENTITY(MAP217(m, __VA_ARGS__))
#define MAP219(m, x, ...) m(x) IDENTITY(MAP218(m, __VA_ARGS__))
#define MAP220(m, x, ...) m(x) IDENTITY(MAP219(m, __VA_ARGS__))
#define MAP221(m, x, ...) m(x) IDENTITY(MAP220(m, __VA_ARGS__))
#define MAP222(m, x, ...) m(x) IDENTITY(MAP221(m, __VA_ARGS__))
#define MAP223(m, x, ...) m(x) IDENTITY(MAP222(m, __VA_ARGS__))
#define MAP224(m, x, ...) m(x) IDENTITY(MAP223(m, __VA_ARGS__))
#define MAP225(m, x, ...) m(x) IDENTITY(MAP224(m, __VA_ARGS__))
#define MAP226(m, x, ...) m(x) IDENTITY(MAP225(m, __VA_ARGS__))
#define MAP227(m, x, ...) m(x) IDENTITY(MAP226(m, __VA_ARGS__))
#define MAP228(m, x, ...) m(x) IDENTITY(MAP227(m, __VA_ARGS__))
#define MAP229(m, x, ...) m(x) IDENTITY(MAP228(m, __VA_ARGS__))
#define MAP230(m, x, ...) m(x) IDENTITY(MAP229(m, __VA_ARGS__))
#define MAP231(m, x, ...) m(x) IDENTITY(MAP230(m, __VA_ARGS__))
#define MAP232(m, x, ...) m(x) IDENTITY(MAP231(m, __VA_ARGS__))
#define MAP233(m, x, ...) m(x) IDENTITY(MAP232(m, __VA_ARGS__))
#define MAP234(m, x, ...) m(x) IDENTITY(MAP233(m, __VA_ARGS__))
#define MAP235(m, x, ...) m(x) IDENTITY(MAP234(m, __VA_ARGS__))
#define MAP236(m, x, ...) m(x) IDENTITY(MAP235(m, __VA_ARGS__))
#define MAP237(m, x, ...) m(x) IDENTITY(MAP236(m, __VA_ARGS__))
#define MAP238(m, x, ...) m(x) IDENTITY(MAP237(m, __VA_ARGS__))
#define MAP239(m, x, ...) m(x) IDENTITY(MAP238(m, __VA_ARGS__))
#define MAP240(m, x, ...) m(x) IDENTITY(MAP239(m, __VA_ARGS__))
#define MAP241(m, x, ...) m(x) IDENTITY(MAP240(m, __VA_ARGS__))
#define MAP242(m, x, ...) m(x) IDENTITY(MAP241(m, __VA_ARGS__))
#define MAP243(m, x, ...) m(x) IDENTITY(MAP242(m, __VA_ARGS__))
#define MAP244(m, x, ...) m(x) IDENTITY(MAP243(m, __VA_ARGS__))
#define MAP245(m, x, ...) m(x) IDENTITY(MAP244(m, __VA_ARGS__))
#define MAP246(m, x, ...) m(x) IDENTITY(MAP245(m, __VA_ARGS__))
#define MAP247(m, x, ...) m(x) IDENTITY(MAP246(m, __VA_ARGS__))
#define MAP248(m, x, ...) m(x) IDENTITY(MAP247(m, __VA_ARGS__))
#define MAP249(m, x, ...) m(x) IDENTITY(MAP248(m, __VA_ARGS__))
#define MAP250(m, x, ...) m(x) IDENTITY(MAP249(m, __VA_ARGS__))
#define MAP251(m, x, ...) m(x) IDENTITY(MAP250(m, __VA_ARGS__))
#define MAP252(m, x, ...) m(x) IDENTITY(MAP251(m, __VA_ARGS__))
#define MAP253(m, x, ...) m(x) IDENTITY(MAP252(m, __VA_ARGS__))
#define MAP254(m, x, ...) m(x) IDENTITY(MAP253(m, __VA_ARGS__))
#define MAP255(m, x, ...) m(x) IDENTITY(MAP254(m, __VA_ARGS__))
#define MAP256(m, x, ...) m(x) IDENTITY(MAP255(m, __VA_ARGS__))
#define MAP257(m, x, ...) m(x) IDENTITY(MAP256(m, __VA_ARGS__))
#define MAP258(m, x, ...) m(x) IDENTITY(MAP257(m, __VA_ARGS__))
#define MAP259(m, x, ...) m(x) IDENTITY(MAP258(m, __VA_ARGS__))
#define MAP260(m, x, ...) m(x) IDENTITY(MAP259(m, __VA_ARGS__))
#define MAP261(m, x, ...) m(x) IDENTITY(MAP260(m, __VA_ARGS__))
#define MAP262(m, x, ...) m(x) IDENTITY(MAP261(m, __VA_ARGS__))
#define MAP263(m, x, ...) m(x) IDENTITY(MAP262(m, __VA_ARGS__))
#define MAP264(m, x, ...) m(x) IDENTITY(MAP263(m, __VA_ARGS__))
#define MAP265(m, x, ...) m(x) IDENTITY(MAP264(m, __VA_ARGS__))
#define MAP266(m, x, ...) m(x) IDENTITY(MAP265(m, __VA_ARGS__))
#define MAP267(m, x, ...) m(x) IDENTITY(MAP266(m, __VA_ARGS__))
#define MAP268(m, x, ...) m(x) IDENTITY(MAP267(m, __VA_ARGS__))
#define MAP269(m, x, ...) m(x) IDENTITY(MAP268(m, __VA_ARGS__))
#define MAP270(m, x, ...) m(x) IDENTITY(MAP269(m, __VA_ARGS__))
#define MAP271(m, x, ...) m(x) IDENTITY(MAP270(m, __VA_ARGS__))
#define MAP272(m, x, ...) m(x) IDENTITY(MAP271(m, __VA_ARGS__))
#define MAP273(m, x, ...) m(x) IDENTITY(MAP272(m, __VA_ARGS__))
#define MAP274(m, x, ...) m(x) IDENTITY(MAP273(m, __VA_ARGS__))
#define MAP275(m, x, ...) m(x) IDENTITY(MAP274(m, __VA_ARGS__))
#define MAP276(m, x, ...) m(x) IDENTITY(MAP275(m, __VA_ARGS__))
#define MAP277(m, x, ...) m(x) IDENTITY(MAP276(m, __VA_ARGS__))
#define MAP278(m, x, ...) m(x) IDENTITY(MAP277(m, __VA_ARGS__))
#define MAP279(m, x, ...) m(x) IDENTITY(MAP278(m, __VA_ARGS__))
#define MAP280(m, x, ...) m(x) IDENTITY(MAP279(m, __VA_ARGS__))
#define MAP281(m, x, ...) m(x) IDENTITY(MAP280(m, __VA_ARGS__))
#define MAP282(m, x, ...) m(x) IDENTITY(MAP281(m, __VA_ARGS__))
#define MAP283(m, x, ...) m(x) IDENTITY(MAP282(m, __VA_ARGS__))
#define MAP284(m, x, ...) m(x) IDENTITY(MAP283(m, __VA_ARGS__))
#define MAP285(m, x, ...) m(x) IDENTITY(MAP284(m, __VA_ARGS__))
#define MAP286(m, x, ...) m(x) IDENTITY(MAP285(m, __VA_ARGS__))
#define MAP287(m, x, ...) m(x) IDENTITY(MAP286(m, __VA_ARGS__))
#define MAP288(m, x, ...) m(x) IDENTITY(MAP287(m, __VA_ARGS__))
#define MAP289(m, x, ...) m(x) IDENTITY(MAP288(m, __VA_ARGS__))
#define MAP290(m, x, ...) m(x) IDENTITY(MAP289(m, __VA_ARGS__))
#define MAP291(m, x, ...) m(x) IDENTITY(MAP290(m, __VA_ARGS__))
#define MAP292(m, x, ...) m(x) IDENTITY(MAP291(m, __VA_ARGS__))
#define MAP293(m, x, ...) m(x) IDENTITY(MAP292(m, __VA_ARGS__))
#define MAP294(m, x, ...) m(x) IDENTITY(MAP293(m, __VA_ARGS__))
#define MAP295(m, x, ...) m(x) IDENTITY(MAP294(m, __VA_ARGS__))
#define MAP296(m, x, ...) m(x) IDENTITY(MAP295(m, __VA_ARGS__))
#define MAP297(m, x, ...) m(x) IDENTITY(MAP296(m, __VA_ARGS__))
#define MAP298(m, x, ...) m(x) IDENTITY(MAP297(m, __VA_ARGS__))
#define MAP299(m, x, ...) m(x) IDENTITY(MAP298(m, __VA_ARGS__))
#define MAP300(m, x, ...) m(x) IDENTITY(MAP299(m, __VA_ARGS__))
#define MAP301(m, x, ...) m(x) IDENTITY(MAP300(m, __VA_ARGS__))
#define MAP302(m, x, ...) m(x) IDENTITY(MAP301(m, __VA_ARGS__))
#define MAP303(m, x, ...) m(x) IDENTITY(MAP302(m, __VA_ARGS__))
#define MAP304(m, x, ...) m(x) IDENTITY(MAP303(m, __VA_ARGS__))
#define MAP305(m, x, ...) m(x) IDENTITY(MAP304(m, __VA_ARGS__))
#define MAP306(m, x, ...) m(x) IDENTITY(MAP305(m, __VA_ARGS__))
#define MAP307(m, x, ...) m(x) IDENTITY(MAP306(m, __VA_ARGS__))
#define MAP308(m, x, ...) m(x) IDENTITY(MAP307(m, __VA_ARGS__))
#define MAP309(m, x, ...) m(x) IDENTITY(MAP308(m, __VA_ARGS__))
#define MAP310(m, x, ...) m(x) IDENTITY(MAP309(m, __VA_ARGS__))
#define MAP311(m, x, ...) m(x) IDENTITY(MAP310(m, __VA_ARGS__))
#define MAP312(m, x, ...) m(x) IDENTITY(MAP311(m, __VA_ARGS__))
#define MAP313(m, x, ...) m(x) IDENTITY(MAP312(m, __VA_ARGS__))
#define MAP314(m, x, ...) m(x) IDENTITY(MAP313(m, __VA_ARGS__))
#define MAP315(m, x, ...) m(x) IDENTITY(MAP314(m, __VA_ARGS__))
#define MAP316(m, x, ...) m(x) IDENTITY(MAP315(m, __VA_ARGS__))
#define MAP317(m, x, ...) m(x) IDENTITY(MAP316(m, __VA_ARGS__))
#define MAP318(m, x, ...) m(x) IDENTITY(MAP317(m, __VA_ARGS__))
#define MAP319(m, x, ...) m(x) IDENTITY(MAP318(m, __VA_ARGS__))
#define MAP320(m, x, ...) m(x) IDENTITY(MAP319(m, __VA_ARGS__))
#define MAP321(m, x, ...) m(x) IDENTITY(MAP320(m, __VA_ARGS__))
#define MAP322(m, x, ...) m(x) IDENTITY(MAP321(m, __VA_ARGS__))
#define MAP323(m, x, ...) m(x) IDENTITY(MAP322(m, __VA_ARGS__))
#define MAP324(m, x, ...) m(x) IDENTITY(MAP323(m, __VA_ARGS__))
#define MAP325(m, x, ...) m(x) IDENTITY(MAP324(m, __VA_ARGS__))
#define MAP326(m, x, ...) m(x) IDENTITY(MAP325(m, __VA_ARGS__))
#define MAP327(m, x, ...) m(x) IDENTITY(MAP326(m, __VA_ARGS__))
#define MAP328(m, x, ...) m(x) IDENTITY(MAP327(m, __VA_ARGS__))
#define MAP329(m, x, ...) m(x) IDENTITY(MAP328(m, __VA_ARGS__))
#define MAP330(m, x, ...) m(x) IDENTITY(MAP329(m, __VA_ARGS__))
#define MAP331(m, x, ...) m(x) IDENTITY(MAP330(m, __VA_ARGS__))
#define MAP332(m, x, ...) m(x) IDENTITY(MAP331(m, __VA_ARGS__))
#define MAP333(m, x, ...) m(x) IDENTITY(MAP332(m, __VA_ARGS__))
#define MAP334(m, x, ...) m(x) IDENTITY(MAP333(m, __VA_ARGS__))
#define MAP335(m, x, ...) m(x) IDENTITY(MAP334(m, __VA_ARGS__))
#define MAP336(m, x, ...) m(x) IDENTITY(MAP335(m, __VA_ARGS__))
#define MAP337(m, x, ...) m(x) IDENTITY(MAP336(m, __VA_ARGS__))
#define MAP338(m, x, ...) m(x) IDENTITY(MAP337(m, __VA_ARGS__))
#define MAP339(m, x, ...) m(x) IDENTITY(MAP338(m, __VA_ARGS__))
#define MAP340(m, x, ...) m(x) IDENTITY(MAP339(m, __VA_ARGS__))
#define MAP341(m, x, ...) m(x) IDENTITY(MAP340(m, __VA_ARGS__))
#define MAP342(m, x, ...) m(x) IDENTITY(MAP341(m, __VA_ARGS__))
#define MAP343(m, x, ...) m(x) IDENTITY(MAP342(m, __VA_ARGS__))
#define MAP344(m, x, ...) m(x) IDENTITY(MAP343(m, __VA_ARGS__))
#define MAP345(m, x, ...) m(x) IDENTITY(MAP344(m, __VA_ARGS__))
#define MAP346(m, x, ...) m(x) IDENTITY(MAP345(m, __VA_ARGS__))
#define MAP347(m, x, ...) m(x) IDENTITY(MAP346(m, __VA_ARGS__))
#define MAP348(m, x, ...) m(x) IDENTITY(MAP347(m, __VA_ARGS__))
#define MAP349(m, x, ...) m(x) IDENTITY(MAP348(m, __VA_ARGS__))
#define MAP350(m, x, ...) m(x) IDENTITY(MAP349(m, __VA_ARGS__))
#define MAP351(m, x, ...) m(x) IDENTITY(MAP350(m, __VA_ARGS__))
#define MAP352(m, x, ...) m(x) IDENTITY(MAP351(m, __VA_ARGS__))
#define MAP353(m, x, ...) m(x) IDENTITY(MAP352(m, __VA_ARGS__))
#define MAP354(m, x, ...) m(x) IDENTITY(MAP353(m, __VA_ARGS__))
#define MAP355(m, x, ...) m(x) IDENTITY(MAP354(m, __VA_ARGS__))
#define MAP356(m, x, ...) m(x) IDENTITY(MAP355(m, __VA_ARGS__))
#define MAP357(m, x, ...) m(x) IDENTITY(MAP356(m, __VA_ARGS__))
#define MAP358(m, x, ...) m(x) IDENTITY(MAP357(m, __VA_ARGS__))
#define MAP359(m, x, ...) m(x) IDENTITY(MAP358(m, __VA_ARGS__))
#define MAP360(m, x, ...) m(x) IDENTITY(MAP359(m, __VA_ARGS__))
#define MAP361(m, x, ...) m(x) IDENTITY(MAP360(m, __VA_ARGS__))
#define MAP362(m, x, ...) m(x) IDENTITY(MAP361(m, __VA_ARGS__))
#define MAP363(m, x, ...) m(x) IDENTITY(MAP362(m, __VA_ARGS__))
#define MAP364(m, x, ...) m(x) IDENTITY(MAP363(m, __VA_ARGS__))
#define MAP365(m, x, ...) m(x) IDENTITY(MAP364(m, __VA_ARGS__))
#define MAP366(m, x, ...) m(x) IDENTITY(MAP365(m, __VA_ARGS__))
#define MAP367(m, x, ...) m(x) IDENTITY(MAP366(m, __VA_ARGS__))
#define MAP368(m, x, ...) m(x) IDENTITY(MAP367(m, __VA_ARGS__))
#define MAP369(m, x, ...) m(x) IDENTITY(MAP368(m, __VA_ARGS__))
#define MAP370(m, x, ...) m(x) IDENTITY(MAP369(m, __VA_ARGS__))
#define MAP371(m, x, ...) m(x) IDENTITY(MAP370(m, __VA_ARGS__))
#define MAP372(m, x, ...) m(x) IDENTITY(MAP371(m, __VA_ARGS__))
#define MAP373(m, x, ...) m(x) IDENTITY(MAP372(m, __VA_ARGS__))
#define MAP374(m, x, ...) m(x) IDENTITY(MAP373(m, __VA_ARGS__))
#define MAP375(m, x, ...) m(x) IDENTITY(MAP374(m, __VA_ARGS__))
#define MAP376(m, x, ...) m(x) IDENTITY(MAP375(m, __VA_ARGS__))
#define MAP377(m, x, ...) m(x) IDENTITY(MAP376(m, __VA_ARGS__))
#define MAP378(m, x, ...) m(x) IDENTITY(MAP377(m, __VA_ARGS__))
#define MAP379(m, x, ...) m(x) IDENTITY(MAP378(m, __VA_ARGS__))
#define MAP380(m, x, ...) m(x) IDENTITY(MAP379(m, __VA_ARGS__))
#define MAP381(m, x, ...) m(x) IDENTITY(MAP380(m, __VA_ARGS__))
#define MAP382(m, x, ...) m(x) IDENTITY(MAP381(m, __VA_ARGS__))
#define MAP383(m, x, ...) m(x) IDENTITY(MAP382(m, __VA_ARGS__))
#define MAP384(m, x, ...) m(x) IDENTITY(MAP383(m, __VA_ARGS__))
#define MAP385(m, x, ...) m(x) IDENTITY(MAP384(m, __VA_ARGS__))
#define MAP386(m, x, ...) m(x) IDENTITY(MAP385(m, __VA_ARGS__))
#define MAP387(m, x, ...) m(x) IDENTITY(MAP386(m, __VA_ARGS__))
#define MAP388(m, x, ...) m(x) IDENTITY(MAP387(m, __VA_ARGS__))
#define MAP389(m, x, ...) m(x) IDENTITY(MAP388(m, __VA_ARGS__))
#define MAP390(m, x, ...) m(x) IDENTITY(MAP389(m, __VA_ARGS__))
#define MAP391(m, x, ...) m(x) IDENTITY(MAP390(m, __VA_ARGS__))
#define MAP392(m, x, ...) m(x) IDENTITY(MAP391(m, __VA_ARGS__))
#define MAP393(m, x, ...) m(x) IDENTITY(MAP392(m, __VA_ARGS__))
#define MAP394(m, x, ...) m(x) IDENTITY(MAP393(m, __VA_ARGS__))
#define MAP395(m, x, ...) m(x) IDENTITY(MAP394(m, __VA_ARGS__))
#define MAP396(m, x, ...) m(x) IDENTITY(MAP395(m, __VA_ARGS__))
#define MAP397(m, x, ...) m(x) IDENTITY(MAP396(m, __VA_ARGS__))
#define MAP398(m, x, ...) m(x) IDENTITY(MAP397(m, __VA_ARGS__))
#define MAP399(m, x, ...) m(x) IDENTITY(MAP398(m, __VA_ARGS__))
#define MAP400(m, x, ...) m(x) IDENTITY(MAP399(m, __VA_ARGS__))
#define MAP401(m, x, ...) m(x) IDENTITY(MAP400(m, __VA_ARGS__))
#define MAP402(m, x, ...) m(x) IDENTITY(MAP401(m, __VA_ARGS__))
#define MAP403(m, x, ...) m(x) IDENTITY(MAP402(m, __VA_ARGS__))
#define MAP404(m, x, ...) m(x) IDENTITY(MAP403(m, __VA_ARGS__))
#define MAP405(m, x, ...) m(x) IDENTITY(MAP404(m, __VA_ARGS__))
#define MAP406(m, x, ...) m(x) IDENTITY(MAP405(m, __VA_ARGS__))
#define MAP407(m, x, ...) m(x) IDENTITY(MAP406(m, __VA_ARGS__))
#define MAP408(m, x, ...) m(x) IDENTITY(MAP407(m, __VA_ARGS__))
#define MAP409(m, x, ...) m(x) IDENTITY(MAP408(m, __VA_ARGS__))
#define MAP410(m, x, ...) m(x) IDENTITY(MAP409(m, __VA_ARGS__))
#define MAP411(m, x, ...) m(x) IDENTITY(MAP410(m, __VA_ARGS__))
#define MAP412(m, x, ...) m(x) IDENTITY(MAP411(m, __VA_ARGS__))
#define MAP413(m, x, ...) m(x) IDENTITY(MAP412(m, __VA_ARGS__))
#define MAP414(m, x, ...) m(x) IDENTITY(MAP413(m, __VA_ARGS__))
#define MAP415(m, x, ...) m(x) IDENTITY(MAP414(m, __VA_ARGS__))
#define MAP416(m, x, ...) m(x) IDENTITY(MAP415(m, __VA_ARGS__))
#define MAP417(m, x, ...) m(x) IDENTITY(MAP416(m, __VA_ARGS__))
#define MAP418(m, x, ...) m(x) IDENTITY(MAP417(m, __VA_ARGS__))
#define MAP419(m, x, ...) m(x) IDENTITY(MAP418(m, __VA_ARGS__))
#define MAP420(m, x, ...) m(x) IDENTITY(MAP419(m, __VA_ARGS__))
#define MAP421(m, x, ...) m(x) IDENTITY(MAP420(m, __VA_ARGS__))
#define MAP422(m, x, ...) m(x) IDENTITY(MAP421(m, __VA_ARGS__))
#define MAP423(m, x, ...) m(x) IDENTITY(MAP422(m, __VA_ARGS__))
#define MAP424(m, x, ...) m(x) IDENTITY(MAP423(m, __VA_ARGS__))
#define MAP425(m, x, ...) m(x) IDENTITY(MAP424(m, __VA_ARGS__))
#define MAP426(m, x, ...) m(x) IDENTITY(MAP425(m, __VA_ARGS__))
#define MAP427(m, x, ...) m(x) IDENTITY(MAP426(m, __VA_ARGS__))
#define MAP428(m, x, ...) m(x) IDENTITY(MAP427(m, __VA_ARGS__))
#define MAP429(m, x, ...) m(x) IDENTITY(MAP428(m, __VA_ARGS__))
#define MAP430(m, x, ...) m(x) IDENTITY(MAP429(m, __VA_ARGS__))
#define MAP431(m, x, ...) m(x) IDENTITY(MAP430(m, __VA_ARGS__))
#define MAP432(m, x, ...) m(x) IDENTITY(MAP431(m, __VA_ARGS__))
#define MAP433(m, x, ...) m(x) IDENTITY(MAP432(m, __VA_ARGS__))
#define MAP434(m, x, ...) m(x) IDENTITY(MAP433(m, __VA_ARGS__))
#define MAP435(m, x, ...) m(x) IDENTITY(MAP434(m, __VA_ARGS__))
#define MAP436(m, x, ...) m(x) IDENTITY(MAP435(m, __VA_ARGS__))
#define MAP437(m, x, ...) m(x) IDENTITY(MAP436(m, __VA_ARGS__))
#define MAP438(m, x, ...) m(x) IDENTITY(MAP437(m, __VA_ARGS__))
#define MAP439(m, x, ...) m(x) IDENTITY(MAP438(m, __VA_ARGS__))
#define MAP440(m, x, ...) m(x) IDENTITY(MAP439(m, __VA_ARGS__))
#define MAP441(m, x, ...) m(x) IDENTITY(MAP440(m, __VA_ARGS__))
#define MAP442(m, x, ...) m(x) IDENTITY(MAP441(m, __VA_ARGS__))
#define MAP443(m, x, ...) m(x) IDENTITY(MAP442(m, __VA_ARGS__))
#define MAP444(m, x, ...) m(x) IDENTITY(MAP443(m, __VA_ARGS__))
#define MAP445(m, x, ...) m(x) IDENTITY(MAP444(m, __VA_ARGS__))
#define MAP446(m, x, ...) m(x) IDENTITY(MAP445(m, __VA_ARGS__))
#define MAP447(m, x, ...) m(x) IDENTITY(MAP446(m, __VA_ARGS__))
#define MAP448(m, x, ...) m(x) IDENTITY(MAP447(m, __VA_ARGS__))
#define MAP449(m, x, ...) m(x) IDENTITY(MAP448(m, __VA_ARGS__))
#define MAP450(m, x, ...) m(x) IDENTITY(MAP449(m, __VA_ARGS__))
#define MAP451(m, x, ...) m(x) IDENTITY(MAP450(m, __VA_ARGS__))
#define MAP452(m, x, ...) m(x) IDENTITY(MAP451(m, __VA_ARGS__))
#define MAP453(m, x, ...) m(x) IDENTITY(MAP452(m, __VA_ARGS__))
#define MAP454(m, x, ...) m(x) IDENTITY(MAP453(m, __VA_ARGS__))
#define MAP455(m, x, ...) m(x) IDENTITY(MAP454(m, __VA_ARGS__))
#define MAP456(m, x, ...) m(x) IDENTITY(MAP455(m, __VA_ARGS__))
#define MAP457(m, x, ...) m(x) IDENTITY(MAP456(m, __VA_ARGS__))
#define MAP458(m, x, ...) m(x) IDENTITY(MAP457(m, __VA_ARGS__))
#define MAP459(m, x, ...) m(x) IDENTITY(MAP458(m, __VA_ARGS__))
#define MAP460(m, x, ...) m(x) IDENTITY(MAP459(m, __VA_ARGS__))
#define MAP461(m, x, ...) m(x) IDENTITY(MAP460(m, __VA_ARGS__))
#define MAP462(m, x, ...) m(x) IDENTITY(MAP461(m, __VA_ARGS__))
#define MAP463(m, x, ...) m(x) IDENTITY(MAP462(m, __VA_ARGS__))
#define MAP464(m, x, ...) m(x) IDENTITY(MAP463(m, __VA_ARGS__))
#define MAP465(m, x, ...) m(x) IDENTITY(MAP464(m, __VA_ARGS__))
#define MAP466(m, x, ...) m(x) IDENTITY(MAP465(m, __VA_ARGS__))
#define MAP467(m, x, ...) m(x) IDENTITY(MAP466(m, __VA_ARGS__))
#define MAP468(m, x, ...) m(x) IDENTITY(MAP467(m, __VA_ARGS__))
#define MAP469(m, x, ...) m(x) IDENTITY(MAP468(m, __VA_ARGS__))
#define MAP470(m, x, ...) m(x) IDENTITY(MAP469(m, __VA_ARGS__))
#define MAP471(m, x, ...) m(x) IDENTITY(MAP470(m, __VA_ARGS__))
#define MAP472(m, x, ...) m(x) IDENTITY(MAP471(m, __VA_ARGS__))
#define MAP473(m, x, ...) m(x) IDENTITY(MAP472(m, __VA_ARGS__))
#define MAP474(m, x, ...) m(x) IDENTITY(MAP473(m, __VA_ARGS__))
#define MAP475(m, x, ...) m(x) IDENTITY(MAP474(m, __VA_ARGS__))
#define MAP476(m, x, ...) m(x) IDENTITY(MAP475(m, __VA_ARGS__))
#define MAP477(m, x, ...) m(x) IDENTITY(MAP476(m, __VA_ARGS__))
#define MAP478(m, x, ...) m(x) IDENTITY(MAP477(m, __VA_ARGS__))
#define MAP479(m, x, ...) m(x) IDENTITY(MAP478(m, __VA_ARGS__))
#define MAP480(m, x, ...) m(x) IDENTITY(MAP479(m, __VA_ARGS__))
#define MAP481(m, x, ...) m(x) IDENTITY(MAP480(m, __VA_ARGS__))
#define MAP482(m, x, ...) m(x) IDENTITY(MAP481(m, __VA_ARGS__))
#define MAP483(m, x, ...) m(x) IDENTITY(MAP482(m, __VA_ARGS__))
#define MAP484(m, x, ...) m(x) IDENTITY(MAP483(m, __VA_ARGS__))
#define MAP485(m, x, ...) m(x) IDENTITY(MAP484(m, __VA_ARGS__))
#define MAP486(m, x, ...) m(x) IDENTITY(MAP485(m, __VA_ARGS__))
#define MAP487(m, x, ...) m(x) IDENTITY(MAP486(m, __VA_ARGS__))
#define MAP488(m, x, ...) m(x) IDENTITY(MAP487(m, __VA_ARGS__))
#define MAP489(m, x, ...) m(x) IDENTITY(MAP488(m, __VA_ARGS__))
#define MAP490(m, x, ...) m(x) IDENTITY(MAP489(m, __VA_ARGS__))
#define MAP491(m, x, ...) m(x) IDENTITY(MAP490(m, __VA_ARGS__))
#define MAP492(m, x, ...) m(x) IDENTITY(MAP491(m, __VA_ARGS__))
#define MAP493(m, x, ...) m(x) IDENTITY(MAP492(m, __VA_ARGS__))
#define MAP494(m, x, ...) m(x) IDENTITY(MAP493(m, __VA_ARGS__))
#define MAP495(m, x, ...) m(x) IDENTITY(MAP494(m, __VA_ARGS__))
#define MAP496(m, x, ...) m(x) IDENTITY(MAP495(m, __VA_ARGS__))
#define MAP497(m, x, ...) m(x) IDENTITY(MAP496(m, __VA_ARGS__))
#define MAP498(m, x, ...) m(x) IDENTITY(MAP497(m, __VA_ARGS__))
#define MAP499(m, x, ...) m(x) IDENTITY(MAP498(m, __VA_ARGS__))
#define MAP500(m, x, ...) m(x) IDENTITY(MAP499(m, __VA_ARGS__))
#define MAP501(m, x, ...) m(x) IDENTITY(MAP500(m, __VA_ARGS__))
#define MAP502(m, x, ...) m(x) IDENTITY(MAP501(m, __VA_ARGS__))
#define MAP503(m, x, ...) m(x) IDENTITY(MAP502(m, __VA_ARGS__))
#define MAP504(m, x, ...) m(x) IDENTITY(MAP503(m, __VA_ARGS__))
#define MAP505(m, x, ...) m(x) IDENTITY(MAP504(m, __VA_ARGS__))
#define MAP506(m, x, ...) m(x) IDENTITY(MAP505(m, __VA_ARGS__))
#define MAP507(m, x, ...) m(x) IDENTITY(MAP506(m, __VA_ARGS__))
#define MAP508(m, x, ...) m(x) IDENTITY(MAP507(m, __VA_ARGS__))
#define MAP509(m, x, ...) m(x) IDENTITY(MAP508(m, __VA_ARGS__))
#define MAP510(m, x, ...) m(x) IDENTITY(MAP509(m, __VA_ARGS__))
#define MAP511(m, x, ...) m(x) IDENTITY(MAP510(m, __VA_ARGS__))
#define MAP512(m, x, ...) m(x) IDENTITY(MAP511(m, __VA_ARGS__))
#define MAP513(m, x, ...) m(x) IDENTITY(MAP512(m, __VA_ARGS__))
#define MAP514(m, x, ...) m(x) IDENTITY(MAP513(m, __VA_ARGS__))
#define MAP515(m, x, ...) m(x) IDENTITY(MAP514(m, __VA_ARGS__))
#define MAP516(m, x, ...) m(x) IDENTITY(MAP515(m, __VA_ARGS__))
#define MAP517(m, x, ...) m(x) IDENTITY(MAP516(m, __VA_ARGS__))
#define MAP518(m, x, ...) m(x) IDENTITY(MAP517(m, __VA_ARGS__))
#define MAP519(m, x, ...) m(x) IDENTITY(MAP518(m, __VA_ARGS__))
#define MAP520(m, x, ...) m(x) IDENTITY(MAP519(m, __VA_ARGS__))
#define MAP521(m, x, ...) m(x) IDENTITY(MAP520(m, __VA_ARGS__))
#define MAP522(m, x, ...) m(x) IDENTITY(MAP521(m, __VA_ARGS__))
#define MAP523(m, x, ...) m(x) IDENTITY(MAP522(m, __VA_ARGS__))
#define MAP524(m, x, ...) m(x) IDENTITY(MAP523(m, __VA_ARGS__))
#define MAP525(m, x, ...) m(x) IDENTITY(MAP524(m, __VA_ARGS__))
#define MAP526(m, x, ...) m(x) IDENTITY(MAP525(m, __VA_ARGS__))
#define MAP527(m, x, ...) m(x) IDENTITY(MAP526(m, __VA_ARGS__))
#define MAP528(m, x, ...) m(x) IDENTITY(MAP527(m, __VA_ARGS__))
#define MAP529(m, x, ...) m(x) IDENTITY(MAP528(m, __VA_ARGS__))
#define MAP530(m, x, ...) m(x) IDENTITY(MAP529(m, __VA_ARGS__))
#define MAP531(m, x, ...) m(x) IDENTITY(MAP530(m, __VA_ARGS__))
#define MAP532(m, x, ...) m(x) IDENTITY(MAP531(m, __VA_ARGS__))
#define MAP533(m, x, ...) m(x) IDENTITY(MAP532(m, __VA_ARGS__))
#define MAP534(m, x, ...) m(x) IDENTITY(MAP533(m, __VA_ARGS__))
#define MAP535(m, x, ...) m(x) IDENTITY(MAP534(m, __VA_ARGS__))
#define MAP536(m, x, ...) m(x) IDENTITY(MAP535(m, __VA_ARGS__))
#define MAP537(m, x, ...) m(x) IDENTITY(MAP536(m, __VA_ARGS__))
#define MAP538(m, x, ...) m(x) IDENTITY(MAP537(m, __VA_ARGS__))
#define MAP539(m, x, ...) m(x) IDENTITY(MAP538(m, __VA_ARGS__))
#define MAP540(m, x, ...) m(x) IDENTITY(MAP539(m, __VA_ARGS__))
#define MAP541(m, x, ...) m(x) IDENTITY(MAP540(m, __VA_ARGS__))
#define MAP542(m, x, ...) m(x) IDENTITY(MAP541(m, __VA_ARGS__))
#define MAP543(m, x, ...) m(x) IDENTITY(MAP542(m, __VA_ARGS__))
#define MAP544(m, x, ...) m(x) IDENTITY(MAP543(m, __VA_ARGS__))
#define MAP545(m, x, ...) m(x) IDENTITY(MAP544(m, __VA_ARGS__))
#define MAP546(m, x, ...) m(x) IDENTITY(MAP545(m, __VA_ARGS__))
#define MAP547(m, x, ...) m(x) IDENTITY(MAP546(m, __VA_ARGS__))
#define MAP548(m, x, ...) m(x) IDENTITY(MAP547(m, __VA_ARGS__))
#define MAP549(m, x, ...) m(x) IDENTITY(MAP548(m, __VA_ARGS__))
#define MAP550(m, x, ...) m(x) IDENTITY(MAP549(m, __VA_ARGS__))
#define MAP551(m, x, ...) m(x) IDENTITY(MAP550(m, __VA_ARGS__))
#define MAP552(m, x, ...) m(x) IDENTITY(MAP551(m, __VA_ARGS__))
#define MAP553(m, x, ...) m(x) IDENTITY(MAP552(m, __VA_ARGS__))
#define MAP554(m, x, ...) m(x) IDENTITY(MAP553(m, __VA_ARGS__))
#define MAP555(m, x, ...) m(x) IDENTITY(MAP554(m, __VA_ARGS__))
#define MAP556(m, x, ...) m(x) IDENTITY(MAP555(m, __VA_ARGS__))
#define MAP557(m, x, ...) m(x) IDENTITY(MAP556(m, __VA_ARGS__))
#define MAP558(m, x, ...) m(x) IDENTITY(MAP557(m, __VA_ARGS__))
#define MAP559(m, x, ...) m(x) IDENTITY(MAP558(m, __VA_ARGS__))
#define MAP560(m, x, ...) m(x) IDENTITY(MAP559(m, __VA_ARGS__))
#define MAP561(m, x, ...) m(x) IDENTITY(MAP560(m, __VA_ARGS__))
#define MAP562(m, x, ...) m(x) IDENTITY(MAP561(m, __VA_ARGS__))
#define MAP563(m, x, ...) m(x) IDENTITY(MAP562(m, __VA_ARGS__))
#define MAP564(m, x, ...) m(x) IDENTITY(MAP563(m, __VA_ARGS__))
#define MAP565(m, x, ...) m(x) IDENTITY(MAP564(m, __VA_ARGS__))
#define MAP566(m, x, ...) m(x) IDENTITY(MAP565(m, __VA_ARGS__))
#define MAP567(m, x, ...) m(x) IDENTITY(MAP566(m, __VA_ARGS__))
#define MAP568(m, x, ...) m(x) IDENTITY(MAP567(m, __VA_ARGS__))
#define MAP569(m, x, ...) m(x) IDENTITY(MAP568(m, __VA_ARGS__))
#define MAP570(m, x, ...) m(x) IDENTITY(MAP569(m, __VA_ARGS__))
#define MAP571(m, x, ...) m(x) IDENTITY(MAP570(m, __VA_ARGS__))
#define MAP572(m, x, ...) m(x) IDENTITY(MAP571(m, __VA_ARGS__))
#define MAP573(m, x, ...) m(x) IDENTITY(MAP572(m, __VA_ARGS__))
#define MAP574(m, x, ...) m(x) IDENTITY(MAP573(m, __VA_ARGS__))
#define MAP575(m, x, ...) m(x) IDENTITY(MAP574(m, __VA_ARGS__))
#define MAP576(m, x, ...) m(x) IDENTITY(MAP575(m, __VA_ARGS__))
#define MAP577(m, x, ...) m(x) IDENTITY(MAP576(m, __VA_ARGS__))
#define MAP578(m, x, ...) m(x) IDENTITY(MAP577(m, __VA_ARGS__))
#define MAP579(m, x, ...) m(x) IDENTITY(MAP578(m, __VA_ARGS__))
#define MAP580(m, x, ...) m(x) IDENTITY(MAP579(m, __VA_ARGS__))
#define MAP581(m, x, ...) m(x) IDENTITY(MAP580(m, __VA_ARGS__))
#define MAP582(m, x, ...) m(x) IDENTITY(MAP581(m, __VA_ARGS__))
#define MAP583(m, x, ...) m(x) IDENTITY(MAP582(m, __VA_ARGS__))
#define MAP584(m, x, ...) m(x) IDENTITY(MAP583(m, __VA_ARGS__))
#define MAP585(m, x, ...) m(x) IDENTITY(MAP584(m, __VA_ARGS__))
#define MAP586(m, x, ...) m(x) IDENTITY(MAP585(m, __VA_ARGS__))
#define MAP587(m, x, ...) m(x) IDENTITY(MAP586(m, __VA_ARGS__))
#define MAP588(m, x, ...) m(x) IDENTITY(MAP587(m, __VA_ARGS__))
#define MAP589(m, x, ...) m(x) IDENTITY(MAP588(m, __VA_ARGS__))
#define MAP590(m, x, ...) m(x) IDENTITY(MAP589(m, __VA_ARGS__))
#define MAP591(m, x, ...) m(x) IDENTITY(MAP590(m, __VA_ARGS__))
#define MAP592(m, x, ...) m(x) IDENTITY(MAP591(m, __VA_ARGS__))
#define MAP593(m, x, ...) m(x) IDENTITY(MAP592(m, __VA_ARGS__))
#define MAP594(m, x, ...) m(x) IDENTITY(MAP593(m, __VA_ARGS__))
#define MAP595(m, x, ...) m(x) IDENTITY(MAP594(m, __VA_ARGS__))
#define MAP596(m, x, ...) m(x) IDENTITY(MAP595(m, __VA_ARGS__))
#define MAP597(m, x, ...) m(x) IDENTITY(MAP596(m, __VA_ARGS__))
#define MAP598(m, x, ...) m(x) IDENTITY(MAP597(m, __VA_ARGS__))
#define MAP599(m, x, ...) m(x) IDENTITY(MAP598(m, __VA_ARGS__))
#define MAP600(m, x, ...) m(x) IDENTITY(MAP499(m, __VA_ARGS__))
#define MAP601(m, x, ...) m(x) IDENTITY(MAP600(m, __VA_ARGS__))
#define MAP602(m, x, ...) m(x) IDENTITY(MAP601(m, __VA_ARGS__))
#define MAP603(m, x, ...) m(x) IDENTITY(MAP602(m, __VA_ARGS__))
#define MAP604(m, x, ...) m(x) IDENTITY(MAP603(m, __VA_ARGS__))
#define MAP605(m, x, ...) m(x) IDENTITY(MAP604(m, __VA_ARGS__))
#define MAP606(m, x, ...) m(x) IDENTITY(MAP605(m, __VA_ARGS__))
#define MAP607(m, x, ...) m(x) IDENTITY(MAP606(m, __VA_ARGS__))
#define MAP608(m, x, ...) m(x) IDENTITY(MAP607(m, __VA_ARGS__))
#define MAP609(m, x, ...) m(x) IDENTITY(MAP608(m, __VA_ARGS__))
#define MAP610(m, x, ...) m(x) IDENTITY(MAP609(m, __VA_ARGS__))
#define MAP611(m, x, ...) m(x) IDENTITY(MAP610(m, __VA_ARGS__))
#define MAP612(m, x, ...) m(x) IDENTITY(MAP611(m, __VA_ARGS__))
#define MAP613(m, x, ...) m(x) IDENTITY(MAP612(m, __VA_ARGS__))
#define MAP614(m, x, ...) m(x) IDENTITY(MAP613(m, __VA_ARGS__))
#define MAP615(m, x, ...) m(x) IDENTITY(MAP614(m, __VA_ARGS__))
#define MAP616(m, x, ...) m(x) IDENTITY(MAP615(m, __VA_ARGS__))
#define MAP617(m, x, ...) m(x) IDENTITY(MAP616(m, __VA_ARGS__))
#define MAP618(m, x, ...) m(x) IDENTITY(MAP617(m, __VA_ARGS__))
#define MAP619(m, x, ...) m(x) IDENTITY(MAP618(m, __VA_ARGS__))
#define MAP620(m, x, ...) m(x) IDENTITY(MAP619(m, __VA_ARGS__))
#define MAP621(m, x, ...) m(x) IDENTITY(MAP620(m, __VA_ARGS__))
#define MAP622(m, x, ...) m(x) IDENTITY(MAP621(m, __VA_ARGS__))
#define MAP623(m, x, ...) m(x) IDENTITY(MAP622(m, __VA_ARGS__))
#define MAP624(m, x, ...) m(x) IDENTITY(MAP623(m, __VA_ARGS__))
#define MAP625(m, x, ...) m(x) IDENTITY(MAP624(m, __VA_ARGS__))
#define MAP626(m, x, ...) m(x) IDENTITY(MAP625(m, __VA_ARGS__))
#define MAP627(m, x, ...) m(x) IDENTITY(MAP626(m, __VA_ARGS__))
#define MAP628(m, x, ...) m(x) IDENTITY(MAP627(m, __VA_ARGS__))
#define MAP629(m, x, ...) m(x) IDENTITY(MAP628(m, __VA_ARGS__))
#define MAP630(m, x, ...) m(x) IDENTITY(MAP629(m, __VA_ARGS__))
#define MAP631(m, x, ...) m(x) IDENTITY(MAP630(m, __VA_ARGS__))
#define MAP632(m, x, ...) m(x) IDENTITY(MAP631(m, __VA_ARGS__))
#define MAP633(m, x, ...) m(x) IDENTITY(MAP632(m, __VA_ARGS__))
#define MAP634(m, x, ...) m(x) IDENTITY(MAP633(m, __VA_ARGS__))
#define MAP635(m, x, ...) m(x) IDENTITY(MAP634(m, __VA_ARGS__))
#define MAP636(m, x, ...) m(x) IDENTITY(MAP635(m, __VA_ARGS__))
#define MAP637(m, x, ...) m(x) IDENTITY(MAP636(m, __VA_ARGS__))
#define MAP638(m, x, ...) m(x) IDENTITY(MAP637(m, __VA_ARGS__))
#define MAP639(m, x, ...) m(x) IDENTITY(MAP638(m, __VA_ARGS__))
#define MAP640(m, x, ...) m(x) IDENTITY(MAP639(m, __VA_ARGS__))
#define MAP641(m, x, ...) m(x) IDENTITY(MAP640(m, __VA_ARGS__))
#define MAP642(m, x, ...) m(x) IDENTITY(MAP641(m, __VA_ARGS__))
#define MAP643(m, x, ...) m(x) IDENTITY(MAP642(m, __VA_ARGS__))
#define MAP644(m, x, ...) m(x) IDENTITY(MAP643(m, __VA_ARGS__))
#define MAP645(m, x, ...) m(x) IDENTITY(MAP644(m, __VA_ARGS__))
#define MAP646(m, x, ...) m(x) IDENTITY(MAP645(m, __VA_ARGS__))
#define MAP647(m, x, ...) m(x) IDENTITY(MAP646(m, __VA_ARGS__))
#define MAP648(m, x, ...) m(x) IDENTITY(MAP647(m, __VA_ARGS__))
#define MAP649(m, x, ...) m(x) IDENTITY(MAP648(m, __VA_ARGS__))
#define MAP650(m, x, ...) m(x) IDENTITY(MAP649(m, __VA_ARGS__))
#define MAP651(m, x, ...) m(x) IDENTITY(MAP650(m, __VA_ARGS__))
#define MAP652(m, x, ...) m(x) IDENTITY(MAP651(m, __VA_ARGS__))
#define MAP653(m, x, ...) m(x) IDENTITY(MAP652(m, __VA_ARGS__))
#define MAP654(m, x, ...) m(x) IDENTITY(MAP653(m, __VA_ARGS__))
#define MAP655(m, x, ...) m(x) IDENTITY(MAP654(m, __VA_ARGS__))
#define MAP656(m, x, ...) m(x) IDENTITY(MAP655(m, __VA_ARGS__))
#define MAP657(m, x, ...) m(x) IDENTITY(MAP656(m, __VA_ARGS__))
#define MAP658(m, x, ...) m(x) IDENTITY(MAP657(m, __VA_ARGS__))
#define MAP659(m, x, ...) m(x) IDENTITY(MAP658(m, __VA_ARGS__))
#define MAP660(m, x, ...) m(x) IDENTITY(MAP659(m, __VA_ARGS__))
#define MAP661(m, x, ...) m(x) IDENTITY(MAP660(m, __VA_ARGS__))
#define MAP662(m, x, ...) m(x) IDENTITY(MAP661(m, __VA_ARGS__))
#define MAP663(m, x, ...) m(x) IDENTITY(MAP662(m, __VA_ARGS__))
#define MAP664(m, x, ...) m(x) IDENTITY(MAP663(m, __VA_ARGS__))
#define MAP665(m, x, ...) m(x) IDENTITY(MAP664(m, __VA_ARGS__))
#define MAP666(m, x, ...) m(x) IDENTITY(MAP665(m, __VA_ARGS__))
#define MAP667(m, x, ...) m(x) IDENTITY(MAP666(m, __VA_ARGS__))
#define MAP668(m, x, ...) m(x) IDENTITY(MAP667(m, __VA_ARGS__))
#define MAP669(m, x, ...) m(x) IDENTITY(MAP668(m, __VA_ARGS__))
#define MAP670(m, x, ...) m(x) IDENTITY(MAP669(m, __VA_ARGS__))
#define MAP671(m, x, ...) m(x) IDENTITY(MAP670(m, __VA_ARGS__))
#define MAP672(m, x, ...) m(x) IDENTITY(MAP671(m, __VA_ARGS__))
#define MAP673(m, x, ...) m(x) IDENTITY(MAP672(m, __VA_ARGS__))
#define MAP674(m, x, ...) m(x) IDENTITY(MAP673(m, __VA_ARGS__))
#define MAP675(m, x, ...) m(x) IDENTITY(MAP674(m, __VA_ARGS__))
#define MAP676(m, x, ...) m(x) IDENTITY(MAP675(m, __VA_ARGS__))
#define MAP677(m, x, ...) m(x) IDENTITY(MAP676(m, __VA_ARGS__))
#define MAP678(m, x, ...) m(x) IDENTITY(MAP677(m, __VA_ARGS__))
#define MAP679(m, x, ...) m(x) IDENTITY(MAP678(m, __VA_ARGS__))
#define MAP680(m, x, ...) m(x) IDENTITY(MAP679(m, __VA_ARGS__))
#define MAP681(m, x, ...) m(x) IDENTITY(MAP680(m, __VA_ARGS__))
#define MAP682(m, x, ...) m(x) IDENTITY(MAP681(m, __VA_ARGS__))
#define MAP683(m, x, ...) m(x) IDENTITY(MAP682(m, __VA_ARGS__))
#define MAP684(m, x, ...) m(x) IDENTITY(MAP683(m, __VA_ARGS__))
#define MAP685(m, x, ...) m(x) IDENTITY(MAP684(m, __VA_ARGS__))
#define MAP686(m, x, ...) m(x) IDENTITY(MAP685(m, __VA_ARGS__))
#define MAP687(m, x, ...) m(x) IDENTITY(MAP686(m, __VA_ARGS__))
#define MAP688(m, x, ...) m(x) IDENTITY(MAP687(m, __VA_ARGS__))
#define MAP689(m, x, ...) m(x) IDENTITY(MAP688(m, __VA_ARGS__))
#define MAP690(m, x, ...) m(x) IDENTITY(MAP689(m, __VA_ARGS__))
#define MAP691(m, x, ...) m(x) IDENTITY(MAP690(m, __VA_ARGS__))
#define MAP692(m, x, ...) m(x) IDENTITY(MAP691(m, __VA_ARGS__))
#define MAP693(m, x, ...) m(x) IDENTITY(MAP692(m, __VA_ARGS__))
#define MAP694(m, x, ...) m(x) IDENTITY(MAP693(m, __VA_ARGS__))
#define MAP695(m, x, ...) m(x) IDENTITY(MAP694(m, __VA_ARGS__))
#define MAP696(m, x, ...) m(x) IDENTITY(MAP695(m, __VA_ARGS__))
#define MAP697(m, x, ...) m(x) IDENTITY(MAP696(m, __VA_ARGS__))
#define MAP698(m, x, ...) m(x) IDENTITY(MAP697(m, __VA_ARGS__))
#define MAP699(m, x, ...) m(x) IDENTITY(MAP698(m, __VA_ARGS__))
#define MAP700(m, x, ...) m(x) IDENTITY(MAP499(m, __VA_ARGS__))
#define MAP701(m, x, ...) m(x) IDENTITY(MAP700(m, __VA_ARGS__))
#define MAP702(m, x, ...) m(x) IDENTITY(MAP701(m, __VA_ARGS__))
#define MAP703(m, x, ...) m(x) IDENTITY(MAP702(m, __VA_ARGS__))
#define MAP704(m, x, ...) m(x) IDENTITY(MAP703(m, __VA_ARGS__))
#define MAP705(m, x, ...) m(x) IDENTITY(MAP704(m, __VA_ARGS__))
#define MAP706(m, x, ...) m(x) IDENTITY(MAP705(m, __VA_ARGS__))
#define MAP707(m, x, ...) m(x) IDENTITY(MAP706(m, __VA_ARGS__))
#define MAP708(m, x, ...) m(x) IDENTITY(MAP707(m, __VA_ARGS__))
#define MAP709(m, x, ...) m(x) IDENTITY(MAP708(m, __VA_ARGS__))
#define MAP710(m, x, ...) m(x) IDENTITY(MAP709(m, __VA_ARGS__))
#define MAP711(m, x, ...) m(x) IDENTITY(MAP710(m, __VA_ARGS__))
#define MAP712(m, x, ...) m(x) IDENTITY(MAP711(m, __VA_ARGS__))
#define MAP713(m, x, ...) m(x) IDENTITY(MAP712(m, __VA_ARGS__))
#define MAP714(m, x, ...) m(x) IDENTITY(MAP713(m, __VA_ARGS__))
#define MAP715(m, x, ...) m(x) IDENTITY(MAP714(m, __VA_ARGS__))
#define MAP716(m, x, ...) m(x) IDENTITY(MAP715(m, __VA_ARGS__))
#define MAP717(m, x, ...) m(x) IDENTITY(MAP716(m, __VA_ARGS__))
#define MAP718(m, x, ...) m(x) IDENTITY(MAP717(m, __VA_ARGS__))
#define MAP719(m, x, ...) m(x) IDENTITY(MAP718(m, __VA_ARGS__))
#define MAP720(m, x, ...) m(x) IDENTITY(MAP719(m, __VA_ARGS__))
#define MAP721(m, x, ...) m(x) IDENTITY(MAP720(m, __VA_ARGS__))
#define MAP722(m, x, ...) m(x) IDENTITY(MAP721(m, __VA_ARGS__))
#define MAP723(m, x, ...) m(x) IDENTITY(MAP722(m, __VA_ARGS__))
#define MAP724(m, x, ...) m(x) IDENTITY(MAP723(m, __VA_ARGS__))
#define MAP725(m, x, ...) m(x) IDENTITY(MAP724(m, __VA_ARGS__))
#define MAP726(m, x, ...) m(x) IDENTITY(MAP725(m, __VA_ARGS__))
#define MAP727(m, x, ...) m(x) IDENTITY(MAP726(m, __VA_ARGS__))
#define MAP728(m, x, ...) m(x) IDENTITY(MAP727(m, __VA_ARGS__))
#define MAP729(m, x, ...) m(x) IDENTITY(MAP728(m, __VA_ARGS__))
#define MAP730(m, x, ...) m(x) IDENTITY(MAP729(m, __VA_ARGS__))
#define MAP731(m, x, ...) m(x) IDENTITY(MAP730(m, __VA_ARGS__))
#define MAP732(m, x, ...) m(x) IDENTITY(MAP731(m, __VA_ARGS__))
#define MAP733(m, x, ...) m(x) IDENTITY(MAP732(m, __VA_ARGS__))
#define MAP734(m, x, ...) m(x) IDENTITY(MAP733(m, __VA_ARGS__))
#define MAP735(m, x, ...) m(x) IDENTITY(MAP734(m, __VA_ARGS__))
#define MAP736(m, x, ...) m(x) IDENTITY(MAP735(m, __VA_ARGS__))
#define MAP737(m, x, ...) m(x) IDENTITY(MAP736(m, __VA_ARGS__))
#define MAP738(m, x, ...) m(x) IDENTITY(MAP737(m, __VA_ARGS__))
#define MAP739(m, x, ...) m(x) IDENTITY(MAP738(m, __VA_ARGS__))
#define MAP740(m, x, ...) m(x) IDENTITY(MAP739(m, __VA_ARGS__))
#define MAP741(m, x, ...) m(x) IDENTITY(MAP740(m, __VA_ARGS__))
#define MAP742(m, x, ...) m(x) IDENTITY(MAP741(m, __VA_ARGS__))
#define MAP743(m, x, ...) m(x) IDENTITY(MAP742(m, __VA_ARGS__))
#define MAP744(m, x, ...) m(x) IDENTITY(MAP743(m, __VA_ARGS__))
#define MAP745(m, x, ...) m(x) IDENTITY(MAP744(m, __VA_ARGS__))
#define MAP746(m, x, ...) m(x) IDENTITY(MAP745(m, __VA_ARGS__))
#define MAP747(m, x, ...) m(x) IDENTITY(MAP746(m, __VA_ARGS__))
#define MAP748(m, x, ...) m(x) IDENTITY(MAP747(m, __VA_ARGS__))
#define MAP749(m, x, ...) m(x) IDENTITY(MAP748(m, __VA_ARGS__))
#define MAP750(m, x, ...) m(x) IDENTITY(MAP749(m, __VA_ARGS__))
#define MAP751(m, x, ...) m(x) IDENTITY(MAP750(m, __VA_ARGS__))
#define MAP752(m, x, ...) m(x) IDENTITY(MAP751(m, __VA_ARGS__))
#define MAP753(m, x, ...) m(x) IDENTITY(MAP752(m, __VA_ARGS__))
#define MAP754(m, x, ...) m(x) IDENTITY(MAP753(m, __VA_ARGS__))
#define MAP755(m, x, ...) m(x) IDENTITY(MAP754(m, __VA_ARGS__))
#define MAP756(m, x, ...) m(x) IDENTITY(MAP755(m, __VA_ARGS__))
#define MAP757(m, x, ...) m(x) IDENTITY(MAP756(m, __VA_ARGS__))
#define MAP758(m, x, ...) m(x) IDENTITY(MAP757(m, __VA_ARGS__))
#define MAP759(m, x, ...) m(x) IDENTITY(MAP758(m, __VA_ARGS__))
#define MAP760(m, x, ...) m(x) IDENTITY(MAP759(m, __VA_ARGS__))
#define MAP761(m, x, ...) m(x) IDENTITY(MAP760(m, __VA_ARGS__))
#define MAP762(m, x, ...) m(x) IDENTITY(MAP761(m, __VA_ARGS__))
#define MAP763(m, x, ...) m(x) IDENTITY(MAP762(m, __VA_ARGS__))
#define MAP764(m, x, ...) m(x) IDENTITY(MAP763(m, __VA_ARGS__))
#define MAP765(m, x, ...) m(x) IDENTITY(MAP764(m, __VA_ARGS__))
#define MAP766(m, x, ...) m(x) IDENTITY(MAP765(m, __VA_ARGS__))
#define MAP767(m, x, ...) m(x) IDENTITY(MAP766(m, __VA_ARGS__))
#define MAP768(m, x, ...) m(x) IDENTITY(MAP767(m, __VA_ARGS__))


#define EVALUATE_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
						_11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
						_21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
						_31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
						_41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
						_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
						_61, _62, _63, _64, _65, _66, _67, _68, _69, _70, \
						_71, _72, _73, _74, _75, _76, _77, _78, _79, _80, \
						_81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
						_91, _92, _93, _94, _95, _96, _97, _98, _99, _100, \
						_101, _102, _103, _104, _105, _106, _107, _108, _109, _110, \
						_111, _112, _113, _114, _115, _116, _117, _118, _119, _120, \
						_121, _122, _123, _124, _125, _126, _127, _128, _129, _130, \
						_131, _132, _133, _134, _135, _136, _137, _138, _139, _140, \
						_141, _142, _143, _144, _145, _146, _147, _148, _149, _150, \
						_151, _152, _153, _154, _155, _156, _157, _158, _159, _160, \
						_161, _162, _163, _164, _165, _166, _167, _168, _169, _170, \
						_171, _172, _173, _174, _175, _176, _177, _178, _179, _180, \
						_181, _182, _183, _184, _185, _186, _187, _188, _189, _190, \
						_191, _192, _193, _194, _195, _196, _197, _198, _199, _200, \
						_201, _202, _203, _204, _205, _206, _207, _208, _209, _210, \
						_211, _212, _213, _214, _215, _216, _217, _218, _219, _220, \
						_221, _222, _223, _224, _225, _226, _227, _228, _229, _230, \
						_231, _232, _233, _234, _235, _236, _237, _238, _239, _240, \
						_241, _242, _243, _244, _245, _246, _247, _248, _249, _250, \
						_251, _252, _253, _254, _255, _256, _257, _258, _259, _260, \
						_261, _262, _263, _264, _265, _266, _267, _268, _269, _270, \
						_271, _272, _273, _274, _275, _276, _277, _278, _279, _280, \
						_281, _282, _283, _284, _285, _286, _287, _288, _289, _290, \
						_291, _292, _293, _294, _295, _296, _297, _298, _299, _300, \
						_301, _302, _303, _304, _305, _306, _307, _308, _309, _310, \
						_311, _312, _313, _314, _315, _316, _317, _318, _319, _320, \
						_321, _322, _323, _324, _325, _326, _327, _328, _329, _330, \
						_331, _332, _333, _334, _335, _336, _337, _338, _339, _340, \
						_341, _342, _343, _344, _345, _346, _347, _348, _349, _350, \
						_351, _352, _353, _354, _355, _356, _357, _358, _359, _360, \
						_361, _362, _363, _364, _365, _366, _367, _368, _369, _370, \
						_371, _372, _373, _374, _375, _376, _377, _378, _379, _380, \
						_381, _382, _383, _384, _385, _386, _387, _388, _389, _390, \
						_391, _392, _393, _394, _395, _396, _397, _398, _399, _400, \
						_401, _402, _403, _404, _405, _406, _407, _408, _409, _410, \
						_411, _412, _413, _414, _415, _416, _417, _418, _419, _420, \
						_421, _422, _423, _424, _425, _426, _427, _428, _429, _430, \
						_431, _432, _433, _434, _435, _436, _437, _438, _439, _440, \
						_441, _442, _443, _444, _445, _446, _447, _448, _449, _450, \
						_451, _452, _453, _454, _455, _456, _457, _458, _459, _460, \
						_461, _462, _463, _464, _465, _466, _467, _468, _469, _470, \
						_471, _472, _473, _474, _475, _476, _477, _478, _479, _480, \
						_481, _482, _483, _484, _485, _486, _487, _488, _489, _490, \
						_491, _492, _493, _494, _495, _496, _497, _498, _499, _500, \
						_501, _502, _503, _504, _505, _506, _507, _508, _509, _510, \
						_511, _512, _513, _514, _515, _516, _517, _518, _519, _520, \
						_521, _522, _523, _524, _525, _526, _527, _528, _529, _530, \
						_531, _532, _533, _534, _535, _536, _537, _538, _539, _540, \
						_541, _542, _543, _544, _545, _546, _547, _548, _549, _550, \
						_551, _552, _553, _554, _555, _556, _557, _558, _559, _560, \
						_561, _562, _563, _564, _565, _566, _567, _568, _569, _570, \
						_571, _572, _573, _574, _575, _576, _577, _578, _579, _580, \
						_581, _582, _583, _584, _585, _586, _587, _588, _589, _590, \
						_591, _592, _593, _594, _595, _596, _597, _598, _599, _600, \
						_601, _602, _603, _604, _605, _606, _607, _608, _609, _610, \
						_611, _612, _613, _614, _615, _616, _617, _618, _619, _620, \
						_621, _622, _623, _624, _625, _626, _627, _628, _629, _630, \
						_631, _632, _633, _634, _635, _636, _637, _638, _639, _640, \
						_641, _642, _643, _644, _645, _646, _647, _648, _649, _650, \
						_651, _652, _653, _654, _655, _656, _657, _658, _659, _660, \
						_661, _662, _663, _664, _665, _666, _667, _668, _669, _670, \
						_671, _672, _673, _674, _675, _676, _677, _678, _679, _680, \
						_681, _682, _683, _684, _685, _686, _687, _688, _689, _690, \
						_691, _692, _693, _694, _695, _696, _697, _698, _699, _700, \
						_701, _702, _703, _704, _705, _706, _707, _708, _709, _710, \
						_711, _712, _713, _714, _715, _716, _717, _718, _719, _720, \
						_721, _722, _723, _724, _725, _726, _727, _728, _729, _730, \
						_731, _732, _733, _734, _735, _736, _737, _738, _739, _740, \
						_741, _742, _743, _744, _745, _746, _747, _748, _749, _750, \
						_751, _752, _753, _754, _755, _756, _757, _758, _759, _760, \
						_761, _762, _763, _764, _765, _766, _767, _768, \
						count, ...) count

#define COUNT(...) \
    IDENTITY(EVALUATE_COUNT(__VA_ARGS__, 768, 767, 766, 765, 764, 763, 762, 761, \
										 760, 759, 758, 757, 756, 755, 754, 753, 752, 751, \
										 750, 749, 748, 747, 746, 745, 744, 743, 742, 741, \
										 740, 739, 738, 737, 736, 735, 734, 733, 732, 731, \
										 730, 729, 728, 727, 726, 725, 724, 723, 722, 721, \
										 720, 719, 718, 717, 716, 715, 714, 713, 712, 711, \
										 710, 709, 708, 707, 706, 705, 704, 703, 702, 701, \
										 700, 699, 698, 697, 696, 695, 694, 693, 692, 691, \
										 690, 689, 688, 687, 686, 685, 684, 683, 682, 681, \
										 680, 679, 678, 677, 676, 675, 674, 673, 672, 671, \
										 670, 669, 668, 667, 666, 665, 664, 663, 662, 661, \
										 660, 659, 658, 657, 656, 655, 654, 653, 652, 651, \
										 650, 649, 648, 647, 646, 645, 644, 643, 642, 641, \
										 640, 639, 638, 637, 636, 635, 634, 633, 632, 631, \
										 630, 629, 628, 627, 626, 625, 624, 623, 622, 621, \
										 620, 619, 618, 617, 616, 615, 614, 613, 612, 611, \
										 610, 609, 608, 607, 606, 605, 604, 603, 602, 601, \
										 600, 599, 598, 597, 596, 595, 594, 593, 592, 591, \
										 590, 589, 588, 587, 586, 585, 584, 583, 582, 581, \
										 580, 579, 578, 577, 576, 575, 574, 573, 572, 571, \
										 570, 569, 568, 567, 566, 565, 564, 563, 562, 561, \
										 560, 559, 558, 557, 556, 555, 554, 553, 552, 551, \
										 550, 549, 548, 547, 546, 545, 544, 543, 542, 541, \
										 540, 539, 538, 537, 536, 535, 534, 533, 532, 531, \
										 530, 529, 528, 527, 526, 525, 524, 523, 522, 521, \
										 520, 519, 518, 517, 516, 515, 514, 513, 512, 511, \
										 510, 509, 508, 507, 506, 505, 504, 503, 502, 501, \
										 500, 499, 498, 497, 496, 495, 494, 493, 492, 491, \
										 490, 489, 488, 487, 486, 485, 484, 483, 482, 481, \
										 480, 479, 478, 477, 476, 475, 474, 473, 472, 471, \
										 470, 469, 468, 467, 466, 465, 464, 463, 462, 461, \
										 460, 459, 458, 457, 456, 455, 454, 453, 452, 451, \
										 450, 449, 448, 447, 446, 445, 444, 443, 442, 441, \
										 440, 439, 438, 437, 436, 435, 434, 433, 432, 431, \
										 430, 429, 428, 427, 426, 425, 424, 423, 422, 421, \
										 420, 419, 418, 417, 416, 415, 414, 413, 412, 411, \
										 410, 409, 408, 407, 406, 405, 404, 403, 402, 401, \
										 400, 399, 398, 397, 396, 395, 394, 393, 392, 391, \
										 390, 389, 388, 387, 386, 385, 384, 383, 382, 381, \
										 380, 379, 378, 377, 376, 375, 374, 373, 372, 371, \
										 370, 369, 368, 367, 366, 365, 364, 363, 362, 361, \
										 360, 359, 358, 357, 356, 355, 354, 353, 352, 351, \
										 350, 349, 348, 347, 346, 345, 344, 343, 342, 341, \
										 340, 339, 338, 337, 336, 335, 334, 333, 332, 331, \
										 330, 329, 328, 327, 326, 325, 324, 323, 322, 321, \
										 320, 319, 318, 317, 316, 315, 314, 313, 312, 311, \
										 310, 309, 308, 307, 306, 305, 304, 303, 302, 301, \
										 300, 299, 298, 297, 296, 295, 294, 293, 292, 291, \
										 290, 289, 288, 287, 286, 285, 284, 283, 282, 281, \
										 280, 279, 278, 277, 276, 275, 274, 273, 272, 271, \
										 270, 269, 268, 267, 266, 265, 264, 263, 262, 261, \
										 260, 259, 258, 257, 256, 255, 254, 253, 252, 251, \
										 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, \
										 240, 239, 238, 237, 236, 235, 234, 233, 232, 231, \
										 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, \
										 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, \
										 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, \
										 200, 199, 198, 197, 196, 195, 194, 193, 192, 191, \
										 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, \
										 180, 179, 178, 177, 176, 175, 174, 173, 172, 171, \
										 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, \
										 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, \
										 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, \
										 140, 139, 138, 137, 136, 135, 134, 133, 132, 131, \
										 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, \
										 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, \
										 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, \
										 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, \
										 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, \
										 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, \
										 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, \
										 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, \
										 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, \
										 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, \
										 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, \
										 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
										 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))

#define IGNORE_ASSIGN_SINGLE(expression) (ignore_assign)expression,
#define IGNORE_ASSIGN(...) IDENTITY(MAP(IGNORE_ASSIGN_SINGLE, __VA_ARGS__))

#define STRINGIZE_SINGLE(expression) #expression,
#define STRINGIZE(...) IDENTITY(MAP(STRINGIZE_SINGLE, __VA_ARGS__))


#define ENUM(EnumName, ...)                                            \
struct EnumName {                                                      \
    enum _enumerated { __VA_ARGS__, Invalid = 0xFFFE };                \
                                                                       \
    _enumerated     _value;                                            \
                                                                       \
    EnumName() : _value(_enumerated::Invalid) {  }					   \
	EnumName(_enumerated value) : _value(value) { }                    \
	EnumName(const int value) 									       \
	{																   \
		if (_is_valid(value)) {										   \
			_value = static_cast<EnumName::_enumerated>(value);		   \
		} else { 													   \
			_value = _enumerated::Invalid;							   \
		}															   \
	} 																   \
    operator _enumerated() const { return _value; }                    \
	int operator =( const int _val)						   			   \
	{																   \
		if (_is_valid(_val)) {										   \
	 		_value = static_cast<EnumName::_enumerated>(_val);		   \
		} else {													   \
			_value = _enumerated::Invalid;							   \
		}															   \
		return _value; 												   \
    } 																   \
    static const bool _is_valid(int _var)                              \
	{ 																   \
		for (size_t index = 0; index < _count; ++index) { 			   \
			if (_values()[index] == _var) { 						   \
				return true;										   \
			}				 										   \
		} 															   \
		return false; 												   \
	} 																   \
    const char* _to_string() const                                     \
    {                                                                  \
        for (size_t index = 0; index < _count; ++index) {              \
            if (_values()[index] == _value) {                          \
                return _names()[index];                                \
			} 														   \
        }                                                              \
        return NULL;                                                   \
    }                                                                  \
    const char *_to_full_string() const                                \
    {                                                                  \
        static std::string val(#EnumName);                             \
        val = #EnumName;											   \
		val.append(".").append(_to_string());                          \
        return val.c_str();                                            \
    }                                                                  \
                                                                       \
    static const size_t _count = IDENTITY(COUNT(__VA_ARGS__, Invalid = 0xFFFE )); \
                                                                       \
    static const int* _values()                                        \
    {                                                                  \
        static const int values[] =                                    \
            { IDENTITY(IGNORE_ASSIGN(__VA_ARGS__, Invalid = 0xFFFE)) };\
        return values;                                                 \
    }                                                                  \
                                                                       \
    static const char* const* _names()                                 \
    {                                                                  \
        static const char* const    raw_names[] =                      \
            { IDENTITY(STRINGIZE(__VA_ARGS__, Invalid = 0xFFFE )) };   \
                                                                       \
        static char*                processed_names[_count];           \
        static bool                 initialized = false;               \
                                                                       \
        if (!initialized) {                                            \
            for (size_t index = 0; index < _count; ++index) {          \
                size_t length =                                        \
                    std::strcspn(raw_names[index], " =\t\n\r");        \
                                                                       \
                processed_names[index] = new char[length + 1];         \
					strncpy(                                           \
                    processed_names[index], raw_names[index], length); \
                processed_names[index][length] = '\0';                 \
            }                                                          \
        }                                                              \
                                                                       \
        return processed_names;                                        \
    }                                                                  \
};

#endif