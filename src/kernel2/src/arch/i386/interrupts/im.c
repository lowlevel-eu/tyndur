/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <types.h>
#include <string.h>

#include "kprintf.h"
#include "cpu.h"
#include "im.h"
#include "apic.h"
#include "pic.h"
#include "debug.h"
#include "syscall.h"

#define IDT_GATE_COUNT 256
#define IDT_GATE_TYPE_TASK 0x5
#define IDT_GATE_TYPE_INT 0xE
#define IDT_GATE_TYPE_TRAP 0xF

typedef struct {
	/// Die untersten 16 Bits der Handler-Adresse
	uint16_t offset_low;
	/// Code-Segment-Selektor
	uint16_t segment;

	/// Reserviert
	uint8_t : 8;
	/// Typ des Gates
	uint8_t type : 4;
	/// Reserviert
	uint8_t : 1;
	/// Noetiges Privileg-Level um den Handler aufrufen zu duerfen
	uint8_t privilege_level : 2;
	/// Bestimmt, ob das Gate praesent
	bool present : 1;

	/// Die naechsten paar Bits der Adresse
	uint16_t offset_high;
} __attribute__((packed)) idt_gate_t;

// Die ganze IDT (Achtung wird auch aus dem SMP-Trampoline-Code geladen!)
idt_gate_t idt[IDT_GATE_COUNT] __attribute__((section(".idt_section")));
bool use_apic;

static void idt_set_gate(size_t interrupt, vaddr_t handler,
    uint8_t privilege_level);
static void idt_set_task_gate(size_t interrupt, uint16_t segment,
    uint8_t privilege_level);

