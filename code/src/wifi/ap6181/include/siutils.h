/*
 * Misc utility routines for accessing the SOC Interconnects
 * of Broadcom HNBU chips.
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: siutils.h,v 13.251.2.10 2011-02-04 05:06:32 Exp $
 */

#ifndef	_siutils_h_
#define	_siutils_h_

#if defined(WLC_HIGH) && !defined(WLC_LOW)
#include "bcm_rpc.h"
#endif
/*
 * Data structure to export all chip specific common variables
 *   public (read-only) portion of siutils handle returned by si_attach()/si_kattach()
 */
struct si_pub {
	uint	socitype;		/* SOCI_SB, SOCI_AI */

	uint	bustype;		/* SI_BUS, PCI_BUS */
	uint	buscoretype;		/* PCI_CORE_ID, PCIE_CORE_ID, PCMCIA_CORE_ID */
	uint	buscorerev;		/* buscore rev */
	uint	buscoreidx;		/* buscore index */
	int	ccrev;			/* chip common core rev */
	uint32	cccaps;			/* chip common capabilities */
	uint32  cccaps_ext;			/* chip common capabilities extension */
	int	pmurev;			/* pmu core rev */
	uint32	pmucaps;		/* pmu capabilities */
	uint	boardtype;		/* board type */
	uint	boardvendor;		/* board vendor */
	uint	boardflags;		/* board flags */
	uint	boardflags2;		/* board flags2 */
	uint	chip;			/* chip number */
	uint	chiprev;		/* chip revision */
	uint	chippkg;		/* chip package option */
	uint32	chipst;			/* chip status */
	bool	issim;			/* chip is in simulation or emulation */
	uint    socirev;		/* SOC interconnect rev */
	bool	pci_pr32414;

#if defined(WLC_HIGH) && !defined(WLC_LOW)
	rpc_info_t *rpc;
#endif
#ifdef SI_SPROM_PROBE
	int	wl_srom_present;
	char 	wl_srom_sw_map[SROM_MAX];
#endif /* SI_SPROM_PROBE */
};

/* for HIGH_ONLY driver, the si_t must be writable to allow states sync from BMAC to HIGH driver
 * for monolithic driver, it is readonly to prevent accident change
 */
#if defined(WLC_HIGH) && !defined(WLC_LOW)
typedef struct si_pub si_t;
#else
typedef const struct si_pub si_t;
#endif

/*
 * Many of the routines below take an 'sih' handle as their first arg.
 * Allocate this by calling si_attach().  Free it by calling si_detach().
 * At any one time, the sih is logically focused on one particular si core
 * (the "current core").
 * Use si_setcore() or si_setcoreidx() to change the association to another core.
 */
#define	SI_OSH		NULL	/* Use for si_kattach when no osh is available */

#define	BADIDX		(SI_MAXCORES + 1)

/* clkctl xtal what flags */
#define	XTAL			0x1	/* primary crystal oscillator (2050) */
#define	PLL			0x2	/* main chip pll */

/* clkctl clk mode */
#define	CLK_FAST		0	/* force fast (pll) clock */
#define	CLK_DYNAMIC		2	/* enable dynamic clock control */

/* GPIO usage priorities */
#define GPIO_DRV_PRIORITY	0	/* Driver */
#define GPIO_APP_PRIORITY	1	/* Application */
#define GPIO_HI_PRIORITY	2	/* Highest priority. Ignore GPIO reservation */

/* GPIO pull up/down */
#define GPIO_PULLUP		0
#define GPIO_PULLDN		1

/* GPIO event regtype */
#define GPIO_REGEVT		0	/* GPIO register event */
#define GPIO_REGEVT_INTMSK	1	/* GPIO register event int mask */
#define GPIO_REGEVT_INTPOL	2	/* GPIO register event int polarity */

/* device path */
#define SI_DEVPATH_BUFSZ	16	/* min buffer size in bytes */

/* SI routine enumeration: to be used by update function with multiple hooks */
#define	SI_DOATTACH	1
#define SI_PCIDOWN	2
#define SI_PCIUP	3

#define	ISSIM_ENAB(sih)	0

/* PMU clock/power control */
#if defined(BCMPMUCTL)
#define PMUCTL_ENAB(sih)	(BCMPMUCTL)
#else
#define PMUCTL_ENAB(sih)	((sih)->cccaps & CC_CAP_PMU)
#endif

/* chipcommon clock/power control (exclusive with PMU's) */
#if defined(BCMPMUCTL) && BCMPMUCTL
#define CCCTL_ENAB(sih)		(0)
#define CCPLL_ENAB(sih)		(0)
#else
#define CCCTL_ENAB(sih)		((sih)->cccaps & CC_CAP_PWR_CTL)
#define CCPLL_ENAB(sih)		((sih)->cccaps & CC_CAP_PLL_MASK)
#endif

typedef void (*gpio_handler_t)(uint32 stat, void *arg);


