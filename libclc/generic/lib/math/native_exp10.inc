_CLC_OVERLOAD _CLC_DEF __CLC_GENTYPE native_exp10(__CLC_GENTYPE val) {
  return native_exp2(val * native_log2(10));
}
