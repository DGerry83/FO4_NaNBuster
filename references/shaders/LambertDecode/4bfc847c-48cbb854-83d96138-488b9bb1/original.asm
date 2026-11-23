Shader hash 4bfc847c-48cbb854-83d96138-488b9bb1

ps_5_0
      dcl_globalFlags refactoringAllowed
      dcl_constantbuffer cb12[31], immediateIndexed
      dcl_constantbuffer cb2[1], immediateIndexed
      dcl_sampler s0, mode_default
      dcl_sampler s1, mode_default
      dcl_sampler s2, mode_default
      dcl_sampler s3, mode_default
      dcl_sampler s4, mode_default
      dcl_sampler s5, mode_default
      dcl_sampler s6, mode_default
      dcl_sampler s7, mode_default
      dcl_sampler s8, mode_default
      dcl_sampler s9, mode_default
      dcl_resource_texture2d (float,float,float,float) t0
      dcl_resource_texture2d (float,float,float,float) t1
      dcl_resource_texture2d (float,float,float,float) t2
      dcl_resource_texture2d (float,float,float,float) t3
      dcl_resource_texture2d (float,float,float,float) t4
      dcl_resource_texture2d (float,float,float,float) t5
      dcl_resource_texture2d (float,float,float,float) t6
      dcl_resource_texture2d (float,float,float,float) t7
      dcl_resource_texturecubearray (float,float,float,float) t8
      dcl_resource_texture2d (float,float,float,float) t9
      dcl_input_ps_siv linear noperspective v0.xy, position
      dcl_output o0.xyzw
      dcl_temps 11
   0: mul r0.xy, v0.xyxx, cb2[0].xyxx
   1: sample_l(texture2d)(float,float,float,float) r1.xyzw, r0.xyxx, t0.xyzw, s0, l(0)
   2: sample_l(texture2d)(float,float,float,float) r2.xy, r0.xyxx, t3.xyzw, s3, l(0)
   3: sample_l(texture2d)(float,float,float,float) r3.xyz, r0.xyxx, t5.xyzw, s5, l(0)
   4: mul r3.xyz, r3.xyzx, l(3.000000, 3.000000, 3.000000, 0.000000)
   5: sample_l(texture2d)(float,float,float,float) r0.w, r0.xyxx, t7.yzwx, s7, l(0)
   6: sample_indexable(texture2d)(float,float,float,float) r4.xyz, r0.xyxx, t4.xyzw, s4
   7: ge r2.z, l(0.010000), r0.w
   8: if_nz r2.z
   9:   mul r5.z, r0.w, l(100.000000)
  10:   mov r6.xyzw, cb12[24].xyzw
  11:   mov r7.xyzw, cb12[25].xyzw
  12:   mov r8.xyzw, cb12[26].xyzw
  13:   mov r9.xyzw, cb12[27].xyzw
  14: else
  15:   mad r5.z, r0.w, l(1.010000), l(-0.010000)
  16:   mov r6.xyzw, cb12[20].xyzw
  17:   mov r7.xyzw, cb12[21].xyzw
  18:   mov r8.xyzw, cb12[22].xyzw
  19:   mov r9.xyzw, cb12[23].xyzw
  20: endif
  21: sample_l(texture2d)(float,float,float,float) r10.xyz, r0.xyxx, t6.xyzw, s6, l(0)
  22: add r4.xyz, r4.xyzx, r10.xyzx
  23: mad r1.xyz, r3.xyzx, r1.xyzx, r4.xyzx
  24: mul r0.w, r2.y, l(3.000000)
  25: add_sat r2.y, r2.x, l(-0.300000)
  26: rsq r2.y, r2.y
  27: div r2.y, l(1.000000, 1.000000, 1.000000, 1.000000), r2.y
  28: min r2.y, r2.y, l(1.000000)
  29: mul r0.w, r0.w, r2.y
  30: sample_l(texture2d)(float,float,float,float) r2.yz, r0.xyxx, t2.xyzw, s2, l(0)
  31: mul r2.z, r2.z, r2.z
  32: mul r2.z, r2.z, l(50.000000)
  33: lt r2.w, l(0.001961), r2.y
  34: if_nz r2.w
  35:   sample_l(texture2d)(float,float,float,float) r4.xy, r0.xyxx, t1.xyzw, s1, l(0)
  36:   mad r4.xy, r4.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)
  37:   dp2 r2.w, r4.xyxx, r4.xyxx
  38:   mad r4.zw, -r2.wwww, l(0.000000, 0.000000, 0.250000, 0.500000), l(0.000000, 0.000000, 1.000000, 1.000000)
  39:   sqrt r2.w, r4.z
  40:   mul r4.xy, r2.wwww, r4.xyxx
  41:   mov r4.z, -r4.w
  42:   mad r0.z, -v0.y, cb2[0].y, l(1.000000)
  43:   mad r5.xy, r0.xzxx, l(2.000000, 2.000000, 0.000000, 0.000000), l(-1.000000, -1.000000, 0.000000, 0.000000)
  44:   mov r5.w, l(1.000000)
  45:   dp4 r6.x, r6.xyzw, r5.xyzw
  46:   dp4 r6.y, r7.xyzw, r5.xyzw
  47:   dp4 r6.z, r8.xyzw, r5.xyzw
  48:   dp4 r0.z, r9.xyzw, r5.xyzw
  49:   div r5.xyz, r6.xyzx, r0.zzzz
  50:   dp3 r0.z, -r5.xyzx, -r5.xyzx
  51:   rsq r0.z, r0.z
  52:   mul r5.xyw, r0.zzzz, -r5.xyxz
  53:   dp3 r0.z, r5.xywx, r4.xyzx
  54:   add r0.z, r0.z, r0.z
  55:   mad r4.xyz, r4.xyzx, -r0.zzzz, r5.xywx
  56:   dp3 r6.x, cb12[12].xyzx, r4.xyzx
  57:   dp3 r6.y, cb12[13].xyzx, r4.xyzx
  58:   dp3 r6.z, cb12[14].xyzx, r4.xyzx
  59:   add r0.z, -r2.x, l(1.000000)
  60:   mul r0.z, r0.z, l(6.000000)
  61:   mad r0.z, r5.z, l(0.001953), r0.z
  62:   mad r2.x, r2.y, l(255.000000), l(-1.000000)
  63:   round_ni r6.w, r2.x
  64:   sample_l(texturecubearray)(float,float,float,float) r2.xyw, r6.xyzw, t8.xywz, s8, r0.z
  65:   dp3 r0.z, r2.xywx, l(0.299000, 0.587000, 0.114000, 0.000000)
  66:   mul r3.w, cb12[30].y, l(0.900000)
  67:   add r4.xyz, -r2.xywx, r0.zzzz
  68:   mad r2.xyw, r3.wwww, r4.xyxz, r2.xyxw
  69: else
  70:   mov r2.xyw, l(0, 0, 0, 0)
  71: endif
  72: mul r2.xyw, r0.wwww, r2.xyxw
  73: mul r2.xyz, r2.zzzz, r2.xywx
  74: mad r1.xyz, r2.xyzx, r3.xyzx, r1.xyzx
  75: sample_indexable(texture2d)(float,float,float,float) r0.x, r0.xyxx, t9.xyzw, s9
  76: mul o0.xyz, r0.xxxx, r1.xyzx
  77: mov o0.w, r1.w
  78: ret
