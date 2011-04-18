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

#include "kprintf.h"
#include "cpu.h"
#include "im.h"
#include "apic.h"
#include "pic.h"
#include "debug.h"
#include "syscall.h"

#define IDT_GATE_COUNT 256
#define IDT_GATE_TYPE_INT 0xE
#define IDT_GATE_TYPE_TRAP 0xF

typedef struct {
	/// Die untersten 16 Bits der Handler-Adresse
	uint16_t offset_low;
	/// Code-Segment-Selektor
	uint16_t segment;

	/// ID des Stack-Table-Eintrages
	uint8_t interrupt_stack_table : 3;
	/// Reserviert
	uint8_t : 5;
	/// Typ des Gates
	uint8_t type : 4;
	/// Reserviert
	uint8_t : 1;
	/// Noetiges Privileg-Level um den Handler aufrufen zu duerfen
	uint8_t privilege_level : 2;
	/// Bestimmt, ob das Gate praesent
	bool present : 1;

	/// Die naechsten paar Bits der Adresse
	uint16_t offset_mid;
	uint32_t offset_high;

	/// Reserviert
	uint32_t : 32;

} __attribute__((packed)) idt_gate_t;

// Die ganze IDT (Achtung wird auch aus dem SMP-Trampoline-Code geladen!)
idt_gate_t idt[IDT_GATE_COUNT];
bool use_apic;

extern void increase_user_stack_size(pm_thread_t* task_ptr, int pages);
static void idt_set_gate(size_t interrupt, vaddr_t handler, uint8_t privilege_level);