// Die ASM-Stubs
extern void im_int_stub_0(void);
extern void im_int_stub_1(void);
extern void im_int_stub_2(void);
extern void im_int_stub_3(void);
extern void im_int_stub_4(void);
extern void im_int_stub_5(void);
extern void im_int_stub_6(void);
extern void im_int_stub_7(void);
extern void im_int_stub_8(void);
extern void im_int_stub_9(void);
extern void im_int_stub_10(void);
extern void im_int_stub_11(void);
extern void im_int_stub_12(void);
extern void im_int_stub_13(void);
extern void im_int_stub_14(void);
extern void im_int_stub_15(void);
extern void im_int_stub_16(void);
extern void im_int_stub_17(void);
extern void im_int_stub_18(void);
extern void im_int_stub_19(void);
extern void im_int_stub_20(void);
extern void im_int_stub_21(void);
extern void im_int_stub_22(void);
extern void im_int_stub_23(void);
extern void im_int_stub_24(void);
extern void im_int_stub_25(void);
extern void im_int_stub_26(void);
extern void im_int_stub_27(void);
extern void im_int_stub_28(void);
extern void im_int_stub_29(void);
extern void im_int_stub_30(void);
extern void im_int_stub_31(void);
extern void im_int_stub_32(void);
extern void im_int_stub_33(void);
extern void im_int_stub_34(void);
extern void im_int_stub_35(void);
extern void im_int_stub_36(void);
extern void im_int_stub_37(void);
extern void im_int_stub_38(void);
extern void im_int_stub_39(void);
extern void im_int_stub_40(void);
extern void im_int_stub_41(void);
extern void im_int_stub_42(void);
extern void im_int_stub_43(void);
extern void im_int_stub_44(void);
extern void im_int_stub_45(void);
extern void im_int_stub_46(void);
extern void im_int_stub_47(void);
extern void im_int_stub_48(void);
extern void im_int_stub_49(void);
extern void im_int_stub_50(void);
extern void im_int_stub_51(void);
extern void im_int_stub_52(void);
extern void im_int_stub_53(void);
extern void im_int_stub_54(void);
extern void im_int_stub_55(void);
extern void im_int_stub_56(void);
extern void im_int_stub_57(void);
extern void im_int_stub_58(void);
extern void im_int_stub_59(void);
extern void im_int_stub_60(void);
extern void im_int_stub_61(void);
extern void im_int_stub_62(void);
extern void im_int_stub_63(void);
extern void im_int_stub_64(void);
extern void im_int_stub_65(void);
extern void im_int_stub_66(void);
extern void im_int_stub_67(void);
extern void im_int_stub_68(void);
extern void im_int_stub_69(void);
extern void im_int_stub_70(void);
extern void im_int_stub_71(void);
extern void im_int_stub_72(void);
extern void im_int_stub_73(void);
extern void im_int_stub_74(void);
extern void im_int_stub_75(void);
extern void im_int_stub_76(void);
extern void im_int_stub_77(void);
extern void im_int_stub_78(void);
extern void im_int_stub_79(void);
extern void im_int_stub_80(void);
extern void im_int_stub_81(void);
extern void im_int_stub_82(void);
extern void im_int_stub_83(void);
extern void im_int_stub_84(void);
extern void im_int_stub_85(void);
extern void im_int_stub_86(void);
extern void im_int_stub_87(void);
extern void im_int_stub_88(void);
extern void im_int_stub_89(void);
extern void im_int_stub_90(void);
extern void im_int_stub_91(void);
extern void im_int_stub_92(void);
extern void im_int_stub_93(void);
extern void im_int_stub_94(void);
extern void im_int_stub_95(void);
extern void im_int_stub_96(void);
extern void im_int_stub_97(void);
extern void im_int_stub_98(void);
extern void im_int_stub_99(void);
extern void im_int_stub_100(void);
extern void im_int_stub_101(void);
extern void im_int_stub_102(void);
extern void im_int_stub_103(void);
extern void im_int_stub_104(void);
extern void im_int_stub_105(void);
extern void im_int_stub_106(void);
extern void im_int_stub_107(void);
extern void im_int_stub_108(void);
extern void im_int_stub_109(void);
extern void im_int_stub_110(void);
extern void im_int_stub_111(void);
extern void im_int_stub_112(void);
extern void im_int_stub_113(void);
extern void im_int_stub_114(void);
extern void im_int_stub_115(void);
extern void im_int_stub_116(void);
extern void im_int_stub_117(void);
extern void im_int_stub_118(void);
extern void im_int_stub_119(void);
extern void im_int_stub_120(void);
extern void im_int_stub_121(void);
extern void im_int_stub_122(void);
extern void im_int_stub_123(void);
extern void im_int_stub_124(void);
extern void im_int_stub_125(void);
extern void im_int_stub_126(void);
extern void im_int_stub_127(void);
extern void im_int_stub_128(void);
extern void im_int_stub_129(void);
extern void im_int_stub_130(void);
extern void im_int_stub_131(void);
extern void im_int_stub_132(void);
extern void im_int_stub_133(void);
extern void im_int_stub_134(void);
extern void im_int_stub_135(void);
extern void im_int_stub_136(void);
extern void im_int_stub_137(void);
extern void im_int_stub_138(void);
extern void im_int_stub_139(void);
extern void im_int_stub_140(void);
extern void im_int_stub_141(void);
extern void im_int_stub_142(void);
extern void im_int_stub_143(void);
extern void im_int_stub_144(void);
extern void im_int_stub_145(void);
extern void im_int_stub_146(void);
extern void im_int_stub_147(void);
extern void im_int_stub_148(void);
extern void im_int_stub_149(void);
extern void im_int_stub_150(void);
extern void im_int_stub_151(void);
extern void im_int_stub_152(void);
extern void im_int_stub_153(void);
extern void im_int_stub_154(void);
extern void im_int_stub_155(void);
extern void im_int_stub_156(void);
extern void im_int_stub_157(void);
extern void im_int_stub_158(void);
extern void im_int_stub_159(void);
extern void im_int_stub_160(void);
extern void im_int_stub_161(void);
extern void im_int_stub_162(void);
extern void im_int_stub_163(void);
extern void im_int_stub_164(void);
extern void im_int_stub_165(void);
extern void im_int_stub_166(void);
extern void im_int_stub_167(void);
extern void im_int_stub_168(void);
extern void im_int_stub_169(void);
extern void im_int_stub_170(void);
extern void im_int_stub_171(void);
extern void im_int_stub_172(void);
extern void im_int_stub_173(void);
extern void im_int_stub_174(void);
extern void im_int_stub_175(void);
extern void im_int_stub_176(void);
extern void im_int_stub_177(void);
extern void im_int_stub_178(void);
extern void im_int_stub_179(void);
extern void im_int_stub_180(void);
extern void im_int_stub_181(void);
extern void im_int_stub_182(void);
extern void im_int_stub_183(void);
extern void im_int_stub_184(void);
extern void im_int_stub_185(void);
extern void im_int_stub_186(void);
extern void im_int_stub_187(void);
extern void im_int_stub_188(void);
extern void im_int_stub_189(void);
extern void im_int_stub_190(void);
extern void im_int_stub_191(void);
extern void im_int_stub_192(void);
extern void im_int_stub_193(void);
extern void im_int_stub_194(void);
extern void im_int_stub_195(void);
extern void im_int_stub_196(void);
extern void im_int_stub_197(void);
extern void im_int_stub_198(void);
extern void im_int_stub_199(void);
extern void im_int_stub_200(void);
extern void im_int_stub_201(void);
extern void im_int_stub_202(void);
extern void im_int_stub_203(void);
extern void im_int_stub_204(void);
extern void im_int_stub_205(void);
extern void im_int_stub_206(void);
extern void im_int_stub_207(void);
extern void im_int_stub_208(void);
extern void im_int_stub_209(void);
extern void im_int_stub_210(void);
extern void im_int_stub_211(void);
extern void im_int_stub_212(void);
extern void im_int_stub_213(void);
extern void im_int_stub_214(void);
extern void im_int_stub_215(void);
extern void im_int_stub_216(void);
extern void im_int_stub_217(void);
extern void im_int_stub_218(void);
extern void im_int_stub_219(void);
extern void im_int_stub_220(void);
extern void im_int_stub_221(void);
extern void im_int_stub_222(void);
extern void im_int_stub_223(void);
extern void im_int_stub_224(void);
extern void im_int_stub_225(void);
extern void im_int_stub_226(void);
extern void im_int_stub_227(void);
extern void im_int_stub_228(void);
extern void im_int_stub_229(void);
extern void im_int_stub_230(void);
extern void im_int_stub_231(void);
extern void im_int_stub_232(void);
extern void im_int_stub_233(void);
extern void im_int_stub_234(void);
extern void im_int_stub_235(void);
extern void im_int_stub_236(void);
extern void im_int_stub_237(void);
extern void im_int_stub_238(void);
extern void im_int_stub_239(void);
extern void im_int_stub_240(void);
extern void im_int_stub_241(void);
extern void im_int_stub_242(void);
extern void im_int_stub_243(void);
extern void im_int_stub_244(void);
extern void im_int_stub_245(void);
extern void im_int_stub_246(void);
extern void im_int_stub_247(void);
extern void im_int_stub_248(void);
extern void im_int_stub_249(void);
extern void im_int_stub_250(void);
extern void im_int_stub_251(void);
extern void im_int_stub_252(void);
extern void im_int_stub_253(void);
extern void im_int_stub_254(void);
extern void im_int_stub_255(void);

