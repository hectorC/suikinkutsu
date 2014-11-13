<CsoundSynthesizer>
<CsOptions>
-odac 
</CsOptions>
<CsInstruments>

sr = 48000
;ksmps = 32
kr = 1500
nchnls = 2
0dbfs = 1

gaw init 0
gax init 0
gay init 0
gaz init 0

gaRev init 0

;;;;;;;;;;; FILE: 4 channel B-Format audio file playback
instr 10

;	SFile strget p4
	SFile1 sprintf "soundscape_%d", p4
	SFile strcat SFile1, ".wav"
	ilen filelen SFile
	
	gaw, gax, gay, gaz diskin2 SFile, 1, p5, 1
	
	kpos line p5, ilen - p5, ilen
	
	if (kpos > ilen) then
	  event "e", 0, 0	  
	endif	
	
	chnset ilen, "fileLength"
	chnset kpos, "position"

endin

;;;;;;;;;;; SNTH: Phase Vocoder chimes
instr 20
	
	SFile1 sprintf "sample_%d", p4
	SFile strcat SFile1, ".pvx"

	idur filelen SFile
	
	instance active 20 
	SchanPos sprintf "posx%d", instance
	SchanPitch sprintf "posy%d", instance

	kpos chnget SchanPos
	kpitch chnget SchanPitch
	koutGain chnget "outgain"
;	printk2  kpitch
	
	kpitch limit kpitch, 0.1, 10
	kpos2 port kpos, 1	
	kpos2 limit kpos2, 0, idur 			
				
	aoutL pvoc kpos2*idur, kpitch, SFile

	denorm aoutL

	
	if (koutGain == 0) then
	kglis = 4
	else
	kglis = 1
	endif
	
	kgain lineto koutGain, kglis
		
	krand randh 90, 10
	krandR lineto krand, 4
	
	kaz1 wrap ((180 * kpos) - 90) + krandR, 0, 360
	kaz2 wrap kaz1 - 30, 0, 360
	
	gaRev = gaRev + aoutL*kgain
	
	awL, axL, ayL, azL bformenc1 aoutL*kgain, kaz1, 0
	
	gaw = gaw + awL
	gax = gax +axL
	gay = gay + ayL
	gaz = gaz + azL

endin

;;;;;;;;;;; DECODE: B-Format decoding, processing and binaural output
instr 30

	i2PI = $M_PI * 2

	Ssubject strget p4
	
	SHRTF strcat "IRC_", Ssubject
	SFLD strcat SHRTF, "_C_R0195_T045_P330.wav"
	SFLU strcat SHRTF, "_C_R0195_T045_P030.wav"
	SBLD strcat SHRTF, "_C_R0195_T135_P330.wav"
	SBLU strcat SHRTF, "_C_R0195_T135_P030.wav"
	SBRD strcat SHRTF, "_C_R0195_T225_P330.wav"
	SBRU strcat SHRTF, "_C_R0195_T225_P030.wav"
	SFRD strcat SHRTF, "_C_R0195_T315_P330.wav"
	SFRU strcat SHRTF, "_C_R0195_T315_P030.wav"
	
	if (sr == 44100) then
	SFLD strcat SHRTF, "_C_R0195_T045_P330.wav"
	SFLU strcat SHRTF, "_C_R0195_T045_P030.wav"
	SBLD strcat SHRTF, "_C_R0195_T135_P330.wav"
	SBLU strcat SHRTF, "_C_R0195_T135_P030.wav"
	SBRD strcat SHRTF, "_C_R0195_T225_P330.wav"
	SBRU strcat SHRTF, "_C_R0195_T225_P030.wav"
	SFRD strcat SHRTF, "_C_R0195_T315_P330.wav"
	SFRU strcat SHRTF, "_C_R0195_T315_P030.wav" 	
	endif
	
	if (sr == 48000) then
	SFLD strcat SHRTF, "_C_R0195_T045_P330_48.wav"
	SFLU strcat SHRTF, "_C_R0195_T045_P030_48.wav"
	SBLD strcat SHRTF, "_C_R0195_T135_P330_48.wav"
	SBLU strcat SHRTF, "_C_R0195_T135_P030_48.wav"
	SBRD strcat SHRTF, "_C_R0195_T225_P330_48.wav"
	SBRU strcat SHRTF, "_C_R0195_T225_P030_48.wav"
	SFRD strcat SHRTF, "_C_R0195_T315_P330_48.wav"
	SFRU strcat SHRTF, "_C_R0195_T315_P030_48.wav"
	endif
							
	iAzFLD = -45
	iEFLD = -35.26
	iAzFLU = -45
	iEFLU = 35.26
	iAzBLD = -135
	iEBLD = -35.26
	iAzBLU = -135
	iEBLU = 35.26
	iAzBRD = 135
	iEBRD = -35.26
	iAzBRU = 135
	iEBRU = 35.26
	iAzFRD = 45
	iEFRD = -35.26
	iAzFRU = 45
	iEFRU = 35.26
	
	kyaw chnget "yaw"
	kpitch chnget "pitch"
	kroll chnget "roll"
	
	;; YAW (Z axis, up/down)
	aYx = (gax * cos(kyaw)) - (gay * sin(kyaw))
	aYy = (gax * sin(kyaw)) + (gay * cos(kyaw))
	aYz = gaz
	
	;; PITCH (Y axis, left/right)
	aPx = (aYx * cos(kpitch)) - (aYz * sin(kpitch))
	aPy = aYy
	aPz = (aYx * sin(kpitch)) + (aYz * cos(kpitch))
	
	;;ROLL (X akis, front/back)
	kroll = i2PI - kroll
	aRx = aPx
	aRy = (aPy * cos(kroll)) - (aPz * sin(kroll))
	aRz = (aPy * sin(kroll)) + (aPz * cos(kroll))
	
	aFLD, aFLU, aBLD, aBLU, aBRD, aBRU, aFRD, aFRU bformdec1 5, gaw, aRx, aRy, aRz
	aleft1, aright1  pconvolve  aFLD, SFLD
	aleft2, aright2  pconvolve  aFLU, SFLU
	aleft3, aright3  pconvolve  aBLD, SBLD
	aleft4, aright4  pconvolve  aBLU, SBLU
	aleft5, aright5  pconvolve  aBRD, SBRD
	aleft6, aright6  pconvolve  aBRU, SBRU
	aleft7, aright7  pconvolve  aFRD, SFRD
	aleft8, aright8  pconvolve  aFRU, SFRU
	
	aleft = aleft1 + aleft2 + aleft3 + aleft4 + aleft5 + aleft6 + aleft7 + aleft8
	aright = aright1 + aright2 + aright3 + aright4 + aright5 + aright6 + aright7 + aright8
	 

	 aleftR, arightR, idel hrtfreverb gaRev, 2, 2, "hrtf48000left.dat", "hrtf48000right.dat", 48000
	 gaRev = 0;
	 denorm gaRev
	 denorm aleftR
	 denorm arightR
	 
	outs aleft + (aleftR * 0.5), aright + (arightR*0.5)
endin


</CsInstruments>
<CsScore>
;i 10 0 14400 "soundscape_1.wav" 0
i 30 0 14400 "1015" 1017 1
e
</CsScore>
</CsoundSynthesizer>
