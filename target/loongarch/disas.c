/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * QEMU LoongArch Disassembler
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited.
 */

#include "qemu/osdep.h"
#include "disas/dis-asm.h"
#include "qemu/bitops.h"
#include "cpu-csr.h"

typedef struct {
    disassemble_info *info;
    uint64_t pc;
    uint32_t insn;
} DisasContext;

static inline int plus_1(DisasContext *ctx, int x)
{
    return x + 1;
}

static inline int shl_2(DisasContext *ctx, int x)
{
    return x << 2;
}

#define CSR_NAME(REG) \
    [LOONGARCH_CSR_##REG] = (#REG)

static const char * const csr_names[] = {
    CSR_NAME(CRMD),
    CSR_NAME(PRMD),
    CSR_NAME(EUEN),
    CSR_NAME(MISC),
    CSR_NAME(ECFG),
    CSR_NAME(ESTAT),
    CSR_NAME(ERA),
    CSR_NAME(BADV),
    CSR_NAME(BADI),
    CSR_NAME(EENTRY),
    CSR_NAME(TLBIDX),
    CSR_NAME(TLBEHI),
    CSR_NAME(TLBELO0),
    CSR_NAME(TLBELO1),
    CSR_NAME(ASID),
    CSR_NAME(PGDL),
    CSR_NAME(PGDH),
    CSR_NAME(PGD),
    CSR_NAME(PWCL),
    CSR_NAME(PWCH),
    CSR_NAME(STLBPS),
    CSR_NAME(RVACFG),
    CSR_NAME(CPUID),
    CSR_NAME(PRCFG1),
    CSR_NAME(PRCFG2),
    CSR_NAME(PRCFG3),
    CSR_NAME(SAVE(0)),
    CSR_NAME(SAVE(1)),
    CSR_NAME(SAVE(2)),
    CSR_NAME(SAVE(3)),
    CSR_NAME(SAVE(4)),
    CSR_NAME(SAVE(5)),
    CSR_NAME(SAVE(6)),
    CSR_NAME(SAVE(7)),
    CSR_NAME(SAVE(8)),
    CSR_NAME(SAVE(9)),
    CSR_NAME(SAVE(10)),
    CSR_NAME(SAVE(11)),
    CSR_NAME(SAVE(12)),
    CSR_NAME(SAVE(13)),
    CSR_NAME(SAVE(14)),
    CSR_NAME(SAVE(15)),
    CSR_NAME(TID),
    CSR_NAME(TCFG),
    CSR_NAME(TVAL),
    CSR_NAME(CNTC),
    CSR_NAME(TICLR),
    CSR_NAME(LLBCTL),
    CSR_NAME(IMPCTL1),
    CSR_NAME(IMPCTL2),
    CSR_NAME(TLBRENTRY),
    CSR_NAME(TLBRBADV),
    CSR_NAME(TLBRERA),
    CSR_NAME(TLBRSAVE),
    CSR_NAME(TLBRELO0),
    CSR_NAME(TLBRELO1),
    CSR_NAME(TLBREHI),
    CSR_NAME(TLBRPRMD),
    CSR_NAME(MERRCTL),
    CSR_NAME(MERRINFO1),
    CSR_NAME(MERRINFO2),
    CSR_NAME(MERRENTRY),
    CSR_NAME(MERRERA),
    CSR_NAME(MERRSAVE),
    CSR_NAME(CTAG),
    CSR_NAME(DMW(0)),
    CSR_NAME(DMW(1)),
    CSR_NAME(DMW(2)),
    CSR_NAME(DMW(3)),
    CSR_NAME(DBG),
    CSR_NAME(DERA),
    CSR_NAME(DSAVE),
};

static const char *get_csr_name(unsigned num)
{
    return ((num < ARRAY_SIZE(csr_names)) && (csr_names[num] != NULL)) ?
           csr_names[num] : "Undefined CSR";
}

#define output(C, INSN, FMT, ...)                                   \
{                                                                   \
    (C)->info->fprintf_func((C)->info->stream, "%08x   %-9s\t" FMT, \
                            (C)->insn, INSN, ##__VA_ARGS__);        \
}

#include "decode-insns.c.inc"

int print_insn_loongarch(bfd_vma memaddr, struct disassemble_info *info)
{
    bfd_byte buffer[4];
    uint32_t insn;
    int status;

    status = (*info->read_memory_func)(memaddr, buffer, 4, info);
    if (status != 0) {
        (*info->memory_error_func)(status, memaddr, info);
        return -1;
    }
    insn = bfd_getl32(buffer);
    DisasContext ctx = {
        .info = info,
        .pc = memaddr,
        .insn = insn
    };

    if (!decode(&ctx, insn)) {
        output(&ctx, "illegal", "");
    }
    return 4;
}

static void output_r_i(DisasContext *ctx, arg_r_i *a, const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, %d", a->rd, a->imm);
}

static void output_rrr(DisasContext *ctx, arg_rrr *a, const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, r%d, r%d", a->rd, a->rj, a->rk);
}

static void output_rr_i(DisasContext *ctx, arg_rr_i *a, const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, r%d, %d", a->rd, a->rj, a->imm);
}

static void output_rrr_sa(DisasContext *ctx, arg_rrr_sa *a,
                          const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, r%d, r%d, %d", a->rd, a->rj, a->rk, a->sa);
}

static void output_rr(DisasContext *ctx, arg_rr *a, const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, r%d", a->rd, a->rj);
}

static void output_rr_ms_ls(DisasContext *ctx, arg_rr_ms_ls *a,
                          const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, r%d, %d, %d", a->rd, a->rj, a->ms, a->ls);
}

static void output_hint_r_i(DisasContext *ctx, arg_hint_r_i *a,
                            const char *mnemonic)
{
    output(ctx, mnemonic, "%d, r%d, %d", a->hint, a->rj, a->imm);
}

static void output_i(DisasContext *ctx, arg_i *a, const char *mnemonic)
{
    output(ctx, mnemonic, "%d", a->imm);
}

static void output_rr_jk(DisasContext *ctx, arg_rr_jk *a,
                         const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, r%d", a->rj, a->rk);
}

static void output_ff(DisasContext *ctx, arg_ff *a, const char *mnemonic)
{
    output(ctx, mnemonic, "f%d, f%d", a->fd, a->fj);
}

static void output_fff(DisasContext *ctx, arg_fff *a, const char *mnemonic)
{
    output(ctx, mnemonic, "f%d, f%d, f%d", a->fd, a->fj, a->fk);
}

static void output_ffff(DisasContext *ctx, arg_ffff *a, const char *mnemonic)
{
    output(ctx, mnemonic, "f%d, f%d, f%d, f%d", a->fd, a->fj, a->fk, a->fa);
}

static void output_fffc(DisasContext *ctx, arg_fffc *a, const char *mnemonic)
{
    output(ctx, mnemonic, "f%d, f%d, f%d, %d", a->fd, a->fj, a->fk, a->ca);
}

static void output_fr(DisasContext *ctx, arg_fr *a, const char *mnemonic)
{
    output(ctx, mnemonic, "f%d, r%d", a->fd, a->rj);
}

static void output_rf(DisasContext *ctx, arg_rf *a, const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, f%d", a->rd, a->fj);
}

static void output_fcsrd_r(DisasContext *ctx, arg_fcsrd_r *a,
                           const char *mnemonic)
{
    output(ctx, mnemonic, "fcsr%d, r%d", a->fcsrd, a->rj);
}

static void output_r_fcsrs(DisasContext *ctx, arg_r_fcsrs *a,
                           const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, fcsr%d", a->rd, a->fcsrs);
}

static void output_cf(DisasContext *ctx, arg_cf *a, const char *mnemonic)
{
    output(ctx, mnemonic, "fcc%d, f%d", a->cd, a->fj);
}

static void output_fc(DisasContext *ctx, arg_fc *a, const char *mnemonic)
{
    output(ctx, mnemonic, "f%d, fcc%d", a->fd, a->cj);
}

static void output_cr(DisasContext *ctx, arg_cr *a, const char *mnemonic)
{
    output(ctx, mnemonic, "fcc%d, r%d", a->cd, a->rj);
}

static void output_rc(DisasContext *ctx, arg_rc *a, const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, fcc%d", a->rd, a->cj);
}

static void output_frr(DisasContext *ctx, arg_frr *a, const char *mnemonic)
{
    output(ctx, mnemonic, "f%d, r%d, r%d", a->fd, a->rj, a->rk);
}

static void output_fr_i(DisasContext *ctx, arg_fr_i *a, const char *mnemonic)
{
    output(ctx, mnemonic, "f%d, r%d, %d", a->fd, a->rj, a->imm);
}

static void output_r_offs(DisasContext *ctx, arg_r_offs *a,
                          const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, %d # 0x%" PRIx64, a->rj, a->offs,
           ctx->pc + a->offs);
}

static void output_c_offs(DisasContext *ctx, arg_c_offs *a,
                          const char *mnemonic)
{
    output(ctx, mnemonic, "fcc%d, %d # 0x%" PRIx64, a->cj, a->offs,
           ctx->pc + a->offs);
}

static void output_offs(DisasContext *ctx, arg_offs *a,
                        const char *mnemonic)
{
    output(ctx, mnemonic, "%d # 0x%" PRIx64, a->offs, ctx->pc + a->offs);
}

static void output_rr_offs(DisasContext *ctx, arg_rr_offs *a,
                           const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, r%d, %d # 0x%" PRIx64, a->rj,
           a->rd, a->offs, ctx->pc + a->offs);
}