/**
 * Die IDT-Eintraege vorbereiten
 */
void im_init()
{
    memset(idt, 0, sizeof(idt));
    
    idt_set_gate(0, &im_int_stub_0, 0);
    idt_set_gate(1, &im_int_stub_1, 0);
    idt_set_gate(2, &im_int_stub_2, 0);
    idt_set_gate(3, &im_int_stub_3, 0);
    idt_set_gate(4, &im_int_stub_4, 0);
    idt_set_gate(5, &im_int_stub_5, 0);
    idt_set_gate(6, &im_int_stub_6, 0);
    idt_set_gate(7, &im_int_stub_7, 0);
    idt_set_task_gate(8, 0x30, 0);
    idt_set_gate(9, &im_int_stub_9, 0);
    idt_set_gate(10, &im_int_stub_10, 0);
    idt_set_gate(11, &im_int_stub_11, 0);
    idt_set_gate(12, &im_int_stub_12, 0);
    idt_set_gate(13, &im_int_stub_13, 0);
    idt_set_gate(14, &im_int_stub_14, 0);
    idt_set_gate(15, &im_int_stub_15, 0);
    idt_set_gate(16, &im_int_stub_16, 0);
    idt_set_gate(17, &im_int_stub_17, 0);
    idt_set_gate(18, &im_int_stub_18, 0);
    idt_set_gate(19, &im_int_stub_19, 0);
    idt_set_gate(20, &im_int_stub_20, 0);
    idt_set_gate(21, &im_int_stub_21, 0);
    idt_set_gate(22, &im_int_stub_22, 0);
    idt_set_gate(23, &im_int_stub_23, 0);
    idt_set_gate(24, &im_int_stub_24, 0);
    idt_set_gate(25, &im_int_stub_25, 0);
    idt_set_gate(26, &im_int_stub_26, 0);
    idt_set_gate(27, &im_int_stub_27, 0);
    idt_set_gate(28, &im_int_stub_28, 0);
    idt_set_gate(29, &im_int_stub_29, 0);
    idt_set_gate(30, &im_int_stub_30, 0);
    idt_set_gate(31, &im_int_stub_31, 0);
    idt_set_gate(32, &im_int_stub_32, 0);
    idt_set_gate(33, &im_int_stub_33, 0);
    idt_set_gate(34, &im_int_stub_34, 0);
    idt_set_gate(35, &im_int_stub_35, 0);
    idt_set_gate(36, &im_int_stub_36, 0);
    idt_set_gate(37, &im_int_stub_37, 0);
    idt_set_gate(38, &im_int_stub_38, 0);
    idt_set_gate(39, &im_int_stub_39, 0);
    idt_set_gate(40, &im_int_stub_40, 0);
    idt_set_gate(41, &im_int_stub_41, 0);
    idt_set_gate(42, &im_int_stub_42, 0);
    idt_set_gate(43, &im_int_stub_43, 0);
    idt_set_gate(44, &im_int_stub_44, 0);
    idt_set_gate(45, &im_int_stub_45, 0);
    idt_set_gate(46, &im_int_stub_46, 0);
    idt_set_gate(47, &im_int_stub_47, 0);
    idt_set_gate(48, &im_int_stub_48, 0);
    idt_set_gate(49, &im_int_stub_49, 0);
    idt_set_gate(50, &im_int_stub_50, 0);
    idt_set_gate(51, &im_int_stub_51, 0);
    idt_set_gate(52, &im_int_stub_52, 0);
    idt_set_gate(53, &im_int_stub_53, 0);
    idt_set_gate(54, &im_int_stub_54, 0);
    idt_set_gate(55, &im_int_stub_55, 0);
    idt_set_gate(56, &im_int_stub_56, 0);
    idt_set_gate(57, &im_int_stub_57, 0);
    idt_set_gate(58, &im_int_stub_58, 0);
    idt_set_gate(59, &im_int_stub_59, 0);
    idt_set_gate(60, &im_int_stub_60, 0);
    idt_set_gate(61, &im_int_stub_61, 0);
    idt_set_gate(62, &im_int_stub_62, 0);
    idt_set_gate(63, &im_int_stub_63, 0);
    idt_set_gate(64, &im_int_stub_64, 0);
    idt_set_gate(65, &im_int_stub_65, 0);
    idt_set_gate(66, &im_int_stub_66, 0);
    idt_set_gate(67, &im_int_stub_67, 0);
    idt_set_gate(68, &im_int_stub_68, 0);
    idt_set_gate(69, &im_int_stub_69, 0);
    idt_set_gate(70, &im_int_stub_70, 0);
    idt_set_gate(71, &im_int_stub_71, 0);
    idt_set_gate(72, &im_int_stub_72, 0);
    idt_set_gate(73, &im_int_stub_73, 0);
    idt_set_gate(74, &im_int_stub_74, 0);
    idt_set_gate(75, &im_int_stub_75, 0);
    idt_set_gate(76, &im_int_stub_76, 0);
    idt_set_gate(77, &im_int_stub_77, 0);
    idt_set_gate(78, &im_int_stub_78, 0);
    idt_set_gate(79, &im_int_stub_79, 0);
    idt_set_gate(80, &im_int_stub_80, 0);
    idt_set_gate(81, &im_int_stub_81, 0);
    idt_set_gate(82, &im_int_stub_82, 0);
    idt_set_gate(83, &im_int_stub_83, 0);
    idt_set_gate(84, &im_int_stub_84, 0);
    idt_set_gate(85, &im_int_stub_85, 0);
    idt_set_gate(86, &im_int_stub_86, 0);
    idt_set_gate(87, &im_int_stub_87, 0);
    idt_set_gate(88, &im_int_stub_88, 0);
    idt_set_gate(89, &im_int_stub_89, 0);
    idt_set_gate(90, &im_int_stub_90, 0);
    idt_set_gate(91, &im_int_stub_91, 0);
    idt_set_gate(92, &im_int_stub_92, 0);
    idt_set_gate(93, &im_int_stub_93, 0);
    idt_set_gate(94, &im_int_stub_94, 0);
    idt_set_gate(95, &im_int_stub_95, 0);
    idt_set_gate(96, &im_int_stub_96, 0);
    idt_set_gate(97, &im_int_stub_97, 0);
    idt_set_gate(98, &im_int_stub_98, 0);
    idt_set_gate(99, &im_int_stub_99, 0);
    idt_set_gate(100, &im_int_stub_100, 0);
    idt_set_gate(101, &im_int_stub_101, 0);
    idt_set_gate(102, &im_int_stub_102, 0);
    idt_set_gate(103, &im_int_stub_103, 0);
    idt_set_gate(104, &im_int_stub_104, 0);
    idt_set_gate(105, &im_int_stub_105, 0);
    idt_set_gate(106, &im_int_stub_106, 0);
    idt_set_gate(107, &im_int_stub_107, 0);
    idt_set_gate(108, &im_int_stub_108, 0);
    idt_set_gate(109, &im_int_stub_109, 0);
    idt_set_gate(110, &im_int_stub_110, 0);
    idt_set_gate(111, &im_int_stub_111, 0);
    idt_set_gate(112, &im_int_stub_112, 0);
    idt_set_gate(113, &im_int_stub_113, 0);
    idt_set_gate(114, &im_int_stub_114, 0);
    idt_set_gate(115, &im_int_stub_115, 0);
    idt_set_gate(116, &im_int_stub_116, 0);
    idt_set_gate(117, &im_int_stub_117, 0);
    idt_set_gate(118, &im_int_stub_118, 0);
    idt_set_gate(119, &im_int_stub_119, 0);
    idt_set_gate(120, &im_int_stub_120, 0);
    idt_set_gate(121, &im_int_stub_121, 0);
    idt_set_gate(122, &im_int_stub_122, 0);
    idt_set_gate(123, &im_int_stub_123, 0);
    idt_set_gate(124, &im_int_stub_124, 0);
    idt_set_gate(125, &im_int_stub_125, 0);
    idt_set_gate(126, &im_int_stub_126, 0);
    idt_set_gate(127, &im_int_stub_127, 0);
    idt_set_gate(128, &im_int_stub_128, 0);
    idt_set_gate(129, &im_int_stub_129, 0);
    idt_set_gate(130, &im_int_stub_130, 0);
    idt_set_gate(131, &im_int_stub_131, 0);
    idt_set_gate(132, &im_int_stub_132, 0);
    idt_set_gate(133, &im_int_stub_133, 0);
    idt_set_gate(134, &im_int_stub_134, 0);
    idt_set_gate(135, &im_int_stub_135, 0);
    idt_set_gate(136, &im_int_stub_136, 0);
    idt_set_gate(137, &im_int_stub_137, 0);
    idt_set_gate(138, &im_int_stub_138, 0);
    idt_set_gate(139, &im_int_stub_139, 0);
    idt_set_gate(140, &im_int_stub_140, 0);
    idt_set_gate(141, &im_int_stub_141, 0);
    idt_set_gate(142, &im_int_stub_142, 0);
    idt_set_gate(143, &im_int_stub_143, 0);
    idt_set_gate(144, &im_int_stub_144, 0);
    idt_set_gate(145, &im_int_stub_145, 0);
    idt_set_gate(146, &im_int_stub_146, 0);
    idt_set_gate(147, &im_int_stub_147, 0);
    idt_set_gate(148, &im_int_stub_148, 0);
    idt_set_gate(149, &im_int_stub_149, 0);
    idt_set_gate(150, &im_int_stub_150, 0);
    idt_set_gate(151, &im_int_stub_151, 0);
    idt_set_gate(152, &im_int_stub_152, 0);
    idt_set_gate(153, &im_int_stub_153, 0);
    idt_set_gate(154, &im_int_stub_154, 0);
    idt_set_gate(155, &im_int_stub_155, 0);
    idt_set_gate(156, &im_int_stub_156, 0);
    idt_set_gate(157, &im_int_stub_157, 0);
    idt_set_gate(158, &im_int_stub_158, 0);
    idt_set_gate(159, &im_int_stub_159, 0);
    idt_set_gate(160, &im_int_stub_160, 0);
    idt_set_gate(161, &im_int_stub_161, 0);
    idt_set_gate(162, &im_int_stub_162, 0);
    idt_set_gate(163, &im_int_stub_163, 0);
    idt_set_gate(164, &im_int_stub_164, 0);
    idt_set_gate(165, &im_int_stub_165, 0);
    idt_set_gate(166, &im_int_stub_166, 0);
    idt_set_gate(167, &im_int_stub_167, 0);
    idt_set_gate(168, &im_int_stub_168, 0);
    idt_set_gate(169, &im_int_stub_169, 0);
    idt_set_gate(170, &im_int_stub_170, 0);
    idt_set_gate(171, &im_int_stub_171, 0);
    idt_set_gate(172, &im_int_stub_172, 0);
    idt_set_gate(173, &im_int_stub_173, 0);
    idt_set_gate(174, &im_int_stub_174, 0);
    idt_set_gate(175, &im_int_stub_175, 0);
    idt_set_gate(176, &im_int_stub_176, 0);
    idt_set_gate(177, &im_int_stub_177, 0);
    idt_set_gate(178, &im_int_stub_178, 0);
    idt_set_gate(179, &im_int_stub_179, 0);
    idt_set_gate(180, &im_int_stub_180, 0);
    idt_set_gate(181, &im_int_stub_181, 0);
    idt_set_gate(182, &im_int_stub_182, 0);
    idt_set_gate(183, &im_int_stub_183, 0);
    idt_set_gate(184, &im_int_stub_184, 0);
    idt_set_gate(185, &im_int_stub_185, 0);
    idt_set_gate(186, &im_int_stub_186, 0);
    idt_set_gate(187, &im_int_stub_187, 0);
    idt_set_gate(188, &im_int_stub_188, 0);
    idt_set_gate(189, &im_int_stub_189, 0);
    idt_set_gate(190, &im_int_stub_190, 0);
    idt_set_gate(191, &im_int_stub_191, 0);
    idt_set_gate(192, &im_int_stub_192, 0);
    idt_set_gate(193, &im_int_stub_193, 0);
    idt_set_gate(194, &im_int_stub_194, 0);
    idt_set_gate(195, &im_int_stub_195, 0);
    idt_set_gate(196, &im_int_stub_196, 0);
    idt_set_gate(197, &im_int_stub_197, 0);
    idt_set_gate(198, &im_int_stub_198, 0);
    idt_set_gate(199, &im_int_stub_199, 0);
    idt_set_gate(200, &im_int_stub_200, 0);
    idt_set_gate(201, &im_int_stub_201, 0);
    idt_set_gate(202, &im_int_stub_202, 0);
    idt_set_gate(203, &im_int_stub_203, 0);
    idt_set_gate(204, &im_int_stub_204, 0);
    idt_set_gate(205, &im_int_stub_205, 0);
    idt_set_gate(206, &im_int_stub_206, 0);
    idt_set_gate(207, &im_int_stub_207, 0);
    idt_set_gate(208, &im_int_stub_208, 0);
    idt_set_gate(209, &im_int_stub_209, 0);
    idt_set_gate(210, &im_int_stub_210, 0);
    idt_set_gate(211, &im_int_stub_211, 0);
    idt_set_gate(212, &im_int_stub_212, 0);
    idt_set_gate(213, &im_int_stub_213, 0);
    idt_set_gate(214, &im_int_stub_214, 0);
    idt_set_gate(215, &im_int_stub_215, 0);
    idt_set_gate(216, &im_int_stub_216, 0);
    idt_set_gate(217, &im_int_stub_217, 0);
    idt_set_gate(218, &im_int_stub_218, 0);
    idt_set_gate(219, &im_int_stub_219, 0);
    idt_set_gate(220, &im_int_stub_220, 0);
    idt_set_gate(221, &im_int_stub_221, 0);
    idt_set_gate(222, &im_int_stub_222, 0);
    idt_set_gate(223, &im_int_stub_223, 0);
    idt_set_gate(224, &im_int_stub_224, 0);
    idt_set_gate(225, &im_int_stub_225, 0);
    idt_set_gate(226, &im_int_stub_226, 0);
    idt_set_gate(227, &im_int_stub_227, 0);
    idt_set_gate(228, &im_int_stub_228, 0);
    idt_set_gate(229, &im_int_stub_229, 0);
    idt_set_gate(230, &im_int_stub_230, 0);
    idt_set_gate(231, &im_int_stub_231, 0);
    idt_set_gate(232, &im_int_stub_232, 0);
    idt_set_gate(233, &im_int_stub_233, 0);
    idt_set_gate(234, &im_int_stub_234, 0);
    idt_set_gate(235, &im_int_stub_235, 0);
    idt_set_gate(236, &im_int_stub_236, 0);
    idt_set_gate(237, &im_int_stub_237, 0);
    idt_set_gate(238, &im_int_stub_238, 0);
    idt_set_gate(239, &im_int_stub_239, 0);
    idt_set_gate(240, &im_int_stub_240, 0);
    idt_set_gate(241, &im_int_stub_241, 0);
    idt_set_gate(242, &im_int_stub_242, 0);
    idt_set_gate(243, &im_int_stub_243, 0);
    idt_set_gate(244, &im_int_stub_244, 0);
    idt_set_gate(245, &im_int_stub_245, 0);
    idt_set_gate(246, &im_int_stub_246, 0);
    idt_set_gate(247, &im_int_stub_247, 0);
    idt_set_gate(248, &im_int_stub_248, 0);
    idt_set_gate(249, &im_int_stub_249, 0);
    idt_set_gate(250, &im_int_stub_250, 0);
    idt_set_gate(251, &im_int_stub_251, 0);
    idt_set_gate(252, &im_int_stub_252, 0);
    idt_set_gate(253, &im_int_stub_253, 0);
    idt_set_gate(254, &im_int_stub_254, 0);
    idt_set_gate(255, &im_int_stub_255, 3);

    // Wenn ein APIC vorhanden ist, wir der bentutzt.
    if (apic_available() == true) {
        apic_init();
        use_apic = true;
    } else {
        // Den traditionellen PIC benutzen
        pic_init(32);
        use_apic = false;
    }
}

