Shader hash 4dc83306-024b9074-9c24d67a-fcdee952

ps_5_0
      dcl_globalFlags refactoringAllowed
      dcl_constantbuffer cb12[30], immediateIndexed
      dcl_constantbuffer cb2[23], immediateIndexed
      dcl_sampler s0, mode_default
      dcl_sampler s1, mode_default
      dcl_sampler s2, mode_default
      dcl_sampler s3, mode_default
      dcl_resource_texture2d (float,float,float,float) t0
      dcl_resource_texture2d (float,float,float,float) t1
      dcl_resource_texture2d (float,float,float,float) t2
      dcl_resource_texture2d (float,float,float,float) t3
      dcl_input_ps_siv linear noperspective v0.xy, position
      dcl_output o0.xyzw
      dcl_output o1.xyzw
      dcl_temps 9
   0: mul r0.xy, v0.xyxx, cb2[22].xyxx
   1: mul r0.xy, r0.xyxx, cb2[0].xyxx
   2: mul r1.x, v0.x, cb2[0].x
   3: deriv_rtx_coarse r0.z, r0.x
   4: deriv_rty_coarse r0.w, r0.y
   5: sample_d(texture2d)(float,float,float,float) r0.z, r0.xyxx, t3.yzxw, s3, r0.zzzz, r0.wwww
   6: ge r0.w, l(0.010000), r0.z
   7: if_nz r0.w
   8:   mul r2.z, r0.z, l(100.000000)
   9:   mov r3.xyzw, cb12[24].xyzw
  10:   mov r4.xyzw, cb12[25].xyzw
  11:   mov r5.xyzw, cb12[26].xyzw
  12:   mov r6.xyzw, cb12[27].xyzw
  13: else
  14:   mad r2.z, r0.z, l(1.010000), l(-0.010000)
  15:   mov r3.xyzw, cb12[20].xyzw
  16:   mov r4.xyzw, cb12[21].xyzw
  17:   mov r5.xyzw, cb12[22].xyzw
  18:   mov r6.xyzw, cb12[23].xyzw
  19: endif
  20: mad r1.z, -v0.y, cb2[0].y, l(1.000000)
  21: mad r2.xy, r1.xzxx, l(2.000000, 2.000000, 0.000000, 0.000000), l(-1.000000, -1.000000, 0.000000, 0.000000)
  22: mov r2.w, l(1.000000)
  23: dp4 r1.x, r3.xyzw, r2.xyzw
  24: dp4 r1.y, r4.xyzw, r2.xyzw
  25: dp4 r1.z, r5.xyzw, r2.xyzw
  26: dp4 r0.z, r6.xyzw, r2.xyzw
  27: div r1.xyz, r1.xyzx, r0.zzzz
  28: add r2.xyz, -r1.xyzx, cb2[1].xyzx
  29: dp3 r0.z, r2.xyzx, r2.xyzx
  30: sqrt r0.w, r0.z
  31: rsq r0.z, r0.z
  32: mul r2.xyz, r0.zzzz, r2.xyzx
  33: div_sat r0.z, r0.w, cb2[1].w
  34: log r0.z, r0.z
  35: mul r0.z, r0.z, cb2[3].z
  36: exp r0.z, r0.z
  37: mad_sat r0.z, cb2[3].y, r0.z, cb2[3].x
  38: add r0.z, -r0.z, l(1.000000)
  39: log r0.z, r0.z
  40: mul r0.z, r0.z, l(2.200000)
  41: exp r0.z, r0.z
  42: dp3_sat r0.w, -r2.xyzx, cb2[5].xyzx
  43: add r0.w, -r0.w, l(1.000000)
  44: add r1.w, -cb2[5].w, l(1.000000)
  45: div r0.w, r0.w, r1.w
  46: add_sat r0.w, -r0.w, l(1.000000)
  47: log r0.w, r0.w
  48: mul r0.w, r0.w, cb2[20].x
  49: exp r0.w, r0.w
  50: min r0.w, r0.w, l(1.000000)
  51: mul r0.z, r0.w, r0.z
  52: ge r0.w, l(0.001000), r0.z
  53: sample_indexable(texture2d)(float,float,float,float) r3.xyzw, r0.xyxx, t2.xyzw, s2
  54: sample_indexable(texture2d)(float,float,float,float) r1.w, r0.xyxx, t0.xyzw, s0
  55: sample_indexable(texture2d)(float,float,float,float) r0.xy, r0.xyxx, t1.xyzw, s1
  56: if_nz r0.w
  57:   mov o0.xyzw, l(0, 0, 0, 0)
  58:   mov o1.xyzw, l(0, 0, 0, 0)
  59:   ret
  60: endif
  61: mad r0.xy, r0.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)
  62: dp2 r0.w, r0.xyxx, r0.xyxx
  63: mad r4.xy, -r0.wwww, l(0.250000, 0.500000, 0.000000, 0.000000), l(1.000000, 1.000000, 0.000000, 0.000000)
  64: sqrt r0.w, r4.x
  65: mul r5.xy, r0.wwww, r0.xyxx
  66: mov r5.z, -r4.y
  67: add r0.x, -r3.x, l(1.000000)
  68: dp3 r0.y, -r1.xyzx, -r1.xyzx
  69: rsq r0.y, r0.y
  70: mul r4.xyz, r0.yyyy, -r1.xyzx
  71: dp3 r0.w, r5.xyzx, r2.xyzx
  72: max r2.w, r0.w, l(0)
  73: min r4.w, r2.w, l(1.000000)
  74: mad r3.w, r3.w, l(255.000000), l(-1.000000)
  75: lt r3.w, abs(r3.w), l(0.250000)
  76: if_nz r3.w
  77:   dp3 r3.w, r3.xyzx, r2.xyzx
  78:   dp3 r3.z, r3.xyzx, r4.xyzx
  79:   mul r5.w, r3.w, r3.w
  80:   min r5.w, r5.w, l(1.000000)
  81:   add r5.w, -r5.w, l(1.000000)
  82:   sqrt r5.w, r5.w
  83:   mul r6.x, r3.z, r3.z
  84:   min r6.x, r6.x, l(1.000000)
  85:   add r6.x, -r6.x, l(1.000000)
  86:   sincos r7.x, r8.x, cb12[29].y
  87:   mul r6.y, r5.w, r7.x
  88:   mad r6.y, -r3.w, r8.x, -r6.y
  89:   mad r6.z, -r6.y, r6.y, l(1.000000)
  90:   sqrt r6.xz, r6.xxzx
  91:   mul r6.z, r6.x, r6.z
  92:   mad r6.y, r6.y, r3.z, r6.z
  93:   max r6.y, r6.y, l(0)
  94:   log r6.y, r6.y
  95:   mul r6.y, r6.y, cb12[28].w
  96:   exp r6.y, r6.y
  97:   mad_sat r6.y, cb12[28].z, r6.y, r2.w
  98:   min r1.w, r1.w, r6.y
  99:   sincos r7.x, r8.x, cb12[29].x
 100:   mul r5.w, r5.w, r7.x
 101:   mad r3.w, -r3.w, r8.x, -r5.w
 102:   mad r5.w, -r3.w, r3.w, l(1.000000)
 103:   sqrt r5.w, r5.w
 104:   mul r5.w, r6.x, r5.w
 105:   mad r3.z, r3.w, r3.z, r5.w
 106:   max r3.z, r3.z, l(0)
 107:   log r3.z, r3.z
 108:   mul r3.z, r3.z, cb12[28].y
 109:   exp r3.z, r3.z
 110:   mul r3.z, r3.z, cb12[28].x
 111:   mul r6.xyz, r3.zzzz, cb2[2].xyzx
 112:   mul r6.xyz, r4.wwww, r6.xyzx
 113: else
 114:   mad r3.x, r3.x, l(10.000000), l(1.000000)
 115:   exp r3.x, r3.x
 116:   dp3 r3.z, r4.xyzx, r5.xyzx
 117:   mad r7.xyz, -r5.xyzx, r3.zzzz, r4.xyzx
 118:   mad r8.xyz, -r5.xyzx, r0.wwww, r2.xyzx
 119:   dp3 r3.w, r7.xyzx, r8.xyzx
 120:   mul r5.w, r0.x, r0.x
 121:   mad r7.xy, r0.xxxx, r0.xxxx, l(0.570000, 0.090000, 0.000000, 0.000000)
 122:   div r7.xy, r5.wwww, r7.xyxx
 123:   mul r5.w, r7.y, l(0.450000)
 124:   mad r6.w, -r7.x, l(0.500000), l(1.000000)
 125:   mad r7.x, -r3.z, r3.z, l(1.000000)
 126:   mad r7.y, -r0.w, r0.w, l(1.000000)
 127:   mul_sat r7.x, r7.y, r7.x
 128:   sqrt r7.x, r7.x
 129:   max r0.w, r0.w, r3.z
 130:   div r0.w, r7.x, r0.w
 131:   max r3.w, r3.w, l(0)
 132:   mul r3.w, r3.w, r5.w
 133:   mad r0.w, r3.w, r0.w, r6.w
 134:   mul r1.w, r0.w, r2.w
 135:   mad r1.xyz, -r1.xyzx, r0.yyyy, r2.xyzx
 136:   dp3 r0.y, r1.xyzx, r1.xyzx
 137:   rsq r0.y, r0.y
 138:   mul r1.xyz, r0.yyyy, r1.xyzx
 139:   mov_sat r3.z, r3.z
 140:   dp3_sat r0.y, r4.xyzx, r1.xyzx
 141:   dp3_sat r0.w, r1.xyzx, r5.xyzx
 142:   add r1.x, r3.x, l(2.000000)
 143:   mul r1.x, r1.x, l(0.159155)
 144:   log r1.y, r0.w
 145:   mul r1.y, r1.y, r3.x
 146:   exp r1.y, r1.y
 147:   mul r1.x, r1.y, r1.x
 148:   max r1.y, r0.y, l(0.000000)
 149:   min r1.z, r4.w, r3.z
 150:   add r0.w, r0.w, r0.w
 151:   mul r2.w, r0.w, r1.z
 152:   ge r2.w, r1.y, r2.w
 153:   eq r1.z, r3.z, r1.z
 154:   div r3.x, r4.w, r3.z
 155:   movc r1.z, r1.z, l(1.000000), r3.x
 156:   mul r0.w, r0.w, r1.z
 157:   div r0.w, r0.w, r1.y
 158:   div r1.y, l(1.000000, 1.000000, 1.000000, 1.000000), r3.z
 159:   movc r0.w, r2.w, r0.w, r1.y
 160:   add r0.y, -r0.y, l(1.000000)
 161:   mul r1.y, r0.y, r0.y
 162:   mul r1.y, r1.y, r1.y
 163:   mul r1.z, r0.y, r1.y
 164:   mad r0.y, -r0.y, r1.y, l(1.000000)
 165:   mad r0.y, r0.y, l(0.200000), r1.z
 166:   min r0.y, r0.y, l(1.000000)
 167:   mul r0.y, r0.w, r0.y
 168:   mul r0.y, r1.x, r0.y
 169:   mul r0.y, r0.y, l(0.250000)
 170:   min r0.y, r0.y, l(15.000000)
 171:   mul r0.y, r3.y, r0.y
 172:   mul r0.y, r0.y, l(3.141593)
 173:   mul r1.xyz, r0.yyyy, cb2[2].xyzx
 174:   mul r6.xyz, r4.wwww, r1.xyzx
 175: endif
 176: dp3_sat r0.y, r5.xyzx, r4.xyzx
 177: add r0.y, -r0.y, l(1.000000)
 178: log r0.y, r0.y
 179: mul r0.y, r0.y, l(0.010000)
 180: exp r0.y, r0.y
 181: dp3_sat r0.w, r4.xyzx, -r2.xyzx
 182: mul r0.y, r0.w, r0.y
 183: mul r0.y, r4.w, r0.y
 184: mul r0.x, r0.x, r0.y
 185: mul r0.xyw, r0.xxxx, cb2[2].xyxz
 186: mad r0.xyw, cb2[2].xyxz, r1.wwww, r0.xyxw
 187: mul o1.xyz, r0.zzzz, r6.xyzx
 188: mul r0.xyz, r0.zzzz, r0.xywx
 189: mov r0.w, l(0)
 190: div o0.xyzw, r0.xyzw, l(3.000000, 3.000000, 3.000000, 3.000000)
 191: mov o1.w, l(1.000000)
 192: ret
