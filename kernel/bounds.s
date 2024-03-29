	.file	"bounds.c"
 # GNU C11 (GCC) version 13.0.0 20220906 (experimental) (loongarch64-unknown-linux-gnu)
 #	compiled by GNU C version 11.0.1 20210324 (Red Hat 11.0.1-0), GMP version 6.2.1, MPFR version 4.1.0, MPC version 1.2.1, isl version none
 # GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
 # options passed: -G 0 -mno-check-zero-division -mabi=lp64s -march=loongarch64 -mfpu=none -mcmodel=normal -mtune=la464 -g -O0 -O2 -Os -std=gnu11 -fno-strict-aliasing -fno-common -fshort-wchar -funsigned-char -fno-asynchronous-unwind-tables -fno-delete-null-pointer-checks -fstack-protector-strong -fno-stack-clash-protection -fstack-check=no -fpie -ffreestanding -fno-stack-protector -fomit-frame-pointer -fno-strict-overflow -fconserve-stack -fno-tree-scev-cprop
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.file 0 "/home/huqingwei/repositories/kernel-travel" "kernel/bounds.c"
	.align	2
	.globl	__main
	.type	__main, @function
__main:
.LFB0 = .
	.file 1 "kernel/bounds.c"
	.loc 1 15 1 view -0
	.cfi_startproc
	.loc 1 18 1 view .LVU1
	jr	$r1		 #
	.cfi_endproc
.LFE0:
	.size	__main, .-__main
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x7d
	.2byte	0x5
	.byte	0x1
	.byte	0x8
	.4byte	.Ldebug_abbrev0
	.uleb128 0x2
	.4byte	.LASF9
	.byte	0x1d
	.4byte	.LASF0
	.4byte	.LASF1
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.4byte	.Ldebug_line0
	.uleb128 0x1
	.byte	0x4
	.byte	0x7
	.4byte	.LASF2
	.uleb128 0x1
	.byte	0x1
	.byte	0x8
	.4byte	.LASF3
	.uleb128 0x1
	.byte	0x2
	.byte	0x7
	.4byte	.LASF4
	.uleb128 0x1
	.byte	0x8
	.byte	0x7
	.4byte	.LASF5
	.uleb128 0x1
	.byte	0x2
	.byte	0x5
	.4byte	.LASF6
	.uleb128 0x1
	.byte	0x8
	.byte	0x5
	.4byte	.LASF7
	.uleb128 0x1
	.byte	0x8
	.byte	0x7
	.4byte	.LASF8
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.ascii	"int\000"
	.uleb128 0x4
	.4byte	.LASF10
	.byte	0x1
	.byte	0xe
	.byte	0x6
	.8byte	.LFB0
	.8byte	.LFE0-.LFB0
	.uleb128 0x1
	.byte	0x9c
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x1f
	.uleb128 0x1b
	.uleb128 0x1f
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7a
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x2c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.8byte	0
	.8byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF7:
	.ascii	"long long int\000"
.LASF2:
	.ascii	"unsigned int\000"
.LASF10:
	.ascii	"__main\000"
.LASF8:
	.ascii	"long unsigned int\000"
.LASF5:
	.ascii	"long long unsigned int\000"
.LASF3:
	.ascii	"unsigned char\000"
.LASF4:
	.ascii	"short unsigned int\000"
.LASF6:
	.ascii	"short int\000"
.LASF9:
	.ascii	"GNU C11 13.0.0 20220906 (experimental) -G 0 -mno-check-z"
	.ascii	"ero-division -mabi=lp64s -march=loongarch64 -mfpu=none -"
	.ascii	"mcmodel=normal -mtune=la464 -g -O0 -O2 -Os -std=gnu11 -f"
	.ascii	"no-strict-aliasing -fno-common -fshort-wchar -funsigned-"
	.ascii	"char -fno-asynchronous-unwind-tables -fno-delete-null-po"
	.ascii	"inter-checks -fstack-protector-strong -fno-stack-clash-p"
	.ascii	"rotection -fstack-check=no -fpie -ffreestanding -fno-sta"
	.ascii	"ck-protector -fomit-frame-pointer -fno-strict-overflow -"
	.ascii	"fconserve-stack -fno-tree-scev-cprop\000"
	.section	.debug_line_str,"MS",@progbits,1
.LASF1:
	.ascii	"/home/huqingwei/repositories/kernel-travel\000"
.LASF0:
	.ascii	"kernel/bounds.c\000"
	.ident	"GCC: (GNU) 13.0.0 20220906 (experimental)"
	.section	.note.GNU-stack,"",@progbits