static void output_r_csr(DisasContext *ctx, arg_r_csr *a,
                         const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, %d # %s", a->rd, a->csr, get_csr_name(a->csr));
}

static void output_rr_csr(DisasContext *ctx, arg_rr_csr *a,
                          const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, r%d, %d # %s",
           a->rd, a->rj, a->csr, get_csr_name(a->csr));
}

static void output_empty(DisasContext *ctx, arg_empty *a,
                         const char *mnemonic)
{
    output(ctx, mnemonic, "");
}

static void output_i_rr(DisasContext *ctx, arg_i_rr *a, const char *mnemonic)
{
    output(ctx, mnemonic, "%d, r%d, r%d", a->imm, a->rj, a->rk);
}

static void output_cop_r_i(DisasContext *ctx, arg_cop_r_i *a,
                           const char *mnemonic)
{
    output(ctx, mnemonic, "%d, r%d, %d", a->cop, a->rj, a->imm);
}

static void output_j_i(DisasContext *ctx, arg_j_i *a, const char *mnemonic)
{
    output(ctx, mnemonic, "r%d, %d", a->rj, a->imm);
}

#define INSN(insn, type)                                    \
static bool trans_##insn(DisasContext *ctx, arg_##type * a) \
{                                                           \
    output_##type(ctx, a, #insn);                           \
    return true;                                            \
}

