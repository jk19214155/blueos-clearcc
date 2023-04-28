; haribote-ipl
; TAB=4

CYLS	EQU		20				; どこまで読み込むか

		ORG		0x7c00			; このプログラムがどこに読み込まれるのか

; 以下は標準的なFAT12フォーマットフロッピーディスクのための記述

		JMP		entry
		DB		0x90
		DB		"BOSCLEAR"		; 8个字?的磁?名称
		DW		512				; ?扇区字?数量
		DB		1				; ?簇扇区数
		DW		1				; 保留扇区数
		DB		2				; 2个文件分配表
		DW		224				; 最大根目?条目个数（fat32?0）
		DW		2880			; ?扇区数（如果是0?使用0x20的大小）
		DB		0xf0			; 介?描述（bit0：双面?志位 bit1：?面8（9）扇区?志位 bit2：40（80）磁道?志位）
		DW		9				; ?个文件分配表的扇区（fat32?0）
		DW		18				; ?磁道扇区数
		DW		2				; 磁?数
		DD		0				; ?藏扇区数量
		DD		2880			; 如果超?65535?使用此?的?扇区数
		DD		9				; ?个fat表占用扇区数（fat32使用）
		DW		0x8f
		DW		0				;FAT32版本号
		DD		2				;根目?所在第一个簇的簇号
		DW		1				;保留区中FAT32卷FSINFO??所占的扇区数，通常?1。
		DW		6				;如果不?0，表示在保留区中引???的?数据所占的扇区数，通常?6。同?不建?使用6以外的其他数?
		RESB 12					;?以后?展保留
		DB		0				; 物理??器?型（操作系?决定）
		DB		0				; 当前磁?（必?是0）
		DB		0x29		; ?展引???可用
		DD		0xffffffff		; 磁?id
		DB		"BLUEOSCLEAR"	; ディスクの名前（11バイト）
		DB		"FAT32   "		; フォーマットの名前（8バイト）
		RESB	18				; とりあえず18バイトあけておく

; プログラム本体

entry:
		MOV		AX,0			; レジスタ初期化
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; ディスクを読む

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; シリンダ0
		MOV		DH,0			; ヘッド0
		MOV		CL,2			; セクタ2
		MOV		BX,18*2*CYLS-1	; 読み込みたい合計セクタ数
		CALL	readfast		; 高速読み込み

; 読み終わったのでharibote.sysを実行だ！

		MOV		BYTE [0x0ff0],CYLS	; IPLがどこまで読んだのかをメモ
		JMP		0xc200

error:
		MOV		AX,0
		MOV		ES,AX
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; SIに1を足す
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 一文字表示ファンクション
		MOV		BX,15			; カラーコード
		INT		0x10			; ビデオBIOS呼び出し
		JMP		putloop
fin:
		HLT						; 何かあるまでCPUを停止させる
		JMP		fin				; 無限ループ
msg:
		DB		0x0a, 0x0a		; 改行を2つ
		DB		"load error"
		DB		0x0a			; 改行
		DB		0

readfast:	; ALを使ってできるだけまとめて読み出す
;	ES:読み込み番地, CH:シリンダ, DH:ヘッド, CL:セクタ, BX:読み込みセクタ数

		MOV		AX,ES			; < ESからALの最大値を計算 >
		SHL		AX,3			; AXを32で割って、その結果をAHに入れたことになる （SHLは左シフト命令）
		AND		AH,0x7f			; AHはAHを128で割った余り（512*128=64K）
		MOV		AL,128			; AL = 128 - AH; 一番近い64KB境界まで最大何セクタ入るか
		SUB		AL,AH

		MOV		AH,BL			; < BXからALの最大値をAHに計算 >
		CMP		BH,0			; if (BH != 0) { AH = 18; }
		JE		.skip1
		MOV		AH,18
.skip1:
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip2
		MOV		AL,AH
.skip2:

		MOV		AH,19			; < CLからALの最大値をAHに計算 >
		SUB		AH,CL			; AH = 19 - CL;
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip3
		MOV		AL,AH
.skip3:

		PUSH	BX
		MOV		SI,0			; 失敗回数を数えるレジスタ
retry:
		MOV		AH,0x02			; AH=0x02 : ディスク読み込み
		MOV		BX,0
		MOV		DL,0x00			; Aドライブ
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH	AX
		INT		0x13			; ディスクBIOS呼び出し
		JNC		next			; エラーがおきなければnextへ
		ADD		SI,1			; SIに1を足す
		CMP		SI,5			; SIと5を比較
		JAE		error			; SI >= 5 だったらerrorへ
		MOV		AH,0x00
		MOV		DL,0x00			; Aドライブ
		INT		0x13			; ドライブのリセット
		POP		AX
		POP		CX
		POP		DX
		POP		ES
		JMP		retry
next:
		POP		AX
		POP		CX
		POP		DX
		POP		BX				; ESの内容をBXで受け取る
		SHR		BX,5			; BXを16バイト単位から512バイト単位へ
		MOV		AH,0
		ADD		BX,AX			; BX += AL;
		SHL		BX,5			; BXを512バイト単位から16バイト単位へ
		MOV		ES,BX			; これで ES += AL * 0x20; になる
		POP		BX
		SUB		BX,AX
		JZ		.ret
		ADD		CL,AL			; CLにALを足す
		CMP		CL,18			; CLと18を比較
		JBE		readfast		; CL <= 18 だったらreadfastへ
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readfast		; DH < 2 だったらreadfastへ
		MOV		DH,0
		ADD		CH,1
		JMP		readfast
.ret:
		RET

		RESB	0x7dfe-$		; 0x7dfeまでを0x00で埋める命令

		DB		0x55, 0xaa