/**
 * Die IDT laden.
 */
void im_init_local()
{
    // Register zum laden der IDT vorbereiten
    struct {
        uint16_t size;
        uint32_t base;
    }  __attribute__((packed)) idt_ptr = {
        .size  = sizeof(idt) - 1,
        .base  = (uint32_t)idt,
    };
    
    asm("lidt %0" : : "m" (idt_ptr));
}

/**
 * Interrupts aktivieren
 */
void im_enable()
{
    asm("sti");
}

/**
 * Interrupts dektivieren
 */
void im_disable()
{
    asm("cli");
}

/**
 * IRQ aktivieren
 */
void im_enable_irq(uint8_t irq)
{
    if (use_apic == false) {
        pic_enable_irq(irq);
    } else {
        // TODO
    }
}

/**
 * Interrupts dektivieren
 */
void im_disable_irq(uint8_t irq)
{
    if (use_apic == false) {
        pic_disable_irq(irq);
    } else {
        // TODO
    }
}

/**
 * Ein Gate in der IDT setzen
 *
 * @param interrupt Interrupt-Nummer
 * @param handler Pointer auf die Handerfunktion.
 * @param privilege_level Bestimmt ob der Interrup fuer den Kernel- (0) oder
 *          fuer den User-Ring (3) bestimmt ist.
 */
