<CsoundSynthesizer>
<CsOptions>
-+max_str_len=10000
</CsOptions>
<CsInstruments>

/****SOUNDFILE MERGER****
joachim heintz june 2010**/

0dbfs = 1
ksmps = 128; if you want to avoid that some samples without sound are added at the end of the soundfile, use ksmps=1. but this will be slow for large files 
nchnls = 8; THIS DETERMINES THE NUMBER OF CHANNELS IN YOUR MERGE FILE

  opcode StrayGetEl, ii, Sijj
;returns the startindex and the endindex (= the first space after the element) for ielindex in String. if startindex returns -1, the element has not been found
Stray, ielindx, isepA, isepB xin
;;DEFINE THE SEPERATORS
isep1		=		(isepA == -1 ? 32 : isepA)
isep2		=		(isepA == -1 && isepB == -1 ? 9 : (isepB == -1 ? isep1 : isepB))
Sep1		sprintf	"%c", isep1
Sep2		sprintf	"%c", isep2
;;INITIALIZE SOME PARAMETERS
ilen		strlen		Stray
istartsel	=		-1; startindex for searched element
iendsel	=		-1; endindex for searched element
iel		=		0; actual number of element while searching
iwarleer	=		1
indx		=		0
 if ilen == 0 igoto end ;don't go into the loop if Stray is empty
loop:
Snext		strsub		Stray, indx, indx+1; next sign
isep1p		strcmp		Snext, Sep1; returns 0 if Snext is sep1
isep2p		strcmp		Snext, Sep2; 0 if Snext is sep2
;;NEXT SIGN IS NOT SEP1 NOR SEP2
if isep1p != 0 && isep2p != 0 then
 if iwarleer == 1 then; first character after a seperator 
  if iel == ielindx then; if searched element index
istartsel	=		indx; set it
iwarleer	=		0
  else 			;if not searched element index
iel		=		iel+1; increase it
iwarleer	=		0; log that it's not a seperator 
  endif 
 endif 
;;NEXT SIGN IS SEP1 OR SEP2
else 
 if istartsel > -1 then; if this is first selector after searched element
iendsel	=		indx; set iendsel
		igoto		end ;break
 else	
iwarleer	=		1
 endif 
endif
		loop_lt	indx, 1, ilen, loop 
end: 		xout		istartsel, iendsel
  endop 

  opcode StrayLen, i, Sjj
;returns the number of elements in Stray. elements are defined by two seperators as ASCII coded characters: isep1 defaults to 32 (= space), isep2 defaults to 9 (= tab). if just one seperator is used, isep2 equals isep1
Stray, isepA, isepB xin
;;DEFINE THE SEPERATORS
isep1		=		(isepA == -1 ? 32 : isepA)
isep2		=		(isepA == -1 && isepB == -1 ? 9 : (isepB == -1 ? isep1 : isepB))
Sep1		sprintf	"%c", isep1
Sep2		sprintf	"%c", isep2
;;INITIALIZE SOME PARAMETERS
ilen		strlen		Stray
icount		=		0; number of elements
iwarsep	=		1
indx		=		0
 if ilen == 0 igoto end ;don't go into the loop if String is empty
loop:
Snext		strsub		Stray, indx, indx+1; next sign
isep1p		strcmp		Snext, Sep1; returns 0 if Snext is sep1
isep2p		strcmp		Snext, Sep2; 0 if Snext is sep2
 if isep1p == 0 || isep2p == 0 then; if sep1 or sep2
iwarsep	=		1; tell the log so
 else 				; if not 
  if iwarsep == 1 then	; and has been sep1 or sep2 before
icount		=		icount + 1; increase counter
iwarsep	=		0; and tell you are ot sep1 nor sep2 
  endif 
 endif	
		loop_lt	indx, 1, ilen, loop 
end: 		xout		icount
  endop 
  
  opcode Merge, 0, Sii
Stray, inels, iel xin
ist, ien	StrayGetEl	Stray, iel, 124
Sel		strsub		Stray, ist, ien
asig		soundin	Sel
		outch		iel+1, asig
 if iel+1 < inels then
		Merge	 	Stray, inels, iel+1
 endif
  endop


instr 1
;user input
Stray		invalue	"_MBrowse"; filenames, seperated by '|'
;get length from first element
ist, ien	StrayGetEl	Stray, 0, 124; get first element
Sfirst		strsub		Stray, ist, ien
idur		filelen	Sfirst
p3		=		(ksmps == 1 ? idur : idur + ksmps/sr); avoid omitting samples at the end
;merge
inels		StrayLen	Stray, 124; number of input files
 if inels > nchnls then
 		prints		"WARNING!%nNUMBER OF INPUT FILES (%d) ARE GREATER THAN NUMBER OF OUTPUT CHANNELS (%d)!%nNOT ALL INPUT FILES WILL BE WRITTEN. ADJUST THE nchnls PARAMETER IN THE ORCHESTRA HEADER TO AVOID THIS.%n", inels, nchnls
 elseif inels < nchnls then; probably crashing
 		prints		"ERROR!%nNUMBER OF INPUT FILES (%d) ARE GREATER THAN NUMBER OF OUTPUT CHANNELS (%d)!%nPLEASE ADJUST THE nchnls PARAMETER IN THE ORCHESTRA HEADER!%n"
 		turnoff
 endif
 		Merge		Stray, inels, 0	