/* === exported functions === */
extern si_t *si_attach(osl_t *osh, void *regs, uint bustype, void *sdh, char **vars, uint *varsz);
extern void si_detach(si_t *sih);
extern bool si_pci_war16165(si_t *sih);
extern uint si_coreid(si_t *sih);
extern uint si_flag(si_t *sih);
extern uint si_coreidx(si_t *sih);
extern uint si_corerev(si_t *sih);
extern void *si_osh(si_t *sih);
extern uint si_corereg(si_t *sih, uint coreidx, uint regoff, uint mask, uint val);
extern void *si_coreregs(si_t *sih);
extern bool si_iscoreup(si_t *sih);
extern uint si_findcoreidx(si_t *sih, uint coreid, uint coreunit);
extern void *si_setcoreidx(si_t *sih, uint coreidx);
extern void *si_setcore(si_t *sih, uint coreid, uint coreunit);
extern void *si_switch_core(si_t *sih, uint coreid, uint *origidx, uint *intr_val);
extern void si_restore_core(si_t *sih, uint coreid, uint intr_val);
extern int si_numaddrspaces(si_t *sih);
extern uint32 si_addrspace(si_t *sih, uint asidx);
extern void si_core_reset(si_t *sih, uint32 bits, uint32 resetbits);
extern void si_core_disable(si_t *sih, uint32 bits);
extern bool si_read_pmu_autopll(si_t *sih);
extern uint32 si_clock(si_t *sih);
extern uint32 si_alp_clock(si_t *sih);
extern uint32 si_ilp_clock(si_t *sih);
extern void si_pci_setup(si_t *sih, uint coremask);
extern void si_pcmcia_init(si_t *sih);
extern bool si_backplane64(si_t *sih);
extern uint16 si_clkctl_fast_pwrup_delay(si_t *sih);
extern bool si_clkctl_cc(si_t *sih, uint mode);
extern int si_clkctl_xtal(si_t *sih, uint what, bool on);
extern void si_btcgpiowar(si_t *sih);
extern bool si_deviceremoved(si_t *sih);
extern uint32 si_socram_size(si_t *sih);
extern bool si_socdevram_pkg(si_t *sih);

/* Wake-on-wireless-LAN (WOWL) */
extern bool si_pci_pmecap(si_t *sih);
struct osl_info;
extern bool si_pci_fastpmecap(struct osl_info *osh);
extern bool si_pci_pmestat(si_t *sih);
extern void si_pci_pmeclr(si_t *sih);
extern void si_pci_pmeen(si_t *sih);
extern uint si_pcie_readreg(void *sih, uint addrtype, uint offset);

extern void si_sdio_init(si_t *sih);

extern uint16 si_d11_devid(si_t *sih);
extern int si_corepciid(si_t *sih, uint func, uint16 *pcivendor, uint16 *pcidevice,
	uint8 *pciclass, uint8 *pcisubclass, uint8 *pciprogif, uint8 *pciheader);

#define si_eci(sih) 0
#define si_eci_init(sih) (0)
#define si_eci_notify_bt(sih, type, val)  (0)
#define si_seci(sih) 0
static INLINE void * si_seci_init(si_t *sih, uint8 use_seci) {return NULL;}
#define si_seci_down(sih) do { } while (0)

/* OTP status */
extern bool si_is_otp_disabled(si_t *sih);
extern bool si_is_otp_powered(si_t *sih);
extern void si_otp_power(si_t *sih, bool on);

/* SPROM availability */
extern bool si_is_sprom_available(si_t *sih);
extern bool si_is_sprom_enabled(si_t *sih);
extern void si_sprom_enable(si_t *sih, bool enable);
#ifdef SI_SPROM_PROBE
extern void si_sprom_init(si_t *sih);
#endif /* SI_SPROM_PROBE */

/* OTP/SROM CIS stuff */
extern int si_cis_source(si_t *sih);
#define CIS_DEFAULT	0
#define CIS_SROM	1
#define CIS_OTP		2

/* Fab-id information */
#define	DEFAULT_FAB	0x0	/* Original/first fab used for this chip */
#define	CSM_FAB7	0x1	/* CSM Fab7 chip */
#define	TSMC_FAB12	0x2	/* TSMC Fab12/Fab14 chip */
#define	SMIC_FAB4	0x3	/* SMIC Fab4 chip */
extern int BCMINITFN(si_otp_fabid)(si_t *sih, uint16 *fabid, bool rw);
extern uint16 BCMINITFN(si_fabid)(si_t *sih);

/*
 * Build device path. Path size must be >= SI_DEVPATH_BUFSZ.
 * The returned path is NULL terminated and has trailing '/'.
 * Return 0 on success, nonzero otherwise.
 */
extern int si_devpath(si_t *sih, char *path, int size);
/* Read variable with prepending the devpath to the name */
extern char *si_getdevpathvar(si_t *sih, const char *name);
extern int si_getdevpathintvar(si_t *sih, const char *name);


extern uint8 si_pcieclkreq(si_t *sih, uint32 mask, uint32 val);
extern uint32 si_pcielcreg(si_t *sih, uint32 mask, uint32 val);
extern void si_war42780_clkreq(si_t *sih, bool clkreq);
extern void si_pci_sleep(si_t *sih);
extern void si_pci_down(si_t *sih);
extern void si_pci_up(si_t *sih);
extern void si_pcie_war_ovr_update(si_t *sih, uint8 aspm);
extern void si_pcie_extendL1timer(si_t *sih, bool extend);
extern int si_pci_fixcfg(si_t *sih);
extern uint si_pll_reset(si_t *sih);

#if defined(BCMDBG_DUMP)
extern void si_dump(si_t *sih, struct bcmstrbuf *b);
extern void si_ccreg_dump(si_t *sih, struct bcmstrbuf *b);
extern void si_clkctl_dump(si_t *sih, struct bcmstrbuf *b);
extern int si_gpiodump(si_t *sih, struct bcmstrbuf *b);
extern int si_dump_pcieregs(si_t *sih, struct bcmstrbuf *b);
#endif
#if defined(BCMDBG_DUMP)
extern void si_dumpregs(si_t *sih, struct bcmstrbuf *b);
#endif

extern uint32 si_pciereg(si_t *sih, uint32 offset, uint32 mask, uint32 val, uint type);
extern uint32 si_pcieserdesreg(si_t *sih, uint32 mdioslave, uint32 offset, uint32 mask, uint32 val);

char *si_getnvramflvar(si_t *sih, const char *name);

#endif	/* _siutils_h_ */