static void idt_set_gate(size_t interrupt, vaddr_t handler,
    uint8_t privilege_level)
{
	// Handler-Adresse
	uintptr_t uint_handler = (uintptr_t) handler;
	idt[interrupt].offset_low = uint_handler & 0xFFFF;
	idt[interrupt].offset_high = (uint_handler >> 16) & 0xFFFF;
	
	// TODO: Segment gehoert hier nicht hartkodiert hin
	idt[interrupt].segment = 0x8;
	idt[interrupt].type = IDT_GATE_TYPE_INT;
	idt[interrupt].present = true;
	idt[interrupt].privilege_level = 3;//privilege_level; FIXME: HACK
}

/**
 * Ein Gate in der IDT angeben (unter Angabe des Typs)
 *
 * @param interrupt Interrupt-Nummer
 * @param handler Pointer auf die Handerfunktion.
 * @param privilege_level Bestimmt ob der Interrup fuer den Kernel- (0) oder
 *          fuer den User-Ring (3) bestimmt ist.
 * @param type Typ des Gates
 */
static void idt_set_task_gate(size_t interrupt, uint16_t segment,
    uint8_t privilege_level)
{
    memset(&idt[interrupt], 0, sizeof(*idt));
    idt[interrupt].present = true;
    idt[interrupt].type = IDT_GATE_TYPE_TASK;
    idt[interrupt].segment = segment;
    idt[interrupt].privilege_level = privilege_level;
}