// Die ASM-Stubs
extern void im_int_stub_0();
extern void im_int_stub_1();
extern void im_int_stub_2();
extern void im_int_stub_3();
extern void im_int_stub_4();
extern void im_int_stub_5();
extern void im_int_stub_6();
extern void im_int_stub_7();
extern void im_int_stub_8();
extern void im_int_stub_9();
extern void im_int_stub_10();
extern void im_int_stub_11();
extern void im_int_stub_12();
extern void im_int_stub_13();
extern void im_int_stub_14();
extern void im_int_stub_15();
extern void im_int_stub_16();
extern void im_int_stub_17();
extern void im_int_stub_18();
extern void im_int_stub_19();
extern void im_int_stub_20();
extern void im_int_stub_21();
extern void im_int_stub_22();
extern void im_int_stub_23();
extern void im_int_stub_24();
extern void im_int_stub_25();
extern void im_int_stub_26();
extern void im_int_stub_27();
extern void im_int_stub_28();
extern void im_int_stub_29();
extern void im_int_stub_30();
extern void im_int_stub_31();
extern void im_int_stub_32();
extern void im_int_stub_33();
extern void im_int_stub_34();
extern void im_int_stub_35();
extern void im_int_stub_36();
extern void im_int_stub_37();
extern void im_int_stub_38();
extern void im_int_stub_39();
extern void im_int_stub_40();
extern void im_int_stub_41();
extern void im_int_stub_42();
extern void im_int_stub_43();
extern void im_int_stub_44();
extern void im_int_stub_45();
extern void im_int_stub_46();
extern void im_int_stub_47();
extern void im_int_stub_48();
extern void im_int_stub_49();
extern void im_int_stub_50();
extern void im_int_stub_51();
extern void im_int_stub_52();
extern void im_int_stub_53();
extern void im_int_stub_54();
extern void im_int_stub_55();
extern void im_int_stub_56();
extern void im_int_stub_57();
extern void im_int_stub_58();
extern void im_int_stub_59();
extern void im_int_stub_60();
extern void im_int_stub_61();
extern void im_int_stub_62();
extern void im_int_stub_63();
extern void im_int_stub_64();
extern void im_int_stub_65();
extern void im_int_stub_66();
extern void im_int_stub_67();
extern void im_int_stub_68();
extern void im_int_stub_69();
extern void im_int_stub_70();
extern void im_int_stub_71();
extern void im_int_stub_72();
extern void im_int_stub_73();
extern void im_int_stub_74();
extern void im_int_stub_75();
extern void im_int_stub_76();
extern void im_int_stub_77();
extern void im_int_stub_78();
extern void im_int_stub_79();
extern void im_int_stub_80();
extern void im_int_stub_81();
extern void im_int_stub_82();
extern void im_int_stub_83();
extern void im_int_stub_84();
extern void im_int_stub_85();
extern void im_int_stub_86();
extern void im_int_stub_87();
extern void im_int_stub_88();
extern void im_int_stub_89();
extern void im_int_stub_90();
extern void im_int_stub_91();
extern void im_int_stub_92();
extern void im_int_stub_93();
extern void im_int_stub_94();
extern void im_int_stub_95();
extern void im_int_stub_96();
extern void im_int_stub_97();
extern void im_int_stub_98();
extern void im_int_stub_99();
extern void im_int_stub_100();
extern void im_int_stub_101();
extern void im_int_stub_102();
extern void im_int_stub_103();
extern void im_int_stub_104();
extern void im_int_stub_105();
extern void im_int_stub_106();
extern void im_int_stub_107();
extern void im_int_stub_108();
extern void im_int_stub_109();
extern void im_int_stub_110();
extern void im_int_stub_111();
extern void im_int_stub_112();
extern void im_int_stub_113();
extern void im_int_stub_114();
extern void im_int_stub_115();
extern void im_int_stub_116();
extern void im_int_stub_117();
extern void im_int_stub_118();
extern void im_int_stub_119();
extern void im_int_stub_120();
extern void im_int_stub_121();
extern void im_int_stub_122();
extern void im_int_stub_123();
extern void im_int_stub_124();
extern void im_int_stub_125();
extern void im_int_stub_126();
extern void im_int_stub_127();
extern void im_int_stub_128();
extern void im_int_stub_129();
extern void im_int_stub_130();
extern void im_int_stub_131();
extern void im_int_stub_132();
extern void im_int_stub_133();
extern void im_int_stub_134();
extern void im_int_stub_135();
extern void im_int_stub_136();
extern void im_int_stub_137();
extern void im_int_stub_138();
extern void im_int_stub_139();
extern void im_int_stub_140();
extern void im_int_stub_141();
extern void im_int_stub_142();
extern void im_int_stub_143();
extern void im_int_stub_144();
extern void im_int_stub_145();
extern void im_int_stub_146();
extern void im_int_stub_147();
extern void im_int_stub_148();
extern void im_int_stub_149();
extern void im_int_stub_150();
extern void im_int_stub_151();
extern void im_int_stub_152();
extern void im_int_stub_153();
extern void im_int_stub_154();
extern void im_int_stub_155();
extern void im_int_stub_156();
extern void im_int_stub_157();
extern void im_int_stub_158();
extern void im_int_stub_159();
extern void im_int_stub_160();
extern void im_int_stub_161();
extern void im_int_stub_162();
extern void im_int_stub_163();
extern void im_int_stub_164();
extern void im_int_stub_165();
extern void im_int_stub_166();
extern void im_int_stub_167();
extern void im_int_stub_168();
extern void im_int_stub_169();
extern void im_int_stub_170();
extern void im_int_stub_171();
extern void im_int_stub_172();
extern void im_int_stub_173();
extern void im_int_stub_174();
extern void im_int_stub_175();
extern void im_int_stub_176();
extern void im_int_stub_177();
extern void im_int_stub_178();
extern void im_int_stub_179();
extern void im_int_stub_180();
extern void im_int_stub_181();
extern void im_int_stub_182();
extern void im_int_stub_183();
extern void im_int_stub_184();
extern void im_int_stub_185();
extern void im_int_stub_186();
extern void im_int_stub_187();
extern void im_int_stub_188();
extern void im_int_stub_189();
extern void im_int_stub_190();
extern void im_int_stub_191();
extern void im_int_stub_192();
extern void im_int_stub_193();
extern void im_int_stub_194();
extern void im_int_stub_195();
extern void im_int_stub_196();
extern void im_int_stub_197();
extern void im_int_stub_198();
extern void im_int_stub_199();
extern void im_int_stub_200();
extern void im_int_stub_201();
extern void im_int_stub_202();
extern void im_int_stub_203();
extern void im_int_stub_204();
extern void im_int_stub_205();
extern void im_int_stub_206();
extern void im_int_stub_207();
extern void im_int_stub_208();
extern void im_int_stub_209();
extern void im_int_stub_210();
extern void im_int_stub_211();
extern void im_int_stub_212();
extern void im_int_stub_213();
extern void im_int_stub_214();
extern void im_int_stub_215();
extern void im_int_stub_216();
extern void im_int_stub_217();
extern void im_int_stub_218();
extern void im_int_stub_219();
extern void im_int_stub_220();
extern void im_int_stub_221();
extern void im_int_stub_222();
extern void im_int_stub_223();
extern void im_int_stub_224();
extern void im_int_stub_225();
extern void im_int_stub_226();
extern void im_int_stub_227();
extern void im_int_stub_228();
extern void im_int_stub_229();
extern void im_int_stub_230();
extern void im_int_stub_231();
extern void im_int_stub_232();
extern void im_int_stub_233();
extern void im_int_stub_234();
extern void im_int_stub_235();
extern void im_int_stub_236();
extern void im_int_stub_237();
extern void im_int_stub_238();
extern void im_int_stub_239();
extern void im_int_stub_240();
extern void im_int_stub_241();
extern void im_int_stub_242();
extern void im_int_stub_243();
extern void im_int_stub_244();
extern void im_int_stub_245();
extern void im_int_stub_246();
extern void im_int_stub_247();
extern void im_int_stub_248();
extern void im_int_stub_249();
extern void im_int_stub_250();
extern void im_int_stub_251();
extern void im_int_stub_252();
extern void im_int_stub_253();
extern void im_int_stub_254();
extern void im_int_stub_255();