endin
</CsInstruments>
<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>593</x>
 <y>215</y>
 <width>484</width>
 <height>322</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="background">
  <r>170</r>
  <g>170</g>
  <b>127</b>
 </bgcolor>
 <bsbObject version="2" type="BSBButton">
  <objectName>_MBrowse</objectName>
  <x>364</x>
  <y>69</y>
  <width>100</width>
  <height>30</height>
  <uuid>{6d6f3649-5d94-4759-afc8-887cc1872c39}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue>/Joachim/Csound/Hui/8kanaltest/Audio_1.aiff|/Joachim/Csound/Hui/8kanaltest/Audio_2.aiff|/Joachim/Csound/Hui/8kanaltest/Audio_3.aiff|/Joachim/Csound/Hui/8kanaltest/Audio_4.aiff|/Joachim/Csound/Hui/8kanaltest/Audio_5.aiff|/Joachim/Csound/Hui/8kanaltest/Audio_6.aiff|/Joachim/Csound/Hui/8kanaltest/Audio_7.aiff|/Joachim/Csound/Hui/8kanaltest/Audio_8.aiff</stringvalue>
  <text>Select Files</text>
  <image>/</image>
  <eventLine/>
  <latch>false</latch>
  <latched>false</latched>
 </bsbObject>
 <bsbObject version="2" type="BSBLineEdit">
  <objectName>_MBrowse</objectName>
  <x>17</x>
  <y>72</y>
  <width>345</width>
  <height>24</height>
  <uuid>{dcd7f769-c32b-4cc8-bc82-9848d5b36937}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label/>
  <alignment>left</alignment>
  <font>Lucida Grande</font>
  <fontsize>12</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <background>nobackground</background>
 </bsbObject>
 <bsbObject version="2" type="BSBButton">
  <objectName>_Render</objectName>
  <x>104</x>
  <y>104</y>
  <width>184</width>
  <height>30</height>
  <uuid>{a9b11523-b4f3-419f-8637-88da098e53f3}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <type>value</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>Merge!</text>
  <image>/</image>
  <eventLine>i1 0 10</eventLine>
  <latch>false</latch>
  <latched>false</latched>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>85</x>
  <y>16</y>
  <width>335</width>
  <height>39</height>
  <uuid>{46df577d-4fc5-4d07-a9cb-08d73733ad30}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>SOUNDFILE MERGER</label>
  <alignment>center</alignment>
  <font>Arial</font>
  <fontsize>24</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject version="2" type="BSBLabel">
  <objectName/>
  <x>19</x>
  <y>147</y>
  <width>444</width>
  <height>158</height>
  <uuid>{e54e17da-3808-49a3-84a6-608f476c3200}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <label>Merges any number of mono files to a multichannel file. The number of channels in the output file is determined by the 'nchnls' parameter in the header of this CSD file.
MAKE SURE YOUR nchnls EQUALS THE NUMBER OF YOU INPUT FILES!
(If nchnls=8 but you just gave 7 mono files as input, it will probably crash.)</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>14</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <objectName/>
 <x>593</x>
 <y>215</y>
 <width>484</width>
 <height>322</height>
 <visible>true</visible>
</bsbPanel>
<bsbPresets>
</bsbPresets>
<MacOptions>
Version: 3
Render: Real
Ask: Yes
Functions: ioObject
Listing: Window
WindowBounds: 593 215 484 322
CurrentView: io
IOViewEdit: On
Options: -b128 -A -s -m167 -R
</MacOptions>
<MacGUI>
ioView background {43690, 43690, 32639}
ioButton {364, 69} {100, 30} value 1.000000 "_MBrowse" "Select Files" "/" 
ioText {17, 72} {345, 24} edit 0.000000 0.00100 "_MBrowse"  "Lucida Grande" 12 {0, 0, 0} {65280, 65280, 65280} falsenoborder 
ioButton {104, 104} {184, 30} value 1.000000 "_Render" "Merge!" "/" i1 0 10
ioText {85, 16} {335, 39} label 0.000000 0.00100 "" center "Arial" 24 {0, 0, 0} {65280, 65280, 65280} nobackground noborder SOUNDFILE MERGER
ioText {19, 147} {447, 148} label 0.000000 0.00100 "" left "Arial" 14 {0, 0, 0} {65280, 65280, 65280} nobackground noborder Merges any number of mono files to a multichannel file. The number of channels in the output file is determined by the 'nchnls' parameter in the header of this CSD file.Â¬MAKE SURE YOUR nchnls EQUALS THE NUMBER OF YOU INPUT FILES!Â¬(If nchnls=8 but you just gave 7 mono files as input, it will probably crash.)
</MacGUI>
<EventPanel name="" tempo="60.00000000" loop="8.00000000" x="360" y="248" width="612" height="322" visible="true" loopStart="0" loopEnd="0">    </EventPanel>