/**
 * TODO Beschreibung der Funktion
 * Muss vom plattformunabhaengigen Handler aufgerufen werden.
 *
 * @param interrupt Interruptnummer
 */
void im_end_of_interrupt(uint8_t interrupt)
{
    // Nur mit dem traditionellen PIC
    if (use_apic == false) {
        if ((interrupt >= IM_IRQ_BASE) && (interrupt < IM_IRQ_BASE + 16)) {
            pic_eoi(interrupt - IM_IRQ_BASE);
        }
    } else {
        // TODO: EOI an APIC
    }
}

extern void increase_user_stack_size(pm_thread_t* task_ptr, int pages);
extern int vm86_exception(interrupt_stack_frame_t* isf);

/**
 * Verarbeitet eine CPU-Exception
 */
void handle_exception(interrupt_stack_frame_t* isf, uint8_t int_num)
{
    uintptr_t cr2;
    asm("mov %%cr2, %0" : "=r" (cr2));

//    kprintf("user_stack_bottom = %x\n",
//        current_thread->user_stack_bottom);

    // Prüfen, ob ein VM86-Task die Exception ausgelöst hat
    // Falls ja lassen wir sie vom VM86-Code behandeln, wenn er kann
    if (current_thread->vm86) {
        if (vm86_exception(isf)) {
            return;
        }
    }

    if (int_num == 0x0d) {
        // Prüfen, ob der #GP möglicherweise durch eine nicht geladene
        // I/O-Bitmap verurscht worden ist
        if (cpu_get_current()->tss.io_bit_map_offset ==
                TSS_IO_BITMAP_NOT_LOADED)
        {
            io_activate_bitmap(current_process);
            return;
        }
    }

    if (int_num == 0x0e) {
        // Überprüfen ob der Pagefault durch einen Stackoverflow
        // hervorgerufen wurde. Falls ja: Stack vergrößern und weitermachen
        if ((cr2 <= isf->esp + 0x800) && (cr2 >= isf->esp - 0x20)
            && (current_thread != NULL)
            && ((void*) cr2 >= current_thread->user_stack_bottom - 0x1000000)
            && ((void*) cr2 <  current_thread->user_stack_bottom))
        {
            increase_user_stack_size(current_thread, 1);
            return;
        }
    }

    // Exception
    kprintf("\033[1;41m");
    kprintf("[CPU%d] Exception %d; Error Code = 0x%x\n",
        cpu_get_current()->id, int_num, isf->error_code);

    kprintf("PID %d: %s\n",
        current_process->pid, current_process->cmdline);
    cpu_dump(isf);
    kprintf("\033[0;40m");

    // Ein Stacktrace dürfen wir nur ausgeben, wenn kein Pagefault wegen
    // dem Stack aufgetreten ist!
    if (!((cr2 < isf->esp + 800) && (cr2 >= isf->esp -0x20))) {
        // Und für VM86 müssten wir wenigstens noch das Segment
        // berücksichtigen, aber eigentlich brauchen wir ihn da gar nicht
        if ((isf->eflags & 0x20000) == 0) {
            stack_backtrace(isf->ebp, isf->eip);
        }
    }

    // Wenn die Exception im Kernel passiert ist, geben wir auf
    if ((isf->cs & 0x03) == 0) {
        // Hier werden alle Prozessoren zum halten gebracht
        if (cpu_count > 1) {
            apic_ipi_broadcast(0xF0, true);
        }

        // Aber da das ohne APIC nicht so funktioniert, wird das noch
        // abgefangen.
        asm("cli; hlt");
    }

    // Ansonsten wird nur der aktuelle Task beendet
    syscall_pm_exit_process();
}