INSN(clo_w,        rr)
INSN(clz_w,        rr)
INSN(cto_w,        rr)
INSN(ctz_w,        rr)
INSN(clo_d,        rr)
INSN(clz_d,        rr)
INSN(cto_d,        rr)
INSN(ctz_d,        rr)
INSN(revb_2h,      rr)
INSN(revb_4h,      rr)
INSN(revb_2w,      rr)
INSN(revb_d,       rr)
INSN(revh_2w,      rr)
INSN(revh_d,       rr)
INSN(bitrev_4b,    rr)
INSN(bitrev_8b,    rr)
INSN(bitrev_w,     rr)
INSN(bitrev_d,     rr)
INSN(ext_w_h,      rr)
INSN(ext_w_b,      rr)
INSN(rdtimel_w,    rr)
INSN(rdtimeh_w,    rr)
INSN(rdtime_d,     rr)
INSN(cpucfg,       rr)
INSN(asrtle_d,     rr_jk)
INSN(asrtgt_d,     rr_jk)
INSN(alsl_w,       rrr_sa)
INSN(alsl_wu,      rrr_sa)
INSN(bytepick_w,   rrr_sa)
INSN(bytepick_d,   rrr_sa)
INSN(add_w,        rrr)
INSN(add_d,        rrr)
INSN(sub_w,        rrr)
INSN(sub_d,        rrr)
INSN(slt,          rrr)
INSN(sltu,         rrr)
INSN(maskeqz,      rrr)
INSN(masknez,      rrr)
INSN(nor,          rrr)
INSN(and,          rrr)
INSN(or,           rrr)
INSN(xor,          rrr)
INSN(orn,          rrr)
INSN(andn,         rrr)
INSN(sll_w,        rrr)
INSN(srl_w,        rrr)
INSN(sra_w,        rrr)
INSN(sll_d,        rrr)
INSN(srl_d,        rrr)
INSN(sra_d,        rrr)
INSN(rotr_w,       rrr)
INSN(rotr_d,       rrr)
INSN(mul_w,        rrr)
INSN(mulh_w,       rrr)
INSN(mulh_wu,      rrr)
INSN(mul_d,        rrr)
INSN(mulh_d,       rrr)
INSN(mulh_du,      rrr)
INSN(mulw_d_w,     rrr)
INSN(mulw_d_wu,    rrr)
INSN(div_w,        rrr)
INSN(mod_w,        rrr)
INSN(div_wu,       rrr)
INSN(mod_wu,       rrr)
INSN(div_d,        rrr)
INSN(mod_d,        rrr)
INSN(div_du,       rrr)
INSN(mod_du,       rrr)
INSN(crc_w_b_w,    rrr)
INSN(crc_w_h_w,    rrr)
INSN(crc_w_w_w,    rrr)
INSN(crc_w_d_w,    rrr)
INSN(crcc_w_b_w,   rrr)
INSN(crcc_w_h_w,   rrr)
INSN(crcc_w_w_w,   rrr)
INSN(crcc_w_d_w,   rrr)
INSN(break,        i)
INSN(syscall,      i)
INSN(alsl_d,       rrr_sa)
INSN(slli_w,       rr_i)
INSN(slli_d,       rr_i)
INSN(srli_w,       rr_i)
INSN(srli_d,       rr_i)
INSN(srai_w,       rr_i)
INSN(srai_d,       rr_i)
INSN(rotri_w,      rr_i)
INSN(rotri_d,      rr_i)
INSN(bstrins_w,    rr_ms_ls)
INSN(bstrpick_w,   rr_ms_ls)
INSN(bstrins_d,    rr_ms_ls)
INSN(bstrpick_d,   rr_ms_ls)
INSN(fadd_s,       fff)
INSN(fadd_d,       fff)
INSN(fsub_s,       fff)
INSN(fsub_d,       fff)
INSN(fmul_s,       fff)
INSN(fmul_d,       fff)
INSN(fdiv_s,       fff)
INSN(fdiv_d,       fff)
INSN(fmax_s,       fff)
INSN(fmax_d,       fff)
INSN(fmin_s,       fff)
INSN(fmin_d,       fff)
INSN(fmaxa_s,      fff)
INSN(fmaxa_d,      fff)
INSN(fmina_s,      fff)
INSN(fmina_d,      fff)
INSN(fscaleb_s,    fff)
INSN(fscaleb_d,    fff)
INSN(fcopysign_s,  fff)
INSN(fcopysign_d,  fff)
INSN(fabs_s,       ff)
INSN(fabs_d,       ff)
INSN(fneg_s,       ff)
INSN(fneg_d,       ff)
INSN(flogb_s,      ff)
INSN(flogb_d,      ff)
INSN(fclass_s,     ff)
INSN(fclass_d,     ff)
INSN(fsqrt_s,      ff)
INSN(fsqrt_d,      ff)
INSN(frecip_s,     ff)
INSN(frecip_d,     ff)
INSN(frsqrt_s,     ff)
INSN(frsqrt_d,     ff)
INSN(fmov_s,       ff)
INSN(fmov_d,       ff)
INSN(movgr2fr_w,   fr)
INSN(movgr2fr_d,   fr)
INSN(movgr2frh_w,  fr)
INSN(movfr2gr_s,   rf)
INSN(movfr2gr_d,   rf)
INSN(movfrh2gr_s,  rf)
INSN(movgr2fcsr,   fcsrd_r)
INSN(movfcsr2gr,   r_fcsrs)
INSN(movfr2cf,     cf)
INSN(movcf2fr,     fc)
INSN(movgr2cf,     cr)
INSN(movcf2gr,     rc)
INSN(fcvt_s_d,     ff)
INSN(fcvt_d_s,     ff)
INSN(ftintrm_w_s,  ff)
INSN(ftintrm_w_d,  ff)
INSN(ftintrm_l_s,  ff)
INSN(ftintrm_l_d,  ff)
INSN(ftintrp_w_s,  ff)
INSN(ftintrp_w_d,  ff)
INSN(ftintrp_l_s,  ff)
INSN(ftintrp_l_d,  ff)
INSN(ftintrz_w_s,  ff)
INSN(ftintrz_w_d,  ff)
INSN(ftintrz_l_s,  ff)
INSN(ftintrz_l_d,  ff)
INSN(ftintrne_w_s, ff)
INSN(ftintrne_w_d, ff)
INSN(ftintrne_l_s, ff)
INSN(ftintrne_l_d, ff)
INSN(ftint_w_s,    ff)
INSN(ftint_w_d,    ff)
INSN(ftint_l_s,    ff)
INSN(ftint_l_d,    ff)
INSN(ffint_s_w,    ff)
INSN(ffint_s_l,    ff)
INSN(ffint_d_w,    ff)
INSN(ffint_d_l,    ff)
INSN(frint_s,      ff)
INSN(frint_d,      ff)
INSN(slti,         rr_i)
INSN(sltui,        rr_i)
INSN(addi_w,       rr_i)
INSN(addi_d,       rr_i)
INSN(lu52i_d,      rr_i)
INSN(andi,         rr_i)
INSN(ori,          rr_i)
INSN(xori,         rr_i)
INSN(fmadd_s,      ffff)
INSN(fmadd_d,      ffff)
INSN(fmsub_s,      ffff)
INSN(fmsub_d,      ffff)
INSN(fnmadd_s,     ffff)
INSN(fnmadd_d,     ffff)
INSN(fnmsub_s,     ffff)
INSN(fnmsub_d,     ffff)
INSN(fsel,         fffc)
INSN(addu16i_d,    rr_i)
INSN(lu12i_w,      r_i)
INSN(lu32i_d,      r_i)
INSN(ll_w,         rr_i)
INSN(sc_w,         rr_i)
INSN(ll_d,         rr_i)
INSN(sc_d,         rr_i)
INSN(ldptr_w,      rr_i)
INSN(stptr_w,      rr_i)
INSN(ldptr_d,      rr_i)
INSN(stptr_d,      rr_i)
INSN(ld_b,         rr_i)
INSN(ld_h,         rr_i)
INSN(ld_w,         rr_i)
INSN(ld_d,         rr_i)
INSN(st_b,         rr_i)
INSN(st_h,         rr_i)
INSN(st_w,         rr_i)
INSN(st_d,         rr_i)
INSN(ld_bu,        rr_i)
INSN(ld_hu,        rr_i)
INSN(ld_wu,        rr_i)
INSN(preld,        hint_r_i)
INSN(fld_s,        fr_i)
INSN(fst_s,        fr_i)
INSN(fld_d,        fr_i)
INSN(fst_d,        fr_i)
INSN(ldx_b,        rrr)
INSN(ldx_h,        rrr)
INSN(ldx_w,        rrr)
INSN(ldx_d,        rrr)
INSN(stx_b,        rrr)
INSN(stx_h,        rrr)
INSN(stx_w,        rrr)
INSN(stx_d,        rrr)
INSN(ldx_bu,       rrr)
INSN(ldx_hu,       rrr)
INSN(ldx_wu,       rrr)
INSN(fldx_s,       frr)
INSN(fldx_d,       frr)
INSN(fstx_s,       frr)
INSN(fstx_d,       frr)
INSN(amswap_w,     rrr)
INSN(amswap_d,     rrr)
INSN(amadd_w,      rrr)
INSN(amadd_d,      rrr)
INSN(amand_w,      rrr)
INSN(amand_d,      rrr)
INSN(amor_w,       rrr)
INSN(amor_d,       rrr)
INSN(amxor_w,      rrr)
INSN(amxor_d,      rrr)
INSN(ammax_w,      rrr)
INSN(ammax_d,      rrr)
INSN(ammin_w,      rrr)
INSN(ammin_d,      rrr)
INSN(ammax_wu,     rrr)
INSN(ammax_du,     rrr)
INSN(ammin_wu,     rrr)
INSN(ammin_du,     rrr)
INSN(amswap_db_w,  rrr)
INSN(amswap_db_d,  rrr)
INSN(amadd_db_w,   rrr)
INSN(amadd_db_d,   rrr)
INSN(amand_db_w,   rrr)
INSN(amand_db_d,   rrr)
INSN(amor_db_w,    rrr)
INSN(amor_db_d,    rrr)
INSN(amxor_db_w,   rrr)
INSN(amxor_db_d,   rrr)
INSN(ammax_db_w,   rrr)
INSN(ammax_db_d,   rrr)
INSN(ammin_db_w,   rrr)
INSN(ammin_db_d,   rrr)
INSN(ammax_db_wu,  rrr)
INSN(ammax_db_du,  rrr)
INSN(ammin_db_wu,  rrr)
INSN(ammin_db_du,  rrr)
INSN(dbar,         i)
INSN(ibar,         i)
INSN(fldgt_s,      frr)
INSN(fldgt_d,      frr)
INSN(fldle_s,      frr)
INSN(fldle_d,      frr)
INSN(fstgt_s,      frr)
INSN(fstgt_d,      frr)
INSN(fstle_s,      frr)
INSN(fstle_d,      frr)
INSN(ldgt_b,       rrr)
INSN(ldgt_h,       rrr)
INSN(ldgt_w,       rrr)
INSN(ldgt_d,       rrr)
INSN(ldle_b,       rrr)
INSN(ldle_h,       rrr)
INSN(ldle_w,       rrr)
INSN(ldle_d,       rrr)
INSN(stgt_b,       rrr)
INSN(stgt_h,       rrr)
INSN(stgt_w,       rrr)
INSN(stgt_d,       rrr)
INSN(stle_b,       rrr)
INSN(stle_h,       rrr)
INSN(stle_w,       rrr)
INSN(stle_d,       rrr)
INSN(beqz,         r_offs)
INSN(bnez,         r_offs)
INSN(bceqz,        c_offs)
INSN(bcnez,        c_offs)
INSN(jirl,         rr_i)
INSN(b,            offs)
INSN(bl,           offs)
INSN(beq,          rr_offs)
INSN(bne,          rr_offs)
INSN(blt,          rr_offs)
INSN(bge,          rr_offs)
INSN(bltu,         rr_offs)
INSN(bgeu,         rr_offs)
INSN(csrrd,        r_csr)
INSN(csrwr,        r_csr)
INSN(csrxchg,      rr_csr)
INSN(iocsrrd_b,    rr)
INSN(iocsrrd_h,    rr)
INSN(iocsrrd_w,    rr)
INSN(iocsrrd_d,    rr)
INSN(iocsrwr_b,    rr)
INSN(iocsrwr_h,    rr)
INSN(iocsrwr_w,    rr)
INSN(iocsrwr_d,    rr)
INSN(tlbsrch,      empty)
INSN(tlbrd,        empty)
INSN(tlbwr,        empty)
INSN(tlbfill,      empty)
INSN(tlbclr,       empty)
INSN(tlbflush,     empty)
INSN(invtlb,       i_rr)
INSN(cacop,        cop_r_i)
INSN(lddir,        rr_i)
INSN(ldpte,        j_i)
INSN(ertn,         empty)
INSN(idle,         i)
INSN(dbcl,         i)