/**
 * Die IDT-Eintraege vorbereiten
 */
void im_init()
{
    idt_set_gate(0, &im_int_stub_0, 0);
    idt_set_gate(1, &im_int_stub_1, 0);
    idt_set_gate(2, &im_int_stub_2, 0);
    idt_set_gate(3, &im_int_stub_3, 0);
    idt_set_gate(4, &im_int_stub_4, 0);
    idt_set_gate(5, &im_int_stub_5, 0);
    idt_set_gate(6, &im_int_stub_6, 0);
    idt_set_gate(7, &im_int_stub_7, 0);
    idt_set_gate(8, &im_int_stub_8, 0);
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
    idt_set_gate(255, &im_int_stub_255, 0);

    use_apic = true;
}

/**
 * Die IDT laden und Interrupts aktivieren.
 */
void im_enable()
{
    // Register zum laden der IDT vorbereiten
    struct {
        uint16_t size;
        uint64_t base;
    }  __attribute__((packed)) idt_ptr = {
        .size  = IDT_GATE_COUNT * sizeof(idt_gate_t) - 1,
        .base  = (uint64_t)idt,
    };

    asm("lidt %0" : : "m" (idt_ptr));
}

/**
 * Ein Gate in der IDT setzen
 *
 * @param interrupt Interrupt-Nummer
 * @param handler Pointer auf die Handerfunktion.
 * @param privilege_level Bestimmt ob der Interrup fuer den Kernel- (0) oder
 *          fuer den User-Ring (3) bestimmt ist.
 */
static void idt_set_gate(size_t interrupt, vaddr_t handler, uint8_t privilege_level)
{
	// Handler-Adresse
	uintptr_t uint_handler = (uintptr_t) handler;
	idt[interrupt].offset_low = uint_handler & 0xFFFF;
	idt[interrupt].offset_mid = (uint_handler >> 16) & 0xFFFF;
	idt[interrupt].offset_high = uint_handler >> 32;
	
	// TODO: Segment gehoert hier nicht hartkodiert hin
	idt[interrupt].segment = 0x8;
	idt[interrupt].type = IDT_GATE_TYPE_TRAP;
	idt[interrupt].present = true;
	idt[interrupt].privilege_level = privilege_level;
}

/**
 * Muss vom plattformunabhaengigen Handler aufgerufen werden.
 *
 * @param interrupt Interruptnummer
 */
void im_end_of_interrupt(uint8_t interrupt)
{
    // Nur mit dem traditionellen PIC
    if (use_apic == false) {
        if ((interrupt >= IM_IRQ_BASE) && (interrupt <= IM_IRQ_BASE + 16)) {
            pic_eoi(interrupt - IM_IRQ_BASE);
        }
    } else {
        // TODO: EOI an APIC
    }
}


/**
 * Verarbeitet eine CPU-Exception
 */
void handle_exception(interrupt_stack_frame_t* isf, uint8_t int_num)
{
    uintptr_t cr2;
    asm("mov %%cr2, %0" : "=r" (cr2));

//    kprintf("user_stack_bottom = %x\n",
//        current_thread->user_stack_bottom);

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
        if ((cr2 <= isf->rsp + 0x800) && (cr2 >= isf->rsp - 0x20)
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
    kprintf("[CPU%d] Exception %d; Error Code = 0x%lx\n",
        cpu_get_current()->id, int_num, isf->error_code);

    kprintf("PID %d: %s\n",
        current_process->pid, current_process->cmdline);
    cpu_dump(isf);
    kprintf("\033[0;40m");

    // Ein Stacktrace dürfen wir nur ausgeben, wenn kein Pagefault wegen
    // dem Stack aufgetreten ist!
    if (!((cr2 < isf->rsp + 800) && (cr2 >= isf->rsp -0x20))) {
        stack_backtrace(isf->rbp, isf->rip);
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

