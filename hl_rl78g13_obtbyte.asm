;
; (C) 2016 hikarimo
;

; 00C0/10C0番地
;
; b7(WDTINT)  0:インターバル割り込みを使用しない、1:する
; b6(WINDOW1) ウォッチドッグ・タイマのウィンドウ・オープン期間
; b5(WINDOW0) 00: 設定禁止
;             01: 50%
;             10: 75%
;             11: 100%
; b4(WDTON)   0:ウォッチドッグ・タイマのカウンタ動作禁止、1:許可
; b3(WDCS2)   ウォッチドッグ・タイマのオーバフロー時間
; b2(WDCS1)   000: 2^6/FIL (最短:3.71ms)   (通常:約4.27ms)
; b1(WDCS0)   001: 2^7/FIL (最短:7.42ms)   (通常:約8.53ms)
;             010: 2^8/FIL (最短:14.84ms)  (通常:約17.07ms)
;             011: 2^9/FIL (最短:29.68ms)  (通常:約34.13ms)
;             100: 2^11/FIL(最短:118.72ms) (通常:約136.53ms)
;             101: 2^13/FIL(最短:474.89ms) (通常:約546.13ms)
;             110: 2^14/FIL(最短:949.79ms) (通常:約1092.27ms)
;             111: 2^16/FIL(最短:3799.18ms)(通常:約4369.07ms)
;             ※最短時間以内にカウンタをリセットすること。
; b0(WDSTBYON) 0:HALT/STOPモード時、カウンタ動作停止、1:許可

; 00C1/10C1番地
;   LVDの設定

; 00C2/10C2番地
;
; b7(CMODE1)  フラッシュの動作モード
; b6(CMODE1)  00: LV(低圧メイン)モード(1-4MHz 1.6-5.5V)
;             10: LS(低速メイン)モード(1-8MHz 1.8-5.5V)
;             11: HS(高速メイン)モード(1-16MHz 2.4-5.5V)
;                                     (1-32MHz 2.7-5.5V)
;             その他: 設定禁止
; b5          1固定
; b4          0固定
; b3(FRQSEL3) 高速オンチップ・オシレータの周波数
; b2(FRQSEL2) 1000: 32MHz
; b1(FRQSEL1) 0000: 24MHz
; b0(FRQSEL0) 1001: 16MHz
;             0001: 12MHz
;             1010: 8MHz
;             0010: 6MHz
;             1011: 4MHz
;             0011: 3MHz
;             1100: 2MHz
;             1101: 1MHz
;             その他: 設定禁止

; 00C3/10C3番地
;   0x04: オンチップ・デバッグ動作禁止
;   0x85: オンチップ・デバッグ許可
;   0x84: オンチップ・デバッグ許可(セキュリティID認証失敗で内蔵フラッシュ全消去)

?@CSEGOB0	CSEG	OPT_BYTE
	DB	0EFH	;00C0/10C0番地
	DB	0FFH	;00C1/10C1番地
	DB	0E8H	;00C2/10C2番地
	DB	085H	;00C3/10C3番地
END