#define output_fcmp(C, PREFIX, SUFFIX)                                         \
{                                                                              \
    (C)->info->fprintf_func((C)->info->stream, "%08x   %s%s\tfcc%d, f%d, f%d", \
                            (C)->insn, PREFIX, SUFFIX, a->cd,                  \
                            a->fj, a->fk);                                     \
}

static bool output_cff_fcond(DisasContext *ctx, arg_cff_fcond * a,
                               const char *suffix)
{
    bool ret = true;
    switch (a->fcond) {
    case 0x0:
        output_fcmp(ctx, "fcmp_caf_", suffix);
        break;
    case 0x1:
        output_fcmp(ctx, "fcmp_saf_", suffix);
        break;
    case 0x2:
        output_fcmp(ctx, "fcmp_clt_", suffix);
        break;
    case 0x3:
        output_fcmp(ctx, "fcmp_slt_", suffix);
        break;
    case 0x4:
        output_fcmp(ctx, "fcmp_ceq_", suffix);
        break;
    case 0x5:
        output_fcmp(ctx, "fcmp_seq_", suffix);
        break;
    case 0x6:
        output_fcmp(ctx, "fcmp_cle_", suffix);
        break;
    case 0x7:
        output_fcmp(ctx, "fcmp_sle_", suffix);
        break;
    case 0x8:
        output_fcmp(ctx, "fcmp_cun_", suffix);
        break;
    case 0x9:
        output_fcmp(ctx, "fcmp_sun_", suffix);
        break;
    case 0xA:
        output_fcmp(ctx, "fcmp_cult_", suffix);
        break;
    case 0xB:
        output_fcmp(ctx, "fcmp_sult_", suffix);
        break;
    case 0xC:
        output_fcmp(ctx, "fcmp_cueq_", suffix);
        break;
    case 0xD:
        output_fcmp(ctx, "fcmp_sueq_", suffix);
        break;
    case 0xE:
        output_fcmp(ctx, "fcmp_cule_", suffix);
        break;
    case 0xF:
        output_fcmp(ctx, "fcmp_sule_", suffix);
        break;
    case 0x10:
        output_fcmp(ctx, "fcmp_cne_", suffix);
        break;
    case 0x11:
        output_fcmp(ctx, "fcmp_sne_", suffix);
        break;
    case 0x14:
        output_fcmp(ctx, "fcmp_cor_", suffix);
        break;
    case 0x15:
        output_fcmp(ctx, "fcmp_sor_", suffix);
        break;
    case 0x18:
        output_fcmp(ctx, "fcmp_cune_", suffix);
        break;
    case 0x19:
        output_fcmp(ctx, "fcmp_sune_", suffix);
        break;
    default:
        ret = false;
    }
    return ret;
}

#define FCMP_INSN(suffix)                               \
static bool trans_fcmp_cond_##suffix(DisasContext *ctx, \
                                     arg_cff_fcond * a) \
{                                                       \
    return output_cff_fcond(ctx, a, #suffix);           \
}

FCMP_INSN(s)
FCMP_INSN(d)

#define PCADD_INSN(name)                                        \
static bool trans_##name(DisasContext *ctx, arg_##name *a)      \
{                                                               \
    output(ctx, #name, "r%d, %d # 0x%" PRIx64,                  \
           a->rd, a->imm, gen_##name(ctx->pc, a->imm));         \
    return true;                                                \
}

static uint64_t gen_pcaddi(uint64_t pc, int imm)
{
    return pc + (imm << 2);
}

static uint64_t gen_pcalau12i(uint64_t pc, int imm)
{
    return (pc + (imm << 12)) & ~0xfff;
}

static uint64_t gen_pcaddu12i(uint64_t pc, int imm)
{
    return pc + (imm << 12);
}

static uint64_t gen_pcaddu18i(uint64_t pc, int imm)
{
    return pc + ((uint64_t)(imm) << 18);
}

PCADD_INSN(pcaddi)
PCADD_INSN(pcalau12i)
PCADD_INSN(pcaddu12i)
PCADD_INSN(pcaddu18i)

#define INSN_LSX(insn, type)                                \
static bool trans_##insn(DisasContext *ctx, arg_##type * a) \
{                                                           \
    output_##type(ctx, a, #insn);                           \
    return true;                                            \
}

static void output_vvv(DisasContext *ctx, arg_vvv *a, const char *mnemonic)
{
    output(ctx, mnemonic, "v%d, v%d, v%d", a->vd, a->vj, a->vk);
}

static void output_vv_i(DisasContext *ctx, arg_vv_i *a, const char *mnemonic)
{
    output(ctx, mnemonic, "v%d, v%d, 0x%x", a->vd, a->vj, a->imm);
}

static void output_vv(DisasContext *ctx, arg_vv *a, const char *mnemonic)
{
    output(ctx, mnemonic, "v%d, v%d", a->vd, a->vj);
}

INSN_LSX(vadd_b,           vvv)
INSN_LSX(vadd_h,           vvv)
INSN_LSX(vadd_w,           vvv)
INSN_LSX(vadd_d,           vvv)
INSN_LSX(vadd_q,           vvv)
INSN_LSX(vsub_b,           vvv)
INSN_LSX(vsub_h,           vvv)
INSN_LSX(vsub_w,           vvv)
INSN_LSX(vsub_d,           vvv)
INSN_LSX(vsub_q,           vvv)

INSN_LSX(vaddi_bu,         vv_i)
INSN_LSX(vaddi_hu,         vv_i)
INSN_LSX(vaddi_wu,         vv_i)
INSN_LSX(vaddi_du,         vv_i)
INSN_LSX(vsubi_bu,         vv_i)
INSN_LSX(vsubi_hu,         vv_i)
INSN_LSX(vsubi_wu,         vv_i)
INSN_LSX(vsubi_du,         vv_i)

INSN_LSX(vneg_b,           vv)
INSN_LSX(vneg_h,           vv)
INSN_LSX(vneg_w,           vv)
INSN_LSX(vneg_d,           vv)

INSN_LSX(vsadd_b,          vvv)
INSN_LSX(vsadd_h,          vvv)
INSN_LSX(vsadd_w,          vvv)
INSN_LSX(vsadd_d,          vvv)
INSN_LSX(vsadd_bu,         vvv)
INSN_LSX(vsadd_hu,         vvv)
INSN_LSX(vsadd_wu,         vvv)
INSN_LSX(vsadd_du,         vvv)
INSN_LSX(vssub_b,          vvv)
INSN_LSX(vssub_h,          vvv)
INSN_LSX(vssub_w,          vvv)
INSN_LSX(vssub_d,          vvv)
INSN_LSX(vssub_bu,         vvv)
INSN_LSX(vssub_hu,         vvv)
INSN_LSX(vssub_wu,         vvv)
INSN_LSX(vssub_du,         vvv)

INSN_LSX(vhaddw_h_b,       vvv)
INSN_LSX(vhaddw_w_h,       vvv)
INSN_LSX(vhaddw_d_w,       vvv)
INSN_LSX(vhaddw_q_d,       vvv)
INSN_LSX(vhaddw_hu_bu,     vvv)
INSN_LSX(vhaddw_wu_hu,     vvv)
INSN_LSX(vhaddw_du_wu,     vvv)
INSN_LSX(vhaddw_qu_du,     vvv)
INSN_LSX(vhsubw_h_b,       vvv)
INSN_LSX(vhsubw_w_h,       vvv)
INSN_LSX(vhsubw_d_w,       vvv)
INSN_LSX(vhsubw_q_d,       vvv)
INSN_LSX(vhsubw_hu_bu,     vvv)
INSN_LSX(vhsubw_wu_hu,     vvv)
INSN_LSX(vhsubw_du_wu,     vvv)
INSN_LSX(vhsubw_qu_du,     vvv)

INSN_LSX(vaddwev_h_b,      vvv)
INSN_LSX(vaddwev_w_h,      vvv)
INSN_LSX(vaddwev_d_w,      vvv)
INSN_LSX(vaddwev_q_d,      vvv)
INSN_LSX(vaddwod_h_b,      vvv)
INSN_LSX(vaddwod_w_h,      vvv)
INSN_LSX(vaddwod_d_w,      vvv)
INSN_LSX(vaddwod_q_d,      vvv)
INSN_LSX(vsubwev_h_b,      vvv)
INSN_LSX(vsubwev_w_h,      vvv)
INSN_LSX(vsubwev_d_w,      vvv)
INSN_LSX(vsubwev_q_d,      vvv)
INSN_LSX(vsubwod_h_b,      vvv)
INSN_LSX(vsubwod_w_h,      vvv)
INSN_LSX(vsubwod_d_w,      vvv)
INSN_LSX(vsubwod_q_d,      vvv)

INSN_LSX(vaddwev_h_bu,     vvv)
INSN_LSX(vaddwev_w_hu,     vvv)
INSN_LSX(vaddwev_d_wu,     vvv)
INSN_LSX(vaddwev_q_du,     vvv)
INSN_LSX(vaddwod_h_bu,     vvv)
INSN_LSX(vaddwod_w_hu,     vvv)
INSN_LSX(vaddwod_d_wu,     vvv)
INSN_LSX(vaddwod_q_du,     vvv)
INSN_LSX(vsubwev_h_bu,     vvv)
INSN_LSX(vsubwev_w_hu,     vvv)
INSN_LSX(vsubwev_d_wu,     vvv)
INSN_LSX(vsubwev_q_du,     vvv)
INSN_LSX(vsubwod_h_bu,     vvv)
INSN_LSX(vsubwod_w_hu,     vvv)
INSN_LSX(vsubwod_d_wu,     vvv)
INSN_LSX(vsubwod_q_du,     vvv)

INSN_LSX(vaddwev_h_bu_b,   vvv)
INSN_LSX(vaddwev_w_hu_h,   vvv)
INSN_LSX(vaddwev_d_wu_w,   vvv)
INSN_LSX(vaddwev_q_du_d,   vvv)
INSN_LSX(vaddwod_h_bu_b,   vvv)
INSN_LSX(vaddwod_w_hu_h,   vvv)
INSN_LSX(vaddwod_d_wu_w,   vvv)
INSN_LSX(vaddwod_q_du_d,   vvv)

INSN_LSX(vavg_b,           vvv)
INSN_LSX(vavg_h,           vvv)
INSN_LSX(vavg_w,           vvv)
INSN_LSX(vavg_d,           vvv)
INSN_LSX(vavg_bu,          vvv)
INSN_LSX(vavg_hu,          vvv)
INSN_LSX(vavg_wu,          vvv)
INSN_LSX(vavg_du,          vvv)
INSN_LSX(vavgr_b,          vvv)
INSN_LSX(vavgr_h,          vvv)
INSN_LSX(vavgr_w,          vvv)
INSN_LSX(vavgr_d,          vvv)
INSN_LSX(vavgr_bu,         vvv)
INSN_LSX(vavgr_hu,         vvv)
INSN_LSX(vavgr_wu,         vvv)
INSN_LSX(vavgr_du,         vvv)

INSN_LSX(vabsd_b,          vvv)
INSN_LSX(vabsd_h,          vvv)
INSN_LSX(vabsd_w,          vvv)
INSN_LSX(vabsd_d,          vvv)
INSN_LSX(vabsd_bu,         vvv)
INSN_LSX(vabsd_hu,         vvv)
INSN_LSX(vabsd_wu,         vvv)
INSN_LSX(vabsd_du,         vvv)

INSN_LSX(vadda_b,          vvv)
INSN_LSX(vadda_h,          vvv)
INSN_LSX(vadda_w,          vvv)
INSN_LSX(vadda_d,          vvv)

INSN_LSX(vmax_b,           vvv)
INSN_LSX(vmax_h,           vvv)
INSN_LSX(vmax_w,           vvv)
INSN_LSX(vmax_d,           vvv)
INSN_LSX(vmin_b,           vvv)
INSN_LSX(vmin_h,           vvv)
INSN_LSX(vmin_w,           vvv)
INSN_LSX(vmin_d,           vvv)
INSN_LSX(vmax_bu,          vvv)
INSN_LSX(vmax_hu,          vvv)
INSN_LSX(vmax_wu,          vvv)
INSN_LSX(vmax_du,          vvv)
INSN_LSX(vmin_bu,          vvv)
INSN_LSX(vmin_hu,          vvv)
INSN_LSX(vmin_wu,          vvv)
INSN_LSX(vmin_du,          vvv)
INSN_LSX(vmaxi_b,          vv_i)
INSN_LSX(vmaxi_h,          vv_i)
INSN_LSX(vmaxi_w,          vv_i)
INSN_LSX(vmaxi_d,          vv_i)
INSN_LSX(vmini_b,          vv_i)
INSN_LSX(vmini_h,          vv_i)
INSN_LSX(vmini_w,          vv_i)
INSN_LSX(vmini_d,          vv_i)
INSN_LSX(vmaxi_bu,         vv_i)
INSN_LSX(vmaxi_hu,         vv_i)
INSN_LSX(vmaxi_wu,         vv_i)
INSN_LSX(vmaxi_du,         vv_i)
INSN_LSX(vmini_bu,         vv_i)
INSN_LSX(vmini_hu,         vv_i)
INSN_LSX(vmini_wu,         vv_i)
INSN_LSX(vmini_du,         vv_i)

INSN_LSX(vmul_b,           vvv)
INSN_LSX(vmul_h,           vvv)
INSN_LSX(vmul_w,           vvv)
INSN_LSX(vmul_d,           vvv)
INSN_LSX(vmuh_b,           vvv)
INSN_LSX(vmuh_h,           vvv)
INSN_LSX(vmuh_w,           vvv)
INSN_LSX(vmuh_d,           vvv)
INSN_LSX(vmuh_bu,          vvv)
INSN_LSX(vmuh_hu,          vvv)
INSN_LSX(vmuh_wu,          vvv)
INSN_LSX(vmuh_du,          vvv)

INSN_LSX(vmulwev_h_b,      vvv)
INSN_LSX(vmulwev_w_h,      vvv)
INSN_LSX(vmulwev_d_w,      vvv)
INSN_LSX(vmulwev_q_d,      vvv)
INSN_LSX(vmulwod_h_b,      vvv)
INSN_LSX(vmulwod_w_h,      vvv)
INSN_LSX(vmulwod_d_w,      vvv)
INSN_LSX(vmulwod_q_d,      vvv)
INSN_LSX(vmulwev_h_bu,     vvv)
INSN_LSX(vmulwev_w_hu,     vvv)
INSN_LSX(vmulwev_d_wu,     vvv)
INSN_LSX(vmulwev_q_du,     vvv)
INSN_LSX(vmulwod_h_bu,     vvv)
INSN_LSX(vmulwod_w_hu,     vvv)
INSN_LSX(vmulwod_d_wu,     vvv)
INSN_LSX(vmulwod_q_du,     vvv)
INSN_LSX(vmulwev_h_bu_b,   vvv)
INSN_LSX(vmulwev_w_hu_h,   vvv)
INSN_LSX(vmulwev_d_wu_w,   vvv)
INSN_LSX(vmulwev_q_du_d,   vvv)
INSN_LSX(vmulwod_h_bu_b,   vvv)
INSN_LSX(vmulwod_w_hu_h,   vvv)
INSN_LSX(vmulwod_d_wu_w,   vvv)
INSN_LSX(vmulwod_q_du_d,   vvv)

INSN_LSX(vmadd_b,          vvv)
INSN_LSX(vmadd_h,          vvv)
INSN_LSX(vmadd_w,          vvv)
INSN_LSX(vmadd_d,          vvv)
INSN_LSX(vmsub_b,          vvv)
INSN_LSX(vmsub_h,          vvv)
INSN_LSX(vmsub_w,          vvv)
INSN_LSX(vmsub_d,          vvv)

INSN_LSX(vmaddwev_h_b,     vvv)
INSN_LSX(vmaddwev_w_h,     vvv)
INSN_LSX(vmaddwev_d_w,     vvv)
INSN_LSX(vmaddwev_q_d,     vvv)
INSN_LSX(vmaddwod_h_b,     vvv)
INSN_LSX(vmaddwod_w_h,     vvv)
INSN_LSX(vmaddwod_d_w,     vvv)
INSN_LSX(vmaddwod_q_d,     vvv)
INSN_LSX(vmaddwev_h_bu,    vvv)
INSN_LSX(vmaddwev_w_hu,    vvv)
INSN_LSX(vmaddwev_d_wu,    vvv)
INSN_LSX(vmaddwev_q_du,    vvv)
INSN_LSX(vmaddwod_h_bu,    vvv)
INSN_LSX(vmaddwod_w_hu,    vvv)
INSN_LSX(vmaddwod_d_wu,    vvv)
INSN_LSX(vmaddwod_q_du,    vvv)
INSN_LSX(vmaddwev_h_bu_b,  vvv)
INSN_LSX(vmaddwev_w_hu_h,  vvv)
INSN_LSX(vmaddwev_d_wu_w,  vvv)
INSN_LSX(vmaddwev_q_du_d,  vvv)
INSN_LSX(vmaddwod_h_bu_b,  vvv)
INSN_LSX(vmaddwod_w_hu_h,  vvv)
INSN_LSX(vmaddwod_d_wu_w,  vvv)
INSN_LSX(vmaddwod_q_du_d,  vvv)

INSN_LSX(vdiv_b,           vvv)
INSN_LSX(vdiv_h,           vvv)
INSN_LSX(vdiv_w,           vvv)
INSN_LSX(vdiv_d,           vvv)
INSN_LSX(vdiv_bu,          vvv)
INSN_LSX(vdiv_hu,          vvv)
INSN_LSX(vdiv_wu,          vvv)
INSN_LSX(vdiv_du,          vvv)
INSN_LSX(vmod_b,           vvv)
INSN_LSX(vmod_h,           vvv)
INSN_LSX(vmod_w,           vvv)
INSN_LSX(vmod_d,           vvv)
INSN_LSX(vmod_bu,          vvv)
INSN_LSX(vmod_hu,          vvv)
INSN_LSX(vmod_wu,          vvv)
INSN_LSX(vmod_du,          vvv)

INSN_LSX(vsat_b,           vv_i)
INSN_LSX(vsat_h,           vv_i)
INSN_LSX(vsat_w,           vv_i)
INSN_LSX(vsat_d,           vv_i)
INSN_LSX(vsat_bu,          vv_i)
INSN_LSX(vsat_hu,          vv_i)
INSN_LSX(vsat_wu,          vv_i)
INSN_LSX(vsat_du,          vv_i)

INSN_LSX(vexth_h_b,        vv)
INSN_LSX(vexth_w_h,        vv)
INSN_LSX(vexth_d_w,        vv)
INSN_LSX(vexth_q_d,        vv)
INSN_LSX(vexth_hu_bu,      vv)
INSN_LSX(vexth_wu_hu,      vv)
INSN_LSX(vexth_du_wu,      vv)
INSN_LSX(vexth_qu_du,      vv)

INSN_LSX(vsigncov_b,       vvv)
INSN_LSX(vsigncov_h,       vvv)
INSN_LSX(vsigncov_w,       vvv)
INSN_LSX(vsigncov_d,       vvv)

INSN_LSX(vmskltz_b,        vv)
INSN_LSX(vmskltz_h,        vv)
INSN_LSX(vmskltz_w,        vv)
INSN_LSX(vmskltz_d,        vv)
INSN_LSX(vmskgez_b,        vv)
INSN_LSX(vmsknz_b,         vv)

INSN_LSX(vand_v,           vvv)
INSN_LSX(vor_v,            vvv)
INSN_LSX(vxor_v,           vvv)
INSN_LSX(vnor_v,           vvv)
INSN_LSX(vandn_v,          vvv)
INSN_LSX(vorn_v,           vvv)

INSN_LSX(vandi_b,          vv_i)
INSN_LSX(vori_b,           vv_i)
INSN_LSX(vxori_b,          vv_i)
INSN_LSX(vnori_b,          vv_i)

INSN_LSX(vsll_b,           vvv)
INSN_LSX(vsll_h,           vvv)
INSN_LSX(vsll_w,           vvv)
INSN_LSX(vsll_d,           vvv)
INSN_LSX(vslli_b,          vv_i)
INSN_LSX(vslli_h,          vv_i)
INSN_LSX(vslli_w,          vv_i)
INSN_LSX(vslli_d,          vv_i)

INSN_LSX(vsrl_b,           vvv)
INSN_LSX(vsrl_h,           vvv)
INSN_LSX(vsrl_w,           vvv)
INSN_LSX(vsrl_d,           vvv)
INSN_LSX(vsrli_b,          vv_i)
INSN_LSX(vsrli_h,          vv_i)
INSN_LSX(vsrli_w,          vv_i)
INSN_LSX(vsrli_d,          vv_i)

INSN_LSX(vsra_b,           vvv)
INSN_LSX(vsra_h,           vvv)
INSN_LSX(vsra_w,           vvv)
INSN_LSX(vsra_d,           vvv)
INSN_LSX(vsrai_b,          vv_i)
INSN_LSX(vsrai_h,          vv_i)
INSN_LSX(vsrai_w,          vv_i)
INSN_LSX(vsrai_d,          vv_i)

INSN_LSX(vrotr_b,          vvv)
INSN_LSX(vrotr_h,          vvv)
INSN_LSX(vrotr_w,          vvv)
INSN_LSX(vrotr_d,          vvv)
INSN_LSX(vrotri_b,         vv_i)
INSN_LSX(vrotri_h,         vv_i)
INSN_LSX(vrotri_w,         vv_i)
INSN_LSX(vrotri_d,         vv_i)

INSN_LSX(vsllwil_h_b,      vv_i)
INSN_LSX(vsllwil_w_h,      vv_i)
INSN_LSX(vsllwil_d_w,      vv_i)
INSN_LSX(vextl_q_d,        vv)
INSN_LSX(vsllwil_hu_bu,    vv_i)
INSN_LSX(vsllwil_wu_hu,    vv_i)
INSN_LSX(vsllwil_du_wu,    vv_i)
INSN_LSX(vextl_qu_du,      vv)

INSN_LSX(vsrlr_b,          vvv)
INSN_LSX(vsrlr_h,          vvv)
INSN_LSX(vsrlr_w,          vvv)
INSN_LSX(vsrlr_d,          vvv)
INSN_LSX(vsrlri_b,         vv_i)
INSN_LSX(vsrlri_h,         vv_i)
INSN_LSX(vsrlri_w,         vv_i)
INSN_LSX(vsrlri_d,         vv_i)

INSN_LSX(vsrar_b,          vvv)
INSN_LSX(vsrar_h,          vvv)
INSN_LSX(vsrar_w,          vvv)
INSN_LSX(vsrar_d,          vvv)
INSN_LSX(vsrari_b,         vv_i)
INSN_LSX(vsrari_h,         vv_i)
INSN_LSX(vsrari_w,         vv_i)
INSN_LSX(vsrari_d,         vv_i)

INSN_LSX(vsrln_b_h,       vvv)
INSN_LSX(vsrln_h_w,       vvv)
INSN_LSX(vsrln_w_d,       vvv)
INSN_LSX(vsran_b_h,       vvv)
INSN_LSX(vsran_h_w,       vvv)
INSN_LSX(vsran_w_d,       vvv)

INSN_LSX(vsrlni_b_h,       vv_i)
INSN_LSX(vsrlni_h_w,       vv_i)
INSN_LSX(vsrlni_w_d,       vv_i)
INSN_LSX(vsrlni_d_q,       vv_i)
INSN_LSX(vsrani_b_h,       vv_i)
INSN_LSX(vsrani_h_w,       vv_i)
INSN_LSX(vsrani_w_d,       vv_i)
INSN_LSX(vsrani_d_q,       vv_i)

INSN_LSX(vsrlrn_b_h,       vvv)
INSN_LSX(vsrlrn_h_w,       vvv)
INSN_LSX(vsrlrn_w_d,       vvv)
INSN_LSX(vsrarn_b_h,       vvv)
INSN_LSX(vsrarn_h_w,       vvv)
INSN_LSX(vsrarn_w_d,       vvv)

INSN_LSX(vsrlrni_b_h,      vv_i)
INSN_LSX(vsrlrni_h_w,      vv_i)
INSN_LSX(vsrlrni_w_d,      vv_i)
INSN_LSX(vsrlrni_d_q,      vv_i)
INSN_LSX(vsrarni_b_h,      vv_i)
INSN_LSX(vsrarni_h_w,      vv_i)
INSN_LSX(vsrarni_w_d,      vv_i)
INSN_LSX(vsrarni_d_q,      vv_i)

INSN_LSX(vssrln_b_h,       vvv)
INSN_LSX(vssrln_h_w,       vvv)
INSN_LSX(vssrln_w_d,       vvv)
INSN_LSX(vssran_b_h,       vvv)
INSN_LSX(vssran_h_w,       vvv)
INSN_LSX(vssran_w_d,       vvv)
INSN_LSX(vssrln_bu_h,      vvv)
INSN_LSX(vssrln_hu_w,      vvv)
INSN_LSX(vssrln_wu_d,      vvv)
INSN_LSX(vssran_bu_h,      vvv)
INSN_LSX(vssran_hu_w,      vvv)
INSN_LSX(vssran_wu_d,      vvv)

INSN_LSX(vssrlni_b_h,      vv_i)
INSN_LSX(vssrlni_h_w,      vv_i)
INSN_LSX(vssrlni_w_d,      vv_i)
INSN_LSX(vssrlni_d_q,      vv_i)
INSN_LSX(vssrani_b_h,      vv_i)
INSN_LSX(vssrani_h_w,      vv_i)
INSN_LSX(vssrani_w_d,      vv_i)
INSN_LSX(vssrani_d_q,      vv_i)
INSN_LSX(vssrlni_bu_h,     vv_i)
INSN_LSX(vssrlni_hu_w,     vv_i)
INSN_LSX(vssrlni_wu_d,     vv_i)
INSN_LSX(vssrlni_du_q,     vv_i)
INSN_LSX(vssrani_bu_h,     vv_i)
INSN_LSX(vssrani_hu_w,     vv_i)
INSN_LSX(vssrani_wu_d,     vv_i)
INSN_LSX(vssrani_du_q,     vv_i)
